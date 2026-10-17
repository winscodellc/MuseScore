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

#include "barsparser.h"

#include <algorithm>
#include <cctype>
#include <utility>

using namespace mu::dorico;
using mu::engraving::BarLineType;

namespace {
const int MAX_BARS = 999;

const std::pair<const char*, BarLineType> BARLINES[] = {
    { "|", BarLineType::NORMAL }, { "single", BarLineType::NORMAL }, { "normal", BarLineType::NORMAL },
    { "||", BarLineType::DOUBLE }, { "double", BarLineType::DOUBLE },
    { ":", BarLineType::BROKEN }, { "dash", BarLineType::BROKEN }, { "dashed", BarLineType::BROKEN },
    { "|]", BarLineType::END }, { "fin", BarLineType::END }, { "final", BarLineType::END },
    { "thick", BarLineType::HEAVY },
    { "doublethick", BarLineType::DOUBLE_HEAVY }, { "thickdouble", BarLineType::DOUBLE_HEAVY },
    { "|:", BarLineType::START_REPEAT }, { "start", BarLineType::START_REPEAT },
    { ":|", BarLineType::END_REPEAT }, { "end", BarLineType::END_REPEAT },
    { ":|:", BarLineType::END_START_REPEAT }, { ":||:", BarLineType::END_START_REPEAT },
    { "end-start", BarLineType::END_START_REPEAT }, { "endstart", BarLineType::END_START_REPEAT },
};

// Dorico Pro barlines MuseScore has no type for. ponytail: tick / short could be span presets on one barline; later.
const char* const UNSUPPORTED[] = { "|||", "triple", "'", "tick", "tickbottom", ",", "short", "shorttop" };

const char* const KEYWORDS[] = { "rest", "trim" };

const char* const BEAT_DENOMINATORS[] = { "1", "2", "4", "8", "16", "32", "64" };

bool beginsWith(const std::string& word, const std::string& prefix)
{
    return word.compare(0, prefix.size(), prefix) == 0;
}

std::string lowerTrimmed(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    if (b == std::string::npos) {
        return std::string();
    }
    std::string t = s.substr(b, s.find_last_not_of(' ') - b + 1);
    std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::tolower(c); });
    return t;
}

bool isDots(const std::string& s)
{
    return s.find_first_not_of('.') == std::string::npos;
}

// The beat unit after the count: q, e. (letters), " 6", "-6" (digit codes 2-9), "/4" (denominator).
ParseStatus beatUnit(const std::string& s)
{
    if (s.empty()) {
        return ParseStatus::Incomplete;
    }
    if (s[0] == '/') {
        const std::string d = s.substr(1);
        bool prefix = d.empty();
        for (const char* den : BEAT_DENOMINATORS) {
            if (d == den) {
                return ParseStatus::Unsupported;
            }
            prefix = prefix || beginsWith(den, d);
        }
        return prefix ? ParseStatus::Incomplete : ParseStatus::Invalid;
    }
    if (s[0] == ' ' || s[0] == '-') {
        if (s.size() == 1) {
            return ParseStatus::Incomplete;
        }
        return s[1] >= '2' && s[1] <= '9' && isDots(s.substr(2)) ? ParseStatus::Unsupported : ParseStatus::Invalid;
    }
    // The count's digits are read greedily, so "+22w" is 22 wholes rather than 2 double wholes.
    if (std::string("zyxeqhw").find(s[0]) == std::string::npos) {
        return ParseStatus::Invalid;
    }
    return isDots(s.substr(1)) ? ParseStatus::Unsupported : ParseStatus::Invalid;
}

// +2, 2, -6; with a beat unit (+2q, -2/4) it is beats, which MuseScore cannot insert or delete.
BarsParse parseCount(const std::string& t)
{
    BarsParse result;
    const bool sign = t[0] == '+' || t[0] == '-';
    const size_t digitsEnd = t.find_first_not_of("0123456789", sign ? 1 : 0);
    const std::string digits = t.substr(sign ? 1 : 0, digitsEnd == std::string::npos ? std::string::npos : digitsEnd - (sign ? 1 : 0));
    const std::string unit = digitsEnd == std::string::npos ? std::string() : t.substr(digitsEnd);

    if (digits.empty()) {
        result.status = sign && unit.empty() ? ParseStatus::Incomplete : ParseStatus::Invalid;
        return result;
    }
    if (digits.size() > 3 || std::stoi(digits) == 0) {
        return result;
    }
    if (!unit.empty()) {
        result.status = beatUnit(unit);
        return result;
    }
    result.status = ParseStatus::Ok;
    result.entry.kind = t[0] == '-' ? BarsEntry::Kind::DeleteBars : BarsEntry::Kind::AddBars;
    result.entry.count = std::min(std::stoi(digits), MAX_BARS);
    return result;
}
}

BarsParse mu::dorico::parseBars(const std::string& text)
{
    const std::string t = lowerTrimmed(text);
    BarsParse result;
    if (t.empty()) {
        result.status = ParseStatus::Incomplete;
        return result;
    }

    // Barlines first, so ":" and "|" are not read as anything else.
    for (const auto& [alias, type] : BARLINES) {
        if (t == alias) {
            result.status = ParseStatus::Ok;
            result.entry.kind = BarsEntry::Kind::Barline;
            result.entry.barline = type;
            return result;
        }
    }
    for (const char* alias : UNSUPPORTED) {
        if (t == alias) {
            result.status = ParseStatus::Unsupported;
            return result;
        }
    }
    if (t == "rest") {
        result.status = ParseStatus::Ok;
        result.entry.kind = BarsEntry::Kind::BarRest;
        return result;
    }
    if (t == "trim") { // bars and beats alike: MuseScore bars cannot end mid-bar, so empty bars it is
        result.status = ParseStatus::Ok;
        result.entry.kind = BarsEntry::Kind::Trim;
        return result;
    }
    if (std::isdigit(static_cast<unsigned char>(t[0])) || t[0] == '+' || t[0] == '-') {
        return parseCount(t);
    }

    auto isPrefix = [&t](const char* word) { return beginsWith(word, t); };
    const bool prefix = std::any_of(std::begin(BARLINES), std::end(BARLINES), [&](const auto& b) { return isPrefix(b.first); })
                        || std::any_of(std::begin(UNSUPPORTED), std::end(UNSUPPORTED), isPrefix)
                        || std::any_of(std::begin(KEYWORDS), std::end(KEYWORDS), isPrefix);
    result.status = prefix ? ParseStatus::Incomplete : ParseStatus::Invalid;
    return result;
}

std::string mu::dorico::barlineText(BarLineType type)
{
    switch (type) {
    case BarLineType::NORMAL: return "single";
    case BarLineType::DOUBLE: return "double";
    case BarLineType::BROKEN: return "dashed";
    case BarLineType::END: return "final";
    case BarLineType::HEAVY: return "thick";
    case BarLineType::DOUBLE_HEAVY: return "doublethick";
    case BarLineType::START_REPEAT: return "start";
    case BarLineType::END_REPEAT: return "end";
    case BarLineType::END_START_REPEAT: return "endstart";
    default: return std::string();
    }
}
