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

#include <vector>

#include "engraving/types/fraction.h"

//! Where Dorico's Paste lands (spec 40). Pure: no score, no clipboard, unit-tested.
namespace mu::dorico {
struct PasteTarget {
    size_t staff = 0;
    mu::engraving::Fraction tick;

    bool operator==(const PasteTarget& o) const { return staff == o.staff && tick == o.tick; }
};

//! Target items selected one by one: one copy per selected staff, all at the earliest tick on the top selected staff.
//! A clipboard of several staves is pasted once, from the top selected staff down.
std::vector<PasteTarget> planListPaste(const std::vector<PasteTarget>& selectedItems, size_t clipStaves);

//! Range selected: the clipboard repeats from `start` while a whole copy still fits before `end` (at least one copy is
//! always made), on every selected staff. A clipboard of several staves is pasted once, from the top staff down.
//! `fill` false: one copy only.
std::vector<PasteTarget> planRangePaste(size_t staffStart, size_t staffEnd, const mu::engraving::Fraction& start,
                                        const mu::engraving::Fraction& end, const mu::engraving::Fraction& clipLen, size_t clipStaves,
                                        bool fill);
}
