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
#include <optional>
#include <string>
#include <vector>

//! Filters (spec 45): Edit > Filter keeps or drops one category of the selected items. No engine types, so it can be unit
//! tested; the caller turns score items into `FilterItem`s and the answer back into items.
namespace mu::dorico {
enum class FilterKind {
    None,     //!< has no category (barlines, systems, ...): dropped by Select Only, kept by Deselect Only
    Part,     //!< stems, beams, hooks, dots, accidentals: belong to a note and are never selected alone
    Attached, //!< articulations, fingerings, tremolos: follow the note they belong to
    Note, Rest, Dynamic, Hairpin, Tempo, GradualTempo, Lyrics, Text, ChordSymbol, FiguredBass, Slur, Tie, Ornament, Arpeggio,
    Glissando, Ottava, Pedal, OtherLine, Tuplet, Hold, RehearsalMark, Technique, Clef
};

//! A written or sounding pitch by spelling: step 0-6 = C..B, alter -2..2, octave in scientific notation (middle C = 4)
struct FilterPitch {
    int step = 0;
    int alter = 0;
    int octave = 4;
};

struct FilterItem {
    FilterKind kind = FilterKind::None;
    int voice = 0;       //!< 0-3
    int verse = 0;       //!< lyrics line, 0-3 for lines 1-4
    int group = -1;      //!< the chord a note or an attached item belongs to
    bool grace = false;
    bool stemUp = false;
    int chordSize = 1;   //!< notes in the chord
    int indexFromTop = 0;
    FilterPitch written;
    FilterPitch sounding;
};

using FilterPredicate = std::function<bool (const FilterItem&)>;

//! One Edit > Filter entry. `id` is the tail of command://dorico/filter/<id>.
struct FilterDef {
    std::string id;
    std::string group; //!< the submenu: Notes, Voices, Dynamics, Tempo, Lyrics; empty for a single category
    std::string title;
    FilterPredicate matches;
};

const std::vector<FilterDef>& filterDefs();
const FilterDef* findFilterDef(const std::string& id);

//! Indices into `items` that stay selected. `deselectOnly` drops the matching items instead of keeping them.
//! Parts never stay; attached items stay with their note.
std::vector<size_t> applyFilter(const std::vector<FilterItem>& items, const FilterPredicate& matches, bool deselectOnly);

//! Notes by Pitch: `c`, `c4`, `eb4`, `f#5`, `bb` (B flat), `cx` (C double sharp). No octave matches every octave.
struct PitchToken {
    int step = 0;
    int alter = 0;
    std::optional<int> octave;
};

//! Separated by commas or spaces; nullopt if any token is not a pitch
std::optional<std::vector<PitchToken> > parsePitchTokens(const std::string& text);
std::string pitchTokensToString(const std::vector<PitchToken>& tokens);
bool matchesPitch(const std::vector<PitchToken>& tokens, const FilterPitch& pitch);
}
