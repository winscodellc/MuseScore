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
//! One valid playing technique (spec 29): a text annotation (pizz., con sord.) or a symbol (bowing, o, +).
struct Technique {
    std::string entry; // the popover entry: "pizz", "con sord"
    std::string text;  // shown text; empty for a symbol
    engraving::PlayingTechniqueType type = engraving::PlayingTechniqueType::Undefined; // playback; Undefined = none
    engraving::SymId sym = engraving::SymId::noSym;                                     // set for a symbol
};

//! Every technique the popover knows, in suggestion order. Dorico accepts only known techniques.
const std::vector<Technique>& techniques();

struct TechniqueItem {
    const Technique* technique = nullptr;
    bool parenthesised = false; // (arco)
    bool hidden = false;        // [pizz]
};

//! `harm ...` (spec 61), look only: the kind of harmonic and how a natural one is drawn.
struct HarmonicEntry {
    enum class Kind {
        Natural,    // harm, nat, nh, hh
        Artificial, // art, ah
        Pinch,      // ph, pinch
        Remove      // noharm
    };

    Kind kind = Kind::Natural;
    bool circle = false; // natural: the circle above the staff instead of a diamond notehead
};

//! A playing techniques popover entry: `pizz`, `(arco)`, `vibrato->`, `pizz->arco`, `?`, `ped`, `*`, `^`.
struct TechniquesEntry {
    enum class Pedal {
        None,
        Start,       // ped
        Stop,        // *
        Retake,      // ^, notch, retake
        RemoveRetake // nonotch
    };

    std::vector<TechniqueItem> items; // two or more for A->B
    bool withDuration = false;        // vibrato->: a duration line
    bool stop = false;                // ?: ends the open duration line
    Pedal pedal = Pedal::None;
    std::optional<HarmonicEntry> harmonic; // harm ...
    int stringNumber = 0;                  // string 3: 1-9, 0 when not a string indicator
};

struct TechniquesParse {
    ParseStatus status = ParseStatus::Invalid;
    TechniquesEntry entry;
};

TechniquesParse parseTechniques(const std::string& text);

//! Valid entries that complete the typed text, keeping its brackets and any `A->` before it.
std::vector<std::string> techniqueSuggestions(const std::string& text);

//! The entry that gives an annotation or a symbol back; empty when none does.
std::string techniqueEntryForText(const std::string& text);
std::string techniqueEntryForSymbol(engraving::SymId sym);
}
