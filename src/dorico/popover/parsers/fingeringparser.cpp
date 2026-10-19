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

#include "fingeringparser.h"

#include <algorithm>

using namespace mu::dorico;
using namespace mu::engraving;

namespace {
bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

// What a fingering may contain besides a hand letter: finger numbers, alternatives 2(3), editorial [4], substitution 1-3, shift 2/.
bool isBodyChar(char c)
{
    return isDigit(c) || std::string("()[]-/").find(c) != std::string::npos;
}

// An item that is a single letter is a hand: fretted right-hand fingers p i m a e x c o, left-hand thumb t, thumb T.
bool letterItem(char c, FingeringItem& item)
{
    static const std::string rightHand = "pimaexco";
    if (rightHand.find(c) != std::string::npos) {
        item = { std::string(1, c), TextStyleType::RH_GUITAR_FINGERING };
    } else if (c == 't') {
        item = { "t", TextStyleType::LH_GUITAR_FINGERING };
    } else if (c == 'T') {
        item = { "T", TextStyleType::FINGERING };
    } else {
        return false;
    }
    return true;
}

// `L2`, `R5`: the letter picks the style and is not part of the text.
bool handItem(const std::string& s, FingeringItem& item, bool& incomplete)
{
    static const std::string left = "LGSIH";
    static const std::string right = "RDM";
    const bool isLeft = left.find(s[0]) != std::string::npos;
    if (!isLeft && right.find(s[0]) == std::string::npos) {
        return false;
    }
    incomplete = s.size() == 1;
    const std::string digits = s.substr(1);
    if (!incomplete && !std::all_of(digits.begin(), digits.end(), isDigit)) {
        return false;
    }
    item = { digits, isLeft ? TextStyleType::LH_GUITAR_FINGERING : TextStyleType::RH_GUITAR_FINGERING };
    return true;
}
}

FingeringParse mu::dorico::parseFingering(const std::string& text, bool fretted)
{
    FingeringParse result;
    if (text.empty()) {
        result.status = ParseStatus::Incomplete;
        return result;
    }

    bool incomplete = false;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t comma = std::min(text.find(',', start), text.size());
        const std::string s = text.substr(start, comma - start);
        const bool last = comma == text.size();
        start = comma + 1;

        if (s.empty()) {
            if (last && !result.items.empty()) { // "1,": waiting for the next note's fingering
                incomplete = true;
                break;
            }
            return FingeringParse();
        }

        FingeringItem item;
        bool itemIncomplete = false;
        if (!(s.size() == 1 && letterItem(s[0], item)) && !handItem(s, item, itemIncomplete)) {
            if (!std::all_of(s.begin(), s.end(), isBodyChar)) {
                return FingeringParse();
            }
            itemIncomplete = std::none_of(s.begin(), s.end(), isDigit); // "(" or "[" so far
            item = { s, fretted ? TextStyleType::LH_GUITAR_FINGERING : TextStyleType::FINGERING };
        }
        incomplete = incomplete || itemIncomplete;
        result.items.push_back(item);
    }

    result.status = incomplete ? ParseStatus::Incomplete : ParseStatus::Ok;
    return result;
}
