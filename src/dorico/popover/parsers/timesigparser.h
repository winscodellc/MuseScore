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

#include "engraving/types/fraction.h"

#include "../popovertypes.h"

namespace mu::dorico {
//! A time signatures popover entry (spec 22): `3/4`, `c`, `cut`, `3+2+2/8`, `[2+3+2]/8`, with an optional pick-up `,1`.
struct TimeSigEntry {
    enum class Kind { Normal, Common, Cut };

    Kind kind = Kind::Normal;
    std::vector<int> groups;   // beat groups of the numerator: [3,2,2]; one entry for a simple meter
    int denominator = 4;
    bool groupingShown = true; // 3+2+2/8 shows "3+2+2", [2+3+2]/8 shows "7"
    engraving::Fraction pickup; // length of the pick-up bar in whole notes; zero = none

    int numerator() const;
    engraving::Fraction sig() const { return engraving::Fraction(numerator(), denominator); }
};

struct TimeSigParse {
    ParseStatus status = ParseStatus::Invalid;
    TimeSigEntry entry;
};

TimeSigParse parseTimeSig(const std::string& text);
}
