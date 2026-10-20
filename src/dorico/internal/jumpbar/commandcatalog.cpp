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

#include "commandcatalog.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>

using namespace mu::dorico;

namespace {
std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::vector<std::string> words(const std::string& s)
{
    std::istringstream in(s);
    return { std::istream_iterator<std::string>(in), std::istream_iterator<std::string>() };
}

int usageOf(const UsageCounts& usage, const CatalogEntry& e)
{
    const auto it = usage.find(e.uri);
    return it == usage.end() ? 0 : it->second;
}

//! Lower is better; -1 = no match
int tier(const CatalogEntry& e, const std::string& query, const std::vector<std::string>& queryWords)
{
    if (!e.alias.empty() && e.alias == query) {
        return 0;
    }
    const std::string title = lower(e.title);
    if (title.rfind(query, 0) == 0) {
        return 1;
    }
    const std::vector<std::string> titleWords = words(title);
    const bool allWordsStart = std::all_of(queryWords.begin(), queryWords.end(), [&](const std::string& q) {
        return std::any_of(titleWords.begin(), titleWords.end(), [&](const std::string& w) { return w.rfind(q, 0) == 0; });
    });
    if (allWordsStart) {
        return 2;
    }
    return title.find(query) != std::string::npos ? 3 : -1;
}
}

CommandEntryText mu::dorico::splitAlias(const std::string& text)
{
    CommandEntryText result;
    const size_t at = text.find("\\=");
    result.name = text.substr(0, at);
    if (at != std::string::npos) {
        std::string alias = lower(text.substr(at + 2));
        alias.erase(0, alias.find_first_not_of(" \t"));
        alias.erase(alias.find_last_not_of(" \t") + 1);
        if (!alias.empty()) {
            result.alias = alias;
        }
    }
    return result;
}

std::vector<CatalogEntry> mu::dorico::rankCommands(const std::vector<CatalogEntry>& entries, const std::string& rawQuery,
                                                   const UsageCounts& usage)
{
    const std::vector<std::string> queryWords = words(lower(rawQuery));
    std::string query;
    for (const std::string& w : queryWords) {
        query += (query.empty() ? "" : " ") + w;
    }
    if (query.empty()) {
        return {};
    }

    std::vector<std::pair<int, const CatalogEntry*> > matches;
    for (const CatalogEntry& e : entries) {
        if (const int t = tier(e, query, queryWords); t >= 0) {
            matches.push_back({ t, &e });
        }
    }
    std::stable_sort(matches.begin(), matches.end(), [&](const auto& a, const auto& b) {
        if (a.first != b.first) {
            return a.first < b.first;
        }
        const int ua = usageOf(usage, *a.second), ub = usageOf(usage, *b.second);
        return ua != ub ? ua > ub : a.second->title < b.second->title;
    });

    std::vector<CatalogEntry> result;
    for (const auto& m : matches) {
        result.push_back(*m.second);
    }
    return result;
}

std::vector<CatalogEntry> mu::dorico::topUsed(const std::vector<CatalogEntry>& entries, const UsageCounts& usage, size_t count)
{
    std::vector<CatalogEntry> used;
    std::copy_if(entries.begin(), entries.end(), std::back_inserter(used), [&](const CatalogEntry& e) { return usageOf(usage, e) > 0; });
    std::stable_sort(used.begin(), used.end(), [&](const CatalogEntry& a, const CatalogEntry& b) {
        return usageOf(usage, a) > usageOf(usage, b);
    });
    used.resize(std::min(used.size(), count));
    return used;
}
