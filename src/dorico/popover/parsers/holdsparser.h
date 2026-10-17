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
//! A holds and pauses popover entry (spec 27): `fer`, `longfer`, `curlew`, `comma`, `,`, `//`, `thick//`.
struct HoldEntry {
    enum class Kind { Fermata, Breath, Caesura };

    Kind kind = Kind::Fermata;
    engraving::SymId sym = engraving::SymId::noSym; // fermatas: the "Above" symbol
};

struct HoldParse {
    ParseStatus status = ParseStatus::Invalid;
    HoldEntry entry;
};

HoldParse parseHold(const std::string& text);

//! The entry that gives this symbol back ("fer", "comma", "//"); empty when no entry does.
std::string holdText(engraving::SymId sym);
}
