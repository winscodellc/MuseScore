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

#include "engraving/types/types.h"

#include "../popovertypes.h"

namespace mu::dorico {
//! One fingering of a chord, bottom note first.
struct FingeringItem {
    std::string text;
    engraving::TextStyleType style = engraving::TextStyleType::FINGERING;
};

//! A fingering popover entry (spec 34): `1,3,5` is one item per chord note. The text is kept as typed (`2(3)`, `[4]`,
//! `1-3`, `2343`, `2/`); only a hand letter is stripped and chosen as the text style: `L2` is left hand, `R5` right
//! hand, `p` right hand, `t` left hand. `fretted` only decides what a bare digit is (left hand on a fretted staff).
struct FingeringParse {
    ParseStatus status = ParseStatus::Invalid;
    std::vector<FingeringItem> items;
};

FingeringParse parseFingering(const std::string& text, bool fretted);
}
