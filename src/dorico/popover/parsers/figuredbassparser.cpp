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

#include "figuredbassparser.h"

#include <algorithm>

using namespace mu::dorico;

// Named, not anonymous: the unity build compiles the parsers together.
namespace figuredbassparser {
bool contains(const std::string& s, const std::string& part)
{
    return s.find(part) != std::string::npos;
}

// Valid Dorico entries we do not implement: suspensions and their durations, hidden figures, tasto solo, triple accidentals,
// lowered and unaltered figures, chord symbols.
bool isUnsupported(const std::string& s)
{
    static const char* parts[] = { "->", "_", "~", "=", "<", ">", "{", "}", "x#", "#x", "###", "bbb", "tb", "-", "u", "tasto" };
    return std::any_of(std::begin(parts), std::end(parts), [&](const char* p) { return contains(s, p); })
           || s == "ts" || (s[0] >= 'A' && s[0] <= 'G');
}

// The accidental starting at s[i]: Dorico's spelling, normalised, and how many characters it takes. Doubles first.
bool accidentalAt(const std::string& s, size_t i, std::string& accidental, size_t& length)
{
    static const struct { const char* entry; const char* accidental; } table[] = {
        { "##", "##" }, { "ds", "##" }, { "bb", "bb" }, { "db", "bb" },
        { "#", "#" }, { "s", "#" }, { "x", "##" }, { "b", "b" }, { "f", "b" }, { "n", "n" }, { "N", "n" },
    };
    for (const auto& row : table) {
        const std::string entry = row.entry;
        if (s.compare(i, entry.size(), entry) == 0) {
            accidental = row.accidental;
            length = entry.size();
            return true;
        }
    }
    return false;
}

std::string museScoreAccidental(const std::string& accidental)
{
    return accidental == "n" ? "h" : accidental;
}

std::string museScoreLine(const Figure& f)
{
    const std::string accidental = museScoreAccidental(f.accidental);
    if (f.number == 0) {
        return f.accidentalBracketed ? "(" + accidental + ")" : accidental;
    }
    const std::string number = std::to_string(f.number) + (f.alteration == '+' ? "+" : f.alteration == 'd' ? "/" : "");
    if (f.accidentalBracketed && f.bracketed) {
        return "(" + accidental + number + ")";
    }
    if (f.accidentalBracketed) {
        return "(" + accidental + ")" + number;
    }
    return accidental + (f.bracketed ? "(" + number + ")" : number);
}

// Largest number on top, as engraved; a lone accidental is a third.
std::string stackedText(std::vector<Figure> figures)
{
    auto rank = [](const Figure& f) { return f.number == 0 ? 3 : f.number; };
    std::stable_sort(figures.begin(), figures.end(), [&](const Figure& a, const Figure& b) { return rank(a) > rank(b); });
    std::string text;
    for (const Figure& f : figures) {
        text += (text.empty() ? "" : "\n") + museScoreLine(f);
    }
    return text;
}
}

FiguredBassParse mu::dorico::parseFiguredBass(const std::string& input)
{
    using namespace figuredbassparser;

    FiguredBassParse result;
    std::string s = input;
    // R r V v ? / O o !: follow Engraving Options / follow input literally. Both are ignored.
    if (!s.empty() && std::string("RrVv?Oo!").find(s[0]) != std::string::npos) {
        s.erase(0, 1);
    }
    if (s.empty()) {
        result.status = ParseStatus::Incomplete;
        return result;
    }
    if (isUnsupported(s)) {
        result.status = ParseStatus::Unsupported;
        return result;
    }

    const bool commas = contains(s, ",");
    bool open = false;          // inside round brackets
    bool pieceHasContent = false; // since the last comma
    bool twoDigits = false;     // the current number was read as 1x
    Figure cur;
    bool have = false;
    auto flush = [&]() {
        if (have) {
            result.figures.push_back(cur);
        }
        cur = Figure();
        have = false;
        twoDigits = false;
    };

    for (size_t i = 0; i < s.size();) {
        const char c = s[i];
        std::string accidental;
        size_t length = 1;
        if (c == '(' || c == ')') {
            if ((c == '(') == open) {
                return result; // Invalid: nested or unmatched
            }
            open = c == '(';
        } else if (c == ',') {
            if (!pieceHasContent) {
                return result;
            }
            flush();
            pieceHasContent = false;
        } else if (c >= '0' && c <= '9') {
            const int digit = c - '0';
            if (cur.number != 0 && !cur.alteration && (commas || (cur.number == 1 && !twoDigits))) {
                // 6,10 or 1 followed by a digit: one number
                cur.number = cur.number * 10 + digit;
                twoDigits = true;
                if (cur.number > 19) {
                    return result;
                }
            } else {
                if (cur.number != 0) {
                    flush();
                }
                if (digit == 0) {
                    return result;
                }
                cur.number = digit;
                cur.bracketed = open;
                have = true;
            }
        } else if ((c == '+' || c == 'd') && cur.number != 0 && !cur.alteration && !(c == 'd' && i + 1 < s.size()
                                                                                     && (s[i + 1] == 'b' || s[i + 1] == 's'))) {
            cur.alteration = c;
        } else if (accidentalAt(s, i, accidental, length)) {
            if (cur.number != 0 || cur.alteration) {
                flush();
            } else if (!cur.accidental.empty()) {
                return result; // two accidentals in a row
            }
            cur.accidental = accidental;
            cur.accidentalBracketed = open;
            have = true;
        } else {
            return result;
        }
        pieceHasContent = pieceHasContent || (c != '(' && c != ')' && c != ',');
        i += length;
    }
    flush();

    if (result.figures.empty()) {
        return result;
    }
    // MuseScore cannot put an accidental before and a sign after the same figure.
    if (std::any_of(result.figures.begin(), result.figures.end(), [](const Figure& f) { return f.alteration && !f.accidental.empty(); })) {
        result.status = ParseStatus::Unsupported;
        return result;
    }
    result.status = open || s.back() == ',' ? ParseStatus::Incomplete : ParseStatus::Ok;
    result.text = stackedText(result.figures);
    return result;
}
