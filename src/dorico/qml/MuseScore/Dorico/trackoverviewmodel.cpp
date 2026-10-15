/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "trackoverviewmodel.h"

#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/part.h"
#include "engraving/dom/segment.h"
#include "engraving/types/constants.h"
#include "notation/inotationplayback.h"
#include "notation/inotationundostack.h"

#include "../../../internal/trackblocks.h"

using namespace mu::dorico;
using namespace mu::engraving;

static double quarters(const Fraction& tick)
{
    return tick.ticks() / double(Constants::DIVISION);
}

static bool hasNotes(const Measure* measure, const Part* part)
{
    for (const Segment* s = measure->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
        for (track_idx_t track = part->trackRange().startTrack; track < part->trackRange().endTrack; ++track) {
            const EngravingItem* item = s->element(track);
            if (item && item->isChord()) {
                return true;
            }
        }
    }
    return false;
}

TrackOverviewModel::TrackOverviewModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void TrackOverviewModel::load()
{
    globalContext()->currentMasterNotationChanged().onNotify(this, [this]() {
        listenToScore();
    });
    globalContext()->playbackState()->playbackPositionChanged().onReceive(this, [this](muse::audio::secs_t secs) {
        updatePlayhead(secs);
    });
    listenToScore();
    updatePlayhead(globalContext()->playbackState()->playbackPosition());
}

void TrackOverviewModel::listenToScore()
{
    if (m_master) {
        m_master->notation()->undoStack()->stackChanged().disconnect(this);
    }
    m_master = globalContext()->currentMasterNotation();
    if (m_master) {
        // ponytail: rebuilds the whole overview on every edit; fine while Play mode does not edit notes
        m_master->notation()->undoStack()->stackChanged().onNotify(this, [this]() {
            rebuild(m_master->masterScore());
        });
    }
    rebuild(m_master ? m_master->masterScore() : nullptr);
}

void TrackOverviewModel::rebuild(const Score* score)
{
    m_tracks.clear();
    m_bars.clear();
    m_length = 0;

    if (score) {
        for (const Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            m_bars << QVariantMap { { "start", quarters(m->tick()) }, { "number", m->measureNumber() + 1 } };
        }
        m_length = quarters(score->endTick());

        for (const Part* part : score->parts()) {
            std::vector<TrackBar> trackBars;
            for (const Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                trackBars.push_back({ quarters(m->tick()), quarters(m->endTick()), hasNotes(m, part) });
            }
            QVariantList blocks;
            for (const TrackBlock& block : trackBlocks(trackBars)) {
                blocks << QVariantMap { { "start", block.start }, { "end", block.end } };
            }

            m_tracks << QVariantMap {
                { "partId", part->id().toQString() },
                { "title", part->partName().toQString() },
                { "blocks", blocks },
            };
        }
    }

    emit contentChanged();
}

void TrackOverviewModel::updatePlayhead(muse::audio::secs_t secs)
{
    // ponytail: no interpolation between position updates (the score view interpolates); add it if the line stutters
    const double position = m_master ? m_master->playback()->secToTick(secs) / double(Constants::DIVISION) : 0;
    if (qFuzzyCompare(position + 1, m_playheadPosition + 1)) {
        return;
    }
    m_playheadPosition = position;
    emit playheadPositionChanged();
}
