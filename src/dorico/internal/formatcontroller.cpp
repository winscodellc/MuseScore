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


#include "formatcontroller.h"

#include <algorithm>
#include <vector>

#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/types/types.h"

#include "notation/inotation.h"

#include "../doricocommands.h"
#include "transact.h"

using namespace mu::dorico;
using namespace mu::engraving;

void DoricoFormatController::init()
{
    // One undo step per command; false (nothing selected that can take it) rolls back and logs.
    const auto onNotes = [this](const muse::rcommand::Command& command, const muse::TranslatableString& title,
                                std::function<void(Note*)> change) {
        dispatcher()->onRequest(this, command, [this, title, change]() {
            if (notation::INotationPtr n = globalContext()->currentNotation()) {
                transact(n, title, [&change](Score* score, Transaction&) {
                    const std::vector<Note*> notes = score->selection().uniqueNotes();
                    std::for_each(notes.begin(), notes.end(), change);
                    return !notes.empty();
                });
            }
            return muse::make_ok();
        });
    };

    const auto stem = [&](const muse::rcommand::Command& command, DirectionV direction) {
        onNotes(command, muse::TranslatableString("undoableAction", "Set stem direction"), [direction](Note* note) {
            note->chord()->undoChangeProperty(Pid::STEM_DIRECTION, PropertyValue::fromValue(direction));
        });
    };
    stem(DORICO_STEM_UP_COMMAND, DirectionV::UP);
    stem(DORICO_STEM_DOWN_COMMAND, DirectionV::DOWN);
    stem(DORICO_STEM_AUTO_COMMAND, DirectionV::AUTO);

    const auto head = [&](const muse::rcommand::Command& command, NoteHeadGroup group) {
        onNotes(command, muse::TranslatableString("undoableAction", "Change notehead"), [group](Note* note) {
            note->undoChangeProperty(Pid::HEAD_GROUP, PropertyValue::fromValue(group));
        });
    };
    head(DORICO_NOTEHEAD_DEFAULT_COMMAND, NoteHeadGroup::HEAD_NORMAL);
    head(DORICO_NOTEHEAD_CROSS_COMMAND, NoteHeadGroup::HEAD_CROSS);
    head(DORICO_NOTEHEAD_TRIANGLE_COMMAND, NoteHeadGroup::HEAD_TRIANGLE_UP);
    head(DORICO_NOTEHEAD_DIAMOND_COMMAND, NoteHeadGroup::HEAD_DIAMOND);
    head(DORICO_NOTEHEAD_SLASH_COMMAND, NoteHeadGroup::HEAD_SLASH);
    head(DORICO_NOTEHEAD_SLASHED1_COMMAND, NoteHeadGroup::HEAD_SLASHED1);
    head(DORICO_NOTEHEAD_SLASHED2_COMMAND, NoteHeadGroup::HEAD_SLASHED2);
    head(DORICO_NOTEHEAD_CIRCLED_COMMAND, NoteHeadGroup::HEAD_CIRCLED);
    head(DORICO_NOTEHEAD_CIRCLED_LARGE_COMMAND, NoteHeadGroup::HEAD_CIRCLED_LARGE);
}
