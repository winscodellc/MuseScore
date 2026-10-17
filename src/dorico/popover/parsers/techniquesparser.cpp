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

#include "techniquesparser.h"

#include <algorithm>
#include <cctype>
#include <regex>

using namespace mu::dorico;
using namespace mu::engraving;
using PType = PlayingTechniqueType;
using PedalAction = TechniquesEntry::Pedal;

namespace {
// The manual's "common techniques" table. It says many more exist: add rows as they are needed.
const std::vector<Technique> TECHNIQUES = {
    { "nat",                 "nat.",           PType::Natural },
    { "vibrato",             "vibrato",        PType::Vibrato },
    { "senza vibrato",       "senza vibrato",  PType::Natural },
    { "legato",              "legato",         PType::Legato },
    { "o",                   "",               PType::Undefined,     SymId::stringsHarmonic },
    { "con sord",            "con sord.",      PType::Mute },
    { "senza sord",          "senza sord.",    PType::Open },
    { "stopped",             "stopped",        PType::Undefined },
    { "+",                   "",               PType::Undefined,     SymId::brassMuteClosed },
    { "downbow",             "",               PType::Undefined,     SymId::stringsDownBow },
    { "upbow",               "",               PType::Undefined,     SymId::stringsUpBow },
    { "strong air pressure", "",               PType::Undefined,     SymId::windStrongAirPressure },
    { "double-tongue",       "",               PType::Undefined,     SymId::doubleTongueAbove },
    { "fluttertongue",       "f.t.",           PType::Undefined },
    { "flt",                 "flt.",           PType::Undefined },
    { "flz",                 "flz.",           PType::Undefined },
    { "sul pont",            "sul pont.",      PType::SulPonticello },
    { "sul tasto",           "sul tasto",      PType::SulTasto },
    { "pst",                 "poco sul tasto", PType::SulTasto },
    { "pizz",                "pizz.",          PType::Pizzicato },
    { "spicc",               "spicc.",         PType::Undefined },
    { "arco",                "arco",           PType::Natural },
    { "tongue click",        "",               PType::Undefined,     SymId::vocalTongueClickStockhausen },
    { "finger click",        "",               PType::Undefined,     SymId::vocalFingerClickStockhausen },
    { "motor on",            "motor on",       PType::Undefined },
    { "motor off",           "motor off",      PType::Undefined },
    { "open",                "open",           PType::Open },
    { "damp",                "",               PType::Undefined,     SymId::pictDamp1 },
    { "damp large",          "",               PType::Undefined,     SymId::pictDamp2 },
    { "full barre",          "B",              PType::Undefined },
    { "half barre",          "½B",             PType::Undefined },
    { "strum up",            "",               PType::Undefined,     SymId::guitarStrumUp },
    { "strum down",          "",               PType::Undefined,     SymId::guitarStrumDown },
    { "lh",                  "l.h.",           PType::Undefined },
    { "rh",                  "r.h.",           PType::Undefined },
};

const std::vector<std::pair<std::string, PedalAction> > PEDAL_WORDS = {
    { "ped", PedalAction::Start }, { "*", PedalAction::Stop }, { "^", PedalAction::Retake }, { "notch", PedalAction::Retake },
    { "retake", PedalAction::Retake }, { "nonotch", PedalAction::RemoveRetake },
};

// Valid Dorico entries without a MuseScore item yet: pedal levels, sostenuto and una corda lines.
const std::vector<std::string> UNSUPPORTED_TECHNIQUES = { "1/4", "1/2", "3/4", "1", "sost", "s*", "unacorda", "u*" };

const std::vector<std::string> HARMONIC_STARTS = { "harm", "harmonic", "harmonics" };

const std::vector<std::string> HARMONIC_WORDS = {
    "nat", "natural", "art", "artificial", "nh", "hh", "ah", "ph", "circle", "circleabove", "diamond", "diamondnotehead",
    "sounding", "white", "whitediamondnotehead", "whitediamond", "touched", "normal", "pinch", "singlesounding",
    "stopped", "singlestopped", "open", "noopen", "paren", "noparen", "noharm",
};

// Longest first, so "partial=4" is not read as "p" + "artial=4".
const std::vector<std::string> HARMONIC_KEYS = { "partial=", "partial", "string=", "string", "node=", "node", "p=", "p",
                                                 "s=", "s", "n=", "n" };

const size_t TECH_MAX_SUGGESTIONS = 8;

std::string techLower(std::string s)
{
    for (char& c : s) {
        c = char(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string techTrim(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    return b == std::string::npos ? std::string() : s.substr(b, s.find_last_not_of(' ') - b + 1);
}

bool techPrefix(const std::string& word, const std::string& prefix)
{
    return word.compare(0, prefix.size(), prefix) == 0;
}

bool isPrefixOfAny(const std::vector<std::string>& words, const std::string& prefix)
{
    return std::any_of(words.begin(), words.end(), [&](const std::string& w) { return techPrefix(w, prefix); });
}

const Technique* findTechnique(const std::string& entry)
{
    auto it = std::find_if(TECHNIQUES.begin(), TECHNIQUES.end(), [&](const Technique& t) { return t.entry == entry; });
    return it == TECHNIQUES.end() ? nullptr : &*it;
}

bool isTechniquePrefix(const std::string& prefix)
{
    return std::any_of(TECHNIQUES.begin(), TECHNIQUES.end(), [&](const Technique& t) { return techPrefix(t.entry, prefix); });
}

// One technique of an entry: "pizz", "(arco)", "[pizz]", or the start of one.
ParseStatus parseItem(const std::string& part, TechniqueItem& item)
{
    std::string inner = part;
    const bool bracketed = !inner.empty() && (inner.front() == '(' || inner.front() == '[');
    bool closed = true;
    if (bracketed) {
        const char close = inner.front() == '(' ? ')' : ']';
        item.parenthesised = close == ')';
        item.hidden = close == ']';
        closed = inner.size() > 1 && inner.back() == close;
        inner = inner.substr(1, inner.size() - (closed ? 2 : 1));
    }
    inner = techTrim(inner);
    item.technique = findTechnique(inner);
    if (item.technique) {
        return closed ? ParseStatus::Ok : ParseStatus::Incomplete;
    }
    if (!closed) {
        return inner.empty() || isTechniquePrefix(inner) ? ParseStatus::Incomplete : ParseStatus::Invalid;
    }
    return !bracketed && isTechniquePrefix(inner) ? ParseStatus::Incomplete : ParseStatus::Invalid; // "(piz)" is closed
}

// "harm nat n1 s1 p4 diamond": valid entries are Unsupported (no harmonics in v1), checked token by token.
ParseStatus parseHarmonics(const std::vector<std::string>& tokens)
{
    for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string& t = tokens[i];
        const bool last = i + 1 == tokens.size();
        if (std::find(HARMONIC_WORDS.begin(), HARMONIC_WORDS.end(), t) != HARMONIC_WORDS.end()) {
            continue;
        }
        auto key = std::find_if(HARMONIC_KEYS.begin(), HARMONIC_KEYS.end(), [&](const std::string& k) { return techPrefix(t, k); });
        if (key != HARMONIC_KEYS.end()) {
            const std::string value = t.substr(key->size());
            const bool string = (*key)[0] == 's';
            static const std::regex NUMBER("[0-9]+");
            static const std::regex PITCH("[a-g][#b]?[0-9]");
            if (std::regex_match(value, NUMBER) || (string && std::regex_match(value, PITCH))) {
                continue;
            }
            if (last && (value.empty() || (string && std::regex_match(value, std::regex("[a-g][#b]?"))))) {
                return ParseStatus::Incomplete;
            }
        }
        if (!(last && isPrefixOfAny(HARMONIC_WORDS, t))) {
            return ParseStatus::Invalid;
        }
        return ParseStatus::Incomplete;
    }
    return ParseStatus::Unsupported;
}

// Harp pedaling: a diagram "--^|^---" (D C B | E F G A; - natural, ^ or b flat, v or # sharp) or note names
// "BbEb", "DCBbEbFGA". Valid entries are Unsupported in v1; Invalid when the text is neither.
ParseStatus parseHarpPedaling(const std::string& text)
{
    static const std::regex NOTES("([A-G][b#^v-]?)+");
    if (std::regex_match(text, NOTES)) {
        return ParseStatus::Unsupported;
    }
    size_t pedals = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '|' && pedals == 3 && (i == 0 || text[i - 1] != '|')) {
            continue;
        }
        if (std::string("-^vb#").find(text[i]) == std::string::npos) {
            return ParseStatus::Invalid;
        }
        ++pedals;
    }
    return pedals < 7 ? ParseStatus::Incomplete : pedals == 7 ? ParseStatus::Unsupported : ParseStatus::Invalid;
}

std::vector<std::string> splitWords(const std::string& text)
{
    std::vector<std::string> words;
    size_t start = 0;
    while (start < text.size()) {
        const size_t end = text.find(' ', start);
        if (end != start) {
            words.push_back(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return words;
}

// "pizz->arco", "vibrato->", "(arco)".
TechniquesParse parseChain(const std::string& text)
{
    std::vector<std::string> parts;
    for (size_t start = 0;;) {
        const size_t arrow = text.find("->", start);
        parts.push_back(techTrim(text.substr(start, arrow == std::string::npos ? std::string::npos : arrow - start)));
        if (arrow == std::string::npos) {
            break;
        }
        start = arrow + 2;
    }

    TechniquesParse result;
    result.entry.withDuration = parts.size() > 1 && parts.back().empty();
    if (result.entry.withDuration) {
        parts.pop_back();
    }
    // "pizz-" is on its way to "pizz->".
    std::string& last = parts.back();
    const bool arrowStarted = !last.empty() && last.back() == '-' && findTechnique(last.substr(0, last.size() - 1));
    if (arrowStarted) {
        last.pop_back();
    }

    ParseStatus status = ParseStatus::Ok;
    for (size_t i = 0; i < parts.size(); ++i) {
        TechniqueItem item;
        const ParseStatus s = parts[i].empty() ? ParseStatus::Invalid : parseItem(parts[i], item);
        if (s == ParseStatus::Invalid || (s == ParseStatus::Incomplete && i + 1 < parts.size())) {
            return { ParseStatus::Invalid, {} };
        }
        if (s == ParseStatus::Incomplete) {
            status = ParseStatus::Incomplete;
        }
        result.entry.items.push_back(item);
    }
    if (arrowStarted) {
        status = ParseStatus::Incomplete;
    }
    // "A->B->": a group with a duration line on its last technique. ponytail: groups are not implemented.
    if (status == ParseStatus::Ok && result.entry.withDuration && result.entry.items.size() > 1) {
        status = ParseStatus::Unsupported;
    }
    result.status = status;
    return result;
}
}

const std::vector<Technique>& mu::dorico::techniques()
{
    return TECHNIQUES;
}

TechniquesParse mu::dorico::parseTechniques(const std::string& input)
{
    const std::string text = techTrim(input);
    if (text.empty()) {
        return { ParseStatus::Incomplete, {} };
    }
    const std::string lower = techLower(text);

    if (lower == "?") {
        TechniquesParse p { ParseStatus::Ok, {} };
        p.entry.stop = true;
        return p;
    }
    for (const auto& [word, pedal] : PEDAL_WORDS) {
        if (lower == word) {
            TechniquesParse p { ParseStatus::Ok, {} };
            p.entry.pedal = pedal;
            return p;
        }
    }
    if (std::find(UNSUPPORTED_TECHNIQUES.begin(), UNSUPPORTED_TECHNIQUES.end(), lower) != UNSUPPORTED_TECHNIQUES.end()) {
        return { ParseStatus::Unsupported, {} };
    }

    const std::vector<std::string> words = splitWords(lower);
    if (std::find(HARMONIC_STARTS.begin(), HARMONIC_STARTS.end(), words.front()) != HARMONIC_STARTS.end()) {
        return { parseHarmonics(words), {} };
    }
    // String indicators "string 1" ... "string 9": no MuseScore item outside the staff.
    if (words.front() == "string" && !findTechnique(lower)) {
        if (words.size() == 1) {
            return { ParseStatus::Incomplete, {} };
        }
        const bool number = words.size() == 2 && words[1].size() == 1 && words[1][0] >= '1' && words[1][0] <= '9';
        return { number ? ParseStatus::Unsupported : ParseStatus::Invalid, {} };
    }

    TechniquesParse chain = parseChain(lower);
    if (chain.status != ParseStatus::Invalid) {
        return chain;
    }
    // Not a technique: the start of another entry, or harp pedaling (case-sensitive note names).
    std::vector<std::string> otherWords = UNSUPPORTED_TECHNIQUES;
    otherWords.insert(otherWords.end(), HARMONIC_STARTS.begin(), HARMONIC_STARTS.end());
    otherWords.push_back("string");
    for (const auto& [word, pedal] : PEDAL_WORDS) {
        otherWords.push_back(word);
    }
    if (isPrefixOfAny(otherWords, lower)) {
        return { ParseStatus::Incomplete, {} };
    }
    return { parseHarpPedaling(text), {} };
}

std::vector<std::string> mu::dorico::techniqueSuggestions(const std::string& input)
{
    const std::string text = techLower(techTrim(input));
    const size_t arrow = text.rfind("->");
    const std::string head = arrow == std::string::npos ? std::string() : text.substr(0, arrow + 2);
    std::string tail = techTrim(arrow == std::string::npos ? text : text.substr(arrow + 2));

    std::string open, close;
    if (!tail.empty() && (tail.front() == '(' || tail.front() == '[')) {
        open = tail.substr(0, 1);
        close = open == "(" ? ")" : "]";
        tail = techTrim(tail.substr(1));
    }

    std::vector<std::string> entries;
    for (const Technique& t : TECHNIQUES) {
        entries.push_back(t.entry);
    }
    if (head.empty() && open.empty()) {
        entries.insert(entries.end(), { "ped", "notch", "retake", "nonotch" });
    }

    std::vector<std::string> result;
    if (tail.empty()) {
        return result;
    }
    for (const std::string& e : entries) {
        if (techPrefix(e, tail) && e != tail && result.size() < TECH_MAX_SUGGESTIONS) { // the typed entry adds nothing
            result.push_back(head + open + e + close);
        }
    }
    return result;
}

std::string mu::dorico::techniqueEntryForText(const std::string& input)
{
    const std::string text = techTrim(input);
    const bool parenthesised = text.size() > 2 && text.front() == '(' && text.back() == ')';
    const std::string inner = parenthesised ? text.substr(1, text.size() - 2) : text;
    for (const Technique& t : TECHNIQUES) {
        if (!t.text.empty() && t.text == inner) {
            return parenthesised ? "(" + t.entry + ")" : t.entry;
        }
    }
    return std::string();
}

std::string mu::dorico::techniqueEntryForSymbol(SymId sym)
{
    if (sym == SymId::doubleTongueBelow) {
        sym = SymId::doubleTongueAbove;
    }
    for (const Technique& t : TECHNIQUES) {
        if (t.sym != SymId::noSym && t.sym == sym) {
            return t.entry;
        }
    }
    return std::string();
}
