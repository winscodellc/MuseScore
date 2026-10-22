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

#include "repeatshandler.h"

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/input.h"
#include "engraving/dom/jump.h"
#include "engraving/dom/marker.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/measurerepeat.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/tremolosinglechord.h"
#include "engraving/dom/tremolotwochord.h"
#include "engraving/dom/volta.h"
#include "engraving/editing/editmeasurerepeat.h"
#include "engraving/editing/transaction/transaction.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/repeatsparser.h"
#include "pendingatcaret.h"

using namespace mu::dorico;
using namespace mu::engraving;
using muse::TranslatableString;
using mu::notation::INotationPtr;
using RepeatsKind = RepeatsEntry::Kind;

namespace {
const TranslatableString ADD_REPEAT("undoableAction", "Add repeat or tremolo");

// The bars an entry acts on: the caret's bar, or the bars from the earliest to the latest selected item.
bool repeatsBars(Score* score, bool caret, Measure*& first, Measure*& last)
{
    first = last = nullptr;
    if (caret) {
        const InputState& is = score->inputState();
        if (!is.segment() || is.beyondScore()) {
            return false;
        }
        first = last = is.segment()->measure();
        return true;
    }
    const Selection& sel = score->selection();
    if (sel.isRange()) {
        first = sel.startSegment()->measure();
        last = score->tick2measure(Fraction::fromTicks(sel.tickEnd().ticks() - 1));
        return first && last;
    }
    for (const EngravingItem* e : sel.elements()) {
        Measure* m = score->tick2measure(e->tick()); // also for markers and endings, which are not inside a segment
        if (!m) {
            continue;
        }
        if (!first || m->tick() < first->tick()) {
            first = m;
        }
        if (!last || m->tick() > last->tick()) {
            last = m;
        }
    }
    return first && last;
}

// The staves a bar repeat goes on.
std::vector<staff_idx_t> repeatsStaves(const Score* score, bool caret)
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

EngravingItem* addRepeatMarking(Score* score, Measure* m, const RepeatsEntry& entry)
{
    TextBase* t = nullptr;
    if (entry.kind == RepeatsKind::Marker) {
        Marker* marker = Factory::createMarker(m);
        marker->setMarkerType(entry.marker); // Fine, To Coda: right-aligned text style
        t = marker;
    } else {
        Jump* jump = Factory::createJump(m);
        jump->setJumpType(entry.jump);
        t = jump;
    }
    t->setTrack(0);
    t->setOwnershipParent(m);
    t->styleChanged();
    score->undoAddElement(t); // what the palette's Measure::drop does
    return t;
}

Volta* addVolta(Score* score, int number, bool closed, const Measure* first, const Measure* last)
{
    Volta* v = Factory::createVolta(score->dummy());
    v->setEndings({ number });
    v->setText(muse::String::number(number) + u".");
    v->setVoltaType(closed ? Volta::Type::CLOSED : Volta::Type::OPEN);
    v->setTrack(0);
    v->setTrack2(0);
    v->setTick(first->tick());
    v->setTick2(last->endTick());
    v->styleChanged();
    score->undoAddElement(v);
    return v;
}

void setRepeatEnd(Measure* m)
{
    if (!m->repeatEnd()) {
        m->undoChangeProperty(Pid::REPEAT_END, true);
    }
}

// `end`: a closed first ending over the bars with an end repeat, and an open second ending on the next bar.
bool addEnding(Score* score, Measure* first, Measure* last)
{
    addVolta(score, 1, true, first, last);
    setRepeatEnd(last);
    if (Measure* next = last->nextMeasure()) {
        addVolta(score, 2, false, next, next);
    }
    return true;
}

// `add`: the selected ending's last segment is closed with an end repeat, and a new open segment follows it.
bool addEndingSegment(Score* score)
{
    const EngravingItem* selected = score->selection().element();
    if (!selected || !selected->isVoltaSegment()) {
        return false;
    }
    Volta* last = toVolta(toVoltaSegment(selected)->spanner());
    for (bool found = true; found;) {
        found = false;
        for (const auto& [tick, sp] : score->spanner()) {
            if (sp->isVolta() && sp != last && sp->tick() == last->tick2()) {
                last = toVolta(sp);
                found = true;
                break;
            }
        }
    }
    Measure* next = last->tick2() < score->endTick() ? score->tick2measure(last->tick2()) : nullptr;
    Measure* lastBar = score->tick2measure(Fraction::fromTicks(last->tick2().ticks() - 1));
    if (!next || !lastBar) {
        return false;
    }
    last->undoChangeProperty(Pid::END_HOOK_TYPE, HookType::HOOK_90);
    setRepeatEnd(lastBar);
    addVolta(score, last->lastEnding() + 1, false, next, next);
    return true;
}

// `%N`: bar repeats of N bars from the first bar, as many groups as fit in the selected bars (at least one).
bool addMeasureRepeats(Score* score, Measure* first, Measure* last, int n, const std::vector<staff_idx_t>& staves)
{
    int bars = 1;
    for (const Measure* m = first; m && m != last; m = m->nextMeasure()) {
        ++bars;
    }
    std::vector<Fraction> starts;
    const Measure* m = first;
    for (int g = 0; g < std::max(1, bars / n) && m; ++g) {
        starts.push_back(m->tick());
        for (int i = 0; i < n && m; ++i) {
            m = m->nextMeasure();
        }
    }
    Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
    bool added = false;
    for (staff_idx_t s : staves) {
        for (const Fraction& tick : starts) { // by tick: adding a repeat rewrites the bars' contents
            Measure* groupStart = score->tick2measure(tick);
            Measure* groupEnd = groupStart;
            for (int i = 1; i < n && groupEnd->nextMeasure(); ++i) {
                groupEnd = groupEnd->nextMeasure();
            }
            // Empty the bars first: on bars with notes MuseScore asks "Current contents of measures will be replaced",
            // a modal box that steals the popover's focus. Dorico just replaces them.
            score->deselectAll();
            score->select(groupStart, SelectType::RANGE, s);
            score->select(groupEnd, SelectType::RANGE, s);
            score->cmdDeleteSelection();
            EditMeasureRepeat::addMeasureRepeat(tx, score, groupStart, n, s);
            added = added || groupStart->isMeasureRepeatGroup(s);
        }
    }
    return added; // false: too few bars of equal length (MuseScore's own rule)
}

bool applyRepeatBars(const RepeatsEntry& entry, Score* score, bool caret, INotationPtr notation)
{
    Measure* first = nullptr;
    Measure* last = nullptr;
    if (!repeatsBars(score, caret, first, last)) {
        return false;
    }
    EngravingItem* selected = caret ? nullptr : score->selection().element();
    if (entry.kind == RepeatsKind::Ending && !entry.add && selected && selected->isVoltaSegment()) {
        return true; // Return on an ending reopens "end": it already is one
    }

    notation->undoStack()->prepareChanges(ADD_REPEAT);
    bool ok = false;
    EngravingItem* added = nullptr;
    switch (entry.kind) {
    case RepeatsKind::Marker:
    case RepeatsKind::Jump:
        if (selected && (selected->isMarker() || selected->isJump())) {
            score->undoRemoveElement(selected); // editing it: the entry replaces it
        }
        added = addRepeatMarking(score, first, entry);
        ok = true;
        break;
    case RepeatsKind::Ending:
        ok = entry.add ? addEndingSegment(score) : addEnding(score, first, last);
        break;
    case RepeatsKind::MeasureRepeat: {
        const Fraction caretTick = score->inputState().tick();
        ok = addMeasureRepeats(score, first, last, entry.measures, repeatsStaves(score, caret));
        if (ok && caret) { // the bar's old notes and rests are gone: put the caret back by tick
            Segment* seg = score->tick2segment(caretTick, true, SegmentType::ChordRest);
            score->inputState().setSegment(seg ? seg : first->first(SegmentType::ChordRest));
        }
        break;
    }
    default:
        break;
    }
    if (!ok) {
        notation->undoStack()->rollbackChanges();
        return false;
    }
    notation->undoStack()->commitChanges();
    if (added) {
        notation->interaction()->select({ added }); // Return reopens the popover on it
    }
    return true;
}

// The chords a tremolo entry acts on, in score order; a selected tremolo stands for its chords.
std::vector<Chord*> tremoloChords(const Selection& sel)
{
    std::vector<Chord*> chords;
    auto take = [&chords](Chord* c) {
        if (c && !c->isGrace() && std::find(chords.begin(), chords.end(), c) == chords.end()) {
            chords.push_back(c);
        }
    };
    for (EngravingItem* e : sel.elements()) {
        if (e->isNote()) {
            take(toNote(e)->chord());
        } else if (e->isChord()) {
            take(toChord(e));
        } else if (e->type() == ElementType::TREMOLO_SINGLECHORD) {
            take(item_cast<TremoloSingleChord*>(e)->chord());
        } else if (e->type() == ElementType::TREMOLO_TWOCHORD) {
            take(item_cast<TremoloTwoChord*>(e)->chord1());
            take(item_cast<TremoloTwoChord*>(e)->chord2());
        }
    }
    std::sort(chords.begin(), chords.end(), [](const Chord* a, const Chord* b) {
        return a->track() != b->track() ? a->track() < b->track() : a->tick() < b->tick();
    });
    return chords;
}

void removeTremolos(Score* score, Chord* c)
{
    if (c->tremoloSingleChord()) {
        score->undoRemoveElement(c->tremoloSingleChord());
    }
    if (c->tremoloTwoChord()) {
        score->undoRemoveElement(c->tremoloTwoChord());
    }
}

// Replaces the chord's tremolos (Chord::drop would toggle off a tremolo of the same type instead).
void addSingleTremolo(Score* score, Chord* c, TremoloType type)
{
    removeTremolos(score, c);
    TremoloSingleChord* t = Factory::createTremoloSingleChord(c);
    t->setTremoloType(type);
    t->setTrack(c->track());
    t->setOwnershipParent(c);
    score->undoAddElement(t);
}

// Two chords next to each other in one voice with the same written length, which MuseScore's two-chord tremolo needs.
bool addTwoChordTremolo(Score* score, Chord* c1, Chord* c2, TremoloType type)
{
    if (c1->track() != c2->track() || c1->ticks() != c2->ticks()) {
        return false;
    }
    Segment* s = c1->segment()->next(SegmentType::ChordRest);
    while (s && !s->element(c1->track())) {
        s = s->next(SegmentType::ChordRest);
    }
    if (!s || s->element(c1->track()) != c2) {
        return false;
    }
    removeTremolos(score, c1);
    removeTremolos(score, c2);
    TremoloTwoChord* t = Factory::createTremoloTwoChord(c1);
    t->setTremoloType(type);
    t->setChords(c1, c2);
    t->setTrack(c1->track());
    t->setOwnershipParent(c1);
    score->undoAddElement(t);
    return true;
}

bool applyTremolo(const RepeatsEntry& entry, Score* score, INotationPtr notation)
{
    const std::vector<Chord*> chords = tremoloChords(score->selection());
    const bool twoChord = entry.kind == RepeatsKind::Tremolo && isTremoloTwoChord(entry.tremolo);
    if (chords.empty() || (twoChord && chords.size() != 2)) {
        return false; // a multi-note tremolo needs exactly two chords
    }

    notation->undoStack()->prepareChanges(ADD_REPEAT);
    bool ok = true;
    if (twoChord) {
        ok = addTwoChordTremolo(score, chords[0], chords[1], entry.tremolo);
    } else {
        for (Chord* c : chords) {
            if (entry.kind == RepeatsKind::ClearTremolo) {
                removeTremolos(score, c);
            } else {
                addSingleTremolo(score, c, entry.tremolo);
            }
        }
    }
    if (!ok) {
        notation->undoStack()->rollbackChanges();
        return false;
    }
    notation->undoStack()->commitChanges();
    return true;
}

std::string repeatsPrefill(const EngravingItem* item)
{
    RepeatsEntry entry;
    switch (item->type()) {
    case ElementType::MARKER:
        entry.kind = RepeatsKind::Marker;
        entry.marker = toMarker(item)->markerType();
        break;
    case ElementType::JUMP:
        entry.kind = RepeatsKind::Jump;
        entry.jump = toJump(item)->jumpType();
        break;
    case ElementType::VOLTA_SEGMENT:
        entry.kind = RepeatsKind::Ending;
        break;
    case ElementType::TREMOLO_SINGLECHORD:
        entry.kind = RepeatsKind::Tremolo;
        entry.tremolo = item_cast<const TremoloSingleChord*>(item)->tremoloType();
        break;
    case ElementType::TREMOLO_TWOCHORD:
        entry.kind = RepeatsKind::Tremolo;
        entry.tremolo = item_cast<const TremoloTwoChord*>(item)->tremoloType();
        break;
    case ElementType::MEASURE_REPEAT:
        entry.kind = RepeatsKind::MeasureRepeat;
        entry.measures = toMeasureRepeat(item)->numMeasures();
        break;
    default:
        return std::string();
    }
    return repeatsText(entry);
}
}

PopoverDef mu::dorico::repeatsPopover()
{
    auto pending = std::make_shared<PendingAtCaret>(ADD_REPEAT);

    PopoverDef def;
    def.id = "repeats";
    def.title = "Repeats and Tremolos";
    def.glyph = "svg:popover-repeats";
    def.supportsLocal = false; // Dorico: Return only
    def.itemTypes = { ElementType::MARKER, ElementType::JUMP, ElementType::VOLTA_SEGMENT, ElementType::TREMOLO_SINGLECHORD,
                      ElementType::TREMOLO_TWOCHORD, ElementType::MEASURE_REPEAT };
    def.validate = [](const std::string& text) { return parseRepeats(text).status; };
    def.prefill = repeatsPrefill;

    def.apply = [pending](const std::string& text, ApplyScope, INotationPtr notation) {
        const RepeatsParse parsed = parseRepeats(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        const RepeatsEntry& entry = parsed.entry;
        Score* score = notation->elements()->msScore();
        const bool caret = notation->interaction()->noteInput()->isNoteInputMode();
        if (!caret && score->selection().isNone()) {
            return false;
        }

        switch (entry.kind) {
        case RepeatsKind::Tremolo:
        case RepeatsKind::ClearTremolo:
            if (caret) {
                // ponytail: at the caret only single-chord tremolos, on the next note input; two-chord and clear need a selection
                if (entry.kind == RepeatsKind::ClearTremolo || isTremoloTwoChord(entry.tremolo)) {
                    return false;
                }
                return pending->add([type = entry.tremolo](Score* s, ChordRest* cr, const Fraction&) {
                    if (!cr->isChord()) {
                        return false;
                    }
                    addSingleTremolo(s, toChord(cr), type);
                    return true;
                }, notation);
            }
            return applyTremolo(entry, score, notation);
        case RepeatsKind::Slash:
            if (caret) {
                return false; // MuseScore fills a selection with slashes; there is no slash region to start at the caret
            }
            notation->interaction()->fillSelectionWithSlashes();
            return true;
        default:
            return applyRepeatBars(entry, score, caret, notation);
        }
    };

    def.noteInputEnded = [pending](INotationPtr) { pending->cancel(); };
    return def;
}
