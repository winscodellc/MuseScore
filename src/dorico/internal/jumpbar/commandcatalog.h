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
#include <string>
#include <vector>

namespace mu::dorico {
//! One runnable command in the jump bar's Commands mode (spec 46).
struct CatalogEntry {
    std::string uri;
    std::string title;
    std::string keys;  //!< its key command, as displayed; may be empty
    std::string alias; //!< lower case; empty = none
};

using UsageCounts = std::map<std::string, int>; // command uri -> times run

//! `Remove Rests\=rr` = the command name and the alias to give it (lower case, trimmed). Without `\=` there is no alias;
//! an empty alias after it is ignored.
struct CommandEntryText {
    std::string name;
    std::optional<std::string> alias;
};
CommandEntryText splitAlias(const std::string& text);

//! The entries that match `query`, best first: exact alias, title starts with the query, every query word starts some
//! word of the title, title contains the query. Ties go to the most used, then the title. Empty query = no match.
std::vector<CatalogEntry> rankCommands(const std::vector<CatalogEntry>& entries, const std::string& query, const UsageCounts& usage);

//! The `count` most used entries (the list Down shows on an empty field).
std::vector<CatalogEntry> topUsed(const std::vector<CatalogEntry>& entries, const UsageCounts& usage, size_t count = 5);
}
