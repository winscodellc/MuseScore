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

#include "../popovertypes.h"

namespace mu::dorico {
//! A bars and barlines popover entry (spec 25): `+4`, `2`, `-2`, `rest`, `trim`, `||`, `|:`, `end`, ...
//! Beats (`+2q`, `-2/4`) and the triple / tick / short barlines are recognised but Unsupported.
struct BarsEntry {
    enum class Kind { AddBars, DeleteBars, BarRest, Trim, Barline };

    Kind kind = Kind::AddBars;
    int count = 0; // bars to add / delete
    engraving::BarLineType barline = engraving::BarLineType::NORMAL;
};

struct BarsParse {
    ParseStatus status = ParseStatus::Invalid;
    BarsEntry entry;
};

BarsParse parseBars(const std::string& text);

//! The entry that gives this barline back ("double", "start"); empty when no entry does (dotted, reverse final).
std::string barlineText(engraving::BarLineType type);
}
