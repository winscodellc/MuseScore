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

#include <string>
#include <vector>

#include "../popovertypes.h"

namespace mu::dorico {
//! One token of a dynamics popover entry (spec 21). `p<f>p` = Immediate p, Hairpin cresc, Immediate f, Hairpin dim,
//! Immediate p.
struct DynamicItem {
    enum class Kind { Immediate, Hairpin, GradualText };
    enum class Niente { None, Circle, Letter }; // o< / >o, n< / >n

    Kind kind = Kind::Immediate;

    // Immediate: MuseScore dynamic names ("mf", "sfz"); f-p / f:p / f/p also set `second` and `separator`.
    std::string dynamic;
    std::string second;
    char separator = 0;
    std::string modifier;        // "molto", "subito", or free text ("espressivo")
    bool modifierBefore = false; // molto, poco, più, meno are placed before the dynamic

    // Hairpin and GradualText
    bool cresc = true;
    std::string text;  // GradualText: "cresc.", "dim.", "decresc."
    bool line = false; // GradualText with a continuation line (cresc...)
    Niente niente = Niente::None;
};

struct DynamicEntry {
    std::vector<DynamicItem> items;
    bool parenthesised = false; // (f)
    bool stop = false;          // ?: stops an open-ended gradual dynamic during note input

    //! Ends with a gradual dynamic (p<, f>): at the caret it extends until stopped.
    bool openEnded() const { return !items.empty() && items.back().kind != DynamicItem::Kind::Immediate; }
};

struct DynamicsParse {
    ParseStatus status = ParseStatus::Invalid;
    DynamicEntry entry;
};

DynamicsParse parseDynamics(const std::string& text);
}
