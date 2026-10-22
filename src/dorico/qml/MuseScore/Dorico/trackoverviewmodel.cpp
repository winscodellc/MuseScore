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
#include "engraving/dom/textbase.h"
#include "engraving/types/constants.h"
#include "notation/inotation.h"
#include "notation/inotationplayback.h"
#include "notation/inotationundostack.h"

#include "../../../internal/trackblocks.h"
#include "../../../internal/doricoperf.h"

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
    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        listenToSoloMute();
    });
    globalContext()->playbackState()->playbackPositionChanged().onReceive(this, [this](muse::audio::secs_t secs) {
        updatePlayhead(secs);
    });
    listenToScore();
    listenToSoloMute();
    updatePlayhead(globalContext()->playbackState()->playbackPosition());
}

void TrackOverviewModel::listenToSoloMute()
{
    if (m_notation) {
        m_notation->soloMuteState()->trackSoloMuteStateChanged().disconnect(this);
    }
    m_notation = globalContext()->currentNotation();
    if (m_notation) {
        m_notation->soloMuteState()->trackSoloMuteStateChanged().onReceive(this, [this](const InstrumentTrackId&, const auto&) {
            updateSoloMute();
        });
    }
    updateSoloMute();
}

void TrackOverviewModel::updateSoloMute()
{
    m_soloMute.clear();
    if (m_master && m_notation) {
        for (const Part* part : m_master->masterScore()->parts()) {
            if (part->instruments().empty()) {
                continue;
            }
            // The part's first instrument stands for the part (MixerPanelModel shows one channel per instrument).
            const auto& state = playbackController()->trackSoloMuteState({ part->id(), part->instruments().begin()->second->id() });
            m_soloMute.insert(part->id().toQString(), QVariantMap { { "muted", state.mute }, { "solo", state.solo } });
        }
    }
    emit soloMuteChanged();
}

void TrackOverviewModel::setMuted(const QString& partId, bool muted)
{
    changeSoloMute(partId, true, muted);
}

void TrackOverviewModel::setSolo(const QString& partId, bool solo)
{
    changeSoloMute(partId, false, solo);
}

void TrackOverviewModel::changeSoloMute(const QString& partId, bool mute, bool value)
{
    if (!m_master || !m_notation) {
        return;
    }
    for (const Part* part : m_master->masterScore()->parts()) {
        if (part->id().toQString() != partId) {
            continue;
        }
        for (const auto& [tick, instrument] : part->instruments()) {
            const InstrumentTrackId trackId { part->id(), instrument->id() };
            auto state = playbackController()->trackSoloMuteState(trackId);
            (mute ? state.mute : state.solo) = value;
            if (value) {
                (mute ? state.solo : state.mute) = false;
            }
            playbackController()->setTrackSoloMuteState(trackId, state);
        }
    }
}

void TrackOverviewModel::listenToScore()
{
    if (m_master) {
        m_master->notation()->undoStack()->stackChanged().disconnect(this);
    }
    m_master = globalContext()->currentMasterNotation();
    if (m_master) {
        // ponytail: recomputes every bar on each edit but only tells QML when the overview changed; per-range
        // recompute (ScoreChanges tickFrom/tickTo) if the "trackOverview rebuild" timing grows on big scores
        m_master->notation()->undoStack()->stackChanged().onNotify(this, [this]() {
            rebuild(m_master->masterScore());
        });
    }
    rebuild(m_master ? m_master->masterScore() : nullptr);
}

void TrackOverviewModel::rebuild(const Score* score)
{
    PerfTimer perf("trackOverview rebuild (all measures)");
    QVariantList tracks;
    QVariantList bars;
    double length = 0;

    if (score) {
        for (const Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
            bars << QVariantMap { { "start", quarters(m->tick()) }, { "number", m->measureNumber() + 1 } };
        }
        length = quarters(score->endTick());

        for (const Part* part : score->parts()) {
            std::vector<TrackBar> trackBars;
            for (const Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
                trackBars.push_back({ quarters(m->tick()), quarters(m->endTick()), hasNotes(m, part) });
            }
            QVariantList blocks;
            for (const TrackBlock& block : trackBlocks(trackBars)) {
                blocks << QVariantMap { { "start", block.start }, { "end", block.end } };
            }

            tracks << QVariantMap {
                { "partId", part->id().toQString() },
                { "title", TextBase::unEscape(part->partName()).toQString() }, // plain text: partName() is rich text
                { "blocks", blocks },
            };
        }
    }

    // Most edits (a pitch, a dynamic) leave the overview as it was: then QML keeps its delegates (spec 96)
    if (tracks == m_tracks && bars == m_bars && length == m_length) {
        return;
    }
    qCInfo(doricoPerf) << "trackOverview changed";
    m_tracks = tracks;
    m_bars = bars;
    m_length = length;

    emit contentChanged();
    updateSoloMute(); // parts may have been added or removed
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
