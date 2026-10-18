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

#pragma once

#include "../popovertypes.h"

namespace mu::dorico {
//! `;` (spec 31): tuplets. Outside note input the selected notes become tuplets. During note input the entered tuplet
//! stays active: notes go into it, a new one starting when it is full, until `:`, an arrow-key caret move or the end of
//! note input. A second entry while one is active nests inside it.
PopoverDef tupletsPopover();

//! Before a note or rest is entered at the caret: starts the active tuplets there when the caret is not inside them.
//! True when the score changed (one undo step of its own, which the caller merges with the entry).
bool startTupletAtCaret(notation::INotationPtr notation);

//! `:` stops the innermost active tuplet and moves the caret past it; `all` (arrow keys, end of note input) stops every
//! one where the caret is.
void stopTupletInput(notation::INotationPtr notation, bool all);
}
