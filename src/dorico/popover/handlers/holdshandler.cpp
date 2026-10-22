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

#include "holdshandler.h"

#include <set>
#include <vector>

#include "engraving/dom/breath.h"
#include "engraving/dom/chordrest.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/fermata.h"
#include "engraving/dom/input.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/holdsparser.h"

using namespace mu::dorico;
using namespace mu::engraving;

namespace {
// The chord/rest on the staff that starts at tick, in any voice.
ChordRest* chordRestStartingAt(const Score* score, const Fraction& tick, staff_idx_t staff)
{
    Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
    if (!seg || seg->tick() != tick) {
        return nullptr;
    }
    for (track_idx_t t = staff2track(staff); t < staff2track(staff + 1); ++t) {
        if (EngravingItem* e = seg->element(t)) {
            return toChordRest(e);
        }
    }
    return nullptr;
}

// Where the hold goes: the caret or the earliest selected item, and the staves breath marks use.
// onBreath: the earliest item is a breath mark or caesura, so tick is already a breath position (editing it).
bool holdTarget(Score* score, bool caret, Fraction& tick, std::set<staff_idx_t>& staves, bool& onBreath)
{
    onBreath = false;
    if (caret) {
        const InputState& is = score->inputState();
        if (!is.segment() || is.beyondScore()) {
            return false;
        }
        tick = is.tick();
        staves.insert(track2staff(is.track()));
        return true;
    }

    const Selection& sel = score->selection();
    if (sel.isRange()) {
        tick = sel.tickStart();
        for (staff_idx_t s = sel.staffStart(); s < sel.staffEnd(); ++s) {
            staves.insert(s);
        }
        return !staves.empty();
    }

    const EngravingItem* first = nullptr;
    for (const EngravingItem* e : sel.elements()) {
        staves.insert(e->staffIdx());
        if (!first || e->tick() < first->tick()) {
            first = e;
        }
    }
    if (!first || (!first->isBreath() && first->tick() >= score->endTick())) { // a breath can follow the last note
        return false;
    }
    tick = first->tick();
    onBreath = first->isBreath();
    return true;
}

// The Breath segment at tick: after the chord/rest that ends there, at the end of the previous bar when tick starts one.
Segment* breathSegment(Score* score, const Fraction& tick)
{
    Measure* m = score->tick2measure(tick.isZero() ? tick : Fraction::fromTicks(tick.ticks() - 1));
    return m ? m->undoGetSegment(SegmentType::Breath, tick) : nullptr;
}

// A breath mark or caesura on the track; one already on its staff there takes the new symbol.
EngravingItem* addBreath(Score* score, const Fraction& tick, track_idx_t track, SymId sym)
{
    Segment* seg = breathSegment(score, tick);
    if (!seg) {
        return nullptr;
    }
    double pause = 0.0;
    for (const BreathType& bt : Breath::BREATH_LIST) {
        if (bt.id == sym) {
            pause = bt.pause;
        }
    }
    const staff_idx_t staff = track2staff(track);
    for (track_idx_t t = staff2track(staff); t < staff2track(staff + 1); ++t) {
        if (EngravingItem* e = seg->element(t)) {
            e->undoChangeProperty(Pid::SYMBOL, sym);
            e->undoChangeProperty(Pid::PAUSE, pause);
            return e;
        }
    }
    Breath* b = Factory::createBreath(seg);
    b->setTrack(track);
    b->setPlacement(track & 1 ? PlacementV::BELOW : PlacementV::ABOVE);
    b->setSymId(sym);
    b->setPause(pause);
    b->setOwnershipParent(seg);
    score->undoAddElement(b);
    return b;
}

// A fermata on the chord/rest; any other fermata on its staff at that position is replaced.
EngravingItem* addFermata(Score* score, ChordRest* cr, SymId sym)
{
    Segment* seg = cr->segment();
    const std::vector<EngravingItem*> annotations = seg->annotations(); // a copy: removing changes the list
    for (EngravingItem* e : annotations) {
        if (e->isFermata() && e->staffIdx() == cr->staffIdx()) {
            score->undoRemoveElement(e);
        }
    }
    Fermata* f = Factory::createFermata(seg);
    f->setSymIdAndTimeStretch(sym);
    f->setTrack(cr->track());
    f->setPlacement(cr->track() & 1 ? PlacementV::BELOW : PlacementV::ABOVE);
    f->setOwnershipParent(seg);
    score->undoAddElement(f);
    return f;
}

bool applyHold(const HoldEntry& entry, Score* score, bool caret, mu::notation::INotationPtr notation)
{
    Fraction tick;
    std::set<staff_idx_t> selected;
    bool onBreath = false;
    if (!holdTarget(score, caret, tick, selected, onBreath)) {
        return false;
    }

    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add hold or pause"));
    // Fermatas and caesuras go on every staff of the whole score (also from a part; linked copies follow),
    // breath marks on the selected or caret staves.
    Score* master = score->masterScore();
    std::vector<EngravingItem*> added;
    if (entry.kind == HoldEntry::Kind::Breath) {
        for (staff_idx_t s : selected) {
            if (onBreath) {
                added.push_back(addBreath(score, tick, staff2track(s), entry.sym));
            } else if (ChordRest* cr = chordRestStartingAt(score, tick, s)) {
                added.push_back(addBreath(score, cr->endTick(), cr->track(), entry.sym)); // to the right of the note
            }
        }
    } else {
        for (staff_idx_t s = 0; s < master->nstaves(); ++s) {
            if (entry.kind == HoldEntry::Kind::Caesura) {
                added.push_back(addBreath(master, tick, staff2track(s), entry.sym)); // to the left of the note
            } else if (ChordRest* cr = chordRestStartingAt(master, tick, s)) {
                added.push_back(addFermata(master, cr, entry.sym));
            }
        }
    }
    std::erase(added, nullptr);
    if (added.empty()) {
        undoStack->rollbackChanges();
        return false;
    }
    undoStack->commitChanges();

    // Outside note input the new item is selected, so Return reopens it for editing.
    if (!caret) {
        for (EngravingItem* e : added) {
            if (EngravingItem* mine = e->score() == score ? e : e->findLinkedInScore(score)) {
                notation->interaction()->select({ mine });
                break;
            }
        }
    }
    return true;
}

std::string holdPrefill(const EngravingItem* item)
{
    if (item->isFermata()) {
        return holdText(SymId(toFermata(item)->subtype())); // subtype: the "Above" symbol
    }
    return item->isBreath() ? holdText(toBreath(item)->symId()) : std::string();
}
}

PopoverDef mu::dorico::holdsPopover()
{
    PopoverDef def;
    def.id = "holds";
    def.title = "Holds and Pauses";
    def.glyph = "\uE4C0";
    def.supportsLocal = false; // Dorico: Return only
    def.itemTypes = { ElementType::FERMATA, ElementType::BREATH };
    def.validate = [](const std::string& text) { return parseHold(text).status; };
    def.prefill = holdPrefill;
    def.apply = [](const std::string& text, ApplyScope, mu::notation::INotationPtr notation) {
        const HoldParse parsed = parseHold(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        Score* score = notation->elements()->msScore();
        return applyHold(parsed.entry, score, notation->interaction()->noteInput()->isNoteInputMode(), notation);
    };
    return def;
}
