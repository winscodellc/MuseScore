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

#include "modularity/imoduleinterface.h"

namespace muse::draw {
class Painter;
}

namespace mu::dorico {
//! Dorico's note-input caret (spec 12): it can sit on any rhythmic grid position, also inside a rest or note,
//! and shows the grid as a ruler above the staff. MuseScore's input position stays on the chord/rest that contains it.
class IDoricoCaret : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IDoricoCaret)

public:
    virtual ~IDoricoCaret() = default;

    //! Moves the caret by the grid or to the next/previous note/rest, whichever is closer. false = not handled
    //! (no note input, tuplet, empty voice ...), the caller falls back to MuseScore's move.
    virtual bool step(bool right) = 0;

    //! Before a note/rest is entered: if the caret is inside a note/rest, cut it there (own undo step) and move
    //! MuseScore's input position onto the cut. true = an undo step was added.
    virtual bool materialize() = 0;

    //! Up / Down (toEnd: Cmd-Up / Cmd-Down). Note input: the caret goes to the staff above / below (top / bottom
    //! staff), same rhythmic position. Otherwise the selection goes to the next note above / below, crossing staves.
    virtual void navigateVertically(bool up, bool toEnd) = 0;

    virtual void finerGrid() = 0;
    virtual void coarserGrid() = 0;
    //! The rhythmic grid as a note value denominator: 8 = eighth notes. Also used outside note input (spec 13).
    virtual int gridDenominator() const = 0;

    //! Called by the score view. false = not handled, MuseScore paints its own caret.
    virtual bool paint(muse::draw::Painter* painter) = 0;
};
}
