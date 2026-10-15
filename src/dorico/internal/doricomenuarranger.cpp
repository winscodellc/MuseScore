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

#include "doricomenuarranger.h"

#include "log.h"
#include "types/translatablestring.h"

using namespace mu::dorico;
using namespace muse::uicomponents;

static bool isSeparator(const MenuItem* item)
{
    return item && item->id().isEmpty() && item->subitems().isEmpty();
}

static void index(const MenuItemList& items, QHash<QString, MenuItem*>& byId)
{
    for (MenuItem* item : items) {
        if (!item || isSeparator(item)) {
            continue;
        }
        if (!byId.contains(item->id())) {
            byId.insert(item->id(), item);
        }
        index(item->subitems(), byId);
    }
}

// Drops leading, trailing and doubled separators (left behind when items moved elsewhere).
static MenuItemList tidy(const MenuItemList& items)
{
    MenuItemList result;
    for (MenuItem* item : items) {
        if (!item || (isSeparator(item) && (result.isEmpty() || isSeparator(result.last())))) {
            continue;
        }
        result << item;
    }
    while (!result.isEmpty() && isSeparator(result.last())) {
        result.removeLast();
    }
    return result;
}

void DoricoMenuArranger::markUsed(MenuItem* item)
{
    m_used.insert(item->id());
    for (MenuItem* sub : item->subitems()) {
        if (sub && !isSeparator(sub)) {
            markUsed(sub);
        }
    }
}

MenuItem* DoricoMenuArranger::build(const MenuSpec& spec, QObject* owner)
{
    if (spec.isSeparator()) {
        auto* separator = new MenuItem(owner);
        separator->setTitle({});
        return separator;
    }

    if (spec.isSubmenu()) {
        MenuItemList children;
        for (const MenuSpec& child : spec.children) {
            children << build(child, owner);
        }
        children = tidy(children);
        if (children.isEmpty()) {
            return nullptr;
        }
        auto* submenu = new MenuItem(owner);
        submenu->setId(spec.id);
        submenu->setTitle(muse::TranslatableString("dorico", spec.title.toUtf8().constData()));
        submenu->setSubitems(children);
        return submenu;
    }

    // An existing MuseScore item is moved here (once); a command placed twice, or one MuseScore's menus do not
    // have (the Dorico commands), gets a new item, built like AbstractMenuModel::makeMenuItem does.
    MenuItem* existing = m_byId.value(spec.id);
    if (existing && !m_used.contains(spec.id)) {
        markUsed(existing);
        return existing;
    }

    const muse::rcommand::Command command(spec.id.toStdString());
    const muse::rcommand::CommandInfo& info = commandsRegister()->commandInfo(command);
    if (!info.isValid()) {
        if (!existing) {
            LOGW() << "Dorico menu: no menu item or command for " << spec.id;
        }
        return nullptr;
    }

    auto* item = new MenuItem(info, owner);
    item->setCommandQuery(muse::rcommand::CommandQuery(command));
    item->setCommandState(commandsState()->commandState(command));
    m_used.insert(spec.id);
    return item;
}

MenuItem* DoricoMenuArranger::leftovers(MenuItem* item)
{
    if (!item) {
        return nullptr;
    }
    if (isSeparator(item)) {
        return item;
    }
    if (m_used.contains(item->id())) {
        return nullptr;
    }
    if (item->subitems().isEmpty()) {
        return item;
    }

    MenuItemList kept;
    for (MenuItem* sub : item->subitems()) {
        kept << leftovers(sub);
    }
    kept = tidy(kept);
    if (kept.isEmpty()) {
        return nullptr;
    }
    item->setSubitems(kept);
    return item;
}

MenuItemList DoricoMenuArranger::arrange(const MenuItemList& museScoreMenus, QObject* owner)
{
    m_byId.clear();
    m_used.clear();
    index(museScoreMenus, m_byId);

    MenuItemList result;
    for (const MenuSpec& spec : doricoMenus()) {
        if (MenuItem* menu = build(spec, owner)) {
            result << menu;
        }
    }

    // Whatever the table did not place stays reachable, grouped by its old MuseScore menu.
    MenuItemList rest;
    for (MenuItem* menu : museScoreMenus) {
        if (MenuItem* kept = leftovers(menu)) {
            rest << kept;
        }
    }
    if (!rest.isEmpty()) {
        for (MenuItem* menu : result) {
            if (menu->id() == "dorico-menu-window") {
                auto* more = new MenuItem(owner);
                more->setId(MORE_MUSESCORE_MENU_ID);
                more->setTitle(muse::TranslatableString("dorico", "More MuseScore Commands"));
                more->setSubitems(rest);

                MenuItemList windowItems = menu->subitems();
                auto* separator = new MenuItem(owner);
                separator->setTitle({});
                windowItems << separator << more;
                menu->setSubitems(windowItems);
                break;
            }
        }
    }

    return result;
}
