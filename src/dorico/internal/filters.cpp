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


#include "filters.h"

#include <algorithm>
#include <cctype>
#include <set>

using namespace mu::dorico;

namespace {
FilterDef filtDef(const char* id, const char* group, const char* title, FilterPredicate matches)
{
    return { id, group, title, std::move(matches) };
}

FilterPredicate filtKinds(std::vector<FilterKind> kinds)
{
    return [kinds](const FilterItem& i) { return std::find(kinds.begin(), kinds.end(), i.kind) != kinds.end(); };
}

bool filtNoteOrRest(const FilterItem& i)
{
    return i.kind == FilterKind::Note || i.kind == FilterKind::Rest;
}

std::vector<FilterDef> filtMakeDefs()
{
    using K = FilterKind;
    std::vector<FilterDef> v;
    v.push_back(filtDef("notes", "Notes", "Notes", filtKinds({ K::Note })));
    v.push_back(filtDef("grace-notes", "Notes", "Grace Notes", [](const FilterItem& i) { return i.kind == K::Note && i.grace; }));
    v.push_back(filtDef("chords", "Notes", "Chords", [](const FilterItem& i) { return i.kind == K::Note && i.chordSize > 1; }));
    v.push_back(filtDef("top-notes", "Notes", "Top Notes Or Single Notes",
                        [](const FilterItem& i) { return i.kind == K::Note && i.indexFromTop == 0; }));
    v.push_back(filtDef("bottom-notes", "Notes", "Bottom Notes Or Single Notes",
                        [](const FilterItem& i) { return i.kind == K::Note && i.indexFromTop == i.chordSize - 1; }));
    v.push_back(filtDef("rests", "Notes", "Rests", filtKinds({ K::Rest })));

    for (int voice = 0; voice < 4; ++voice) {
        const std::string n = std::to_string(voice + 1);
        v.push_back({ "voice-" + n, "Voices", "Voice " + n, [voice](const FilterItem& i) { return filtNoteOrRest(i) && i.voice == voice; } });
    }
    v.push_back(filtDef("stem-up", "Voices", "Stem Up", [](const FilterItem& i) { return i.kind == K::Note && i.stemUp; }));
    v.push_back(filtDef("stem-down", "Voices", "Stem Down", [](const FilterItem& i) { return i.kind == K::Note && !i.stemUp; }));

    v.push_back(filtDef("dynamics", "Dynamics", "All Dynamics", filtKinds({ K::Dynamic, K::Hairpin })));
    v.push_back(filtDef("dynamics-gradual", "Dynamics", "Gradual", filtKinds({ K::Hairpin })));
    v.push_back(filtDef("dynamics-immediate", "Dynamics", "Immediate", filtKinds({ K::Dynamic })));

    v.push_back(filtDef("tempo", "Tempo", "All Tempos", filtKinds({ K::Tempo, K::GradualTempo })));
    v.push_back(filtDef("tempo-gradual", "Tempo", "Gradual", filtKinds({ K::GradualTempo })));

    v.push_back(filtDef("lyrics", "Lyrics", "All Lyrics", filtKinds({ K::Lyrics })));
    for (int verse = 0; verse < 4; ++verse) {
        const std::string n = std::to_string(verse + 1);
        v.push_back({ "lyrics-line-" + n, "Lyrics", "Line " + n, [verse](const FilterItem& i) { return i.kind == K::Lyrics && i.verse == verse; } });
    }

    v.push_back(filtDef("text", "", "Text", filtKinds({ K::Text })));
    v.push_back(filtDef("chord-symbols", "", "Chord Symbols", filtKinds({ K::ChordSymbol })));
    v.push_back(filtDef("figured-bass", "", "Figured Bass", filtKinds({ K::FiguredBass })));
    v.push_back(filtDef("slurs", "", "Slurs", filtKinds({ K::Slur })));
    v.push_back(filtDef("ties", "", "Ties", filtKinds({ K::Tie })));
    v.push_back(filtDef("ornaments", "", "Ornaments", filtKinds({ K::Ornament })));
    v.push_back(filtDef("arpeggios", "", "Arpeggio Signs", filtKinds({ K::Arpeggio })));
    v.push_back(filtDef("glissandi", "", "Glissandi", filtKinds({ K::Glissando })));
    v.push_back(filtDef("octave-lines", "", "Octave Lines", filtKinds({ K::Ottava })));
    v.push_back(filtDef("pedal-lines", "", "Pedal Lines", filtKinds({ K::Pedal })));
    v.push_back(filtDef("other-lines", "", "Other Lines", filtKinds({ K::OtherLine })));
    v.push_back(filtDef("tuplets", "", "Tuplets", filtKinds({ K::Tuplet })));
    v.push_back(filtDef("holds-pauses", "", "Holds and Pauses", filtKinds({ K::Hold })));
    v.push_back(filtDef("rehearsal-marks", "", "Rehearsal Marks", filtKinds({ K::RehearsalMark })));
    v.push_back(filtDef("playing-techniques", "", "Playing Techniques", filtKinds({ K::Technique })));
    v.push_back(filtDef("clefs", "", "Clefs", filtKinds({ K::Clef })));
    return v;
}

int filtStep(char c)
{
    const int k = std::tolower(static_cast<unsigned char>(c));
    return k >= 'a' && k <= 'g' ? (k - 'a' + 5) % 7 : -1; // a..g -> A=5 B=6 C=0 ...
}

std::optional<PitchToken> filtParseOne(const std::string& s)
{
    if (s.empty()) {
        return std::nullopt;
    }
    PitchToken t;
    t.step = filtStep(s[0]);
    if (t.step < 0) {
        return std::nullopt;
    }
    size_t p = 1;
    if (p < s.size() && (s[p] == 'x' || s[p] == 'X')) {
        t.alter = 2;
        ++p;
    } else if (p < s.size() && (s[p] == '#' || s[p] == 'b')) {
        const char c = s[p];
        while (p < s.size() && s[p] == c) {
            t.alter += c == '#' ? 1 : -1;
            ++p;
        }
    }
    if (std::abs(t.alter) > 2) {
        return std::nullopt;
    }
    if (p < s.size()) {
        size_t q = p + (s[p] == '-' ? 1 : 0);
        if (q >= s.size() || s.size() - q > 2 || !std::all_of(s.begin() + q, s.end(), [](unsigned char c) { return std::isdigit(c); })) {
            return std::nullopt;
        }
        t.octave = std::stoi(s.substr(p));
    }
    return t;
}
}

const std::vector<FilterDef>& mu::dorico::filterDefs()
{
    static const std::vector<FilterDef> defs = filtMakeDefs();
    return defs;
}

const FilterDef* mu::dorico::findFilterDef(const std::string& id)
{
    for (const FilterDef& d : filterDefs()) {
        if (d.id == id) {
            return &d;
        }
    }
    return nullptr;
}

std::vector<size_t> mu::dorico::applyFilter(const std::vector<FilterItem>& items, const FilterPredicate& matches, bool deselectOnly)
{
    std::set<int> keptNotes; // chords with a note that stays
    std::vector<bool> keep(items.size(), false);
    for (size_t i = 0; i < items.size(); ++i) {
        const FilterKind kind = items[i].kind;
        if (kind == FilterKind::Part || kind == FilterKind::Attached) {
            continue;
        }
        keep[i] = matches(items[i]) != deselectOnly;
        if (keep[i] && kind == FilterKind::Note) {
            keptNotes.insert(items[i].group);
        }
    }
    std::vector<size_t> result;
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].kind == FilterKind::Attached) {
            keep[i] = keptNotes.count(items[i].group) > 0;
        }
        if (keep[i]) {
            result.push_back(i);
        }
    }
    return result;
}

std::optional<std::vector<PitchToken> > mu::dorico::parsePitchTokens(const std::string& text)
{
    std::vector<PitchToken> tokens;
    std::string cur;
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i < text.size() && text[i] != ',' && !std::isspace(static_cast<unsigned char>(text[i]))) {
            cur += text[i];
            continue;
        }
        if (!cur.empty()) {
            const std::optional<PitchToken> t = filtParseOne(cur);
            if (!t) {
                return std::nullopt;
            }
            tokens.push_back(*t);
            cur.clear();
        }
    }
    return tokens;
}

std::string mu::dorico::pitchTokensToString(const std::vector<PitchToken>& tokens)
{
    static const char LETTERS[] = "cdefgab";
    std::string out;
    for (const PitchToken& t : tokens) {
        out += out.empty() ? "" : ",";
        out += LETTERS[t.step];
        out += t.alter == 2 ? std::string("x") : std::string(static_cast<size_t>(std::abs(t.alter)), t.alter < 0 ? 'b' : '#');
        if (t.octave) {
            out += std::to_string(*t.octave);
        }
    }
    return out;
}

bool mu::dorico::matchesPitch(const std::vector<PitchToken>& tokens, const FilterPitch& pitch)
{
    return std::any_of(tokens.begin(), tokens.end(), [&](const PitchToken& t) {
        return t.step == pitch.step && t.alter == pitch.alter && (!t.octave || *t.octave == pitch.octave);
    });
}
