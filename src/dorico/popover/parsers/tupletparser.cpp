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

#include "tupletparser.h"

#include <cctype>

using namespace mu::dorico;

namespace {
bool isTupletDigit(char c)
{
    return std::isdigit(static_cast<unsigned char>(c));
}

// Reads digits at `pos`; -1 when there are none or the number is absurdly long.
int readCount(const std::string& s, size_t& pos)
{
    const size_t start = pos;
    while (pos < s.size() && isTupletDigit(s[pos])) {
        ++pos;
    }
    return pos == start || pos - start > 3 ? -1 : std::stoi(s.substr(start, pos - start));
}
}

int mu::dorico::defaultTupletNormal(int actual)
{
    // Assumption (spec 31): the common tuplets' usual ratio, else the largest power of two below the count.
    switch (actual) {
    case 2: return 3;
    case 3: return 2;
    case 4: return 3;
    case 8: return 6;
    default: break;
    }
    int normal = 1;
    while (normal * 2 < actual) {
        normal *= 2;
    }
    return normal;
}

TupletParse mu::dorico::parseTuplet(const std::string& text)
{
    const size_t b = text.find_first_not_of(' ');
    if (b == std::string::npos) {
        return { ParseStatus::Incomplete, {} };
    }
    const std::string s = text.substr(b, text.find_last_not_of(' ') - b + 1);

    TupletParse result;
    size_t pos = 0;
    result.entry.actual = readCount(s, pos);
    if (result.entry.actual < 0) {
        return result;
    }

    // The ratio: "3", "3:2", "3/2", "3 2". After ':' or '/', the normal count is required.
    if (pos < s.size() && (s[pos] == ':' || s[pos] == '/')) {
        if (++pos == s.size()) {
            return { ParseStatus::Incomplete, {} };
        }
        result.entry.normal = readCount(s, pos);
    } else if (pos + 1 < s.size() && s[pos] == ' ' && isTupletDigit(s[pos + 1])) {
        ++pos;
        result.entry.normal = readCount(s, pos);
    } else {
        result.entry.normal = defaultTupletNormal(result.entry.actual);
    }
    if (result.entry.actual <= 0 || result.entry.normal <= 0) {
        return result;
    }

    // The beat unit: a letter token may be attached ("3:2e"), a digit code needs a space or hyphen ("5:4-6.").
    if (pos < s.size()) {
        const bool separated = s[pos] == ' ' || s[pos] == '-';
        const std::string token = s.substr(separated ? pos + 1 : pos);
        if (token.empty()) {
            return { ParseStatus::Incomplete, {} };
        }
        result.entry.unit = parseBeatUnit(token, !separated);
        if (!result.entry.unit) {
            return result;
        }
    }
    result.status = ParseStatus::Ok;
    return result;
}

std::string mu::dorico::tupletText(const TupletEntry& entry)
{
    std::string text = std::to_string(entry.actual) + ":" + std::to_string(entry.normal);
    return entry.unit ? text + beatUnitText(*entry.unit) : text;
}
