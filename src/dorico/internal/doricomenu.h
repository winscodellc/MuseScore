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

#include <QString>
#include <QStringList>

#include "../idoricomodestate.h"

//! Spec 59: Dorico's menu bar as a table of MuseScore menu item ids (command URIs and MuseScore submenu ids such as
//! "menu-file-open"), Dorico commands, separators and new submenus. Pure data, unit-tested; DoricoMenuArranger turns
//! it into MenuItems.
namespace mu::dorico {
struct MenuSpec {
    QString id;                      // item / submenu id, or "-" for a separator
    QString title;                   // for submenus this table creates (and the ten top-level menus); on an item it replaces the command's title
    std::vector<MenuSpec> children;  // non-empty: a submenu created here

    bool isSeparator() const { return id == "-"; }
    bool isSubmenu() const { return !children.empty(); }
};

//! Spec 102: the menu bar in Dorico's order: File, Edit, View, <mode menu>, Play, Library, Window, Help, Script.
//! The mode menu is the one entry with id MODE_MENU_ID and no children: DoricoMenuArranger fills it with `modeMenu()` of
//! the current mode and swaps it when the mode changes.
const std::vector<MenuSpec>& doricoMenus();

//! Dorico shows only the menu of the current mode: Setup, Write or Engrave. Play mode has none (the Play menu is always
//! there) and neither has Print mode; both return nullptr.
const MenuSpec* modeMenu(Mode mode);
inline const QString MODE_MENU_ID("dorico-menu-mode");

//! Edit > Notations (spec 102), the same submenus as the score context menu (spec 98).
MenuSpec notationsMenu();

//! A stub has no behaviour yet: the arranger shows it disabled with a "Not available yet" tooltip.
inline const QString STUB_ID_PREFIX("dorico-menu-stub-");
inline bool isStubId(const QString& id) { return id.startsWith(STUB_ID_PREFIX) || id.startsWith("dorico-ctx-"); }

//! Edit > Filter (spec 45); the score context menu (spec 98) shares it.
MenuSpec filterMenu();

//! Id of the Window submenu that collects every MuseScore menu item the table does not place, so nothing is lost.
inline const QString MORE_MUSESCORE_MENU_ID("dorico-menu-more-musescore");

//! Every item id the table places (not separators, not the submenus it creates), in table order.
QStringList placedItemIds(const std::vector<MenuSpec>& menus);
}
