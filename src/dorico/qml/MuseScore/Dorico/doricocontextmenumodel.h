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

#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "uicomponents/qml/Muse/UiComponents/abstractmenumodel.h"

#include "../../../idoricomodestate.h"
#include "../../../internal/contextmenu.h"

class QQuickItem;

namespace mu::dorico {
//! Dorico's right-click menus (spec 98): the score's, built from the current selection, and the Setup lists'. Shown as a
//! QMenu so that it looks and behaves like Dorico's native one, not like MuseScore's QML popup.
class DoricoContextMenuModel : public muse::uicomponents::AbstractMenuModel
{
    Q_OBJECT

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };

public:
    explicit DoricoContextMenuModel(QObject* parent = nullptr);

    //! The score's menu at `viewPos` of `view` (the notation paint view); nothing opens when the mode has no score menu.
    Q_INVOKABLE void popup(QQuickItem* view, const QPointF& viewPos);

    //! A Setup list's menu (`kind`: player, flow or layout) at `pos` of `anchor`. Rows the panel handles itself come back
    //! through setupItemChosen; the others are dispatched as commands.
    //! `disabledIds`: rows that do not apply to the clicked entry (the full score cannot be deleted).
    Q_INVOKABLE void popupSetup(const QString& kind, QQuickItem* anchor, const QPointF& pos, const QStringList& disabledIds = {});

signals:
    void setupItemChosen(const QString& kind, const QString& itemId);

private:
    ContextMenuInput inputFromSelection() const;
    muse::uicomponents::MenuItemList build(const std::vector<MenuSpec>& specs);
    void replaceItems(const std::vector<MenuSpec>& specs);
    void show(QQuickItem* anchor, const QPointF& pos, const QString& kind);
};
}
