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

// The single Write-mode lower-zone dock (spec 54); its tabs are chosen by IDoricoModeState::lowerZoneTab().
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

// Dock names are the ones in appshell/appshelltypes.h (not included: that header is upstream QML-module-internal).
// ponytail: Write still hosts MuseScore's own panels (specs 52-54 replace them); other modes have no zones until 55-58.
inline std::vector<QString> zoneDocks(Mode mode, Zone zone)
{
    if (mode != Mode::Write) {
        return {};
    }

    switch (zone) {
    case Zone::Left: return { "palettesPanel", "layoutPanel" };
    case Zone::Right: return { "selectionFiltersPanel", "undoHistoryPanel" };
    case Zone::Lower: return { LOWER_ZONE_PANEL };
    }
    return {};
}

// Top-level toolbars of every Dorico page (Cmd-6 toggles them).
inline std::vector<QString> toolbarDocks()
{
    return { "doricoTopBar", "playbackToolBar", "undoRedoToolBar" };
}
}
