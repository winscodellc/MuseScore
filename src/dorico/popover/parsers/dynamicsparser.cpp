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

#include "dynamicsparser.h"

#include <cstring>

using namespace mu::dorico;

namespace {
// MuseScore's dynamic names (TConv xml names), minus the single letters m, r, s, z.
const char* const DYNAMICS[] = {
    "pppppp", "ppppp", "pppp", "ppp", "pp", "p", "mp", "mf", "f", "ff", "fff", "ffff", "fffff", "ffffff",
    "fp", "pf", "sf", "sfz", "sff", "sffz", "sfff", "sfffz", "sfp", "sfpp", "rfz", "rf", "fz", "n"
};

struct Gradual {
    const char* word;
    bool cresc;
    const char* text;
};
const Gradual GRADUALS[] = {
    { "cresc", true, "cresc." }, { "crescendo", true, "cresc." }, { "dim", false, "dim." }, { "diminuendo", false, "dim." },
    { "decresc", false, "decresc." }, { "decrescendo", false, "decresc." }
};

struct Modifier {
    const char* word;
    const char* text;
    bool before;
};
const Modifier MODIFIERS[] = {
    { "subito", "subito", false }, { "sub", "subito", false }, { "sub.", "subito", false },
    { "possibile", "possibile", false }, { "poss", "possibile", false }, { "poss.", "possibile", false },
    { "poco", "poco", true }, { "molto", "molto", true }, { "piu", "più", true }, { "più", "più", true },
    { "meno", "meno", true }, { "mosso", "mosso", false }
};

bool isWordChar(char c)
{
    const unsigned char u = static_cast<unsigned char>(c);
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || u >= 0x80;
}

bool isDynamic(const std::string& w)
{
    for (const char* d : DYNAMICS) {
        if (w == d) {
            return true;
        }
    }
    return false;
}

bool startsWith(const char* s, const std::string& prefix)
{
    return std::strncmp(s, prefix.c_str(), prefix.size()) == 0;
}

// The start of a dynamic or gradual word: typing it is not an error yet.
bool isPrefix(const std::string& w)
{
    for (const char* d : DYNAMICS) {
        if (startsWith(d, w)) {
            return true;
        }
    }
    for (const Gradual& g : GRADUALS) {
        if (startsWith(g.word, w)) {
            return true;
        }
    }
    return false;
}

const Gradual* findGradual(const std::string& w)
{
    for (const Gradual& g : GRADUALS) {
        if (w == g.word) {
            return &g;
        }
    }
    return nullptr;
}

std::string trim(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    if (b == std::string::npos) {
        return std::string();
    }
    return s.substr(b, s.find_last_not_of(' ') - b + 1);
}

DynamicsParse result(ParseStatus status, DynamicEntry entry = {})
{
    return { status, std::move(entry) };
}
}

DynamicsParse mu::dorico::parseDynamics(const std::string& input)
{
    const std::string t = trim(input);
    if (t.empty()) {
        return result(ParseStatus::Incomplete);
    }

    DynamicEntry e;
    if (t == "?") {
        e.stop = true;
        return result(ParseStatus::Ok, e);
    }

    if (t.front() == '(') {
        if (t.size() < 2 || t.back() != ')') {
            DynamicsParse inner = parseDynamics(t.substr(1));
            return result(inner.status == ParseStatus::Invalid ? ParseStatus::Invalid : ParseStatus::Incomplete);
        }
        DynamicsParse inner = parseDynamics(t.substr(1, t.size() - 2));
        if (inner.status == ParseStatus::Ok) {
            inner.entry.parenthesised = true;
        }
        return inner;
    }

    DynamicItem::Niente pendingNiente = DynamicItem::Niente::None; // "o" / "n" just before '<'
    const size_t n = t.size();
    size_t i = 0;
    while (i < n) {
        const char c = t[i];
        if (c == ' ') {
            ++i;
            continue;
        }

        if (c == '<' || c == '>') {
            DynamicItem h;
            h.kind = DynamicItem::Kind::Hairpin;
            h.cresc = c == '<';
            h.niente = pendingNiente;
            pendingNiente = DynamicItem::Niente::None;
            ++i;
            // ">o" / ">n": the diminuendo ends at niente.
            if (!h.cresc && i < n && (t[i] == 'o' || t[i] == 'n') && (i + 1 == n || !isWordChar(t[i + 1]))) {
                h.niente = t[i] == 'o' ? DynamicItem::Niente::Circle : DynamicItem::Niente::Letter;
                ++i;
            }
            e.items.push_back(h);
            continue;
        }

        if (!isWordChar(c)) {
            return result(ParseStatus::Invalid);
        }

        size_t j = i;
        while (j < n && isWordChar(t[j])) {
            ++j;
        }
        const std::string w = t.substr(i, j - i);

        if ((w == "o" || w == "n") && j < n && t[j] == '<') {
            pendingNiente = w == "o" ? DynamicItem::Niente::Circle : DynamicItem::Niente::Letter;
            i = j;
            continue;
        }

        if (isDynamic(w)) {
            if (!e.items.empty() && e.items.back().kind == DynamicItem::Kind::Immediate) {
                return result(ParseStatus::Invalid); // two immediate dynamics need a gradual one between them
            }
            DynamicItem d;
            d.dynamic = w;
            if (j < n && (t[j] == '-' || t[j] == ':' || t[j] == '/')) {
                d.separator = t[j];
                size_t k = j + 1;
                while (k < n && isWordChar(t[k])) {
                    ++k;
                }
                d.second = t.substr(j + 1, k - j - 1);
                if (!isDynamic(d.second)) {
                    return result(k == n && isPrefix(d.second) ? ParseStatus::Incomplete : ParseStatus::Invalid);
                }
                j = k;
            }
            e.items.push_back(d);
            i = j;
            continue;
        }

        // Gradual word, optionally followed by "..." for a continuation line.
        const size_t base = w.find_last_not_of('.') + 1;
        const size_t dots = w.size() - base;
        if (const Gradual* g = base > 0 ? findGradual(w.substr(0, base)) : nullptr) {
            if (dots > 3) {
                return result(ParseStatus::Invalid);
            }
            if (dots == 2) {
                return result(j == n ? ParseStatus::Incomplete : ParseStatus::Invalid);
            }
            DynamicItem gt;
            gt.kind = DynamicItem::Kind::GradualText;
            gt.cresc = g->cresc;
            gt.text = g->text;
            gt.line = dots == 3;
            e.items.push_back(gt);
            i = j;
            continue;
        }

        // Anything else is a modifier: the rest of the entry, attached to the last immediate dynamic.
        auto last = e.items.rbegin();
        while (last != e.items.rend() && last->kind != DynamicItem::Kind::Immediate) {
            ++last;
        }
        if (last == e.items.rend() || i == 0 || t[i - 1] != ' ') {
            const bool typing = j == n && (isPrefix(w) || w == "o");
            return result(typing ? ParseStatus::Incomplete : ParseStatus::Invalid);
        }
        last->modifier = trim(t.substr(i));
        for (const Modifier& m : MODIFIERS) {
            if (last->modifier == m.word) {
                last->modifier = m.text;
                last->modifierBefore = m.before;
            }
        }
        break;
    }

    if (pendingNiente != DynamicItem::Niente::None) {
        return result(ParseStatus::Incomplete);
    }
    return e.items.empty() ? result(ParseStatus::Invalid) : result(ParseStatus::Ok, e);
}
