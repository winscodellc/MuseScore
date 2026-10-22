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

#include "figuredbasshandler.h"

#include "engraving/dom/figuredbass.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationundostack.h"

#include "../parsers/figuredbassparser.h"

using namespace mu::dorico;
using namespace mu::engraving;

// Named, not anonymous: the unity build compiles the handlers together.
namespace figuredbasspopover {
// Puts the figured bass at the position, replacing the staff's figured bass there. Built from its lines the way the MusicXML
// import does: FiguredBass::regenerateText would open a transaction of its own.
FiguredBass* addFiguredBass(Score* score, const std::string& text, const PopoverPosition& pos)
{
    Measure* measure = score->tick2measure(pos.tick);
    if (!measure) {
        return nullptr;
    }
    Segment* segment = measure->undoGetSegment(SegmentType::ChordRest, pos.tick);
    const staff_idx_t staff = track2staff(pos.track);
    std::vector<EngravingItem*> old;
    for (EngravingItem* e : segment->annotations()) {
        if (e->isFiguredBass() && e->staffIdx() == staff) {
            old.push_back(e);
        }
    }
    for (EngravingItem* e : old) {
        score->undoRemoveElement(e);
    }

    bool isNew = true;
    FiguredBass* fb = FiguredBass::addFiguredBassToSegment(segment, pos.track, Fraction(0, 1), &isNew);
    String normalized;
    int line = 0;
    for (const muse::String& l : muse::String::fromStdString(text).split(u"\n")) {
        FiguredBassItem* item = fb->createItem(line++);
        String lineText = l;
        if (!item->parse(lineText)) {
            delete item;
            delete fb;
            return nullptr;
        }
        item->setTrack(fb->track());
        item->setOwnershipParent(fb);
        fb->appendItem(item);
        normalized += (normalized.isEmpty() ? u"" : u"\n") + item->normalizedText();
    }
    fb->setXmlText(normalized);
    score->undoAddElement(fb);
    return fb;
}

// MuseScore's lines back to popover syntax: `h` is a natural, `/` a diminished figure.
std::string entryText(const FiguredBass* fb)
{
    std::string text;
    for (const char16_t c : fb->plainText().toStdU16String()) {
        text += c == u'\n' ? ',' : c == u'h' ? 'n' : c == u'/' ? 'd' : static_cast<char>(c);
    }
    return text;
}
}

PopoverDef mu::dorico::figuredBassPopover()
{
    PopoverDef def;
    def.id = "figuredbass";
    def.title = "Figured Bass";
    def.glyph = "\uEA5B\n\uEA54";
    def.scopeButton = true;
    def.fieldWidth = 171;
    def.itemTypes = { ElementType::FIGURED_BASS };
    def.validate = [](const std::string& text) { return parseFiguredBass(text).status; };
    def.prefill = [](const EngravingItem* item) {
        return item->isFiguredBass() ? figuredbasspopover::entryText(toFiguredBass(item)) : std::string();
    };

    def.applyAt = [](const std::string& text, const PopoverPosition& pos, mu::notation::INotationPtr notation) {
        const FiguredBassParse parsed = parseFiguredBass(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add figured bass"));
        FiguredBass* fb = figuredbasspopover::addFiguredBass(notation->elements()->msScore(), parsed.text, pos);
        if (!fb) {
            undoStack->rollbackChanges();
            return false;
        }
        undoStack->commitChanges();
        notation->interaction()->select({ fb }); // Return on it reopens the popover
        return true;
    };
    return def;
}
