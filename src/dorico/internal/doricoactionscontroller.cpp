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

#include "doricoactionscontroller.h"

#include "dockwindow/idockwindow.h"
#include "notation/inotationparts.h"
#include "notation/inotationstyle.h"

#include "../doricocommands.h"

#include "log.h"

// Dorico 6 Print mode music area (measured)
static const QColor PRINT_BACKGROUND("#AAAAAE");

using namespace mu::dorico;

void DoricoActionsController::init()
{
    auto cd = commandDispatcher();
    cd->onRequest(this, DORICO_NOOP_COMMAND, []() { LOGI() << "dorico noop"; return muse::make_ok(); });

    // Mode = which page is loaded. Commands only open the page; the URI listener below updates the mode state,
    // so tab clicks, keys, the Score tab and project opening all end up in the same place.
    for (int i = 0; i < MODE_COUNT; ++i) {
        const mu::dorico::Mode mode = static_cast<mu::dorico::Mode>(i);
        cd->onRequest(this, DORICO_MODE_COMMANDS[i], [this, mode]() {
            interactive()->open(modeUri(mode));
            return muse::make_ok();
        });
    }

    interactive()->currentUri().ch.onReceive(this, [this](const muse::Uri& uri) {
        if (auto mode = modeFromUri(uri)) {
            const mu::dorico::Mode previous = modes()->mode();
            modes()->setMode(*mode);
            applyEngraveViewMode(previous, *mode);
            applyPrintBackground(previous, *mode);
        }
    });

    // The Print-mode grey is a stored setting: if the app quit while in Print, put the normal background back.
    if (notationConfiguration()->backgroundColor() == PRINT_BACKGROUND) {
        notationConfiguration()->setBackgroundColor(QColor("#2C4D6F"));
    }

    for (int i = 0; i < ZONE_COUNT; ++i) {
        const Zone zone = static_cast<Zone>(i);
        cd->onRequest(this, DORICO_TOGGLE_ZONE_COMMANDS[i], [this, zone]() {
            toggleZone(zone);
            return muse::make_ok();
        });
    }
    // MuseScore's panel toggles (F3, F8, View menu, ...) select a lower-zone tab instead of toggling a dock that no longer exists.
    cd->preDispatch().onReceive(this, [this](const muse::rcommand::Command& command, bool* allowDispatch) {
        const auto tab = lowerZoneTabForCommand(command.toString());
        if (tab && selectLowerZoneTab(*tab)) {
            *allowDispatch = false;
        }
    });

    cd->onRequest(this, DORICO_TOGGLE_ALL_ZONES_COMMAND, [this]() { toggleAllZones(); return muse::make_ok(); });
    cd->onRequest(this, DORICO_TOGGLE_TOOLBAR_COMMAND, [this]() { toggleToolbar(); return muse::make_ok(); });
    cd->onRequest(this, DORICO_OPEN_HUB_COMMAND, [this]() { interactive()->open("musescore://home"); return muse::make_ok(); });
    cd->onRequest(this, DORICO_TOGGLE_STAVE_SHARING_COMMAND, [this]() {
        // Same call as MuseScore's instrument-tree switch (LayoutPanelTreeModel::toggleStaveSharing)
        if (const notation::INotationPtr notation = globalContext()->currentNotation()) {
            notation->parts()->toggleStaveSharing(!notation->style()->styleValue(engraving::Sid::enableStaveSharing).toBool());
        }
        return muse::make_ok();
    });
    cd->onRequest(this, DORICO_OPEN_PUBLISH_COMMAND, [this]() { interactive()->open("musescore://publish"); return muse::make_ok(); });
}

std::vector<QString> DoricoActionsController::openDocks(Zone zone) const
{
    std::vector<QString> result;
    const muse::dock::IDockWindow* window = dockWindowProvider()->window();
    if (!window) {
        return result;
    }
    for (const QString& name : zoneDocks(modes()->mode(), zone)) {
        if (window->isDockOpen(name)) {
            result.push_back(name);
        }
    }
    return result;
}

void DoricoActionsController::toggleZone(Zone zone)
{
    muse::dock::IDockWindow* window = dockWindowProvider()->window();
    const std::vector<QString> all = zoneDocks(modes()->mode(), zone);
    if (!window || all.empty()) {
        return;
    }

    const ZoneKey key { modes()->mode(), zone };
    m_hiddenByAll.erase(key);

    const std::vector<QString> open = openDocks(zone);
    if (!open.empty()) {
        m_remembered[key] = open;
        for (const QString& name : open) {
            window->setDockOpen(name, false);
        }
        return;
    }

    auto it = m_remembered.find(key);
    const std::vector<QString> toOpen = (it != m_remembered.end() && !it->second.empty()) ? it->second : std::vector<QString> { all.front() };
    for (const QString& name : toOpen) {
        window->setDockOpen(name, true);
    }
}

bool DoricoActionsController::selectLowerZoneTab(const QString& tab)
{
    muse::dock::IDockWindow* window = dockWindowProvider()->window();
    const std::vector<QString> docks = zoneDocks(modes()->mode(), Zone::Lower);
    if (!window || docks.size() != 1 || docks.front() != pageDockName(modes()->mode(), LOWER_ZONE_PANEL)) {
        return false;
    }

    const QString dock = docks.front();
    if (window->isDockOpen(dock) && modes()->lowerZoneTab() == tab) {
        window->setDockOpen(dock, false);
    } else {
        modes()->setLowerZoneTab(tab);
        window->setDockOpen(dock, true);
    }
    return true;
}

void DoricoActionsController::toggleAllZones()
{
    muse::dock::IDockWindow* window = dockWindowProvider()->window();
    if (!window) {
        return;
    }

    const mu::dorico::Mode mode = modes()->mode();

    bool anyOpen = false;
    for (int i = 0; i < ZONE_COUNT; ++i) {
        const Zone zone = static_cast<Zone>(i);
        const ZoneKey key { mode, zone };
        const std::vector<QString> open = openDocks(zone);
        if (open.empty()) {
            continue;
        }
        anyOpen = true;
        m_remembered[key] = open;
        m_hiddenByAll.insert(key);
        for (const QString& name : open) {
            window->setDockOpen(name, false);
        }
    }

    if (anyOpen) {
        return;
    }

    for (int i = 0; i < ZONE_COUNT; ++i) {
        const ZoneKey key { mode, static_cast<Zone>(i) };
        if (!m_hiddenByAll.erase(key)) {
            continue;
        }
        for (const QString& name : m_remembered[key]) {
            window->setDockOpen(name, true);
        }
    }
}

void DoricoActionsController::toggleToolbar()
{
    muse::dock::IDockWindow* window = dockWindowProvider()->window();
    if (!window) {
        return;
    }

    const std::vector<QString> names = toolbarDocks(modes()->mode());
    bool anyOpen = false;
    for (const QString& name : names) {
        anyOpen = anyOpen || window->isDockOpen(name);
    }
    for (const QString& name : names) {
        window->setDockOpen(name, !anyOpen);
    }
}

void DoricoActionsController::applyPrintBackground(mu::dorico::Mode from, mu::dorico::Mode to)
{
    // Dorico shows Print mode's preview on grey (measured #AAAAAE). MuseScore has one background setting, so it is
    // swapped while Print is open and restored on the way out.
    if (from == to) {
        return;
    }
    if (to == mu::dorico::Mode::Print) {
        m_backgroundBeforePrint = notationConfiguration()->backgroundColor();
        notationConfiguration()->setBackgroundColor(PRINT_BACKGROUND);
    } else if (from == mu::dorico::Mode::Print && m_backgroundBeforePrint) {
        notationConfiguration()->setBackgroundColor(*m_backgroundBeforePrint);
        m_backgroundBeforePrint.reset();
    }
}

void DoricoActionsController::applyEngraveViewMode(mu::dorico::Mode from, mu::dorico::Mode to)
{
    const notation::INotationPtr notation = globalContext()->currentNotation();
    if (!notation || from == to) {
        return;
    }

    if (to == mu::dorico::Mode::Engrave) {
        m_viewModeBeforeEngrave = notation->viewMode();
        notation->setViewMode(notation::ViewMode::PAGE);
    } else if (from == mu::dorico::Mode::Engrave && m_viewModeBeforeEngrave) {
        notation->setViewMode(*m_viewModeBeforeEngrave);
        m_viewModeBeforeEngrave.reset();
    }
}
