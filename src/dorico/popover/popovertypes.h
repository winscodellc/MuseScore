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

#include <functional>
#include <string>
#include <vector>

#include "engraving/types/types.h"
#include "notation/inotation.h"

namespace mu::engraving {
class EngravingItem;
}

namespace mu::dorico {
enum class ParseStatus {
    Ok,
    Incomplete,  // valid prefix: neutral border, Return does nothing
    Unsupported, // valid Dorico syntax we deliberately do not implement: red border + "not supported yet"
    Invalid      // red border, Return does nothing
};

enum class ApplyScope {
    Default, // Return
    Local    // Opt-Return
};

//! Space / Shift-Space, Tab / Shift-Tab, Cmd-Right / Cmd-Left in a navigating popover (spec 32).
enum class PopoverMove {
    NextBeat,
    PrevBeat,
    NextBar,
    PrevBar, // start of the bar if mid-bar, otherwise the previous bar
    NextItem,
    PrevItem
};

//! Where a navigating popover sits: a beat on a staff, which may have no note of its own (inside a half note).
struct PopoverPosition {
    engraving::Fraction tick;
    engraving::track_idx_t track = 0; // voice 1 of the staff
};

//! One popover (spec 20). A plain record of callbacks, like a TS object literal; specs 21-36 each provide one.
//! `apply` runs inside the popover's undo step and acts on the current selection or note-input caret.
//! When the popover edits an existing item (Return on it, or opened on a note that already has one), that item is
//! selected before `apply`, so applying replaces it instead of adding a second one.
struct PopoverDef {
    std::string id;    // URI tail: command://dorico/popover/<id>
    std::string title;
    //! Box icon (spec 99): SMuFL glyphs drawn in Bravura Text (plain text falls back to the UI font), "\n" stacks lines;
    //! or "svg:<name>" from the Dorico icons. Never a Dorico asset.
    std::string glyph;
    bool supportsLocal = false; // Opt-Return means something
    bool scopeButton = false;   // Dorico shows the Local/Global person button (spec 99)
    int fieldWidth = 133;       // entry field width in px, border included (spec 99, measured on Dorico 6)
    std::vector<engraving::ElementType> itemTypes;                  // selected item types whose Return reopens this popover
    bool editsAttached = true; // opened on a note that has one of itemTypes, edit it (dynamics); false adds another

    std::function<ParseStatus(const std::string&)> validate;
    std::function<bool(const std::string&, ApplyScope, notation::INotationPtr)> apply; // true = applied
    std::function<std::string(const engraving::EngravingItem*)> prefill;              // entry text for Return-to-edit
    std::function<std::string(notation::INotationPtr)> selectionPrefill; // optional: entry text suggested for the selection
    std::function<void(notation::INotationPtr)> noteInputEnded; // optional: close open-ended items (p<) when input stops
    std::function<std::vector<std::string>(const std::string&)> suggest; // optional: suggestion menu for the typed text

    //! Navigating popovers (spec 32) sit at a PopoverPosition instead of acting on the selection: opening one stops
    //! note input, Return enters the text there, and Space / Tab / Cmd-arrows enter it and move on. Set it instead of
    //! `apply`.
    std::function<bool(const std::string&, const PopoverPosition&, notation::INotationPtr)> applyAt;

    //! Fingering (spec 34): the position is a note's voice, Space / Shift-Space step to the next / previous chord of that
    //! voice, and the item edited at a position is that chord's. Tab and Cmd-arrows do nothing.
    bool stepsByChord = false;

    //! Popovers typed straight into the score (lyrics, spec 33): the Shift-key runs this and no box opens.
    std::function<void(notation::INotationPtr)> openInline;
};
}
