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

#include "popoverregistry.h"

#include <algorithm>
#include <memory>

#include "handlers/barshandler.h"
#include "handlers/clefhandler.h"
#include "handlers/dynamicshandler.h"
#include "handlers/holdshandler.h"
#include "handlers/keysighandler.h"
#include "handlers/ornamentshandler.h"
#include "handlers/techniqueshandler.h"
#include "handlers/tempohandler.h"
#include "handlers/timesighandler.h"

using namespace mu::dorico;
using namespace mu::engraving;
using IconCode = muse::ui::IconCode::Code;

bool PopoverRegistry::add(PopoverDef def)
{
    if (find(def.id)) {
        return false;
    }
    m_defs.push_back(std::move(def));
    return true;
}

const PopoverDef* PopoverRegistry::find(const std::string& id) const
{
    auto it = std::find_if(m_defs.begin(), m_defs.end(), [&](const PopoverDef& d) { return d.id == id; });
    return it == m_defs.end() ? nullptr : &*it;
}

const PopoverDef* PopoverRegistry::findByType(ElementType type) const
{
    auto it = std::find_if(m_defs.begin(), m_defs.end(), [&](const PopoverDef& d) {
        return std::find(d.itemTypes.begin(), d.itemTypes.end(), type) != d.itemTypes.end();
    });
    return it == m_defs.end() ? nullptr : &*it;
}

//! A popover whose handler spec is not implemented yet: it opens, but every entry is "not supported yet".
static PopoverDef placeholder(const std::string& id, const std::string& title, IconCode icon)
{
    PopoverDef def;
    def.id = id;
    def.title = title;
    def.icon = icon;
    def.validate = [](const std::string& text) { return text.empty() ? ParseStatus::Incomplete : ParseStatus::Unsupported; };
    def.apply = [](const std::string&, ApplyScope, mu::notation::INotationPtr) { return false; };
    def.prefill = [](const EngravingItem*) { return std::string(); };
    return def;
}

PopoverRegistry mu::dorico::makePopoverRegistry()
{
    PopoverRegistry r;
    r.add(barsPopover());                                                                  // spec 25
    r.add(clefPopover());                                                                  // spec 24
    r.add(dynamicsPopover());                                                              // spec 21
    r.add(placeholder("fingering", "Fingering", IconCode::FRETBOARD_DIAGRAM));             // spec 34
    r.add(placeholder("figuredbass", "Figured Bass", IconCode::MUSIC_NOTES));              // spec 36
    r.add(holdsPopover());                                                                 // spec 27
    r.add(placeholder("notetools", "Note Tools", IconCode::NOTE_HEAD));                    // spec 35
    r.add(keySigPopover());                                                                // spec 23
    r.add(placeholder("lyrics", "Lyrics", IconCode::LYRICS));                              // spec 33
    r.add(timeSigPopover());                                                               // spec 22
    r.add(ornamentsPopover());                                                             // spec 28
    r.add(techniquesPopover());                                                            // spec 29
    r.add(placeholder("chordsymbols", "Chord Symbols", IconCode::CHORD_SYMBOL));           // spec 32
    r.add(placeholder("repeats", "Repeats and Tremolos", IconCode::REPEAT_START));         // spec 30
    r.add(tempoPopover());                                                                 // spec 26
    r.add(placeholder("tuplets", "Tuplets", IconCode::NOTE_TUPLET));                       // spec 31
    return r;
}
