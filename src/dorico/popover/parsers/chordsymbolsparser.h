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
//! A chord symbols popover entry (spec 32), translated to MuseScore's chord spelling: `C^7` -> `Cmaj7`, `Chd` -> `Cm7b5`,
//! `Des` -> `Db`, `Cmajb5,Eb` -> `Cmajb5/Eb`, `NC` -> `N.C.`. MuseScore's own chord parser then decides whether it is valid.
struct ChordSymbolParse {
    ParseStatus status = ParseStatus::Invalid;
    std::string normalized; // the text handed to Harmony::setHarmony
};

ChordSymbolParse parseChordSymbol(const std::string& text);
}
