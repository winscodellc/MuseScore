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
//! A key signatures popover entry (spec 23): `Eb`, `f#`, `3#`, `4b`, `open`.
struct KeySigEntry {
    enum class Kind { Open, Tonal };

    Kind kind = Kind::Tonal;
    int fifths = 0;     // -7..7: flats < 0 < sharps
    bool minor = false;
};

struct KeySigParse {
    ParseStatus status = ParseStatus::Invalid;
    KeySigEntry entry;
};

KeySigParse parseKeySig(const std::string& text);

//! The entry that gives this key back: "Eb", "f#", "open". Sharp tonics are written `#`, flat ones `b`.
std::string keySigText(const KeySigEntry& entry);
}
