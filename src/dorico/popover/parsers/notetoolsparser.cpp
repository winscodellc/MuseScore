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

#include "notetoolsparser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

using namespace mu::dorico;

// Named, not anonymous: the unity build compiles the parsers together.
namespace notetoolsparse {
struct ItemParse {
    ParseStatus status = ParseStatus::Invalid;
    IntervalSpec spec;
};

bool isNoteToolsDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool isAlpha(char c)
{
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trimNoteTools(const std::string& s)
{
    const size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a, s.find_last_not_of(" \t") - a + 1);
}

// `part` is a valid start of `word` but not all of it.
bool isProperPrefix(const std::string& part, const std::string& word)
{
    return !part.empty() && part.size() < word.size() && word.compare(0, part.size(), part) == 0;
}

bool isProperPrefixOfAny(const std::string& part, const std::vector<std::string>& words)
{
    return std::any_of(words.begin(), words.end(), [&](const std::string& w) { return isProperPrefix(part, w); });
}

bool isOneOf(const std::string& s, const std::vector<std::string>& words)
{
    return std::find(words.begin(), words.end(), s) != words.end();
}

// The other note tools (repeat): valid Dorico, not implemented.
const std::vector<std::string> UNSUPPORTED_TOOLS = { "rep", "repeat" };

const std::vector<std::string> INVERT_WORDS = { "inv", "invert", "inversion", "mirror" };
const std::vector<std::string> REVERSE_WORDS = { "ret", "retrograde", "rev", "reverse" };
const std::vector<std::string> ROTATE_WORDS = { "rot", "rotate" };
// Everything else a transformation entry may contain.
const std::vector<std::string> TRANSFORM_MODIFIERS = {
    "pitch", "pt", "pit", "rhythm", "rhy", "rm", "diatonic", "diat", "chromatic", "chrom", "root", "first", "top", "bottom",
    "backwards", "bw", "prev", "player", "voice"
};

const std::vector<std::string> SEMITONE_UNITS = { "s", "st", "semitones" };
const std::vector<std::string> DIVISION_UNITS = { "d", "div", "divisions" };
const std::vector<std::string> QUARTERTONE_UNITS = { "q", "qt", "quartertones" };
const std::vector<std::string> ALL_UNIT_WORDS = { "semitones", "divisions", "quartertones" };
// Interval stores its steps in 8 bits.
constexpr int MAX_UNITS = 100;

const std::vector<std::string> ORDINALS = { "st", "nd", "rd", "th" };

struct QualityWord {
    const char* word;
    IntervalQuality quality;
};

// Everything but `M` and `m` is case-insensitive.
const QualityWord QUALITY_WORDS[] = {
    { "maj", IntervalQuality::Major }, { "major", IntervalQuality::Major },
    { "min", IntervalQuality::Minor }, { "minor", IntervalQuality::Minor },
    { "perf", IntervalQuality::Perfect }, { "per", IntervalQuality::Perfect }, { "p", IntervalQuality::Perfect },
    { "diminished", IntervalQuality::Diminished }, { "dim", IntervalQuality::Diminished }, { "d", IntervalQuality::Diminished },
    { "augmented", IntervalQuality::Augmented }, { "aug", IntervalQuality::Augmented }, { "a", IntervalQuality::Augmented },
    { "diatonic", IntervalQuality::None }, { "diat", IntervalQuality::None },
};

// Sets `quality` for a complete word; `partial` for the start of one.
bool lookupQuality(const std::string& word, IntervalQuality& quality, bool& partial)
{
    partial = false;
    if (word == "M" || word == "m") {
        quality = word == "M" ? IntervalQuality::Major : IntervalQuality::Minor;
        return true;
    }
    const std::string lw = lower(word);
    for (const QualityWord& q : QUALITY_WORDS) {
        if (lw == q.word) {
            quality = q.quality;
            return true;
        }
    }
    for (const QualityWord& q : QUALITY_WORDS) {
        if (isProperPrefix(lw, q.word)) {
            partial = true;
            return false;
        }
    }
    return false;
}

// Perfect intervals are unison, fourth, fifth (and their octaves); the rest are major / minor.
bool isPerfectDegree(int degree)
{
    const int cls = (std::abs(degree) - 1) % 7;
    return cls == 0 || cls == 3 || cls == 4;
}

// `[-][quality]degree[ordinal][ count unit]`, e.g. `3`, `-4th`, `M3`, `dim5`, `5 8 st`.
ItemParse parseItem(const std::string& item, bool transpose)
{
    ItemParse out;
    const size_t n = item.size();
    size_t i = 0;

    bool negative = false;
    if (item[0] == '-') {
        negative = true;
        ++i;
    }

    IntervalQuality quality = IntervalQuality::None;
    const size_t q0 = i;
    while (i < n && isAlpha(item[i])) {
        ++i;
    }
    if (i > q0) {
        bool partial = false;
        if (!lookupQuality(item.substr(q0, i - q0), quality, partial)) {
            out.status = partial && i == n ? ParseStatus::Incomplete : ParseStatus::Invalid;
            return out;
        }
    }

    const size_t d0 = i;
    while (i < n && isNoteToolsDigit(item[i])) {
        ++i;
    }
    if (i == d0) {
        out.status = i == n ? ParseStatus::Incomplete : ParseStatus::Invalid; // `-`, `M`
        return out;
    }
    if (i - d0 > 3) {
        return out;
    }
    const int degree = std::stoi(item.substr(d0, i - d0));
    if (degree < 1 || degree > 15) {
        return out;
    }

    // `3rd`, `4th`
    const size_t s0 = i;
    while (i < n && isAlpha(item[i])) {
        ++i;
    }
    if (i > s0) {
        const std::string suffix = lower(item.substr(s0, i - s0));
        if (!isOneOf(suffix, ORDINALS)) {
            out.status = i == n && isProperPrefixOfAny(suffix, ORDINALS) ? ParseStatus::Incomplete : ParseStatus::Invalid;
            return out;
        }
    }

    // `5 8 st`
    std::optional<int> semitones;
    while (i < n && item[i] == ' ') {
        ++i;
    }
    if (i < n) {
        const size_t c0 = i;
        while (i < n && isNoteToolsDigit(item[i])) {
            ++i;
        }
        if (i == c0 || i - c0 > 3) {
            return out;
        }
        const int count = std::stoi(item.substr(c0, i - c0));
        if (count > MAX_UNITS) {
            return out;
        }
        while (i < n && item[i] == ' ') {
            ++i;
        }
        if (i == n) {
            out.status = ParseStatus::Incomplete;
            return out;
        }
        const size_t u0 = i;
        while (i < n && isAlpha(item[i])) {
            ++i;
        }
        const std::string unit = lower(item.substr(u0, i - u0));
        if (i != n || unit.empty() || quality != IntervalQuality::None) {
            return out;
        }
        if (isOneOf(unit, SEMITONE_UNITS) || isOneOf(unit, DIVISION_UNITS)) { // 12-EDO: octave divisions are semitones
            semitones = count;
        } else if (isOneOf(unit, QUARTERTONE_UNITS)) {
            out.status = ParseStatus::Unsupported; // no 24-EDO
            return out;
        } else {
            out.status = isProperPrefixOfAny(unit, ALL_UNIT_WORDS) ? ParseStatus::Incomplete : ParseStatus::Invalid;
            return out;
        }
    }

    const bool perfectOnly = quality == IntervalQuality::Perfect;
    const bool majorMinor = quality == IntervalQuality::Major || quality == IntervalQuality::Minor;
    if ((perfectOnly && !isPerfectDegree(degree)) || (majorMinor && isPerfectDegree(degree))) {
        return out; // `p3`, `M5`
    }

    out.spec.degree = negative ? -degree : degree;
    out.spec.quality = quality;
    out.spec.semitones = semitones;
    // A plain unison adds nothing, and `1` may still become `12`: wait.
    out.status = !transpose && degree == 1 && out.spec.followsKey() ? ParseStatus::Incomplete : ParseStatus::Ok;
    return out;
}

std::vector<std::string> splitWords(const std::string& s)
{
    std::vector<std::string> words;
    size_t i = 0;
    while (i < s.size()) {
        const size_t a = s.find_first_not_of(' ', i);
        if (a == std::string::npos) {
            break;
        }
        const size_t b = s.find(' ', a);
        words.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a));
        i = b == std::string::npos ? s.size() : b;
    }
    return words;
}

// The unsupported tools and mappings, and the start of one; nullopt if the text is none of them.
std::optional<ParseStatus> otherToolStatus(const std::string& text)
{
    const std::string lt = lower(text);
    size_t a = 0;
    while (a < lt.size() && isAlpha(lt[a])) {
        ++a;
    }
    const std::string first = lt.substr(0, a);
    if (isOneOf(first, UNSUPPORTED_TOOLS)) {
        return ParseStatus::Unsupported;
    }
    if (a == lt.size() && isProperPrefixOfAny(first, UNSUPPORTED_TOOLS)) {
        return ParseStatus::Incomplete;
    }
    if (lt.find('=') != std::string::npos) {
        return ParseStatus::Unsupported; // pitch mapping
    }
    const std::vector<std::string> words = splitWords(lt);
    for (size_t i = 0; i < words.size(); ++i) {
        // `snap`, `spell`, `to` (scale mapping); a trailing `to` may be the start of `top`
        if (words[i] == "snap" || words[i] == "spell" || (words[i] == "to" && i + 1 < words.size())) {
            return ParseStatus::Unsupported;
        }
    }
    return std::nullopt;
}

ParseStatus worse(ParseStatus a, ParseStatus b)
{
    auto rank = [](ParseStatus s) {
        switch (s) {
        case ParseStatus::Invalid: return 3;
        case ParseStatus::Unsupported: return 2;
        case ParseStatus::Incomplete: return 1;
        case ParseStatus::Ok: return 0;
        }
        return 0;
    };
    return rank(a) >= rank(b) ? a : b;
}

// A note name and its octave: `c4`, `eb4`, `g#6`, `bb-1`. `partial` for one that still lacks its octave.
bool parseCentreName(const std::string& w, TransformSpec& spec, bool& partial)
{
    static const int LETTER[7] = { 5, 6, 0, 1, 2, 3, 4 }; // a..g -> C = 0
    partial = false;
    if (w.empty() || w[0] < 'a' || w[0] > 'g') {
        return false;
    }
    size_t i = 1;
    int alter = 0;
    while (i < w.size() && (w[i] == '#' || w[i] == 'b')) {
        alter += w[i] == '#' ? 1 : -1;
        ++i;
    }
    const size_t d = i + (i < w.size() && w[i] == '-' ? 1 : 0);
    if (d == w.size()) {
        partial = true; // `e`, `eb`, `e-`
        return false;
    }
    if (std::abs(alter) > 2 || w.size() - d > 2 || !std::all_of(w.begin() + d, w.end(), isNoteToolsDigit)) {
        return false;
    }
    spec.letter = LETTER[w[0] - 'a'];
    spec.alter = alter;
    spec.octave = std::stoi(w.substr(i));
    return true;
}

// Invert, reverse and rotate entries: `inv`, `inveb4`, `ret inv pt`, `rot rm bw 3`. nullopt if the text is not one.
std::optional<NoteToolsParse> parseTransform(const std::string& lt)
{
    std::vector<std::string> words = splitWords(lt);
    if (words.empty()) {
        return std::nullopt;
    }
    std::vector<std::string> tools = INVERT_WORDS;
    tools.insert(tools.end(), REVERSE_WORDS.begin(), REVERSE_WORDS.end());
    tools.insert(tools.end(), ROTATE_WORDS.begin(), ROTATE_WORDS.end());

    NoteToolsParse out;
    if (!isOneOf(words[0], tools)) {
        if (isProperPrefixOfAny(words[0], tools)) {
            out.status = words.size() == 1 ? ParseStatus::Incomplete : ParseStatus::Invalid;
            return out;
        }
        // `inveb4`, `rot2`: the tool word with the rest glued on
        std::string tool;
        for (const std::string& t : tools) {
            if (t.size() < words[0].size() && words[0].compare(0, t.size(), t) == 0 && t.size() > tool.size()) {
                tool = t;
            }
        }
        if (tool.empty()) {
            return std::nullopt;
        }
        words.insert(words.begin() + 1, words[0].substr(tool.size()));
        words[0] = tool;
    }

    std::vector<std::string> vocabulary = tools;
    vocabulary.insert(vocabulary.end(), TRANSFORM_MODIFIERS.begin(), TRANSFORM_MODIFIERS.end());

    TransformSpec spec;
    bool inv = false, rev = false, rot = false, pt = false, rm = false;
    bool centreSet = false, invertOnly = false, rotateOnly = false, backwards = false, needSteps = false;
    ParseStatus status = ParseStatus::Ok;
    for (size_t i = 0; i < words.size(); ++i) {
        const std::string& w = words[i];
        const bool last = i + 1 == words.size();
        bool partial = false;
        if (isOneOf(w, INVERT_WORDS)) {
            inv = true;
        } else if (isOneOf(w, REVERSE_WORDS)) {
            rev = true;
        } else if (isOneOf(w, ROTATE_WORDS)) {
            rot = true;
        } else if (w == "pitch" || w == "pt" || w == "pit") {
            pt = true;
        } else if (w == "rhythm" || w == "rhy" || w == "rm") {
            rm = true;
        } else if (w == "diatonic" || w == "diat" || w == "chromatic" || w == "chrom") {
            spec.diatonic = w[0] == 'd';
            invertOnly = true;
        } else if (w == "root" || w == "first" || w == "top" || w == "bottom") {
            status = centreSet ? ParseStatus::Invalid : status;
            centreSet = true;
            spec.centre = w == "root" ? InvertCentre::Root : w == "first" ? InvertCentre::First
                          : w == "top" ? InvertCentre::Top : InvertCentre::Bottom;
            invertOnly = true;
        } else if (w == "backwards" || w == "bw" || w == "prev" || w == "-") {
            backwards = true;
            rotateOnly = true;
            needSteps = last; // `rot bw` waits for its count
        } else if (w == "voice") {
            // the transformation only touches the selected voices anyway
        } else if (w == "player") {
            status = worse(status, ParseStatus::Unsupported);
        } else if (isNoteToolsDigit(w[0]) || (w[0] == '-' && w.size() > 1 && isNoteToolsDigit(w[1]))) {
            const bool negative = w[0] == '-';
            const std::string digits = w.substr(negative ? 1 : 0);
            if (digits.size() > 3 || !std::all_of(digits.begin(), digits.end(), isNoteToolsDigit)) {
                status = ParseStatus::Invalid;
                continue;
            }
            backwards = backwards || negative;
            spec.steps = std::stoi(digits);
            rotateOnly = true;
            needSteps = false;
        } else if (parseCentreName(w, spec, partial)) {
            status = centreSet ? ParseStatus::Invalid : status;
            centreSet = true;
            spec.centre = InvertCentre::Pitch;
            invertOnly = true;
        } else if (last && (partial || isProperPrefixOfAny(w, vocabulary))) {
            status = worse(status, ParseStatus::Incomplete);
        } else {
            status = ParseStatus::Invalid;
        }
    }
    if (needSteps) {
        status = worse(status, ParseStatus::Incomplete);
    }

    const bool misplaced = (rot && (inv || rev)) || (rotateOnly && !rot) || (invertOnly && !inv);
    if (misplaced) {
        status = ParseStatus::Invalid;
    }
    spec.op = rot ? TransformOp::Rotate : rev ? TransformOp::Reverse : TransformOp::Invert;
    spec.invert = inv;
    if (spec.op == TransformOp::Invert) {
        if (rm) {
            status = ParseStatus::Invalid; // there is no rhythm to invert
        }
        spec.pitches = true;
        spec.rhythms = false;
    } else {
        spec.pitches = pt || !rm;
        spec.rhythms = rm || !pt;
        if (inv && !spec.pitches) {
            status = ParseStatus::Invalid;
        }
    }
    if (backwards) {
        spec.steps = -spec.steps;
    }
    out.status = status;
    if (status == ParseStatus::Ok) {
        out.transform = spec;
    }
    return out;
}
}

NoteToolsParse mu::dorico::parseNoteTools(const std::string& raw)
{
    using namespace notetoolsparse;
    NoteToolsParse out;
    const std::string text = trimNoteTools(raw);
    if (text.empty()) {
        out.status = ParseStatus::Incomplete;
        return out;
    }
    if (std::optional<NoteToolsParse> transform = parseTransform(lower(text))) {
        return *transform;
    }
    if (const std::optional<ParseStatus> other = otherToolStatus(text)) {
        out.status = *other;
        return out;
    }

    // `t3`, `t -6`, `t top -8`; `top` is not a transposition.
    std::string rest = text;
    if (text[0] == 't' && (text.size() == 1 || isNoteToolsDigit(text[1]) || text[1] == '-' || text[1] == ' ')) {
        out.transpose = true;
        rest = trimNoteTools(text.substr(1));
    }

    // `top` / `bottom` may stand anywhere between the intervals; the start of one waits.
    std::vector<std::string> words = splitWords(rest);
    bool scopePending = false;
    for (size_t i = 0; i < words.size();) {
        const std::string w = lower(words[i]);
        if (w == "top" || w == "bottom") {
            out.scope = w == "top" ? NoteToolsScope::Top : NoteToolsScope::Bottom;
            words.erase(words.begin() + i);
        } else if (i + 1 == words.size() && (isProperPrefix(w, "top") || isProperPrefix(w, "bottom"))) {
            scopePending = true;
            words.erase(words.begin() + i);
        } else {
            ++i;
        }
    }
    std::string body;
    for (const std::string& w : words) {
        body += (body.empty() ? "" : " ") + w;
    }
    if (body.empty()) {
        out.status = ParseStatus::Incomplete;
        return out;
    }

    std::vector<std::string> items;
    size_t from = 0;
    for (size_t comma; (comma = body.find(',', from)) != std::string::npos; from = comma + 1) {
        items.push_back(trimNoteTools(body.substr(from, comma - from)));
    }
    items.push_back(trimNoteTools(body.substr(from)));

    ParseStatus status = scopePending ? ParseStatus::Incomplete : ParseStatus::Ok;
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].empty()) {
            status = worse(status, i + 1 == items.size() ? ParseStatus::Incomplete : ParseStatus::Invalid); // `3,`
            continue;
        }
        const ItemParse item = parseItem(items[i], out.transpose);
        status = worse(status, item.status);
        out.intervals.push_back(item.spec);
    }
    if (out.transpose && items.size() > 1) {
        status = ParseStatus::Invalid;
    }
    out.status = status;
    if (status != ParseStatus::Ok) {
        out.intervals.clear();
    }
    return out;
}

IntervalSteps mu::dorico::toSteps(const IntervalSpec& spec)
{
    static const int MAJOR_OR_PERFECT[7] = { 0, 2, 4, 5, 7, 9, 11 };
    const int sign = spec.degree < 0 ? -1 : 1;
    const int steps = std::abs(spec.degree) - 1;
    IntervalSteps out;
    out.diatonic = sign * steps;
    if (spec.semitones) {
        out.semitones = sign * *spec.semitones;
        return out;
    }

    const bool perfect = notetoolsparse::isPerfectDegree(spec.degree);
    int semitones = MAJOR_OR_PERFECT[steps % 7] + 12 * (steps / 7);
    switch (spec.quality) {
    case IntervalQuality::Minor: semitones -= 1;
        break;
    case IntervalQuality::Diminished: semitones -= perfect ? 1 : 2;
        break;
    case IntervalQuality::Augmented: semitones += 1;
        break;
    case IntervalQuality::None:
    case IntervalQuality::Major:
    case IntervalQuality::Perfect:
        break;
    }
    out.semitones = sign * semitones;
    return out;
}
