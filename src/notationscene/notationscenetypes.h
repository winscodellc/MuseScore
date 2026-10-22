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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

namespace mu::notation {
enum class PercussionPanelAutoShowMode {
    UNPITCHED_STAFF,
    UNPITCHED_STAFF_NOTE_INPUT,
    NEVER,
};

//! Dorico fork (spec 97): what the mouse may edit in the score view, as bit flags. All are allowed by default.
enum InteractionCaps : int {
    INTERACTION_DRAG_NOTES = 1 << 0,    // drag a note up or down to change its pitch
    INTERACTION_DRAG_ITEMS = 1 << 1,    // drag any other item to change its offset
    INTERACTION_GRIPS = 1 << 2,         // handles (slurs, hairpins, lines, ...)
    INTERACTION_EDIT_TEXT = 1 << 3,     // start editing text in the score
    INTERACTION_DROP = 1 << 4,          // drops from palettes and files, Alt-click paste
    INTERACTION_OUTGOING_DRAG = 1 << 5, // drag the selection out to copy it
    INTERACTION_SELECT = 1 << 6,        // click to select, context menus
    INTERACTION_REANCHOR = 1 << 7,      // a drag may move an item to another beat (else it changes offsets only)
    INTERACTION_ALL = (1 << 8) - 1,
};
}
