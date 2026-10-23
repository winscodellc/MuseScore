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

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "global/types/val.h"

namespace mu::notation {
class INotationConfiguration;
}
namespace mu::project {
class IProjectConfiguration;
}
namespace mu::playback {
class IPlaybackConfiguration;
}
namespace mu::iex::musicxml {
class IMusicXmlConfiguration;
}
namespace muse::ui {
class IUiConfiguration;
}
namespace muse::languages {
class ILanguagesConfiguration;
class ILanguagesService;
}

namespace mu::dorico {
//! The Preferences dialog's content (spec 100): categories -> pages -> sections -> rows, in Dorico's order. The table is
//! plain data, so the dialog model and the tests both read it. A row is "backed" when it has a getter and a setter on a
//! MuseScore configuration; the others are shown disabled.
enum class PrefType {
    Check,
    Dropdown,
    Segmented,
    Spinner,
    Text,
    Path,
    Hex,
    Slider,
    Button
};

struct PrefOption {
    std::string text;
    muse::Val value;
};

struct PrefRow {
    std::string id; // unique, "<category>.<page>.<name>"
    std::string label;
    std::string help; // italic text under the control
    PrefType type = PrefType::Check;
    std::vector<PrefOption> options; // Dropdown, Segmented
    double min = 0, max = 100;
    std::string unit; // Spinner suffix
    std::shared_ptr<PrefRow> extra; // a second control on the same line (Auto-save's minutes, Dorico's "[check] ... [spinner]")
    muse::Val fallback; // what a row without a MuseScore setting shows (disabled)

    std::function<muse::Val()> get;
    std::function<void(const muse::Val&)> set;

    bool backed() const { return get && set; }
};

//! A MuseScore preferences page (MuseScore.Preferences "<page>PreferencesPage") shown inside a section, for the MuseScore
//! settings Dorico has no row for. Such a section is titled "MuseScore".
struct PrefEmbedded {
    std::string page; // QML type name, for example "FoldersPreferencesPage"
    std::string title;
};

struct PrefSection {
    std::string title; // empty: the page's first card, no band
    std::vector<PrefRow> rows;
    std::vector<PrefEmbedded> embedded;
};

struct PrefPage {
    std::string id;
    std::string title;
    std::vector<PrefSection> sections;
};

struct PrefCategory {
    std::string id;
    std::string title;
    std::vector<PrefPage> pages;
};

using PrefTable = std::vector<PrefCategory>;

//! The MuseScore configurations that back rows. A null one leaves its rows unbacked (tests, or a module not built).
struct PrefBackends {
    std::shared_ptr<notation::INotationConfiguration> notation;
    std::shared_ptr<project::IProjectConfiguration> project;
    std::shared_ptr<playback::IPlaybackConfiguration> playback;
    std::shared_ptr<iex::musicxml::IMusicXmlConfiguration> musicXml;
    std::shared_ptr<muse::ui::IUiConfiguration> ui;
    std::shared_ptr<muse::languages::ILanguagesConfiguration> languagesConfig;
    std::shared_ptr<muse::languages::ILanguagesService> languages;
};

PrefTable buildPrefTable(const PrefBackends& backends);

//! Play > Default mini transport display (spec 91): "Elapsed Time", "Timecode" or "Bar.Beat.Ticks", in app settings
inline constexpr const char* TRANSPORT_DISPLAY_SETTING = "dorico/play/transportDisplay";

//! Row by id, searching the extra controls too; nullptr when there is none.
const PrefRow* findPrefRow(const PrefTable& table, const std::string& id);

//! Every row id (extras included), in table order.
std::vector<std::string> prefRowIds(const PrefTable& table);
}
