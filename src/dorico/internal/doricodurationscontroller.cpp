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

#include "doricodurationscontroller.h"

#include <algorithm>
#include <optional>
#include <set>
#include <vector>

#include "engraving/dom/anchors.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/spanner.h"
#include "engraving/dom/tie.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"
#include "notationscene/notationcommands.h"

#include "../doricocommands.h"
#include "noteinpututils.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;
using namespace muse;
using namespace muse::rcommand;

namespace {
// Selected chords (a selected note stands for its chord), each once, in score order. Grace notes and tuplets are
// skipped. tieChains: a tied note stands for the first chord of its tie chain (Dorico edits a tie chain as one note).
std::vector<Chord*> selectedChords(const std::vector<EngravingItem*>& items, bool tieChains = false)
{
    std::vector<Chord*> chords;
    for (EngravingItem* e : items) {
        Note* n = e->isNote() ? toNote(e) : (e->isChord() ? toChord(e)->upNote() : nullptr);
        Chord* c = n ? (tieChains ? n->firstTiedNote() : n)->chord() : nullptr;
        if (c && !c->isGrace() && !c->tuplet() && std::find(chords.begin(), chords.end(), c) == chords.end()) {
            chords.push_back(c);
        }
    }
    std::sort(chords.begin(), chords.end(), [](const Chord* a, const Chord* b) { return a->tick() < b->tick(); });
    return chords;
}

// Selected lines and hairpins that sit on rhythmic positions (not slurs, ties or glissandi, which hang on notes).
std::vector<Spanner*> selectedSpanners(const std::vector<EngravingItem*>& items)
{
    std::set<Spanner*> spanners;
    for (EngravingItem* e : items) {
        Spanner* sp = e->isSpannerSegment() ? toSpannerSegment(e)->spanner() : (e->isSpanner() ? toSpanner(e) : nullptr);
        if (sp && sp->anchor() == Spanner::Anchor::SEGMENT) {
            spanners.insert(sp);
        }
    }
    return { spanners.begin(), spanners.end() };
}

bool hasItem(const Segment* s, staff_idx_t staff, bool chordsOnly)
{
    for (voice_idx_t v = 0; v < VOICES; ++v) {
        const EngravingItem* e = s->element(staff * VOICES + v);
        if (e && (!chordsOnly || e->isChord())) {
            return true;
        }
    }
    return false;
}

// Start of the nearest chord (or, with !chordsOnly, chord or rest) on the staff strictly after / before tick.
// ponytail: scans the whole score per key press; start from tick2measure() if long scores make it slow.
std::optional<Fraction> itemTickBeside(const Score* score, staff_idx_t staff, const Fraction& tick, bool right, bool chordsOnly)
{
    std::optional<Fraction> found;
    for (const Segment* s = score->firstSegment(SegmentType::ChordRest); s; s = s->next1(SegmentType::ChordRest)) {
        if (right ? s->tick() <= tick : s->tick() >= tick) {
            if (right) {
                continue;
            }
            break;
        }
        if (hasItem(s, staff, chordsOnly)) {
            found = s->tick();
            if (right) {
                break;
            }
        }
    }
    return found;
}

// The chord and the chords its top note is tied into, in order.
std::vector<Chord*> tieChain(Chord* c)
{
    std::vector<Chord*> chain { c };
    for (const Tie* t = c->upNote()->tieFor(); t && t->isTie() && t->endNote(); t = chain.back()->upNote()->tieFor()) {
        Chord* next = t->endNote()->chord();
        if (next->track() != c->track() || next->isGrace() || next->tick() != chain.back()->endTick()) {
            break;
        }
        chain.push_back(next);
    }
    return chain;
}

// How long the chord can grow without covering another chord of its voice: up to the next chord or the barline.
Fraction room(const Chord* c)
{
    Fraction limit = c->measure()->endTick();
    for (const Segment* s = c->segment()->next(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
        const EngravingItem* e = s->element(c->track());
        if (e && e->isChord()) {
            limit = s->tick();
            break;
        }
    }
    return limit - c->tick();
}

// New end of a line or hairpin, or std::nullopt when it cannot change. byGrid: the next / previous grid line or
// note, whichever is closer (Shift-Opt-Right / Left); otherwise the next / previous note (Cmd-Shift-Opt).
std::optional<Fraction> spannerEnd(const Score* score, const Spanner* sp, bool longer, bool byGrid, int grid)
{
    const Fraction end = sp->tick2();
    const std::optional<Fraction> note = itemTickBeside(score, sp->staffIdx(), end, longer, /*chordsOnly*/ true);
    Fraction to;
    if (!byGrid) {
        if (!note) {
            return std::nullopt;
        }
        to = *note;
    } else if (longer) {
        if (end >= score->endTick()) {
            return std::nullopt;
        }
        const int barStart = score->tick2measure(end)->tick().ticks();
        to = Fraction::fromTicks(nextCaretTick(end.ticks(), barStart, grid, note.value_or(score->endTick()).ticks()));
    } else {
        const int barStart = score->tick2measure(end - Fraction::eps())->tick().ticks();
        to = Fraction::fromTicks(prevCaretTick(end.ticks(), barStart, grid, note.value_or(sp->tick()).ticks()));
    }
    if (to <= sp->tick() || to == end) {
        return std::nullopt;
    }
    return to;
}

// true when a slur, glissando or similar hangs on one of the chords (removing the chord would leave it dangling).
// Hairpins and other lines sit on rhythmic positions: layout recomputes their anchors, so they do not count.
bool hasAttachedSpanners(const Score* score, const ChordRest* a, const ChordRest* b)
{
    auto attached = [a, b](const EngravingItem* e) {
        const EngravingItem* owner = e && e->isNote() ? toNote(e)->chord() : e;
        return owner && (owner == a || owner == b);
    };
    for (const auto& iv : score->spannerMap().findOverlapping(a->tick().ticks(), b->endTick().ticks())) {
        if (iv.value->anchor() == Spanner::Anchor::SEGMENT) {
            continue;
        }
        if (attached(iv.value->startElement()) || attached(iv.value->endElement())) {
            return true;
        }
    }
    return false;
}

bool movable(const ChordRest* cr)
{
    if (!cr || cr->tuplet() || cr->isGrace() || cr->isMeasureRepeat() || cr->measure()->isMMRest()) {
        return false;
    }
    if (cr->isChord()) {
        const Chord* c = toChord(cr);
        if (c->tremoloTwoChord()) {
            return false;
        }
        for (const Note* n : c->notes()) {
            if (n->tieFor() || n->tieBack()) {
                return false;
            }
        }
    }
    return true;
}
}

void DoricoDurationsController::init()
{
    reg(DORICO_LENGTHEN_BY_GRID_COMMAND, [this]() { onResize(true, true); });
    reg(DORICO_SHORTEN_BY_GRID_COMMAND, [this]() { onResize(false, true); });
    reg(DORICO_DOUBLE_DURATION_COMMAND, [this]() { onResize(true, false); });
    reg(DORICO_HALVE_DURATION_COMMAND, [this]() { onResize(false, false); });
    reg(DORICO_MOVE_RIGHT_COMMAND, [this]() { onMove(true); });
    reg(DORICO_MOVE_LEFT_COMMAND, [this]() { onMove(false); });
    reg(DORICO_SCISSORS_COMMAND, [this]() { onScissors(); });
}

void DoricoDurationsController::reg(const Command& command, std::function<void()> handler)
{
    dispatcher()->onRequest(this, command, [handler]() {
        handler();
        return muse::make_ok();
    });
}

void DoricoDurationsController::onResize(bool longer, bool byGrid)
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }
    INotationInteractionPtr interaction = notation->interaction();
    const std::vector<EngravingItem*> items = interaction->selection()->elements();
    std::vector<Chord*> chords = selectedChords(items, /*tieChains*/ true);
    const std::vector<Spanner*> spanners = selectedSpanners(items);
    Score* score = notation->elements()->msScore();
    const int grid = Fraction(1, caret()->gridDenominator()).ticks();

    if (!spanners.empty()) {
        if (interaction->isEditingElement()) {
            interaction->endEditElement(); // a clicked line is in edit mode; its segments may be rebuilt
        }
        notation->undoStack()->prepareChanges(TranslatableString("undoableAction", longer ? "Lengthen line" : "Shorten line"));
        for (Spanner* sp : spanners) {
            if (const std::optional<Fraction> end = spannerEnd(score, sp, longer, byGrid, grid)) {
                sp->undoChangeProperty(Pid::SPANNER_TICKS, *end - sp->tick());
            }
        }
        notation->undoStack()->commitChanges();
    }

    // Double / halve is MuseScore's command; it also changes the input duration during note input.
    if (!byGrid) {
        if (!chords.empty() || spanners.empty()) {
            dispatcher()->dispatch(longer ? SET_DOUBLE_DURATION_COMMAND : SET_HALVE_DURATION_COMMAND);
        }
        return;
    }
    if (chords.empty()) {
        return;
    }

    const Fraction step = Fraction::fromTicks(grid);
    if (longer) {
        std::reverse(chords.begin(), chords.end()); // a later chord grows first, so an earlier one's room is final
    }
    notation->undoStack()->prepareChanges(TranslatableString("undoableAction", longer ? "Lengthen notes" : "Shorten notes"));
    for (Chord* c : chords) {
        std::vector<Chord*> chain = tieChain(c);
        if (longer) {
            // A note only grows into rests and stops at the barline. changeCRlen ties on extra pieces when needed.
            Chord* last = chain.back();
            const Fraction target = std::min(last->ticks() + step, room(last));
            if (target > last->ticks()) {
                score->changeCRlen(last, target);
            }
            continue;
        }
        // Shorten from the end of the chain, never below one grid unit: pieces that are cut off completely become rests.
        // ponytail: a length that needs two note values (half + eighth) is written as note + rest (MuseScore's
        // changeCRlen); Dorico writes tied notes.
        const Fraction total = chain.back()->endTick() - c->tick();
        Fraction cut = total - std::max(total - step, step);
        while (cut > Fraction(0, 1)) {
            Chord* last = chain.back();
            if (last->ticks() > cut) {
                score->changeCRlen(last, last->ticks() - cut);
                break;
            }
            cut -= last->ticks();
            score->deleteItem(last); // a rest takes its place, and its tie goes with it
            chain.pop_back();
        }
    }
    notation->undoStack()->commitChanges();

    std::vector<EngravingItem*> notes;
    for (Chord* c : chords) {
        notes.insert(notes.end(), c->notes().begin(), c->notes().end());
    }
    interaction->select(notes, SelectType::REPLACE);
}

void DoricoDurationsController::onMove(bool right)
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }
    INotationInteractionPtr interaction = notation->interaction();
    Score* score = notation->elements()->msScore();
    const std::vector<EngravingItem*> items = interaction->selection()->elements();

    // A chord counts as one item, also when several of its notes are selected.
    EngravingItem* item = items.size() == 1 ? items.front() : nullptr;
    if (const std::vector<Chord*> chords = selectedChords(items); chords.size() == 1
        && std::all_of(items.begin(), items.end(), [](const EngravingItem* e) { return e->isNote(); })) {
        item = chords.front();
    }
    if (item && item->isNote()) {
        item = toNote(item)->chord();
    }
    if (!item) {
        return;
    }

    // A dynamic, text, tempo mark ...: to the next / previous note or rest of its staff.
    if (item->allowTimeAnchor() && item->ownershipParent() && item->ownershipParent()->isSegment()) {
        const Segment* from = toSegment(item->ownershipParent());
        const std::optional<Fraction> tick = itemTickBeside(score, item->staffIdx(), from->tick(), right, /*chordsOnly*/ false);
        Segment* to = tick ? score->tick2segment(*tick, true, SegmentType::ChordRest) : nullptr;
        if (!to) {
            return;
        }
        notation->undoStack()->prepareChanges(TranslatableString("undoableAction", "Move item"));
        MoveElementAnchors::moveSegment(item, to, to->tick() - from->tick());
        notation->undoStack()->commitChanges();
        interaction->select({ item }, SelectType::SINGLE);
        return;
    }

    if (!item->isChordRest()) {
        return;
    }

    // A chord or rest swaps places with its neighbour (approximation of Dorico's move to the nearest position).
    ChordRest* cr = toChordRest(item);
    ChordRest* first = cr;
    ChordRest* second = cr;
    if (right) {
        second = score->findCR(cr->endTick(), cr->track());
        if (!second || second->tick() != cr->endTick()) {
            return;
        }
    } else {
        first = cr->tick().isZero() ? nullptr : score->findCR(cr->tick() - Fraction::eps(), cr->track());
        if (!first || first->endTick() != cr->tick()) {
            return;
        }
    }
    if (!movable(first) || !movable(second) || first->measure() != second->measure()
        || first->isFullMeasureRest() || second->isFullMeasureRest() || hasAttachedSpanners(score, first, second)) {
        return;
    }

    // Remove both and add copies the other way round, so durations may differ (MuseScore's swap needs equal ones).
    Measure* measure = first->measure();
    const Fraction tick = first->tick();
    ChordRest* newFirst = toChordRest(second->clone());
    ChordRest* newSecond = toChordRest(first->clone());
    ChordRest* moved = cr == first ? newSecond : newFirst;

    notation->undoStack()->prepareChanges(TranslatableString("undoableAction", "Move chord/rest"));
    score->undoRemoveElement(first);
    score->undoRemoveElement(second);
    score->undoAddCR(newFirst, measure, tick);
    score->undoAddCR(newSecond, measure, tick + newFirst->ticks());
    notation->undoStack()->commitChanges();

    if (moved->isChord()) {
        const std::vector<Note*>& notes = toChord(moved)->notes();
        interaction->select(std::vector<EngravingItem*>(notes.begin(), notes.end()), SelectType::REPLACE);
    } else {
        interaction->select({ moved }, SelectType::SINGLE);
    }
}

void DoricoDurationsController::onScissors()
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation || notation->interaction()->noteInput()->isNoteInputMode()) {
        return; // ponytail: splitting at the caret during note input is deferred (spec 13)
    }

    // Every tie of the tie chains the selection touches (Dorico selects whole tie chains).
    std::set<Tie*> ties;
    for (EngravingItem* e : notation->interaction()->selection()->elements()) {
        // ponytail: partial ties (repeat jumps) are left to undoRemoveTiesFromJumpPoints, never removed directly
        if (e->isTieSegment()) {
            ties.insert(toTieSegment(e)->tie());
        }
        if (!e->isNote()) {
            continue;
        }
        for (Note* n = toNote(e)->firstTiedNote(); n && n->tieFor() && n->tieFor()->isTie()
             && ties.insert(n->tieFor()).second; n = n->tieFor()->endNote()) {
        }
    }
    if (ties.empty()) {
        return;
    }

    Score* score = notation->elements()->msScore();
    notation->undoStack()->prepareChanges(TranslatableString("undoableAction", "Remove ties"));
    for (Tie* tie : ties) {
        if (tie->tieJumpPoints()) {
            tie->undoRemoveTiesFromJumpPoints();
        }
        score->undoRemoveElement(tie);
    }
    notation->undoStack()->commitChanges();
}
