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
//! A repeats and tremolos popover entry (spec 30): `end`, `add`, `segno`, `D.C. al Coda`, `//`, `22`, `0`, `%2`, `slash`.
struct RepeatsEntry {
    enum class Kind { Marker, Jump, Ending, Tremolo, ClearTremolo, MeasureRepeat, Slash };

    Kind kind = Kind::Marker;
    engraving::MarkerType marker = engraving::MarkerType::SEGNO;          // Marker
    engraving::JumpType jump = engraving::JumpType::DC;                   // Jump
    bool add = false;                                                     // Ending: add a segment to the selected ending
    engraving::TremoloType tremolo = engraving::TremoloType::INVALID_TREMOLO; // Tremolo: R8..R64, BUZZ_ROLL, C8..C64
    int measures = 0;                                                     // MeasureRepeat: 1, 2 or 4

    bool operator==(const RepeatsEntry&) const = default;
};

struct RepeatsParse {
    ParseStatus status = ParseStatus::Invalid;
    RepeatsEntry entry;
};

RepeatsParse parseRepeats(const std::string& text);

//! The entry that gives this item back ("segno", "D.C. al Coda", "//", "%2"); empty when no entry does.
std::string repeatsText(const RepeatsEntry& entry);
}
