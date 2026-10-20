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

//! The ten top-level menus in Dorico's order: File, Edit, Setup, Write, Engrave, Play, Library, View, Window, Help.
const std::vector<MenuSpec>& doricoMenus();

//! Id of the Window submenu that collects every MuseScore menu item the table does not place, so nothing is lost.
inline const QString MORE_MUSESCORE_MENU_ID("dorico-menu-more-musescore");

//! Every item id the table places (not separators, not the submenus it creates), in table order.
QStringList placedItemIds(const std::vector<MenuSpec>& menus);
}
