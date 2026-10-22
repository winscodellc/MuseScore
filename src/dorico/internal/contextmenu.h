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

#include <vector>

#include "../idoricomodestate.h"
#include "doricomenu.h"

//! Spec 98: Dorico's right-click menus as tables of MenuSpec. Pure data, unit-tested; DoricoContextMenuModel turns them
//! into a popup. Row ids:
//!  - `command://...`: a registered command, enabled by its command state;
//!  - `dorico-setup-*`: a Setup-list action the panel handles itself (enabled);
//!  - `dorico-ctx-*`: a stub, shown disabled ("Not available yet").
namespace mu::dorico {
//! What the selection holds. A range selection sets notes, dynamics and text: Dorico shows the union of the types.
struct ContextMenuInput {
    Mode mode = Mode::Write;
    bool hasSelection = false;
    bool notes = false;        // notes, chords and rests
    bool dynamics = false;
    bool slurs = false;
    bool text = false;         // tempo or other text
    bool clipboardHasMusic = false;
};

//! The score's menu. Empty in a mode with no score menu (Setup, Play, Print).
std::vector<MenuSpec> scoreContextMenu(const ContextMenuInput& in);

//! Setup mode's lists.
std::vector<MenuSpec> playerContextMenu();
std::vector<MenuSpec> flowContextMenu();
std::vector<MenuSpec> layoutContextMenu();
}
