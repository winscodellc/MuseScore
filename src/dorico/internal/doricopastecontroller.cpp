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

#include "doricopastecontroller.h"

#include <algorithm>
#include <functional>
#include <optional>

#include "engraving/dom/chord.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/editing/noteinput.h"
#include "engraving/editing/paste.h"
#include "engraving/editing/transaction/transaction.h"
#include "engraving/rw/iwriter.h"
#include "engraving/rw/rwregister.h"
#include "engraving/rw/xmlreader.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"
#include "notationscene/notationcommands.h"

#include "../doricocommands.h"
#include "pastespecial.h"

#include "log.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;
using namespace muse;

namespace {
//! End of the last bar of `staff` that has a chord starting at or after `from`; `from` itself when there is none.
Fraction pasteStaffTailEnd(const Score* score, staff_idx_t staff, const Fraction& from)
{
    for (Measure* m = score->lastMeasure(); m && m->endTick() > from; m = m->prevMeasure()) {
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            if (s->tick() < from) {
                continue;
            }
            for (track_idx_t t = staff2track(staff); t < staff2track(staff + 1); ++t) {
                if (s->element(t) && s->element(t)->isChord()) {
                    return m->endTick();
                }
            }
        }
    }
    return from;
}

//! Moves everything on `staff` from `from` to the end of its last chord's bar, so that it starts at `to` instead (all
//! voices; barlines stay, notes are re-split across them). Anything between the two positions is replaced: with
//! to > from the gap [from, to) is left for the caller to fill, with to < from [to, from) is removed.
//! ponytail: the tail goes through a clipboard-style copy, so ties and slurs that cross `from` break, and `from` inside a
//! long note starts at the next chord.
bool pasteShiftStaffTail(Score* score, Transaction& tx, staff_idx_t staff, const Fraction& from, const Fraction& to)
{
    const Fraction low = std::min(from, to);
    const Fraction tailEnd = pasteStaffTailEnd(score, staff, from);
    const Fraction high = std::max(from, tailEnd);

    Segment* s1 = score->tick2rightSegment(low);
    Segment* s2 = score->tick2rightSegment(high);
    if (!s1 || low == high) {
        return true;
    }

    // The writer's filter reads the live selection, so make the tail the selection. A tail that runs to the end of the score
    // has no end segment, but the spanner writer needs one: the last measure's last segment stands in.
    ByteArray tail;
    if (tailEnd > from) {
        Segment* tailStart = score->tick2rightSegment(from);
        score->selection().setRange(tailStart, s2, staff, staff + 1);
        tail = rw::RWRegister::writer()->writeStaffSelection(score, score->selectionFilter(), staff, staff + 1, from, tailEnd,
                                                             tailStart, s2 ? s2 : score->lastMeasure()->last());
    }
    score->deleteRange(s1, s2, staff2track(staff), staff2track(staff + 1), score->selectionFilter(), true);
    score->deselectAll(); // it pointed at segments that deleteRange rebuilt
    if (tail.empty()) {
        return true;
    }

    const Fraction newEnd = tailEnd + (to - from);
    while (score->lastMeasure()->endTick() < newEnd) {
        score->appendMeasures(1);
    }
    Segment* dst = pasteSegmentAt(score, to);
    XmlReader xml(tail);
    return dst && Paste::pasteStaff(tx, score, xml, dst, staff);
}

struct PasteChordNotes {
    track_idx_t track = 0;
    Fraction tick;
    std::vector<NoteVal> notes;
};

//! The notes already in [from, to) on the pasted staves: Chord mode puts them back into the chords that replace them.
std::vector<PasteChordNotes> pasteChordNotesIn(Score* score, const PasteTarget& target, size_t staves, const Fraction& len)
{
    std::vector<PasteChordNotes> result;
    const Fraction to = target.tick + len;
    const track_idx_t t1 = staff2track(target.staff);
    const track_idx_t t2 = staff2track(std::min(target.staff + staves, score->nstaves()));
    for (Measure* m = score->tick2measure(target.tick); m && m->tick() < to; m = m->nextMeasure()) {
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            if (s->tick() < target.tick || s->tick() >= to) {
                continue;
            }
            for (track_idx_t t = t1; t < t2; ++t) {
                EngravingItem* e = s->element(t);
                if (!e || !e->isChord() || toChord(e)->isGrace()) {
                    continue;
                }
                PasteChordNotes snap { t, s->tick(), {} };
                for (Note* n : toChord(e)->notes()) {
                    snap.notes.push_back(n->noteVal());
                }
                result.push_back(snap);
            }
        }
    }
    return result;
}

void pasteRestoreChordNotes(Score* score, Transaction& tx, const std::vector<PasteChordNotes>& before)
{
    for (const PasteChordNotes& snap : before) {
        ChordRest* cr = score->findCR(snap.tick, snap.track);
        if (!cr || !cr->isChord() || cr->tick() != snap.tick) {
            continue;
        }
        Chord* chord = toChord(cr);
        for (const NoteVal& nv : snap.notes) {
            if (!chord->findNote(nv.pitch)) {
                NoteInput::addNote(tx, score, chord, nv);
            }
        }
    }
}

//! One copy of `clip` at `target`, in the caller's transaction, with the Insert and Chord rules (spec 40). false = no room.
bool pasteClipAt(Score* score, Transaction& tx, const PasteClip& clip, const PasteTarget& target, bool insertMode, bool chordMode)
{
    if (insertMode) {
        const size_t lastStaff = std::min(target.staff + clip.staves, score->nstaves());
        for (staff_idx_t staff = target.staff; staff < lastStaff; ++staff) {
            if (!pasteShiftStaffTail(score, tx, staff, target.tick, target.tick + clip.len)) {
                return false;
            }
        }
    }
    std::vector<PasteChordNotes> before;
    if (chordMode) {
        before = pasteChordNotesIn(score, target, clip.staves, clip.len);
    }

    Segment* dst = pasteSegmentAt(score, target.tick);
    XmlReader xml(clip.data);
    if (!dst || !Paste::pasteStaff(tx, score, xml, dst, target.staff)) {
        return false;
    }
    pasteRestoreChordNotes(score, tx, before);
    return true;
}

std::vector<PasteTarget> pasteTargets(const Score* score, const PasteClip& clip, bool insertMode)
{
    if (score->noteEntryMode()) {
        const InputState& is = score->inputState();
        return { { track2staff(is.track()), is.tick() } };
    }

    const Selection& sel = score->selection();
    if (sel.isRange()) {
        return planRangePaste(sel.staffStart(), sel.staffEnd(), sel.tickStart(), sel.tickEnd(), clip.len, clip.staves, !insertMode);
    }

    std::vector<PasteTarget> items;
    for (const EngravingItem* e : sel.elements()) {
        const EngravingItem* anchor = e->isNote() ? static_cast<const EngravingItem*>(toNote(e)->chord()) : e;
        items.push_back({ anchor->staffIdx(), anchor->tick() });
    }
    return planListPaste(items, clip.staves);
}
}

namespace {
//! Insert mode: what follows [from, to) on the staves closes the gap instead of leaving rests (one undo step).
bool closeGap(const INotationPtr& notation, const TranslatableString& title, staff_idx_t staffStart, staff_idx_t staffEnd,
              const Fraction& from, const Fraction& to)
{
    Score* score = notation->elements()->msScore();
    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(title);
    Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
    bool ok = true;
    for (staff_idx_t staff = staffStart; ok && staff < staffEnd; ++staff) {
        ok = pasteShiftStaffTail(score, tx, staff, to, from);
    }
    if (!ok) {
        undoStack->rollbackChanges();
        return false;
    }
    undoStack->commitChanges();
    return true;
}

//! Dorico selects the nearest logical item after a delete: the first chord of the voice from `to` on, else the last one
//! before `from`. ponytail: walks the score from the start, fine for one key press; use tick2rightSegment if it ever shows.
ChordRest* nearestChord(const Score* score, track_idx_t track, const Fraction& from, const Fraction& to)
{
    ChordRest* before = nullptr;
    for (Segment* s = score->firstSegment(SegmentType::ChordRest); s; s = s->next1(SegmentType::ChordRest)) {
        EngravingItem* e = s->element(track);
        if (!e || !e->isChord()) {
            continue;
        }
        if (s->tick() >= to) {
            return toChordRest(e);
        }
        if (s->tick() < from) {
            before = toChordRest(e);
        }
    }
    return before;
}

void selectChord(const INotationPtr& notation, ChordRest* cr)
{
    if (cr && cr->isChord()) {
        notation->interaction()->select({ toChord(cr)->upNote() });
    } else if (cr) {
        notation->interaction()->select({ cr });
    }
}

//! The rests Remove Rests acts on: the selected ones, or those inside a range.
std::vector<Rest*> selectedRests(const Score* score)
{
    std::vector<Rest*> rests;
    const Selection& sel = score->selection();
    if (sel.isRange()) {
        for (Segment* s = sel.startSegment(); s && s->tick() < sel.tickEnd(); s = s->next1(SegmentType::ChordRest)) {
            for (track_idx_t t = staff2track(sel.staffStart()); t < staff2track(sel.staffEnd()); ++t) {
                if (s->element(t) && s->element(t)->isRest()) {
                    rests.push_back(toRest(s->element(t)));
                }
            }
        }
    } else {
        for (EngravingItem* e : sel.elements()) {
            if (e->isRest()) {
                rests.push_back(toRest(e));
            }
        }
    }
    return rests;
}

//! Voice 1 cannot lose a rest, so it is hidden; the other voices drop it (one undo step).
bool removeRests(const INotationPtr& notation)
{
    Score* score = notation->elements()->msScore();
    const std::vector<Rest*> rests = selectedRests(score);
    if (rests.empty()) {
        return false;
    }
    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(TranslatableString("undoableAction", "Remove rests"));
    for (Rest* rest : rests) {
        if (rest->voice() > 0) {
            score->deleteItem(rest);
        } else {
            rest->undoChangeProperty(Pid::VISIBLE, false);
        }
    }
    undoStack->commitChanges();
    return true;
}
}

void DoricoPasteController::init()
{
    // Edit > Paste Special (spec 41)
    const auto onNotation = [this](const muse::rcommand::Command& command, std::function<bool(const INotationPtr&)> run) {
        dispatcher()->onRequest(this, command, [this, run]() {
            if (INotationPtr notation = globalContext()->currentNotation()) {
                run(notation);
            }
            return muse::make_ok();
        });
    };
    onNotation(DORICO_DUPLICATE_TO_STAFF_ABOVE_COMMAND, [](const INotationPtr& n) { return pasteSpecialDuplicate(n, -1); });
    onNotation(DORICO_DUPLICATE_TO_STAFF_BELOW_COMMAND, [](const INotationPtr& n) { return pasteSpecialDuplicate(n, 1); });
    onNotation(DORICO_MOVE_TO_STAFF_ABOVE_COMMAND, [](const INotationPtr& n) { return pasteSpecialMove(n, -1); });
    onNotation(DORICO_MOVE_TO_STAFF_BELOW_COMMAND, [](const INotationPtr& n) { return pasteSpecialMove(n, 1); });
    for (int voice = 0; voice < 4; ++voice) {
        onNotation(DORICO_PASTE_INTO_VOICE_COMMANDS[voice], [voice](const INotationPtr& n) { return pasteSpecialIntoVoice(n, voice); });
    }
    onNotation(DORICO_PASTE_INTO_NEW_VOICE_COMMAND, [](const INotationPtr& n) { return pasteSpecialIntoVoice(n, -1); });
    onNotation(DORICO_SWAP_STAVES_COMMAND, [](const INotationPtr& n) { return pasteSpecialSwap(n); });
    onNotation(DORICO_EXPLODE_COMMAND, [](const INotationPtr& n) { return pasteSpecialExplode(n); });
    onNotation(DORICO_REDUCE_COMMAND, [](const INotationPtr& n) { return pasteSpecialReduce(n); });
    onNotation(DORICO_PASTE_ARTICULATIONS_COMMAND, [](const INotationPtr& n) { return pasteSpecialArticulations(n); });

    onNotation(DORICO_REMOVE_RESTS_COMMAND, [](const INotationPtr& n) { return removeRests(n); });
    dispatcher()->onRequest(this, DORICO_DELETE_BARS_COMMAND, [this]() {
        dispatcher()->dispatch(mu::notation::REMOVE_SELECTED_RANGE_COMMAND);
        return muse::make_ok();
    });

    dispatcher()->preDispatch().onReceive(this, [this](const muse::rcommand::Command& command, bool* allowDispatch) {
        if (!*allowDispatch) {
            return; // blocked in this mode (spec 97)
        }
        if (command == mu::notation::DELETE_COMMAND && tryDelete()) {
            *allowDispatch = false;
        } else if (command == mu::notation::PASTE_COMMAND && tryPaste()) {
            *allowDispatch = false;
        } else if (command == mu::notation::CUT_COMMAND && tryCut()) {
            *allowDispatch = false;
        } else if (command == mu::notation::REPEAT_SELECTION_COMMAND && tryRepeat()) {
            *allowDispatch = false;
        }
    });
}

bool DoricoPasteController::tryPaste()
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation || notation->interaction()->isTextEditingStarted()) {
        return false;
    }
    const std::optional<PasteClip> clip = pasteClipFromClipboard();
    if (!clip) {
        return false; // symbols (an immediate dynamic, ...) are pasted once: MuseScore's own paste does that
    }

    Score* score = notation->elements()->msScore();
    const bool insertMode = modes()->isInsertMode();
    const bool chordMode = modes()->isChordMode();
    const std::vector<PasteTarget> targets = pasteTargets(score, *clip, insertMode);
    if (targets.empty() || (targets.size() == 1 && !score->noteEntryMode() && !insertMode && !chordMode)) {
        return false;
    }

    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(TranslatableString("undoableAction", "Paste"));
    Transaction& tx = score->transactionManager()->currentOrDummyTransaction();

    bool ok = true;
    for (const PasteTarget& target : targets) {
        ok = pasteClipAt(score, tx, *clip, target, insertMode, chordMode);
        if (!ok) {
            break;
        }
    }

    if (!ok) {
        undoStack->rollbackChanges();
        LOGW() << "Paste: no room for the clipboard at the destination";
        return true;
    }

    Fraction first = targets.front().tick;
    Fraction end = first;
    staff_idx_t top = targets.front().staff;
    staff_idx_t bottom = top;
    for (const PasteTarget& t : targets) {
        first = std::min(first, t.tick);
        end = std::max(end, t.tick + clip->len);
        top = std::min(top, t.staff);
        bottom = std::max(bottom, std::min(t.staff + clip->staves, score->nstaves()) - 1);
    }
    undoStack->commitChanges();

    if (score->noteEntryMode()) {
        // The caret goes to the end of what was pasted
        if (Segment* seg = score->tick2segment(end, true, SegmentType::ChordRest)) {
            score->inputState().setSegment(seg);
            notation->interaction()->noteInput()->setCurrentTrack(score->inputState().track());
        }
    } else {
        score->selection().setRangeTicks(first, end, top, bottom + 1);
        score->update();
    }
    return true;
}

bool DoricoPasteController::tryCut()
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation || !modes()->isInsertMode() || notation->interaction()->isTextEditingStarted()) {
        return false;
    }
    Score* score = notation->elements()->msScore();
    const Selection& sel = score->selection();
    if (!sel.isRange() || !sel.canCopy()) {
        return false;
    }

    // Insert mode: what follows the cut music closes the gap instead of leaving rests
    const staff_idx_t staffStart = sel.staffStart();
    const staff_idx_t staffEnd = sel.staffEnd();
    const Fraction from = sel.tickStart();
    const Fraction to = sel.tickEnd();
    notation->interaction()->copySelection();

    if (closeGap(notation, TranslatableString("undoableAction", "Cut"), staffStart, staffEnd, from, to)) {
        if (ChordRest* cr = score->findCR(from, staff2track(staffStart))) {
            notation->interaction()->select({ cr });
        }
    }
    return true;
}

bool DoricoPasteController::tryDelete()
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation || notation->interaction()->isTextEditingStarted()) {
        return false;
    }
    Score* score = notation->elements()->msScore();
    if (score->noteEntryMode()) {
        return false; // Backspace in note input stays MuseScore's
    }
    const Selection& sel = score->selection();
    const bool insertMode = modes()->isInsertMode();
    const TranslatableString title("undoableAction", "Delete");

    if (sel.isRange()) {
        // Bars selected with Insert off: MuseScore already empties them and keeps the barlines
        if (!insertMode || sel.tickEnd() <= sel.tickStart()) {
            return false;
        }
        const track_idx_t track = staff2track(sel.staffStart());
        const Fraction from = sel.tickStart();
        if (closeGap(notation, title, sel.staffStart(), sel.staffEnd(), from, sel.tickEnd())) {
            selectChord(notation, nearestChord(score, track, from, from));
        }
        return true;
    }
    if (!sel.isList()) {
        return false;
    }

    // Only notes and rests; any other item is deleted by MuseScore
    std::vector<ChordRest*> crs;
    size_t notes = 0;
    for (EngravingItem* e : sel.elements()) {
        if (e->isNote()) {
            ++notes;
            ChordRest* cr = toNote(e)->chord();
            if (std::find(crs.begin(), crs.end(), cr) == crs.end()) {
                crs.push_back(cr);
            }
        } else if (e->isRest()) {
            crs.push_back(toRest(e));
        } else {
            return false;
        }
    }
    if (crs.empty()) {
        return false;
    }
    const track_idx_t track = crs.front()->track();
    Fraction from = crs.front()->tick();
    Fraction to = crs.front()->endTick();
    for (ChordRest* cr : crs) {
        from = std::min(from, cr->tick());
        to = std::max(to, cr->endTick());
    }

    if (insertMode) {
        // One whole chord or rest: a chord with only some notes selected just loses those notes
        const bool whole = crs.size() == 1 && (crs.front()->isRest() || notes == toChord(crs.front())->notes().size());
        if (!whole) {
            return false;
        }
        if (closeGap(notation, title, crs.front()->staffIdx(), crs.front()->staffIdx() + 1, from, to)) {
            selectChord(notation, nearestChord(score, track, from, from));
        }
        return true;
    }

    if (notes == 0 || std::any_of(crs.begin(), crs.end(), [track](ChordRest* cr) { return cr->track() != track; })) {
        return false;
    }
    notation->interaction()->deleteSelection();
    // Whole chords became rests: the selection goes to the nearest note of the voice instead
    const std::vector<EngravingItem*>& left = sel.elements();
    if (!left.empty() && std::all_of(left.begin(), left.end(), [](EngravingItem* e) { return e->isRest(); })) {
        selectChord(notation, nearestChord(score, track, from, to));
    }
    return true;
}

bool DoricoPasteController::tryRepeat()
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation || notation->interaction()->isTextEditingStarted()) {
        return false;
    }
    Score* score = notation->elements()->msScore();
    const Selection& sel = score->selection();
    // Only a range is handled here (copies it with the Insert and Chord rules). Note input, a single chord and a list of notes
    // repeat the way MuseScore does it, which is already what Dorico does.
    if (score->noteEntryMode() || !sel.isRange() || !sel.canCopy() || sel.tickEnd() <= sel.tickStart()) {
        return false;
    }
    const std::optional<PasteClip> clip = pasteClipFrom(sel.mimeData());
    if (!clip) {
        return false;
    }

    const PasteTarget target { sel.staffStart(), sel.tickEnd() };
    const staff_idx_t bottom = std::min(target.staff + clip->staves, score->nstaves());

    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(TranslatableString("undoableAction", "Repeat selection"));
    Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
    while (score->lastMeasure()->endTick() < target.tick + clip->len) {
        score->appendMeasures(1);
    }
    if (!pasteClipAt(score, tx, *clip, target, modes()->isInsertMode(), modes()->isChordMode())) {
        undoStack->rollbackChanges();
        LOGW() << "Repeat: no room after the selection";
        return true;
    }
    undoStack->commitChanges();

    score->selection().setRangeTicks(target.tick, target.tick + clip->len, target.staff, bottom);
    score->update();
    return true;
}

bool DoricoPasteController::pasteStaff(Transaction& tx, Score* score, const ByteArray& data, Segment* dst, staff_idx_t staff)
{
    const std::optional<PasteClip> clip = pasteClipFrom(data);
    return clip && dst && pasteClipAt(score, tx, *clip, { staff, dst->tick() }, modes()->isInsertMode(), modes()->isChordMode());
}
