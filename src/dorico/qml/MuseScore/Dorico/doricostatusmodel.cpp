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


#include "doricostatusmodel.h"

#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/repeatlist.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/sig.h"

#include "notation/inotation.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"
#include "notation/inotationselectionrange.h"

#include "../../../internal/selectionsummary.h"

using namespace mu::dorico;
using namespace mu::engraving;

DoricoStatusModel::DoricoStatusModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void DoricoStatusModel::load()
{
    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        subscribeToSelection();
    });

    subscribeToSelection();
}

void DoricoStatusModel::subscribeToSelection()
{
    notation::INotationPtr notation = globalContext()->currentNotation();
    if (notation) {
        notation->interaction()->selectionChanged().onNotify(this, [this]() {
            update();
        });
    }

    update();
}

static double seconds(const Score* score, const Fraction& tick)
{
    return score->utick2utime(score->repeatList().tick2utick(tick.ticks()));
}

void DoricoStatusModel::update()
{
    SelectionInfo info;

    notation::INotationPtr notation = globalContext()->currentNotation();
    notation::INotationSelectionPtr selection = notation ? notation->interaction()->selection() : nullptr;

    if (selection && !selection->isNone()) {
        if (selection->isRange()) {
            notation::INotationSelectionRangePtr range = selection->range();
            const Segment* start = range ? range->rangeStartSegment() : nullptr;
            if (start) {
                const Score* score = start->score();
                const int startBar = start->barbeat().bar;
                int endBar = startBar;
                int beat = 0;
                int ticks = 0;
                // endTick is exclusive, so look one tick before it.
                // ponytail: bar numbers ignore measure-number offsets on the last bar; use displayedBar math if it matters.
                score->sigmap()->tickValues(std::max(range->endTick().ticks() - 1, start->tick().ticks()), &endBar, &beat, &ticks);
                endBar += 1;

                info.kind = SelectionInfo::Kind::Range;
                info.firstBar = start->barbeat().displayedBar;
                info.lastBar = info.firstBar + std::max(0, endBar - startBar);
                info.startSeconds = seconds(score, range->startTick());
                info.endSeconds = seconds(score, range->endTick());
            }
        } else if (selection->elements().size() > 1) {
            info.kind = SelectionInfo::Kind::Multiple;
        } else if (const EngravingItem* element = selection->element()) {
            info.kind = SelectionInfo::Kind::Single;
            info.firstBar = element->barbeat().displayedBar;
            if (element->isNote()) {
                const Note* note = toNote(element);
                // ponytail: octave from the MIDI pitch, wrong by one for B#/Cb; use the tpc-aware octave if it matters.
                info.pitch = note->tpcUserName().toQString() + QString::number(note->epitch() / 12 - 1);
                info.voice = static_cast<int>(note->voice()) + 1;
            } else if (element->isChordRest()) {
                info.voice = static_cast<int>(element->voice()) + 1;
            }
        }
    }

    const QString text = summarise(info);
    if (text != m_summary) {
        m_summary = text;
        emit summaryChanged();
    }
}
