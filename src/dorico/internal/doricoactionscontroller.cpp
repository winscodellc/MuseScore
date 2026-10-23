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

#include <algorithm>

#include <QQuickWindow>

#include "dockwindow/idockwindow.h"
#include "async/async.h"
#include "settings.h"
#include "notation/inotationparts.h"
#include "notation/inotationstyle.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"
#include "engraving/dom/engravingitem.h"

#include "../doricocommands.h"

#include "log.h"

// Dorico 6 Print mode music area (measured). Spec 95 paints it as a view override; older builds stored it as the
// background setting, which init() repairs.
static const QColor PRINT_BACKGROUND("#AAAAAE");

static const muse::Uri DAC_NOTATION_PAGE_URI("musescore://notation");
static const muse::rcommand::Command DAC_RESTORE_LAYOUT_COMMAND("command://dock/restore-default-layout");

static muse::Settings::Key dacZoneKey(mu::dorico::Mode mode, mu::dorico::Zone zone, const char* field)
{
    return muse::Settings::Key("dorico", mu::dorico::zoneSettingsKey(mode, zone, field));
}

static bool dacContains(const std::vector<QString>& names, const QString& name)
{
    return std::find(names.begin(), names.end(), name) != names.end();
}

using namespace mu::dorico;

void DoricoActionsController::init()
{
    auto cd = commandDispatcher();
    cd->onRequest(this, DORICO_NOOP_COMMAND, []() { LOGI() << "dorico noop"; return muse::make_ok(); });

    // All modes are one page (spec 95): a mode command only changes the mode; the zones follow in modeChanged.
    for (int i = 0; i < MODE_COUNT; ++i) {
        const mu::dorico::Mode mode = static_cast<mu::dorico::Mode>(i);
        cd->onRequest(this, DORICO_MODE_COMMANDS[i], [this, mode]() {
            m_switchTimer.start();
            modes()->setMode(mode);
            if (interactive()->currentUri().val != DAC_NOTATION_PAGE_URI) {
                interactive()->open(DAC_NOTATION_PAGE_URI);
            }
            return muse::make_ok();
        });
    }

    m_shownMode = modes()->mode();
    modes()->modeChanged().onNotify(this, [this]() {
        const mu::dorico::Mode from = m_shownMode;
        const mu::dorico::Mode to = modes()->mode();
        m_shownMode = to;
        switchZones(from, to);
        applyEngraveViewMode(from, to);
        applyPrintZoom(from, to);
        logSwitchTime();
    });

    // The page's restored layout may show another mode's zones (or a stale layout after a crash): show the current
    // mode's. Later, so currentUri().val is the new page and its layout is restored.
    interactive()->currentUri().ch.onReceive(this, [this](const muse::Uri& uri) {
        m_zonesApplied = false;
        if (uri != DAC_NOTATION_PAGE_URI) {
            return;
        }
        muse::async::Async::call(this, [this]() {
            applyZones(modes()->mode());
            m_zonesApplied = isShellShown();
        });
    });

    // A zone shown or hidden by the user (Cmd-7/8/9, View menu, toolbox buttons ...) is remembered for this mode.
    if (muse::dock::IDockWindow* window = dockWindowProvider()->window()) {
        window->docksOpenStatusChanged().onReceive(this, [this](const QStringList&) { onDocksOpenStatusChanged(); });
    } else {
        dockWindowProvider()->windowChanged().onNotify(this, [this]() {
            if (muse::dock::IDockWindow* window = dockWindowProvider()->window()) {
                window->docksOpenStatusChanged().onReceive(this, [this](const QStringList&) { onDocksOpenStatusChanged(); });
            }
        });
    }

    // Engrave is always page view: a score opened or switched to while in Engrave gets it too. Later, so it runs
    // after the score view's own onLoadNotation, which sets the score's saved view mode.
    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        muse::async::Async::call(this, [this]() {
            const notation::INotationPtr notation = globalContext()->currentNotation();
            // ponytail: one extra layout when a galley score is opened in Engrave
            if (notation && modes()->mode() == mu::dorico::Mode::Engrave && notation->viewMode() != notation::ViewMode::PAGE) {
                notation->setViewMode(notation::ViewMode::PAGE);
            }
        });
    });

    // The Print-mode grey was a stored setting before spec 95: if the app quit while in Print, put the normal background back.
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
    cd->preDispatch().onReceive(this, [this](const muse::rcommand::Command& command, bool* allowDispatch) {
        // MuseScore's panel toggles (F3, F8, View menu, ...) select a lower-zone tab instead of toggling a dock that no longer exists.
        const auto tab = lowerZoneTabForCommand(command.toString());
        if (tab && selectLowerZoneTab(*tab)) {
            *allowDispatch = false;
        }
        // Restore Default Layout: every mode starts again from its default zones, once the page reload it queues has
        // put the QML default docks back (see onDocksOpenStatusChanged).
        if (command == DAC_RESTORE_LAYOUT_COMMAND) {
            m_restoringLayout = true;
        }
    });

    cd->onRequest(this, DORICO_TOGGLE_ALL_ZONES_COMMAND, [this]() { toggleAllZones(); return muse::make_ok(); });
    cd->onRequest(this, DORICO_TOGGLE_TOOLBAR_COMMAND, [this]() { toggleToolbar(); return muse::make_ok(); });
    cd->onRequest(this, DORICO_OPEN_HUB_COMMAND, [this]() { interactive()->open("musescore://home"); return muse::make_ok(); });
    cd->onRequest(this, DORICO_NOTATION_OPTIONS_COMMAND, [this]() {
        commandDispatcher()->dispatch(muse::rcommand::Command("command://notation/open-edit-style"));
        return muse::make_ok();
    });
    // The export dialog opens on the format of the last export: set it first
    const std::pair<muse::rcommand::Command, const char*> exports[] = {
        { DORICO_EXPORT_AUDIO_COMMAND, "wav" }, { DORICO_EXPORT_MIDI_COMMAND, "mid" }, { DORICO_EXPORT_MUSICXML_COMMAND, "mxl" },
    };
    for (const auto& [command, format] : exports) {
        cd->onRequest(this, command, [this, format]() {
            project::ExportInfo info = exportProjectScenario()->exportInfo();
            info.id = format;
            exportProjectScenario()->setExportInfo(info);
            commandDispatcher()->dispatch(muse::rcommand::Command("command://project/export"));
            return muse::make_ok();
        });
    }
    cd->onRequest(this, DORICO_TOGGLE_STAVE_SHARING_COMMAND, [this]() {
        // Same call as MuseScore's instrument-tree switch (LayoutPanelTreeModel::toggleStaveSharing)
        if (const notation::INotationPtr notation = globalContext()->currentNotation()) {
            notation->parts()->toggleStaveSharing(!notation->style()->styleValue(engraving::Sid::enableStaveSharing).toBool());
        }
        return muse::make_ok();
    });
    // Engrave mode (spec 97): Dorico 6 nudges items graphically but still changes the pitch of selected notes
    for (const auto& [command, direction] : { std::pair { DORICO_NUDGE_LEFT_COMMAND, notation::MoveDirection::Left },
                                              std::pair { DORICO_NUDGE_RIGHT_COMMAND, notation::MoveDirection::Right },
                                              std::pair { DORICO_NUDGE_UP_COMMAND, notation::MoveDirection::Up },
                                              std::pair { DORICO_NUDGE_DOWN_COMMAND, notation::MoveDirection::Down } }) {
        cd->onRequest(this, command, [this, direction]() {
            const notation::INotationPtr notation = globalContext()->currentNotation();
            if (!notation) {
                return muse::make_ok();
            }
            const notation::INotationInteractionPtr interaction = notation->interaction();
            const std::vector<engraving::EngravingItem*>& items = interaction->selection()->elements();
            const bool notes = !items.empty() && std::all_of(items.begin(), items.end(), [](const engraving::EngravingItem* e) {
                return e->isNote();
            });
            const bool vertical = direction == notation::MoveDirection::Up || direction == notation::MoveDirection::Down;
            if (notes && vertical) {
                interaction->movePitch(direction, notation::PitchMode::DIATONIC);
            } else {
                interaction->nudge(direction, false);
            }
            return muse::make_ok();
        });
    }
    cd->onRequest(this, DORICO_OPEN_PUBLISH_COMMAND, [this]() { interactive()->open("musescore://publish"); return muse::make_ok(); });
    cd->onRequest(this, DORICO_PREFERENCES_COMMAND, [this]() {
        // ponytail: upstream waits for playback to stop before opening; we stop it and open at once
        if (globalContext()->playbackState() && globalContext()->playbackState()->isPlaying()) {
            commandDispatcher()->dispatch(muse::rcommand::Command("command://playback/stop"));
        }
        interactive()->open("musescore://dorico/preferences");
        return muse::make_ok();
    });
    cd->onRequest(this, DORICO_ADD_MARKER_COMMAND, [this]() { interactive()->open("musescore://dorico/addmarker"); return muse::make_ok(); });
    cd->onRequest(this, DORICO_VIDEO_PROPERTIES_COMMAND, [this]() {
        interactive()->open("musescore://dorico/videoproperties");
        return muse::make_ok();
    });
}

bool DoricoActionsController::isShellShown() const
{
    return dockWindowProvider()->window() && interactive()->currentUri().val == DAC_NOTATION_PAGE_URI;
}

std::vector<QString> DoricoActionsController::openDocks(mu::dorico::Mode mode, Zone zone) const
{
    std::vector<QString> result;
    const muse::dock::IDockWindow* window = dockWindowProvider()->window();
    if (!window) {
        return result;
    }
    for (const QString& name : zoneDocks(mode, zone)) {
        if (window->isDockOpen(name)) {
            result.push_back(name);
        }
    }
    return result;
}

void DoricoActionsController::onDocksOpenStatusChanged()
{
    // The reload after Restore Default Layout reports the QML default docks: put the modes' defaults on top of them
    // instead of saving them.
    if (m_restoringLayout) {
        m_restoringLayout = false;
        resetZoneSettings();
        modes()->zonesReset().notify();
        return;
    }
    saveOpenZones();
}

void DoricoActionsController::saveOpenZones() const
{
    if (!m_zonesApplied || m_switching || !isShellShown()) {
        return;
    }
    for (int i = 0; i < ZONE_COUNT; ++i) {
        saveZone(modes()->mode(), static_cast<Zone>(i));
    }
}

void DoricoActionsController::saveZone(mu::dorico::Mode mode, Zone zone) const
{
    const std::vector<QString> all = zoneDocks(mode, zone);
    if (all.empty()) {
        return;
    }

    const std::vector<QString> open = openDocks(mode, zone);
    QStringList names(open.begin(), open.end());
    muse::settings()->setSharedValue(dacZoneKey(mode, zone, "docks"), muse::Val(names.join(',').toStdString()));
}

std::vector<QString> DoricoActionsController::zoneTarget(mu::dorico::Mode mode, Zone zone) const
{
    const std::vector<QString> all = zoneDocks(mode, zone);
    if (all.empty()) {
        return {};
    }

    const muse::Val saved = muse::settings()->value(dacZoneKey(mode, zone, "docks"));
    if (saved.isNull()) {
        return zoneOpenByDefault(mode, zone) ? std::vector<QString> { all.front() } : std::vector<QString> {};
    }

    std::vector<QString> result;
    for (const QString& name : QString::fromStdString(saved.toString()).split(',', Qt::SkipEmptyParts)) {
        if (dacContains(all, name)) {
            result.push_back(name);
        }
    }
    return result;
}

void DoricoActionsController::switchZones(mu::dorico::Mode from, mu::dorico::Mode to)
{
    if (!isShellShown()) {
        return; // the notation page is not shown: the currentUri listener applies the zones when it comes back
    }

    // Zone sizes are the page's job (DoricoShellPage: saved on modeAboutToChange, put back after the switch).
    for (int i = 0; i < ZONE_COUNT; ++i) {
        saveZone(from, static_cast<Zone>(i));
    }
    applyZones(to);
}

void DoricoActionsController::applyZones(mu::dorico::Mode mode)
{
    muse::dock::IDockWindow* window = dockWindowProvider()->window();
    if (!window || !isShellShown()) {
        return;
    }

    std::vector<QString> target = toolboxDocks(mode);
    for (int i = 0; i < ZONE_COUNT; ++i) {
        const std::vector<QString> docks = zoneTarget(mode, static_cast<Zone>(i));
        target.insert(target.end(), docks.begin(), docks.end());
    }

    // Close first, then open. A dock already in the right state is not touched, so a zone open in both modes keeps
    // its dock (only its content changes page).
    const bool wasSwitching = m_switching;
    m_switching = true;
    const std::vector<QString> all = managedDocks();
    for (const QString& name : all) {
        if (!dacContains(target, name) && window->isDockOpen(name)) {
            window->setDockOpen(name, false);
        }
    }
    for (const QString& name : target) {
        if (!window->isDockOpen(name)) {
            window->setDockOpen(name, true);
        }
    }
    m_switching = wasSwitching;
}

void DoricoActionsController::resetZoneSettings()
{
    for (int m = 0; m < MODE_COUNT; ++m) {
        const mu::dorico::Mode mode = static_cast<mu::dorico::Mode>(m);
        for (int z = 0; z < ZONE_COUNT; ++z) {
            const Zone zone = static_cast<Zone>(z);
            const std::vector<QString> all = zoneDocks(mode, zone);
            const QString docks = zoneOpenByDefault(mode, zone) && !all.empty() ? all.front() : QString();
            muse::settings()->setSharedValue(dacZoneKey(mode, zone, "docks"), muse::Val(docks.toStdString()));
            muse::settings()->setSharedValue(dacZoneKey(mode, zone, "size"), muse::Val(zoneDefaultSize(mode, zone)));
        }
    }
    m_remembered.clear();
    m_hiddenByAll.clear();

    applyZones(modes()->mode());
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

    const std::vector<QString> open = openDocks(modes()->mode(), zone);
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
    // Setup's lower zone is the Flows panel and Print has none: MuseScore's toggle goes through unchanged there.
    muse::dock::IDockWindow* window = dockWindowProvider()->window();
    const mu::dorico::Mode mode = modes()->mode();
    if (!window || mode == mu::dorico::Mode::Setup || zoneDocks(mode, Zone::Lower).empty()) {
        return false;
    }

    const QString dock = LOWER_ZONE_PANEL;
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
        const std::vector<QString> open = openDocks(mode, zone);
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

    const std::vector<QString> names = toolbarDocks();
    bool anyOpen = false;
    for (const QString& name : names) {
        anyOpen = anyOpen || window->isDockOpen(name);
    }
    for (const QString& name : names) {
        window->setDockOpen(name, !anyOpen);
    }
}

void DoricoActionsController::applyPrintZoom(mu::dorico::Mode from, mu::dorico::Mode to)
{
    // Print opens on a whole page (DoricoShellPage); the score view is shared, so the zoom and scroll the score had
    // are put back on the way out.
    const notation::INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }
    if (to == mu::dorico::Mode::Print) {
        m_viewBeforePrint = ViewBeforePrint { notation, notation->viewState()->matrix(), notation->viewState()->zoomType().val };
    } else if (from == mu::dorico::Mode::Print && m_viewBeforePrint) {
        // Only on the score it was saved from: another layout or project may be shown now.
        if (m_viewBeforePrint->notation.lock() == notation) {
            notation->viewState()->setZoomType(m_viewBeforePrint->zoomType);
            notation->viewState()->setMatrix(m_viewBeforePrint->matrix, nullptr);
        }
        m_viewBeforePrint.reset();
    }
}

void DoricoActionsController::applyEngraveViewMode(mu::dorico::Mode from, mu::dorico::Mode to)
{
    // Engrave is always page view (spec 56). Only the painting changes: the score's own view mode (viewState, saved
    // in the file) stays, and leaving Engrave goes back to it. setViewMode lays out only when the mode changes.
    const notation::INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }

    if (to == mu::dorico::Mode::Engrave) {
        notation->setViewMode(notation::ViewMode::PAGE);
    } else if (from == mu::dorico::Mode::Engrave) {
        notation->setViewMode(notation->viewState()->viewMode());
    }
}

void DoricoActionsController::logSwitchTime()
{
    const muse::dock::IDockWindow* window = dockWindowProvider()->window();
    QQuickWindow* quickWindow = window ? window->asItem().window() : nullptr;
    if (!quickWindow || !m_switchTimer.isValid()) {
        return;
    }
    QObject::connect(quickWindow, &QQuickWindow::frameSwapped, quickWindow, [this]() {
        if (!m_switchTimer.isValid()) { // two switches before one frame: the first lambda logged it
            return;
        }
        LOGI() << "Dorico mode switch: " << m_switchTimer.elapsed() << " ms";
        m_switchTimer.invalidate();
    }, Qt::SingleShotConnection);
}
