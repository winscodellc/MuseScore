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

#include "timesigparser.h"

#include <numeric>

using namespace mu::dorico;
using mu::engraving::Fraction;

namespace {
const int MAX_NUMERATOR = 99;
const int MAX_PICKUP_DENOMINATOR = 128;
const int DENOMINATORS[] = { 1, 2, 4, 8, 16, 32, 64 };

// Alternating, aggregate and interchangeable signatures join two meters: 6/8 + 3/4, 2/4|6/8, 2/4:6/8, 2/4 (6/8),
// 2/4 / 6/8, 2/4 = 6/8, 2/4 – 6/8. Dorico wants spaces around the spaced separators; we only look for the leading one.
const char* const SEPARATORS[] = { " +", "|", ":", " (", " /", " =", " \xE2\x80\x93", " -" };

std::string trimmed(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    if (b == std::string::npos) {
        return std::string();
    }
    return s.substr(b, s.find_last_not_of(' ') - b + 1);
}

bool isDigits(const std::string& s)
{
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}

bool startsWith(const std::string& word, const std::string& prefix)
{
    return word.compare(0, prefix.size(), prefix) == 0;
}

// "3+2+2" -> [3,2,2]. Incomplete for a trailing "+" or nothing yet.
ParseStatus parseGroups(const std::string& s, std::vector<int>& groups)
{
    size_t start = 0;
    for (;;) {
        const size_t plus = s.find('+', start);
        const std::string part = s.substr(start, plus == std::string::npos ? std::string::npos : plus - start);
        if (part.empty()) {
            return plus == std::string::npos ? ParseStatus::Incomplete : ParseStatus::Invalid;
        }
        if (!isDigits(part) || part.size() > 2 || std::stoi(part) == 0) {
            return ParseStatus::Invalid;
        }
        groups.push_back(std::stoi(part));
        if (plus == std::string::npos) {
            break;
        }
        start = plus + 1;
    }
    return std::accumulate(groups.begin(), groups.end(), 0) <= MAX_NUMERATOR ? ParseStatus::Ok : ParseStatus::Invalid;
}

ParseStatus parseDenominator(const std::string& s, int& denominator)
{
    if (s.empty()) {
        return ParseStatus::Incomplete;
    }
    if (!isDigits(s)) {
        return ParseStatus::Invalid;
    }
    bool prefix = false;
    for (int d : DENOMINATORS) {
        const std::string ds = std::to_string(d);
        if (ds == s) {
            denominator = d;
            return ParseStatus::Ok;
        }
        prefix = prefix || startsWith(ds, s);
    }
    return prefix ? ParseStatus::Incomplete : ParseStatus::Invalid; // 3/3 is the start of 3/32
}

// One meter without a pick-up: c, cut, 3/4, 3+2+2/8, [2+3+2]/8.
ParseStatus parseMeter(const std::string& s, TimeSigEntry& e)
{
    if (s == "c") {
        e.kind = TimeSigEntry::Kind::Common;
        e.groups = { 4 };
        e.denominator = 4;
        return ParseStatus::Ok;
    }
    if (s == "cut" || s == "cutc" || s == "\xC2\xA2") { // ¢
        e.kind = TimeSigEntry::Kind::Cut;
        e.groups = { 2 };
        e.denominator = 2;
        return ParseStatus::Ok;
    }
    if (s == "x" || s == "X" || s == "open") {
        return ParseStatus::Unsupported; // open meter: no MuseScore equivalent
    }
    if (startsWith("cutc", s) || startsWith("open", s)) {
        return ParseStatus::Incomplete;
    }

    std::string numerator = s;
    std::string rest;
    const size_t slash = s.find('/');
    if (slash != std::string::npos) {
        numerator = s.substr(0, slash);
        rest = s.substr(slash + 1);
    }

    e.groupingShown = true;
    if (!numerator.empty() && numerator.front() == '[') {
        e.groupingShown = false;
        const size_t close = numerator.find(']');
        if (close == std::string::npos) {
            if (slash != std::string::npos) {
                return ParseStatus::Invalid;
            }
            const ParseStatus g = parseGroups(numerator.substr(1), e.groups);
            return g == ParseStatus::Invalid ? g : ParseStatus::Incomplete;
        }
        if (close + 1 != numerator.size()) {
            return ParseStatus::Invalid;
        }
        numerator = numerator.substr(1, close - 1);
    }

    const ParseStatus g = parseGroups(numerator, e.groups);
    if (g == ParseStatus::Invalid) {
        return g;
    }
    if (slash == std::string::npos) {
        return ParseStatus::Incomplete; // "3", "3+2", "[2+3]"
    }
    if (g != ParseStatus::Ok) {
        return ParseStatus::Invalid; // "3+/8"
    }
    return parseDenominator(rest, e.denominator);
}

// ",1.5" counts denominator units: 1.5 in 4/4 is a dotted quarter.
ParseStatus parsePickup(const std::string& s, TimeSigEntry& e)
{
    if (s.empty()) {
        return ParseStatus::Incomplete;
    }
    const size_t dot = s.find('.');
    const std::string whole = s.substr(0, dot);
    const std::string frac = dot == std::string::npos ? std::string() : s.substr(dot + 1);
    if ((!whole.empty() && !isDigits(whole)) || (!frac.empty() && !isDigits(frac)) || whole.size() > 2 || frac.size() > 4) {
        return ParseStatus::Invalid;
    }
    if (dot != std::string::npos && frac.empty()) {
        return ParseStatus::Incomplete; // "1."
    }

    int scale = 1;
    for (size_t i = 0; i < frac.size(); ++i) {
        scale *= 10;
    }
    const int units = (whole.empty() ? 0 : std::stoi(whole)) * scale + (frac.empty() ? 0 : std::stoi(frac));
    const Fraction pickup = Fraction(units, scale * e.denominator).reduced();

    // Must be shorter than a bar and a plain note value (no 0.3 of a quarter).
    const int d = pickup.denominator();
    if (units == 0 || pickup >= e.sig() || d > MAX_PICKUP_DENOMINATOR || (d & (d - 1)) != 0) {
        return ParseStatus::Invalid;
    }
    e.pickup = pickup;
    return ParseStatus::Ok;
}

ParseStatus valid(ParseStatus s)
{
    return s == ParseStatus::Ok || s == ParseStatus::Unsupported ? ParseStatus::Ok : s;
}
}

int TimeSigEntry::numerator() const
{
    return std::accumulate(groups.begin(), groups.end(), 0);
}

TimeSigParse mu::dorico::parseTimeSig(const std::string& input)
{
    const std::string text = trimmed(input);
    if (text.empty()) {
        return { ParseStatus::Incomplete, {} };
    }

    // Two meters joined: recognised, not supported.
    for (const char* sep : SEPARATORS) {
        const size_t at = text.find(sep);
        if (at == std::string::npos) {
            continue;
        }
        std::string right = trimmed(text.substr(at + std::string(sep).size()));
        if (std::string(sep) == " (" && !right.empty() && right.back() == ')') {
            right.pop_back();
        }
        const ParseStatus left = valid(parseTimeSig(text.substr(0, at)).status);
        const ParseStatus r = right.empty() ? ParseStatus::Incomplete : valid(parseTimeSig(right).status);
        if (left != ParseStatus::Ok || r == ParseStatus::Invalid) {
            return { ParseStatus::Invalid, {} };
        }
        return { r == ParseStatus::Ok ? ParseStatus::Unsupported : ParseStatus::Incomplete, {} };
    }

    TimeSigParse result;
    const size_t comma = text.find(',');
    result.status = parseMeter(text.substr(0, comma), result.entry);
    if (comma != std::string::npos && result.status == ParseStatus::Ok) {
        result.status = parsePickup(text.substr(comma + 1), result.entry);
    } else if (comma != std::string::npos && result.status == ParseStatus::Incomplete) {
        result.status = ParseStatus::Invalid; // "3/,1"
    }
    return result;
}
