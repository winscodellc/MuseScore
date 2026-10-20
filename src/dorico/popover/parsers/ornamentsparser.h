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

#include "../popovertypes.h"

namespace mu::dorico {
//! An ornaments popover entry (spec 28): `tr`, `tr m3`, `mor`, `flip`, `fall`, `arpdown`, `glisswavy`.
struct OrnamentEntry {
    enum class Kind {
        Ornament,         // ornament symbol on the note: mordent, turn, short trill
        Trill,            // trill line over the note's duration or the selection
        JazzOrnament,     // brass symbol above the note: flip, smear, jazz turn, bend
        JazzArticulation, // line beside the notehead: plop, scoop, doit, fall
        Arpeggio,
        Glissando,
        Guitar            // spec 61: bends, vibrato bar, hammer-on / pull-off, tapping
    };

    enum class GuitarTech {
        Bend,        // bend
        Dive,        // vibbend: a dive, or a return when the notes ascend
        Scoop,       // vibscoop
        Dip,         // vibdip
        WhammyBar,   // wbar, w/bar: the vibrato bar line
        HammerPull,  // hammer, pull, hopo, ligado ...: MuseScore picks H or P from the pitches
        Tap,         // tap: right-hand tapping
        LeftTap,     // lhtap
        TapPull,     // tappull: right-hand tapping then a pull-off
        LeftTapPull  // lhtappull
    };

    Kind kind = Kind::Ornament;
    engraving::SymId sym = engraving::SymId::noSym;                    // Ornament, JazzOrnament
    std::optional<engraving::OrnamentInterval> interval;               // Trill: "tr m3"
    engraving::ChordLineType chordLine = engraving::ChordLineType::NOTYPE; // JazzArticulation
    bool smooth = false;                                               // JazzArticulation: straight line, not a bend
    engraving::ArpeggioType arpeggio = engraving::ArpeggioType::NORMAL;
    std::optional<engraving::GlissandoType> glissando;                 // empty: the default style
    GuitarTech guitar = GuitarTech::Bend;                              // Guitar
};

struct OrnamentParse {
    ParseStatus status = ParseStatus::Invalid;
    OrnamentEntry entry;
};

OrnamentParse parseOrnament(const std::string& text);

//! The entries that give these items back ("mor", "tr m3", "arpdown", "fallsmooth"); empty when no entry does.
std::string ornamentText(engraving::SymId sym);
std::string trillText(const engraving::OrnamentInterval& interval);
std::string arpeggioText(engraving::ArpeggioType type);
std::string glissandoText(engraving::GlissandoType type);
std::string jazzArticulationText(engraving::ChordLineType type, bool smooth);
std::string guitarText(OrnamentEntry::GuitarTech tech);
}
