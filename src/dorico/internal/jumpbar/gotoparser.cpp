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

#include "gotoparser.h"

#include <cctype>

using namespace mu::dorico;

namespace {
constexpr int MAX_NUMBER = 1000000;

//! The rest of the entry from `pos` must be a number: `32`, or `+4` when `allowRelative`. Absolute numbers start at 1.
bool readNumber(const std::string& s, size_t pos, bool allowRelative, int& value, bool& relative)
{
    relative = allowRelative && pos < s.size() && s[pos] == '+';
    pos += relative ? 1 : 0;
    if (pos >= s.size()) {
        return false;
    }
    long n = 0;
    for (; pos < s.size(); ++pos) {
        if (!std::isdigit(static_cast<unsigned char>(s[pos])) || (n = n * 10 + (s[pos] - '0')) > MAX_NUMBER) {
            return false;
        }
    }
    value = static_cast<int>(n);
    return relative || value >= 1;
}
}

GoToTarget mu::dorico::parseGoTo(const std::string& text)
{
    std::string s;
    for (char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            s += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    GoToTarget t;
    int n = 0;
    bool relative = false;
    if (s.empty()) {
        return t;
    }

    if (s[0] == 'r' && s.size() > 1) {
        t.rehearsal = s.substr(1);
        t.valid = true;
    } else if (s[0] == 'p' && readNumber(s, 1, true, n, relative)) {
        t.page = n;
        t.pageRelative = relative;
        t.valid = true;
    } else if (s[0] == 'b' && readNumber(s, 1, true, n, relative)) {
        t.bar = n;
        t.barRelative = relative;
        t.valid = true;
    } else if (s[0] == 'f') {
        // `f1` or `f1b20`: the flow number is read up to the first `b`
        const size_t b = s.find('b');
        if (readNumber(s.substr(0, b), 1, false, n, relative) && n == 1) {
            if (b == std::string::npos) {
                t.bar = 1;
                t.valid = true;
            } else if (readNumber(s, b + 1, false, n, relative)) {
                t.bar = n;
                t.valid = true;
            }
        }
    }
    return t;
}
