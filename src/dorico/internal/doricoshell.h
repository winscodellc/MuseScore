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

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <QString>

#include "types/uri.h"
#include "translation.h"
#include "project/inotationwriter.h"

#include "../idoricomodestate.h"

//! Pure helpers for the UI shell (spec 50): which page URI belongs to a mode, and which dock names make up a zone.
//! No UI and no services, so they are unit-tested directly.
namespace mu::dorico {
enum class Zone {
    Left,
    Lower,
    Right
};

inline constexpr int MODE_COUNT = 5;
inline constexpr int ZONE_COUNT = 3;

// Write keeps MuseScore's own page URI so the Score tab and project opening need no change.
inline muse::Uri modeUri(Mode mode)
{
    switch (mode) {
    case Mode::Setup: return muse::Uri("musescore://dorico/setup");
    case Mode::Write: return muse::Uri("musescore://notation");
    case Mode::Engrave: return muse::Uri("musescore://dorico/engrave");
    case Mode::Play: return muse::Uri("musescore://dorico/play");
    case Mode::Print: return muse::Uri("musescore://dorico/print");
    }
    return {};
}

inline std::optional<Mode> modeFromUri(const muse::Uri& uri)
{
    for (int i = 0; i < MODE_COUNT; ++i) {
        const Mode mode = static_cast<Mode>(i);
        if (modeUri(mode) == uri) {
            return mode;
        }
    }
    return std::nullopt;
}

// The single lower-zone dock (spec 54); its tabs are chosen by IDoricoModeState::lowerZoneTab().
inline constexpr const char* LOWER_ZONE_PANEL = "lowerZonePanel";

// Docks every mode page has (top bars, status bar, lower zone) need a distinct name per page: KDDockWidgets looks docks
// up by name when it restores a page layout, so a shared name lets one page grab another page's dock (broken layout,
// crash on mode switch). Write keeps MuseScore's names (the View menu toggles them); the other pages prefix their
// objectName, like PublishPage does. Must match DoricoPage.qml dockName().
inline QString pageDockName(Mode mode, const QString& name)
{
    switch (mode) {
    case Mode::Write: return name;
    case Mode::Setup: return "DoricoSetup_" + name;
    case Mode::Engrave: return "DoricoEngrave_" + name;
    case Mode::Play: return "DoricoPlay_" + name;
    case Mode::Print: return "DoricoPrint_" + name;
    }
    return name;
}

// MuseScore's own panel toggles that now select a lower-zone tab instead (spec 54).
inline std::optional<QString> lowerZoneTabForCommand(const std::string& uri)
{
    static const std::pair<const char*, const char*> table[] = {
        { "command://app/dock/toggle-properties", "properties" },
        { "command://app/dock/toggle-piano-keyboard", "keyboard" },
        { "command://app/dock/toggle-percussion", "drumPads" },
        { "command://app/dock/toggle-mixer", "mixer" },
        { "command://app/dock/toggle-timeline", "timeline" },
    };
    for (const auto& [command, tab] : table) {
        if (uri == command) {
            return QString::fromLatin1(tab);
        }
    }
    return std::nullopt;
}

// Dock names are the ones in appshell/appshelltypes.h (not included: that header is upstream QML-module-internal)
// and the objectNames of the Dorico pages' DockPanels.
inline std::vector<QString> zoneDocks(Mode mode, Zone zone)
{
    switch (mode) {
    case Mode::Setup:
        switch (zone) {
        case Zone::Left: return { "playersPanel" };
        case Zone::Right: return { "layoutsPanel" };
        case Zone::Lower: return { "flowsPanel" };
        }
        break;
    case Mode::Write:
        // The first dock of a zone is the one a closed zone opens with (when nothing was open before).
        switch (zone) {
        case Zone::Left: return { "notesPanel" };
        case Zone::Right: return { "notationsPanel", "palettesPanel", "selectionFiltersPanel", "undoHistoryPanel" };
        case Zone::Lower: return { LOWER_ZONE_PANEL };
        }
        break;
    case Mode::Print:
        switch (zone) {
        case Zone::Left: return { "printLayoutsPanel" };
        case Zone::Right: return { "printOptionsPanel" };
        case Zone::Lower: return {};
        }
        break;
    case Mode::Engrave:
        switch (zone) {
        case Zone::Left: return { "engraveLeftPanel" };
        case Zone::Right: return { "pagesPanel" };
        case Zone::Lower: return { pageDockName(mode, LOWER_ZONE_PANEL) };
        }
        break;
    case Mode::Play:
        // No right zone in Play mode.
        switch (zone) {
        case Zone::Left: return { "playLeftPanel" };
        case Zone::Lower: return { pageDockName(mode, LOWER_ZONE_PANEL) };
        case Zone::Right: return {};
        }
        break;
    }
    return {};
}

// Subtitle of a layout card in Setup/Print (specs 55, 58): page size, orientation and staff space, as Dorico shows them.
inline QString layoutPageInfo(double widthInch, double heightInch, double spacePt)
{
    const double mm = 25.4;
    const QString orientation = widthInch > heightInch ? muse::qtrc("dorico", "landscape") : muse::qtrc("dorico", "portrait");
    return QString("%1 x %2 mm, %3, %4 pt").arg(qRound(widthInch * mm)).arg(qRound(heightInch * mm)).arg(orientation)
           .arg(spacePt, 0, 'f', 1);
}

// Print mode (spec 58): how the export writers split the chosen layouts. PNG and SVG only write one file per page;
// PDF writes one file per layout or all layouts into one file.
inline project::INotationWriter::UnitType exportUnitType(const QString& suffix, bool filePerLayout)
{
    using UnitType = project::INotationWriter::UnitType;
    if (suffix != "pdf") {
        return UnitType::PER_PAGE;
    }
    return filePerLayout ? UnitType::PER_PART : UnitType::MULTI_PART;
}

// Top-level toolbars of every Dorico page (Cmd-6 toggles them).
inline std::vector<QString> toolbarDocks(Mode mode)
{
    return { pageDockName(mode, "doricoTopBar"), pageDockName(mode, "playbackToolBar"), pageDockName(mode, "undoRedoToolBar") };
}
}
