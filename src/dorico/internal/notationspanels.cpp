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

#include "notationspanels.h"

#include <set>

using namespace mu::dorico;
using mu::engraving::ElementType;

const std::vector<NotationsSection>& mu::dorico::notationsSections()
{
    using SP = SourcePalette;
    using ET = ElementType;

    // ponytail: names are MuseScore's untranslated cell names; a rename upstream shows up as a LOGW from
    // DoricoPanelModel (unknownNames) and the cell just drops out of its section.
    static const std::vector<NotationsSection> sections = {
        { "clefs", "Common", SP::Clefs, { ET::CLEF },
          { "Treble clef", "Bass clef", "Alto clef", "Tenor clef", "Treble clef 8va bassa", "Bass clef 8va bassa", "Percussion",
            "Tablature" } },
        { "clefs", "Uncommon", SP::Clefs, { ET::CLEF }, {}, true },
        { "clefs", "Archaic Clefs", SP::Clefs, { ET::CLEF },
          { "*century)*" } },
        { "clefs", "Octave Lines", SP::Lines, { ET::OTTAVA }, {} },

        { "keySignatures", "Key Signatures", SP::KeySignatures, { ET::KEYSIG }, {} },
        { "keySignatures", "Accidentals", SP::Accidentals, {}, {} },

        { "timeSignatures", "Common", SP::TimeSignatures, { ET::TIMESIG }, {} },

        { "tempo", "Absolute Tempo Change", SP::Tempo, { ET::TEMPO_TEXT }, { "*= 80 BPM*" } },
        { "tempo", "Gradual Tempo Change", SP::Tempo, { ET::GRADUAL_TEMPO_CHANGE }, {} },
        { "tempo", "Reset Tempo", SP::Tempo, {}, { "a tempo", "tempo primo" } },
        { "tempo", "Tempo Equation", SP::Tempo, {}, { "Metric modulation*" } },

        { "dynamics", "Immediate Dynamics", SP::Dynamics, { ET::DYNAMIC }, {}, true },
        { "dynamics", "Gradual Dynamics", SP::Dynamics, { ET::HAIRPIN }, {} },
        { "dynamics", "Force/Intensity of Attack", SP::Dynamics, { ET::DYNAMIC },
          { "sf (*", "sfz (*", "sff (*", "sffz (*", "sfff (*", "sfffz (*", "rfz (*", "rf (*", "fz (*" } },
        { "dynamics", "Combined Dynamics", SP::Dynamics, { ET::DYNAMIC }, { "fp (*", "pf (*", "sfp (*", "sfpp (*" } },

        { "ornaments", "Baroque and Classical", SP::Ornaments, {}, {} },
        { "ornaments", "Arpeggiation", SP::Arpeggios, { ET::ARPEGGIO }, {} },
        { "ornaments", "Glissandi", SP::Arpeggios, { ET::GLISSANDO, ET::CHORDLINE }, {} },

        { "repeatStructures", "Repeat Endings", SP::Repeats, { ET::VOLTA }, {} },
        { "repeatStructures", "Repeat Jumps", SP::Repeats, { ET::JUMP }, {} },
        { "repeatStructures", "Repeat Sections", SP::Repeats, { ET::MARKER }, {} },
        { "repeatStructures", "Tremolos", SP::Tremolos, {}, {} },
        { "repeatStructures", "Bar Repeats", SP::Repeats, { ET::MEASURE_REPEAT }, {} },

        { "barsAndBarlines", "Create Barline", SP::BarLines, {}, {} },

        { "holdsAndPauses", "Fermatas", SP::Breaths, { ET::FERMATA }, {} },
        { "holdsAndPauses", "Breath Marks", SP::Breaths, { ET::BREATH }, {}, true },
        { "holdsAndPauses", "Caesuras", SP::Breaths, { ET::BREATH }, { "*caesura*" } },

        { "playingTechniques", "Common", SP::Text, { ET::PLAYTECH_ANNOTATION }, {} },
        { "playingTechniques", "Brass", SP::Articulations, { ET::ARTICULATION }, { "Muted (closed)", "Open", "Half-open 2 (Weinberg)" } },
        { "playingTechniques", "Keyboard", SP::Keyboard, {}, {} },
        { "playingTechniques", "Strings", SP::Articulations, { ET::ARTICULATION },
          { "Up bow", "Down bow", "Harmonic", "Snap pizzicato above" } },
        { "playingTechniques", "Guitar", SP::Guitar, {}, {} },
        { "playingTechniques", "Harp Pedals", SP::Harp, {}, {} },

        { "lines", "Horizontal", SP::Lines, { ET::TEXTLINE, ET::LET_RING, ET::PALM_MUTE, ET::VIBRATO, ET::NOTELINE }, {} },
        { "lines", "Vertical", SP::Lines, { ET::CHORD_BRACKET }, {} },

        // Engrave mode's Formatting panel (spec 56), Bracketing: MuseScore's system brackets and braces.
        { "engraveBracketing", "Bracketing", SP::Brackets, { ET::BRACKET }, {} },
    };
    return sections;
}

std::vector<NotationsSection> mu::dorico::sectionsOfPanel(const QString& panel)
{
    std::vector<NotationsSection> result;
    for (const NotationsSection& section : notationsSections()) {
        if (section.panel == panel) {
            result.push_back(section);
        }
    }
    return result;
}

bool mu::dorico::nameMatches(const QString& pattern, const QString& name)
{
    if (pattern.size() > 2 && pattern.startsWith('*') && pattern.endsWith('*')) {
        return name.contains(pattern.mid(1, pattern.size() - 2), Qt::CaseInsensitive);
    }
    if (pattern.endsWith('*')) {
        return name.startsWith(pattern.chopped(1));
    }
    return name == pattern;
}

static bool typeMatches(const NotationsSection& section, ElementType type)
{
    return section.types.empty() || std::find(section.types.begin(), section.types.end(), type) != section.types.end();
}

std::vector<std::vector<size_t> > mu::dorico::resolveSections(const std::vector<NotationsSection>& sections, const CellsOf& cellsOf,
                                                              QStringList* unknownNames)
{
    std::vector<std::vector<size_t> > result(sections.size());
    std::set<std::pair<SourcePalette, size_t> > taken;

    // Named / typed sections first, then the "rest" sections take what is left of their palette.
    for (bool restPass : { false, true }) {
        for (size_t s = 0; s < sections.size(); ++s) {
            const NotationsSection& section = sections[s];
            if (section.rest != restPass) {
                continue;
            }

            const std::vector<PaletteCellInfo>& cells = cellsOf(section.palette);
            std::vector<bool> patternUsed(section.names.size(), false);

            for (size_t i = 0; i < cells.size(); ++i) {
                if (!typeMatches(section, cells[i].type)) {
                    continue;
                }
                if (restPass && taken.count({ section.palette, i })) {
                    continue;
                }

                bool match = section.names.isEmpty();
                for (qsizetype p = 0; p < section.names.size(); ++p) {
                    if (nameMatches(section.names[p], cells[i].name)) {
                        patternUsed[p] = true;
                        match = true;
                    }
                }
                if (match) {
                    result[s].push_back(i);
                }
            }

            if (!restPass) {
                for (size_t i : result[s]) {
                    taken.insert({ section.palette, i });
                }
            }
            for (size_t p = 0; unknownNames && p < patternUsed.size(); ++p) {
                if (!patternUsed[p]) {
                    unknownNames->append(section.title + ": " + section.names[p]);
                }
            }
        }
    }

    return result;
}
