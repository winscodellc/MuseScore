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
#include <vector>

#include "../popovertypes.h"

namespace mu::dorico {
enum class IntervalQuality {
    None, // no quality (or `diatonic`): the degree follows the key
    Major,
    Minor,
    Perfect,
    Diminished,
    Augmented
};

//! Which notes of a chord a note tool touches: `top` / `bottom` in the entry.
enum class NoteToolsScope {
    All,
    Top,
    Bottom
};

//! One interval of a note tools entry: `-M2`, `5 8 st`. `degree` is signed (negative = below), 1-15 in size. `semitones`
//! is the count of the unit form (`5 8 st` is degree 5, 8 semitones); `div` is the same as `st` in 12-EDO.
struct IntervalSpec {
    int degree = 0;
    IntervalQuality quality = IntervalQuality::None;
    std::optional<int> semitones;

    //! No quality and no unit: the interval is diatonic in the key of the note.
    bool followsKey() const { return quality == IntervalQuality::None && !semitones; }
};

//! The size of an interval as MuseScore counts it: diatonic steps and semitones, both negative for a downward interval.
struct IntervalSteps {
    int diatonic = 0;
    int semitones = 0;
};

//! A note tools entry (spec 35): adds notes at intervals (`3,6`), or transposes (`t-min6`). Invert, reverse, rotate, repeat
//! and the mappings, and quarter tones, are `Unsupported`.
struct NoteToolsParse {
    ParseStatus status = ParseStatus::Invalid;
    bool transpose = false;
    NoteToolsScope scope = NoteToolsScope::All;
    std::vector<IntervalSpec> intervals;
};

NoteToolsParse parseNoteTools(const std::string& text);

//! For an interval that does not follow the key: `Major 3rd` is {2, 4}, `Perfect 8th` {7, 12}, degree 3 with 8 semitones
//! {2, 8}.
IntervalSteps toSteps(const IntervalSpec& spec);
}
