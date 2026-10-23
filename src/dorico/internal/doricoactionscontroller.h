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

#include <QElapsedTimer>

#include "global/async/asyncable.h"
#include "modularity/ioc.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "interactive/iinteractive.h"
#include "dockwindow/idockwindowprovider.h"
#include "context/iglobalcontext.h"
#include "notation/inotation.h"
#include "notation/inotationconfiguration.h"
#include "notation/inotationviewstate.h"
#include "project/iexportprojectscenario.h"

#include "../idoricomodestate.h"
#include "doricoshell.h"

namespace mu::dorico {
//! The Dorico UI shell's commands (specs 50, 95): mode switching, zone toggles (Cmd-7/8/9, Cmd-0), toolbar toggle.
//! All modes live in one page; a mode switch shows the new mode's zones and toolboxes and puts back the sizes and
//! visibility the user left that mode with (kept in the settings, so they survive a restart).
class DoricoActionsController : public muse::async::Asyncable, public muse::Contextable, public muse::rcommand::Commandable
{
    muse::ContextInject<muse::rcommand::ICommandDispatcher> commandDispatcher = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<muse::dock::IDockWindowProvider> dockWindowProvider = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<project::IExportProjectScenario> exportProjectScenario = { this };
    muse::GlobalInject<notation::INotationConfiguration> notationConfiguration;

public:
    DoricoActionsController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

private:
    using ZoneKey = std::pair<mu::dorico::Mode, Zone>;

    std::vector<QString> openDocks(mu::dorico::Mode mode, Zone zone) const;
    void toggleZone(Zone zone);
    bool selectLowerZoneTab(const QString& tab);
    void toggleAllZones();
    void toggleToolbar();

    bool isShellShown() const;
    void switchZones(mu::dorico::Mode from, mu::dorico::Mode to);
    void applyZones(mu::dorico::Mode mode);
    void saveZone(mu::dorico::Mode mode, Zone zone) const;
    void onDocksOpenStatusChanged();
    void saveOpenZones() const;
    std::vector<QString> zoneTarget(mu::dorico::Mode mode, Zone zone) const;
    void resetZoneSettings();

    void applyEngraveViewMode(mu::dorico::Mode from, mu::dorico::Mode to);
    void applyPrintZoom(mu::dorico::Mode from, mu::dorico::Mode to);
    void logSwitchTime();

    // The mode whose zones are on screen (the previous mode while modeChanged is handled).
    mu::dorico::Mode m_shownMode = mu::dorico::Mode::Write;
    // The page shows the current mode's zones, so dock open/close by the user is that mode's state.
    bool m_zonesApplied = false;
    // Docks are being opened/closed for a mode switch: their changes are not the user's.
    bool m_switching = false;
    // Restore Default Layout ran: the page reload it queued reports its default docks next, then the zones are reset.
    bool m_restoringLayout = false;
    // Docks that were open when their zone was last hidden, so showing the zone brings back the same panels.
    std::map<ZoneKey, std::vector<QString>> m_remembered;
    // Zones hidden by "Hide Zones", so "Restore Zones" does not reopen zones the user had closed on purpose.
    std::set<ZoneKey> m_hiddenByAll;
    // Print shows a whole page (spec 58); the score's zoom and scroll before Print, put back when leaving it.
    struct ViewBeforePrint {
        std::weak_ptr<notation::INotation> notation;
        muse::draw::Transform matrix;
        notation::ZoomType zoomType;
    };
    std::optional<ViewBeforePrint> m_viewBeforePrint;
    // Mode switch time (spec 95): from the mode command to the next frame on screen.
    QElapsedTimer m_switchTimer;
};
}
