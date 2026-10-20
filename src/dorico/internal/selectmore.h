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

#include <functional>
#include <tuple>
#include <vector>

//! Select More (Cmd-Shift-A, spec 44), with no engine types so it can be unit tested.
//! The caller turns score items into `SelectMoreItem`s (ticks are plain ints) and turns the answer back into items.
namespace mu::dorico {
//! Items only expand to others with the same key: same item type, staff and voice (and lyric verse / placement)
struct SelectKey {
    int type = 0;
    int staff = 0;
    int voice = 0;
    int variant = 0;

    bool operator==(const SelectKey& o) const { return type == o.type && staff == o.staff && voice == o.voice && variant == o.variant; }
    bool operator<(const SelectKey& o) const
    {
        return std::tie(type, staff, voice, variant) < std::tie(o.type, o.staff, o.voice, o.variant);
    }
};

struct SelectMoreItem {
    SelectKey key;
    int tick = 0; //!< start tick; a spanner counts in the bar it starts in
};

//! [start, end)
struct SelectMoreRange {
    int start = 0;
    int end = 0;
};

struct SelectMoreContext {
    std::function<SelectMoreRange(int tick)> bar;
    std::function<SelectMoreRange(int tick)> system;
    SelectMoreRange flow;
    bool galley = false; //!< a single system: the second press already goes to the whole flow
};

struct SelectMoreResult {
    std::vector<size_t> indices; //!< into `all`, ascending
    int stage = 0;               //!< presses so far, 1-3
};

//! `selected` indexes `all`. `stage` is the number of presses already made on this selection (0 for a fresh one).
//! A press that would not add anything moves straight on to the next stage; the last stage repeats.
SelectMoreResult selectMore(const std::vector<SelectMoreItem>& all, std::vector<size_t> selected, int stage,
                            const SelectMoreContext& ctx);
}
