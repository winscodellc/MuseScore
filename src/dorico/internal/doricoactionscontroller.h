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

#include <map>
#include <optional>
#include <set>
#include <vector>

#include "global/async/asyncable.h"
#include "modularity/ioc.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "interactive/iinteractive.h"
#include "dockwindow/idockwindowprovider.h"
#include "context/iglobalcontext.h"
#include "notation/inotation.h"

#include "../idoricomodestate.h"
#include "doricoshell.h"

namespace mu::dorico {
class DoricoActionsController : public muse::async::Asyncable, public muse::Contextable, public muse::rcommand::Commandable
{
    muse::ContextInject<muse::rcommand::ICommandDispatcher> commandDispatcher = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<muse::dock::IDockWindowProvider> dockWindowProvider = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    DoricoActionsController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

private:
    using ZoneKey = std::pair<mu::dorico::Mode, Zone>;

    std::vector<QString> openDocks(Zone zone) const;
    void toggleZone(Zone zone);
    bool selectLowerZoneTab(const QString& tab);
    void toggleAllZones();
    void toggleToolbar();
    void applyEngraveViewMode(mu::dorico::Mode from, mu::dorico::Mode to);

    // Docks that were open when their zone was last hidden, so showing the zone brings back the same panels.
    std::map<ZoneKey, std::vector<QString>> m_remembered;
    // Zones hidden by "Hide Zones", so "Restore Zones" does not reopen zones the user had closed on purpose.
    std::set<ZoneKey> m_hiddenByAll;
    // Engrave is always page view (spec 56); the view mode the score had before, restored when leaving Engrave.
    std::optional<notation::ViewMode> m_viewModeBeforeEngrave;
};
}
