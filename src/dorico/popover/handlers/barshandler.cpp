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

#include "barshandler.h"

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

#include "engraving/dom/barline.h"
#include "engraving/dom/chordrest.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/input.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/editing/editdata.h"
#include "engraving/editing/splitjoinmeasure.h"
#include "engraving/editing/transaction/transaction.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/barsparser.h"

using namespace mu::dorico;
using namespace mu::engraving;
using muse::TranslatableString;
using mu::notation::INotationPtr;

namespace {
bool isRepeat(BarLineType t)
{
    return t == BarLineType::START_REPEAT || t == BarLineType::END_REPEAT || t == BarLineType::END_START_REPEAT;
}

// Dorico's position for a new item: the caret, or the earliest selected item (a barline's tick is the bar boundary).
Fraction startTick(const Score* score, bool caret)
{
    if (caret) {
        const InputState& is = score->inputState();
        return is.beyondScore() ? score->lastMeasure()->endTick() : is.tick();
    }
    const Selection& sel = score->selection();
    if (sel.isRange()) {
        return sel.tickStart();
    }
    Fraction tick = score->lastMeasure()->endTick();
    for (const EngravingItem* e : sel.elements()) {
        tick = std::min(tick, e->tick());
    }
    return tick;
}

// The staves Opt-Return acts on.
std::vector<staff_idx_t> targetStaves(const Score* score, bool caret)
{
    if (caret) {
        return { track2staff(score->inputState().track()) };
    }
    const Selection& sel = score->selection();
    std::set<staff_idx_t> staves;
    if (sel.isRange()) {
        for (staff_idx_t s = sel.staffStart(); s < sel.staffEnd(); ++s) {
            staves.insert(s);
        }
    }
    for (const EngravingItem* e : sel.elements()) {
        staves.insert(e->staffIdx());
    }
    return std::vector<staff_idx_t>(staves.begin(), staves.end());
}

// Deleting or rewriting bars removes segments, so the caret is put back by tick.
void restoreCaret(Score* score, const Fraction& tick)
{
    Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
    score->inputState().setSegment(seg ? seg : score->lastMeasure()->first(SegmentType::ChordRest));
}

bool addBars(int count, Score* score, bool caret, INotationPtr notation)
{
    if (!caret) {
        // Before the selected items, after a selected barline: MuseScore's own Insert Measures rule, which is Dorico's.
        notation->interaction()->addBoxes(mu::notation::BoxType::Measure, count, mu::notation::AddBoxesTarget::BeforeSelection);
        return true;
    }

    // At the caret: before its bar when it is at the bar's start, else after the bar.
    // ponytail: Dorico adds beats so the caret's bar is split at the caret; we add whole bars at the nearest barline.
    const InputState& is = score->inputState();
    MeasureBase* before = nullptr;
    if (!is.beyondScore() && is.segment()) {
        Measure* m = is.segment()->measure();
        before = is.tick() == m->tick() ? m : m->next();
    }

    notation->undoStack()->prepareChanges(TranslatableString("undoableAction", "Add %Ln measure(s)", nullptr, count));
    Score::InsertMeasureOptions options;
    options.createMeasureRests = false;
    options.needDeselectAll = false;
    for (int i = 0; i < count; ++i) {
        score->insertMeasure(ElementType::MEASURE, before, options);
    }
    notation->undoStack()->commitChanges();
    return true;
}

bool deleteBars(int count, Score* score, bool caret, INotationPtr notation)
{
    if (caret && score->inputState().beyondScore()) {
        return false;
    }
    Measure* first = caret ? score->inputState().segment()->measure() : score->tick2measure(startTick(score, caret));
    if (!first) {
        return false;
    }
    Measure* last = first;
    for (int i = 1; i < count && last->nextMeasure(); ++i) {
        last = last->nextMeasure();
    }
    const Fraction tick = first->tick();

    notation->undoStack()->prepareChanges(TranslatableString("undoableAction", "Delete %Ln measure(s)", nullptr, count));
    score->select(first, SelectType::REPLACE, 0);
    score->select(last, SelectType::RANGE, 0);
    score->cmdTimeDelete(); // music to the right moves left
    if (caret) {
        restoreCaret(score, tick);
    }
    notation->undoStack()->commitChanges();
    return true;
}

// The bars and voices that get a bar rest: the caret's, each selected item's, or voice 1 of a selected range.
// ponytail: a range only rewrites voice 1; other voices keep their music.
std::vector<std::pair<Measure*, track_idx_t> > restTargets(const Score* score, bool caret)
{
    std::vector<std::pair<Measure*, track_idx_t> > targets;
    auto add = [&targets](Measure* m, track_idx_t track) {
        if (m && std::find(targets.begin(), targets.end(), std::make_pair(m, track)) == targets.end()) {
            targets.emplace_back(m, track);
        }
    };

    if (caret) {
        const InputState& is = score->inputState();
        if (!is.beyondScore() && is.segment()) {
            add(is.segment()->measure(), is.track());
        }
        return targets;
    }

    const Selection& sel = score->selection();
    if (sel.isRange()) {
        for (Measure* m = score->tick2measure(sel.tickStart()); m && m->tick() < sel.tickEnd(); m = m->nextMeasure()) {
            for (staff_idx_t s = sel.staffStart(); s < sel.staffEnd(); ++s) {
                add(m, staff2track(s));
            }
        }
        return targets;
    }
    for (const EngravingItem* e : sel.elements()) {
        add(score->tick2measure(e->tick()), e->track());
    }
    return targets;
}

bool barRest(Score* score, bool caret, INotationPtr notation)
{
    const auto targets = restTargets(score, caret);
    if (targets.empty()) {
        return false;
    }

    notation->undoStack()->prepareChanges(TranslatableString("undoableAction", "Enter full measure rest"));
    Rest* last = nullptr;
    for (const auto& [m, track] : targets) {
        Segment* s1 = m->first(SegmentType::ChordRest);
        if (!s1) {
            continue;
        }
        // Delete the voice's music in the bar (rests take its place), then swap those rests for one bar rest.
        score->deleteRange(s1, score->tick2segment(m->endTick()), track, track + 1, score->selectionFilter(), false);
        if (!m->isOnlyRests(track)) {
            continue; // something the selection filter keeps
        }
        std::vector<ChordRest*> rests;
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            if (EngravingItem* e = s->element(track)) {
                rests.push_back(toChordRest(e));
            }
        }
        for (ChordRest* cr : rests) {
            score->removeChordRest(cr, true);
        }
        last = score->addRest(m->tick(), track, TDuration(DurationType::V_MEASURE), nullptr);
    }
    if (caret && last) {
        score->inputState().setSegment(last->segment()); // Dorico leaves the caret in the bar
    }
    notation->undoStack()->commitChanges();

    if (!caret && last) {
        notation->interaction()->select({ last });
    }
    return true;
}

bool trimBars(Score* score, bool caret, INotationPtr notation)
{
    const Fraction caretTick = caret ? startTick(score, caret) : Fraction();
    notation->undoStack()->prepareChanges(TranslatableString("undoableAction", "Remove empty trailing measures"));
    score->cmdRemoveEmptyTrailingMeasures();
    if (caret) {
        restoreCaret(score, std::min(caretTick, score->lastMeasure()->tick()));
    }
    notation->undoStack()->commitChanges();
    return true;
}

// Drops a barline of `type` on a bar the way the Barlines palette does (Measure::drop handles repeats and staves).
void dropBarline(Transaction& tx, Score* score, Measure* m, BarLineType type, staff_idx_t staff, bool thisStaffOnly)
{
    BarLine* bl = Factory::createBarLine(score->dummy());
    bl->setBarLineType(type);
    EditData ed;
    ed.dropElement = bl;
    ed.track = staff2track(staff);
    ed.modifiers = thisStaffOnly ? ControlModifier : NoModifier;
    m->drop(tx, ed);
}

// The barline at a bar boundary, split there first if it is mid-bar (Dorico re-bars around a new barline).
bool placeBarline(Transaction& tx, Score* score, const Fraction& tick, BarLineType type, const std::vector<staff_idx_t>& staves,
                  bool local)
{
    Measure* last = score->lastMeasure();
    Measure* next = tick < last->endTick() ? score->tick2measure(tick) : nullptr;
    if (next && next->tick() != tick) {
        SplitJoinMeasure::splitMeasure(tx, score->masterScore(), tick);
        next = score->tick2measure(tick);
        if (!next || next->tick() != tick) {
            return false; // could not split there (inside a tuplet, ...)
        }
    }
    Measure* prev = next ? next->prevMeasure() : last;

    // A repeat where the other repeat already is makes end/start.
    if ((type == BarLineType::START_REPEAT && prev && prev->repeatEnd())
        || (type == BarLineType::END_REPEAT && next && next->repeatStart())) {
        type = BarLineType::END_START_REPEAT;
    }

    const staff_idx_t staff = staves.front();
    if (isRepeat(type)) {
        // A repeat replaces a double / final / ... barline there instead of sitting next to it.
        const BarLine* old = prev ? prev->endBarLine() : nullptr;
        if (old && old->barLineType() != BarLineType::NORMAL && !isRepeat(old->barLineType())) {
            dropBarline(tx, score, prev, BarLineType::NORMAL, staff, false);
        }
        Measure* target = type == BarLineType::START_REPEAT ? next : prev;
        if (!target) {
            return false;
        }
        dropBarline(tx, score, target, type, staff, false); // repeats span every staff in MuseScore
        return true;
    }

    if (!prev) {
        return false; // nothing before the first bar but a start repeat
    }
    if (local) {
        for (staff_idx_t s : staves) {
            dropBarline(tx, score, prev, type, s, true);
        }
    } else {
        dropBarline(tx, score, prev, type, staff, false);
    }
    return true;
}

// The barline now at `tick` on `staff`, to select it so Return reopens the popover on it.
EngravingItem* barlineAt(const Score* score, const Fraction& tick, staff_idx_t staff)
{
    const Measure* last = score->lastMeasure();
    const Measure* next = tick < last->endTick() ? score->tick2measure(tick) : nullptr;
    const Segment* seg = nullptr;
    if (next && next->repeatStart()) {
        seg = next->findSegmentR(SegmentType::StartRepeatBarLine, Fraction(0, 1));
    } else if (const Measure* prev = next ? next->prevMeasure() : last) {
        seg = prev->findSegmentR(SegmentType::EndBarLine, prev->ticks());
    }
    return seg ? seg->element(staff2track(staff)) : nullptr;
}

bool barline(BarLineType type, bool local, Score* score, bool caret, INotationPtr notation)
{
    const Selection& sel = score->selection();
    BarLine* selected = !caret && sel.isSingle() && sel.element()->isBarLine() ? toBarLine(sel.element()) : nullptr;
    std::vector<staff_idx_t> staves = targetStaves(score, caret);
    if (staves.empty()) {
        return false;
    }
    const Fraction tick = selected ? selected->tick() : startTick(score, caret);
    const bool restore = caret && !score->inputState().beyondScore(); // a split bar takes the caret's segment with it
    // |: over a range also closes it with :| (Dorico adds both).
    const Fraction rangeEnd = !caret && sel.isRange() && type == BarLineType::START_REPEAT ? sel.tickEnd() : Fraction();

    notation->undoStack()->prepareChanges(TranslatableString("undoableAction", "Add barline"));
    Transaction& tx = score->transactionManager()->currentOrDummyTransaction();

    bool ok = true;
    if (selected && selected->segment()->segmentType() == SegmentType::BarLine) {
        // A mid-bar barline (from MuseScore) changes type in place.
        BarLine* bl = Factory::createBarLine(score->dummy());
        bl->setBarLineType(type);
        EditData ed;
        ed.dropElement = bl;
        ed.modifiers = local ? ControlModifier : NoModifier;
        selected->drop(tx, ed);
    } else {
        // The later position first, so splitting a bar there does not move the earlier one.
        if (rangeEnd > tick) {
            placeBarline(tx, score, rangeEnd, BarLineType::END_REPEAT, staves, false);
        }
        ok = placeBarline(tx, score, tick, type, staves, local);
    }
    if (!ok) {
        notation->undoStack()->rollbackChanges();
        return false;
    }
    if (restore) {
        restoreCaret(score, tick);
    }
    notation->undoStack()->commitChanges();

    if (!caret) {
        EngravingItem* added = barlineAt(score, tick, staves.front());
        if (added) {
            notation->interaction()->select({ added });
        } else {
            notation->interaction()->clearSelection(); // a split bar leaves stale selected items
        }
    }
    return true;
}

// MuseScore keeps end/start as an end repeat on one bar and a start repeat on the next.
std::string barsPrefill(const EngravingItem* item)
{
    const BarLine* bl = toBarLine(item);
    const Measure* m = bl->measure();
    BarLineType type = bl->barLineType();
    if ((type == BarLineType::START_REPEAT && m && m->prevMeasure() && m->prevMeasure()->repeatEnd())
        || (type == BarLineType::END_REPEAT && m && m->nextMeasure() && m->nextMeasure()->repeatStart())) {
        type = BarLineType::END_START_REPEAT;
    }
    return barlineText(type);
}
}

PopoverDef mu::dorico::barsPopover()
{
    PopoverDef def;
    def.id = "bars";
    def.title = "Bars and Barlines";
    def.icon = muse::ui::IconCode::Code::DOUBLE_BAR_LINE;
    def.supportsLocal = true;
    def.itemTypes = { ElementType::BAR_LINE };
    def.validate = [](const std::string& text) { return parseBars(text).status; };
    def.prefill = barsPrefill;

    def.apply = [](const std::string& text, ApplyScope scope, INotationPtr notation) {
        const BarsParse parsed = parseBars(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        const BarsEntry& entry = parsed.entry;
        const bool local = scope == ApplyScope::Local;
        Score* score = notation->elements()->msScore();
        const bool caret = notation->interaction()->noteInput()->isNoteInputMode();
        if (!caret && score->selection().isNone()) {
            return false;
        }

        if (entry.kind == BarsEntry::Kind::Barline) {
            return barline(entry.barline, local, score, caret, notation);
        }
        if (local && entry.kind != BarsEntry::Kind::BarRest) {
            return false; // MuseScore bars span every staff, so bars cannot be added or deleted on some staves only
        }
        switch (entry.kind) {
        case BarsEntry::Kind::AddBars: return addBars(entry.count, score, caret, notation);
        case BarsEntry::Kind::DeleteBars: return deleteBars(entry.count, score, caret, notation);
        case BarsEntry::Kind::BarRest: return barRest(score, caret, notation);
        case BarsEntry::Kind::Trim: return trimBars(score, caret, notation);
        case BarsEntry::Kind::Barline: break;
        }
        return false;
    };
    return def;
}
