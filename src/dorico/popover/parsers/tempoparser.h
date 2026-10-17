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
//! The note value of a metronome mark: q = quarter, e. = dotted eighth (Dorico's beat units).
struct BeatUnit {
    engraving::DurationType type = engraving::DurationType::V_QUARTER;
    int dots = 0;
};

//! A tempo popover entry (spec 26): `Allegro`, `q=120`, `Presto q=176`, `(q=72)`, `[q=96]`, `rit....`, free text.
//! Metronome ranges, tempo equations and rhythmic feels are recognised but Unsupported.
struct TempoEntry {
    enum class Kind { Absolute, Gradual, Relative, ATempo, TempoPrimo, FreeText };

    Kind kind = Kind::FreeText;
    std::string text;                // the words as typed ("Allegro"); Gradual: "rit.", "ritardando", ...
    std::optional<BeatUnit> unit;    // set when the entry has a metronome mark
    double bpm = 0;                  // metronome mark: units per minute
    double wordBpm = 0;              // quarter-note speed the Tempo palette gives the words (Allegro 144), 0 = none
    bool parenthesised = false;      // Presto (q=176): the metronome mark is in parentheses
    bool hidden = false;             // [q=96], [Allegro]: the whole item is not printed, but plays
    bool markHidden = false;         // Allegro [q=96]: the words are printed, the metronome mark only plays
    engraving::GradualTempoChangeType gradual = engraving::GradualTempoChangeType::Undefined;
    bool line = false;               // rit....: continuation line
};

struct TempoParse {
    ParseStatus status = ParseStatus::Invalid;
    TempoEntry entry;
};

TempoParse parseTempo(const std::string& text);

//! Tempo words and gradual changes starting with `prefix` (case-insensitive), for the suggestion menu.
std::vector<std::string> tempoSuggestions(const std::string& prefix);

//! The entry form of a beat unit: "q", "e.", "2w".
std::string beatUnitText(const BeatUnit& unit);
}
