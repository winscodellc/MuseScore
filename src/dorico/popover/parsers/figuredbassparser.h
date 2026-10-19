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

#include <string>
#include <vector>

#include "../popovertypes.h"

namespace mu::dorico {
//! One figure of a figured bass entry (spec 36), in Dorico's terms.
struct Figure {
    std::string accidental;           // "", "#", "b", "n", "##" or "bb"
    int number = 0;                   // 1-19, 0 = accidental only (a raised third)
    char alteration = 0;              // 0, '+' (raised) or 'd' (diminished): a suffix on the number
    bool bracketed = false;           // the number is in round brackets
    bool accidentalBracketed = false; // the accidental is in round brackets
};

//! A figured bass popover entry. `figures` are in the order typed; `text` is the same figures stacked top to bottom (largest
//! number first), one MuseScore figured bass line each, joined with '\n': `#`, `b`, `h` (natural), `##`, `bb` before the
//! number, `+` or `/` (diminished) after it, round brackets around the parts.
struct FiguredBassParse {
    ParseStatus status = ParseStatus::Invalid;
    std::vector<Figure> figures;
    std::string text;
};

FiguredBassParse parseFiguredBass(const std::string& text);
}
