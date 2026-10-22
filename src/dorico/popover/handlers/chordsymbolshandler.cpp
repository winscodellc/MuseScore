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

#include "chordsymbolshandler.h"

#include "engraving/dom/factory.h"
#include "engraving/dom/harmony.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationundostack.h"

#include "../parsers/chordsymbolsparser.h"

using namespace mu::dorico;
using namespace mu::engraving;

// Named, not anonymous: the unity build compiles the handlers together.
namespace chordsymbolspopover {
// Puts the chord symbol at the position, replacing the staff's chord symbol there. A beat with no note of its own
// gets an empty segment, as MuseScore's own chord symbol Space does.
Harmony* addHarmony(Score* score, const std::string& normalized, const PopoverPosition& pos)
{
    Measure* measure = score->tick2measure(pos.tick);
    if (!measure) {
        return nullptr;
    }
    Segment* segment = measure->undoGetSegment(SegmentType::ChordRest, pos.tick);
    const staff_idx_t staff = track2staff(pos.track);
    std::vector<EngravingItem*> old;
    for (EngravingItem* e : segment->annotations()) {
        if (e->isHarmony() && e->staffIdx() == staff) {
            old.push_back(e);
        }
    }
    for (EngravingItem* e : old) {
        score->undoRemoveElement(e);
    }

    Harmony* harmony = Factory::createHarmony(segment);
    harmony->setTrack(pos.track);
    harmony->setHarmonyType(HarmonyType::STANDARD);
    harmony->setHarmony(muse::String::fromStdString(normalized));
    harmony->setXmlText(harmony->harmonyName());
    score->undoAddElement(harmony);
    return harmony;
}
}

PopoverDef mu::dorico::chordSymbolsPopover()
{
    PopoverDef def;
    def.id = "chordsymbols";
    def.title = "Chord Symbols";
    def.glyph = "C\u2077";
    def.scopeButton = true;
    def.fieldWidth = 171;
    def.itemTypes = { ElementType::HARMONY };
    def.validate = [](const std::string& text) { return parseChordSymbol(text).status; };
    def.prefill = [](const EngravingItem* item) {
        return item->isHarmony() ? toHarmony(item)->harmonyName().toStdString() : std::string();
    };

    def.applyAt = [](const std::string& text, const PopoverPosition& pos, mu::notation::INotationPtr notation) {
        const ChordSymbolParse parsed = parseChordSymbol(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add chord symbol"));
        Harmony* harmony = chordsymbolspopover::addHarmony(notation->elements()->msScore(), parsed.normalized, pos);
        if (!harmony) {
            undoStack->rollbackChanges();
            return false;
        }
        undoStack->commitChanges();
        notation->interaction()->select({ harmony }); // Return on it reopens the popover
        return true;
    };
    return def;
}
