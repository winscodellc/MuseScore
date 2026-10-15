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

namespace mu::dorico {
//! Play mode's track overview (spec 57) draws one block per run of consecutive bars that have notes.
struct TrackBar {
    double start = 0;
    double end = 0;
    bool hasNotes = false;
};

struct TrackBlock {
    double start = 0;
    double end = 0;
};

inline std::vector<TrackBlock> trackBlocks(const std::vector<TrackBar>& bars)
{
    std::vector<TrackBlock> blocks;
    bool open = false;
    for (const TrackBar& bar : bars) {
        if (!bar.hasNotes) {
            open = false;
        } else if (open) {
            blocks.back().end = bar.end;
        } else {
            blocks.push_back({ bar.start, bar.end });
            open = true;
        }
    }
    return blocks;
}
}
