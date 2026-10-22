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

#include <array>

#include <QHash>
#include <QPointer>
#include <QSet>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"
#include "muse_framework_config.h"
#ifdef MUSE_MODULE_SHORTCUTS_V2
#include "shortcuts_v2/icommandshortcutsregister.h"
#else
#include "shortcuts/ishortcutsregister.h"
#endif

#include "../idoricomenuarranger.h"
#include "../idoricomodestate.h"
#include "doricomenu.h"

namespace mu::dorico {
class DoricoMenuArranger : public IDoricoMenuArranger, public muse::Contextable, public muse::async::Asyncable
{
    muse::GlobalInject<muse::rcommand::ICommandsRegister> commandsRegister;
    muse::ContextInject<muse::rcommand::ICommandsState> commandsState = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };
#ifdef MUSE_MODULE_SHORTCUTS_V2
    muse::GlobalInject<muse::shortcuts::ICommandShortcutsRegister> commandShortcutsRegister;
#else
    muse::ContextInject<muse::shortcuts::IShortcutsRegister> shortcutsRegister = { this };
#endif

public:
    explicit DoricoMenuArranger(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    muse::uicomponents::MenuItemList arrange(const muse::uicomponents::MenuItemList& museScoreMenus, QObject* owner) override;

private:
    muse::uicomponents::MenuItem* build(const MenuSpec& spec, QObject* owner);
    muse::uicomponents::MenuItem* leftovers(muse::uicomponents::MenuItem* item);
    void markUsed(muse::uicomponents::MenuItem* item);

    //! Spec 102: the top-level mode menu. Dorico shows only the current mode's menu, so each mode's items are built
    //! once and the slot swaps between them when the mode changes.
    struct ModeSlot {
        QPointer<muse::uicomponents::MenuItem> item;
        std::array<muse::uicomponents::MenuItemList, 5> items; // by `Mode`; empty for Play and Print
    };
    muse::uicomponents::MenuItem* buildModeSlot(QObject* owner);
    void applyMode(ModeSlot& slot);
    void refresh(const muse::uicomponents::MenuItemList& items);
    std::vector<ModeSlot> m_modeSlots;

    QHash<QString, muse::uicomponents::MenuItem*> m_byId;
    QSet<QString> m_used;
};
}
