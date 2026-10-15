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

#include <QObject>

#include "modularity/imoduleinterface.h"
#include "uicomponents/qml/Muse/UiComponents/menuitem.h"

namespace mu::dorico {
//! Spec 59: regroups MuseScore's menu bar into Dorico's menus. AppMenuModel::load() (one upstream hook) passes its
//! top-level menus here when the Dorico module is built; otherwise nothing is registered and the menus stay MuseScore's.
class IDoricoMenuArranger : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IDoricoMenuArranger)

public:
    virtual ~IDoricoMenuArranger() = default;

    //! Returns the menus to show instead of `museScoreMenus`. Existing items are reused (moved), so AppMenuModel's
    //! own refreshers (recent files, extensions, workspaces, undo/redo) keep working. New items are parented to `owner`.
    virtual muse::uicomponents::MenuItemList arrange(const muse::uicomponents::MenuItemList& museScoreMenus, QObject* owner) = 0;
};
}
