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

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <QString>

#include "translation.h"
#include "project/inotationwriter.h"

#include "../idoricomodestate.h"

//! Pure helpers for the UI shell (specs 50, 95): which dock names make up a zone in each mode, and their defaults.
//! No UI and no services, so they are unit-tested directly.
namespace mu::dorico {
enum class Zone {
    Left,
    Lower,
    Right
};

inline constexpr int MODE_COUNT = 5;
inline constexpr int ZONE_COUNT = 3;

// The zone docks of the Dorico page (spec 95): one dock per zone, whose content changes with the mode.
inline constexpr const char* LEFT_ZONE_PANEL = "doricoLeftZone";
inline constexpr const char* RIGHT_ZONE_PANEL = "doricoRightZone";
// The single lower-zone dock (spec 54); its tabs are chosen by IDoricoModeState::lowerZoneTab().
inline constexpr const char* LOWER_ZONE_PANEL = "lowerZonePanel";

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
// and the objectNames of DoricoShellPage's docks. The first dock of a zone is the one a closed zone opens with (when
// nothing was open before). An empty list = the mode has no such zone.
inline std::vector<QString> zoneDocks(Mode mode, Zone zone)
{
    switch (zone) {
    case Zone::Left:
        return { LEFT_ZONE_PANEL };
    case Zone::Right:
        switch (mode) {
        case Mode::Write: return { RIGHT_ZONE_PANEL, "palettesPanel", "selectionFiltersPanel", "undoHistoryPanel" };
        case Mode::Play: return {};
        default: return { RIGHT_ZONE_PANEL };
        }
    case Zone::Lower:
        return mode == Mode::Print ? std::vector<QString> {} : std::vector<QString> { LOWER_ZONE_PANEL };
    }
    return {};
}

// The vertical toolboxes at the zone edges: Notes and Notations in Write, Engrave in Engrave.
inline std::vector<QString> toolboxDocks(Mode mode)
{
    switch (mode) {
    case Mode::Write: return { "doricoLeftToolbox", "doricoRightToolbox" };
    case Mode::Engrave: return { "doricoLeftToolbox" };
    default: return {};
    }
}

// Every dock whose open state follows the mode: the toolboxes and the zone docks of all modes.
inline std::vector<QString> managedDocks()
{
    std::vector<QString> result;
    auto add = [&result](const std::vector<QString>& names) {
        for (const QString& name : names) {
            if (std::find(result.begin(), result.end(), name) == result.end()) {
                result.push_back(name);
            }
        }
    };
    for (int m = 0; m < MODE_COUNT; ++m) {
        add(toolboxDocks(static_cast<Mode>(m)));
        for (int z = 0; z < ZONE_COUNT; ++z) {
            add(zoneDocks(static_cast<Mode>(m), static_cast<Zone>(z)));
        }
    }
    return result;
}

// Settings key of a mode's zone state: dorico/zones/<mode>/<zone>/<field>, field "docks" (comma-separated open
// docks, empty = hidden) or "size" (width, or height for the lower zone).
inline std::string zoneSettingsKey(Mode mode, Zone zone, const char* field)
{
    static const char* modeNames[] = { "setup", "write", "engrave", "play", "print" };
    static const char* zoneNames[] = { "left", "lower", "right" };
    return std::string("dorico/zones/") + modeNames[static_cast<int>(mode)] + "/" + zoneNames[static_cast<int>(zone)] + "/" + field;
}

// Whether a zone is shown the first time its mode is shown (as Dorico 6 opens a new project).
inline bool zoneOpenByDefault(Mode mode, Zone zone)
{
    switch (zone) {
    case Zone::Left: return true;
    case Zone::Right: return mode == Mode::Setup || mode == Mode::Print;
    case Zone::Lower: return mode == Mode::Setup;
    }
    return false;
}

// The zone dock's width (left / right) or height (lower) the first time its mode is shown.
inline int zoneDefaultSize(Mode mode, Zone zone)
{
    switch (zone) {
    case Zone::Left:
        switch (mode) {
        case Mode::Write: return 92;
        case Mode::Play: return 240;
        default: return 300;
        }
    case Zone::Right: return mode == Mode::Write ? 230 : 300;
    case Zone::Lower: return mode == Mode::Setup ? 130 : 300;
    }
    return 0;
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

// Top-level toolbars of the Dorico page (Cmd-6 toggles them).
inline std::vector<QString> toolbarDocks()
{
    return { "doricoTopBar", "playbackToolBar", "undoRedoToolBar" };
}
}
