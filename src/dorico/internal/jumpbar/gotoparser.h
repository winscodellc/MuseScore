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
#include <string>

namespace mu::dorico {
//! A Go To entry (spec 46): `b32`, `b+4`, `rK`, `p6`, `p+2`, `f1`, `f1b20`.
struct GoToTarget {
    bool valid = false;
    std::optional<int> bar; //!< 1-based; with barRelative: bars on from the current one
    bool barRelative = false;
    std::optional<int> page;
    bool pageRelative = false;
    std::string rehearsal; //!< lower case; empty = none
};

//! Case and spaces are ignored. MuseScore has one flow, so `f` must be 1 (`fn`, `fp`, `f2` are invalid).
//! A bare `f1` means the start of the flow (bar 1).
GoToTarget parseGoTo(const std::string& text);
}
