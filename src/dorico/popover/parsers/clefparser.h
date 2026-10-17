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
//! A clefs and octave lines popover entry (spec 24): `bass`, `g8ba`, `g+1`, `8va`, `15mb`, `|`.
struct ClefEntry {
    enum class Kind { Clef, OctaveLine, Stop };

    Kind kind = Kind::Clef;
    engraving::ClefType clef = engraving::ClefType::INVALID;
    int octaves = 0; // octave lines: -3..3 (never 0), down < 0 < up
};

struct ClefParse {
    ParseStatus status = ParseStatus::Invalid;
    ClefEntry entry;
};

ClefParse parseClef(const std::string& text);

//! The entry that gives this clef back ("treble", "bass8ba"); empty when no entry does (serif tab, French C clefs, ...).
std::string clefText(engraving::ClefType type);
//! The entry for an octave line: "8va", "15ba", ...
std::string octaveLineText(int octaves);
}
