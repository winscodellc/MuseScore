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

#include <optional>
#include <string>

#include "../popovertypes.h"
#include "tempoparser.h"

namespace mu::dorico {
//! A tuplets popover entry (spec 31): `3`, `3:2`, `3/2`, `3 2`, `3:2e`, `5:4q.`, `5:4-6.`, `5:4 6.`.
struct TupletEntry {
    int actual = 3;                // notes in the tuplet
    int normal = 2;                // ... in the time of this many beat units
    std::optional<BeatUnit> unit;  // empty: the selected duration
};

struct TupletParse {
    ParseStatus status = ParseStatus::Invalid;
    TupletEntry entry;
};

TupletParse parseTuplet(const std::string& text);

//! The normal count when only the actual count is typed: 3 -> 2, 5 -> 4, 2 -> 3, ...
int defaultTupletNormal(int actual);

//! The entry that gives this tuplet back: "3:2", "5:4q.".
std::string tupletText(const TupletEntry& entry);
}
