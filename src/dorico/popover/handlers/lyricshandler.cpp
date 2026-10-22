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

#include "lyricshandler.h"

#include "engraving/dom/chordrest.h"
#include "engraving/dom/lyrics.h"
#include "engraving/dom/note.h"
#include "engraving/dom/segment.h"

#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationselectionrange.h"

using namespace mu::dorico;
using namespace mu::engraving;

PopoverDef mu::dorico::lyricsPopover()
{
    PopoverDef def;
    def.id = "lyrics";
    def.title = "Lyrics";

    // Same target as MuseScore's add-lyrics (NotationActionController::addText), but a note that already has a
    // line 1 lyric edits it instead of adding line 2.
    def.openInline = [](mu::notation::INotationPtr notation) {
        mu::notation::INotationInteractionPtr interaction = notation->interaction();
        mu::notation::INotationSelectionPtr selection = interaction->selection();
        EngravingItem* item = selection->isRange()
                              ? selection->range()->rangeStartSegment()->firstElementForNavigation(selection->range()->startStaffIndex())
                              : interaction->contextItem();
        EngravingItem* cr = item && item->isNote() ? toNote(item)->chord() : item;
        if (cr && cr->isChordRest()) {
            for (Lyrics* lyric : toChordRest(cr)->lyrics()) {
                if (lyric->verse() == 0) {
                    if (interaction->noteInput()->isNoteInputMode()) {
                        interaction->noteInput()->endNoteInput();
                    }
                    interaction->select({ lyric });
                    interaction->startEditText(lyric);
                    return;
                }
            }
        }
        interaction->addTextToItem(TextStyleType::LYRICS_ODD, item);
    };
    return def;
}
