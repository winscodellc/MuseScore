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

#include "pasteplanner.h"

#include <algorithm>
#include <optional>
#include <set>

using namespace mu::dorico;
using mu::engraving::Fraction;

std::vector<PasteTarget> mu::dorico::planListPaste(const std::vector<PasteTarget>& selectedItems, size_t clipStaves)
{
    if (selectedItems.empty()) {
        return {};
    }

    size_t top = selectedItems.front().staff;
    for (const PasteTarget& t : selectedItems) {
        top = std::min(top, t.staff);
    }
    std::optional<Fraction> tick;
    std::set<size_t> staves;
    for (const PasteTarget& t : selectedItems) {
        staves.insert(t.staff);
        if (t.staff == top && (!tick || t.tick < *tick)) {
            tick = t.tick;
        }
    }

    if (clipStaves > 1) {
        return { { top, *tick } };
    }
    std::vector<PasteTarget> result;
    for (size_t staff : staves) {
        result.push_back({ staff, *tick });
    }
    return result;
}

std::vector<PasteTarget> mu::dorico::planRangePaste(size_t staffStart, size_t staffEnd, const Fraction& start, const Fraction& end,
                                                    const Fraction& clipLen, size_t clipStaves, bool fill)
{
    const size_t lastStaff = clipStaves > 1 ? staffStart + 1 : std::max(staffEnd, staffStart + 1);

    std::vector<PasteTarget> result;
    for (Fraction t = start;; t += clipLen) {
        for (size_t staff = staffStart; staff < lastStaff; ++staff) {
            result.push_back({ staff, t });
        }
        if (!fill || clipLen <= Fraction(0, 1) || t + clipLen + clipLen > end) {
            break;
        }
    }
    return result;
}
