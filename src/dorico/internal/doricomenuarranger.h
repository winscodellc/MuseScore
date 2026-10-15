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

#include <QHash>
#include <QSet>

#include "modularity/ioc.h"
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"

#include "../idoricomenuarranger.h"
#include "doricomenu.h"

namespace mu::dorico {
class DoricoMenuArranger : public IDoricoMenuArranger, public muse::Contextable
{
    muse::GlobalInject<muse::rcommand::ICommandsRegister> commandsRegister;
    muse::ContextInject<muse::rcommand::ICommandsState> commandsState = { this };

public:
    explicit DoricoMenuArranger(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    muse::uicomponents::MenuItemList arrange(const muse::uicomponents::MenuItemList& museScoreMenus, QObject* owner) override;

private:
    muse::uicomponents::MenuItem* build(const MenuSpec& spec, QObject* owner);
    muse::uicomponents::MenuItem* leftovers(muse::uicomponents::MenuItem* item);
    void markUsed(muse::uicomponents::MenuItem* item);

    QHash<QString, muse::uicomponents::MenuItem*> m_byId;
    QSet<QString> m_used;
};
}
