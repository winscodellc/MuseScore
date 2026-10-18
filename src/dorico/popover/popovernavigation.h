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
#include <vector>

#include "draw/types/geometry.h"

#include "popovertypes.h"

namespace mu::engraving {
class Score;
}

namespace mu::dorico {
//! One beat of a bar in this time signature: the dotted quarter in 6/8, the half in 2/2 (as MuseScore's own chord symbol
//! navigation counts beats).
engraving::Fraction beatLength(const engraving::Fraction& timeSig);

//! The start of the beat holding `tickInBar` (both from the bar start).
engraving::Fraction beatStart(const engraving::Fraction& tickInBar, const engraving::Fraction& timeSig);

//! Where a navigating popover goes (spec 32), or nullopt past either end of the score. NextItem / PrevItem find the
//! nearest item of `itemTypes` on the same staff.
std::optional<PopoverPosition> movePosition(const engraving::Score* score, const PopoverPosition& from, PopoverMove move,
                                            const std::vector<engraving::ElementType>& itemTypes);

//! The item of `itemTypes` at the position on its staff (the chord symbol to edit there), or nullptr.
engraving::EngravingItem* itemAt(const engraving::Score* score, const PopoverPosition& pos,
                                 const std::vector<engraving::ElementType>& itemTypes);

//! The position on the canvas, a staff high, for anchoring the popover where nothing is there yet.
muse::RectF positionRect(const engraving::Score* score, const PopoverPosition& pos);
}
