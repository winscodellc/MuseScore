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

#include "clefhandler.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "engraving/dom/chord.h"
#include "engraving/dom/chordrest.h"
#include "engraving/dom/clef.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/input.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/ottava.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafftype.h"
#include "engraving/editing/editclef.h"
#include "engraving/editing/transaction/transaction.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/clefparser.h"

using namespace mu::dorico;
using namespace mu::engraving;

namespace {
// An octave line entered at the caret (8va): created when stopped (|, stop, the next octave line, end of note input).
// ponytail: the line appears when stopped, it does not grow while notes are entered (as dynamics, spec 21).
struct PendingOttava {
    bool active = false;
    const Score* score = nullptr;
    Fraction start;
    track_idx_t track = 0;
    OttavaType type = OttavaType::OTTAVA_8VA;
};

OttavaType ottavaType(int octaves)
{
    switch (octaves) {
    case 1: return OttavaType::OTTAVA_8VA;
    case 2: return OttavaType::OTTAVA_15MA;
    case 3: return OttavaType::OTTAVA_22MA;
    case -1: return OttavaType::OTTAVA_8VB;
    case -2: return OttavaType::OTTAVA_15MB;
    default: return OttavaType::OTTAVA_22MB;
    }
}

int octaves(OttavaType type)
{
    switch (type) {
    case OttavaType::OTTAVA_8VA: return 1;
    case OttavaType::OTTAVA_15MA: return 2;
    case OttavaType::OTTAVA_22MA: return 3;
    case OttavaType::OTTAVA_8VB: return -1;
    case OttavaType::OTTAVA_15MB: return -2;
    case OttavaType::OTTAVA_22MB: return -3;
    }
    return 0;
}

// Octave lines are staff-wide in MuseScore (all voices), so they go on the staff's first track.
Ottava* addOttava(Score* score, OttavaType type, const Fraction& from, const Fraction& to, staff_idx_t staff)
{
    if (to <= from) {
        return nullptr;
    }
    Ottava* o = Factory::createOttava(score->dummy());
    o->setOttavaType(type);
    o->styleChanged(); // placement (above / below) comes from the type's style
    o->setTrack(staff2track(staff));
    o->setTrack2(o->track());
    o->setTick(from);
    o->setTick2(to);
    score->undoAddElement(o);
    return o;
}

void finishPending(PendingOttava& p, Score* score, const Fraction& end)
{
    if (p.active && p.score == score) {
        addOttava(score, p.type, p.start, end, track2staff(p.track));
    }
    p.active = false;
}

Fraction endOf(const ChordRest* cr)
{
    return cr->isChord() ? toChord(cr)->endTickIncludingTied() : cr->endTick();
}

// The chord/rest on the staff that starts at tick, in any voice.
ChordRest* chordRestAt(const Score* score, const Fraction& tick, staff_idx_t staff)
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

// A selected item's chord/rest: a note's chord, or the chord/rest where a clef, dynamic, line... starts.
ChordRest* chordRestOf(const Score* score, EngravingItem* e)
{
    if (e->isNote()) {
        return toNote(e)->chord();
    }
    if (e->isChordRest()) {
        return toChordRest(e);
    }
    return chordRestAt(score, e->tick(), e->staffIdx());
}

// The clef at tick on the staff: mid-bar, or at the end of the previous bar when tick starts a bar.
Clef* clefAt(const Score* score, const Fraction& tick, staff_idx_t staff)
{
    Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
    for (Segment* s = seg ? seg->prev1() : nullptr; s && s->tick() == tick; s = s->prev1()) {
        if ((s->isClefType() || s->isHeaderClefType()) && s->element(staff2track(staff))) {
            return toClef(s->element(staff2track(staff)));
        }
    }
    return nullptr;
}

// Where a clef goes on one staff: `at` (a chord/rest, or a selected clef it replaces); `restoreAt` gets the clef that
// was in force there back (end of a range), when set.
struct ClefTarget {
    Staff* staff = nullptr;
    EngravingItem* at = nullptr;
    EngravingItem* restoreAt = nullptr;
    ClefType restore = ClefType::INVALID;
};

std::vector<ClefTarget> clefTargets(Score* score, bool caret)
{
    if (caret) {
        const InputState& is = score->inputState();
        if (!is.segment() || is.beyondScore() || !is.cr()) {
            return {};
        }
        return { { score->staff(track2staff(is.track())), is.cr() } };
    }

    std::vector<ClefTarget> result;
    const Selection& sel = score->selection();
    if (sel.isRange()) {
        const Fraction end = sel.tickEnd();
        for (staff_idx_t s = sel.staffStart(); s < sel.staffEnd(); ++s) {
            ClefTarget t { score->staff(s), chordRestAt(score, sel.tickStart(), s) };
            // Restorative clef: the one in force at the end of the range, unless a clef change is already there.
            if (end < score->endTick()) {
                const ClefType before = t.staff->clef(Fraction::fromTicks(end.ticks() - 1));
                if (t.staff->clef(end) == before) {
                    t.restoreAt = chordRestAt(score, end, s);
                    t.restore = before;
                }
            }
            result.push_back(t);
        }
        return result;
    }

    // List selection: the earliest selected item of each staff. A selected clef is replaced.
    for (EngravingItem* e : sel.elements()) {
        EngravingItem* at = e->isClef() ? e : chordRestOf(score, e);
        if (!at) {
            continue;
        }
        auto same = std::find_if(result.begin(), result.end(), [&](const ClefTarget& t) { return t.staff == e->staff(); });
        if (same == result.end()) {
            result.push_back({ e->staff(), at });
        } else if (at->tick() < same->at->tick()) {
            same->at = at;
        }
    }
    return result;
}

bool fitsStaff(ClefType type, const Staff* staff, const Fraction& tick)
{
    return ClefInfo::staffGroup(type) == staff->staffType(tick)->group(); // EditClef skips the others silently
}

bool applyClef(ClefType type, Score* score, bool caret, mu::notation::INotationPtr notation)
{
    std::vector<ClefTarget> targets = clefTargets(score, caret);
    std::erase_if(targets, [&](const ClefTarget& t) { return !t.staff || !t.at || !fitsStaff(type, t.staff, t.at->tick()); });
    if (targets.empty()) {
        return false; // nothing to put it on, or the clef does not suit the staff (perc on a pitched staff, tab6 ...)
    }

    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add clef"));
    Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
    // A clef at a bar's first item goes to the bar (as MuseScore's palette drop): the first bar's changes its
    // initial clef instead of adding a second one after it.
    auto anchor = [](EngravingItem* e) -> EngravingItem* {
        return !e->isClef() && e->rtick().isZero() && e->findMeasure() ? e->findMeasure() : e;
    };
    for (const ClefTarget& t : targets) {
        EditClef::undoChangeClef(tx, score, t.staff, anchor(t.at), type);
        if (t.restoreAt && t.restore != type && t.restore != ClefType::INVALID) {
            EditClef::undoChangeClef(tx, score, t.staff, anchor(t.restoreAt), t.restore);
        }
    }
    undoStack->commitChanges();

    // Outside note input the new clef is selected, so Return reopens it for editing.
    if (!caret) {
        const ClefTarget& first = targets.front();
        if (Clef* clef = first.at->isClef() ? toClef(first.at) : clefAt(score, first.at->tick(), first.staff->idx())) {
            notation->interaction()->select({ clef });
        }
    }
    return true;
}

bool applyOctaveLine(const ClefEntry& entry, Score* score, bool caret, PendingOttava& pending,
                     mu::notation::INotationPtr notation)
{
    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    if (caret) {
        // Any octave line entry stops the open one at the caret; 8va starts a new one there.
        const InputState& is = score->inputState();
        if (entry.kind == ClefEntry::Kind::OctaveLine && (!is.segment() || is.beyondScore())) {
            return false;
        }
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add ottava"));
        finishPending(pending, score, is.tick());
        undoStack->commitChanges();
        if (entry.kind == ClefEntry::Kind::OctaveLine) {
            pending = { true, score, is.tick(), is.track(), ottavaType(entry.octaves) };
        }
        return true;
    }
    if (entry.kind == ClefEntry::Kind::Stop) {
        return true; // our choice: nothing to stop outside note input
    }

    const OttavaType type = ottavaType(entry.octaves);
    const Selection& sel = score->selection();
    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add ottava"));
    std::vector<Ottava*> added;
    if (sel.isRange()) {
        for (staff_idx_t s = sel.staffStart(); s < sel.staffEnd(); ++s) {
            added.push_back(addOttava(score, type, sel.tickStart(), sel.tickEnd(), s));
        }
    } else {
        // From the earliest selected item to the end of the latest, on the earliest one's staff.
        // A selected octave line is replaced by one of the new type.
        const EngravingItem* first = nullptr;
        Fraction start, end;
        const std::vector<EngravingItem*> items = sel.elements(); // a copy: removing a line changes the selection
        for (EngravingItem* e : items) {
            Fraction from, to;
            if (e->isOttavaSegment()) {
                Ottava* old = toOttavaSegment(e)->ottava();
                from = old->tick();
                to = old->tick2();
                score->undoRemoveElement(old);
            } else if (const ChordRest* cr = chordRestOf(score, e)) {
                from = cr->tick();
                to = endOf(cr);
            } else {
                continue;
            }
            if (!first || from < start) {
                first = e;
                start = from;
            }
            end = std::max(end, to);
        }
        if (first) {
            added.push_back(addOttava(score, type, start, end, first->staffIdx()));
        }
    }
    std::erase(added, nullptr);
    if (added.empty()) {
        undoStack->rollbackChanges();
        return false;
    }
    undoStack->commitChanges();

    // The new line is selected, so Return reopens it for editing.
    if (LineSegment* seg = added.front()->frontSegment()) {
        notation->interaction()->select({ seg });
    }
    return true;
}

std::string clefPrefill(const EngravingItem* item)
{
    if (item->isClef()) {
        return clefText(toClef(item)->clefType());
    }
    if (item->isOttavaSegment()) {
        return octaveLineText(octaves(toOttavaSegment(item)->ottava()->ottavaType()));
    }
    return std::string();
}
}

PopoverDef mu::dorico::clefPopover()
{
    auto pending = std::make_shared<PendingOttava>();

    PopoverDef def;
    def.id = "clef";
    def.title = "Clefs and Octave Lines";
    def.icon = muse::ui::IconCode::Code::CLEF_TREBLE;
    def.supportsLocal = false; // Opt-Return (octave line in one voice only): MuseScore octave lines are staff-wide
    def.itemTypes = { ElementType::CLEF, ElementType::OTTAVA_SEGMENT };
    def.validate = [](const std::string& text) { return parseClef(text).status; };
    def.prefill = clefPrefill;

    def.apply = [pending](const std::string& text, ApplyScope, mu::notation::INotationPtr notation) {
        const ClefParse parsed = parseClef(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        Score* score = notation->elements()->msScore();
        const bool caret = notation->interaction()->noteInput()->isNoteInputMode();
        if (parsed.entry.kind == ClefEntry::Kind::Clef) {
            return applyClef(parsed.entry.clef, score, caret, notation);
        }
        return applyOctaveLine(parsed.entry, score, caret, *pending, notation);
    };

    def.noteInputEnded = [pending](mu::notation::INotationPtr notation) {
        if (!pending->active) {
            return;
        }
        Score* score = notation->elements()->msScore();
        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add ottava"));
        finishPending(*pending, score, score->inputState().tick());
        undoStack->commitChanges();
    };
    return def;
}
