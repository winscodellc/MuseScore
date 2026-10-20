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

enum class TransformOp {
    Invert,
    Reverse, // with `invert` set: reverse and invert
    Rotate
};

//! The pitch an inversion mirrors around.
enum class InvertCentre {
    Outermost, // the highest and lowest notes of the selection swap
    Pitch,     // `letter`, `alter`, `octave`
    Root,      // the tonic of the key
    First,     // the first note of the selection
    Top,       // the top note of the first chord
    Bottom
};

//! Invert / reverse / rotate (spec 65): `inv`, `ret inv pt`, `rot rm bw 3`.
struct TransformSpec {
    TransformOp op = TransformOp::Invert;
    bool pitches = true;
    bool rhythms = false;
    bool invert = false; // pitches are inverted (always for Invert, optional for Reverse)
    bool diatonic = true;
    InvertCentre centre = InvertCentre::Outermost;
    int letter = 0; // C = 0 ... B = 6
    int alter = 0;
    int octave = 4; // middle C is C4
    int steps = 1;  // Rotate: positive = forwards, negative = backwards
};

//! A note tools entry (spec 35): adds notes at intervals (`3,6`), or transposes (`t-min6`); or a transformation (spec 65).
//! Repeat, the mappings and quarter tones are `Unsupported`.
struct NoteToolsParse {
    ParseStatus status = ParseStatus::Invalid;
    bool transpose = false;
    NoteToolsScope scope = NoteToolsScope::All;
    std::vector<IntervalSpec> intervals;
    std::optional<TransformSpec> transform;
};

NoteToolsParse parseNoteTools(const std::string& text);

//! For an interval that does not follow the key: `Major 3rd` is {2, 4}, `Perfect 8th` {7, 12}, degree 3 with 8 semitones
//! {2, 8}.
IntervalSteps toSteps(const IntervalSpec& spec);
}
