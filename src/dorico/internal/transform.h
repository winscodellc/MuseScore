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

#include "engraving/dom/noteval.h"
#include "engraving/types/fraction.h"

#include "../popover/parsers/notetoolsparser.h"

//! The maths of spec 65 (invert / reverse / rotate), on plain values so it can be unit tested without a score.
namespace mu::dorico::transform {
//! One chord (`notes` ascending by pitch) or rest (`notes` empty) with its length; a tie chain is one event.
struct Event {
    engraving::Fraction len;
    std::vector<engraving::NoteVal> notes;
    size_t id = 0; //!< which source event the chord came from, for what travels with it (articulations, stem)

    bool isRest() const { return notes.empty(); }
};

//! What an inversion mirrors around. `twiceCentre` is a semitone sum (2c), `stepSum` the same in scale steps.
struct InvertContext {
    bool diatonic = true;
    int key = 0; // engraving::Key
    int twiceCentre = 0;
    int stepSum = 0;
};

//! The scale step (C0 = 0, D0 = 1 ...) of a MIDI pitch in the key, and its alteration against that key's degree.
int stepOf(int pitch, int key, int& alter);

//! The mirror image of `pitch`. Diatonic: the step is mirrored in the key's scale and an accidental the note carries
//! against the key is mirrored with it. The result may leave 0-127.
int invertPitch(int pitch, const InvertContext& ctx);

//! `result[i]` is the index the value at position i comes from: reversed, or rotated by `steps` (positive = forwards,
//! so the value at i moves to i + steps), modulo `n`.
std::vector<size_t> reverseOrder(size_t n);
std::vector<size_t> rotateOrder(size_t n, int steps);

//! Runs the transformation. Pitch operations permute the pitch sets of the note events (rests stay put), rhythm
//! operations the lengths of all events, both together move whole events. An inversion then mirrors every pitch; the
//! notes it creates have no spelling (tpc), the caller derives it.
std::vector<Event> apply(const std::vector<Event>& events, const TransformSpec& spec, const InvertContext& ctx);
}
