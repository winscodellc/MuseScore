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
#include <optional>

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

#include "engraving/dom/chord.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
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

#include "log.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;
using namespace muse;

namespace {
struct PasteClip {
    Fraction len;
    size_t staves = 0;
    ByteArray data;
};

std::optional<PasteClip> pasteClipFromClipboard()
{
    const QMimeData* md = QGuiApplication::clipboard()->mimeData();
    const QString format = QString::fromLatin1(mimeStaffListFormat);
    if (!md || !md->hasFormat(format)) {
        return std::nullopt;
    }

    PasteClip clip;
    clip.data = ByteArray::fromQByteArray(md->data(format));
    XmlReader xml(clip.data);
    if (!xml.readNextStartElement() || xml.name() != "StaffList") {
        return std::nullopt;
    }
    clip.len = Fraction::fromString(xml.attribute("len"));
    clip.staves = static_cast<size_t>(xml.intAttribute("staves", 0));
    if (clip.len.isZero() || clip.staves == 0) {
        return std::nullopt;
    }
    return clip;
}

Segment* pasteSegmentAt(Score* score, const Fraction& tick)
{
    Measure* m = score->tick2measure(tick);
    return m ? m->undoGetSegment(SegmentType::ChordRest, tick) : nullptr;
}

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

void DoricoPasteController::init()
{
    dispatcher()->preDispatch().onReceive(this, [this](const muse::rcommand::Command& command, bool* allowDispatch) {
        if (command == mu::notation::PASTE_COMMAND && tryPaste()) {
            *allowDispatch = false;
        } else if (command == mu::notation::CUT_COMMAND && tryCut()) {
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
        const size_t lastStaff = std::min(target.staff + clip->staves, score->nstaves());
        if (insertMode) {
            for (staff_idx_t staff = target.staff; ok && staff < lastStaff; ++staff) {
                ok = pasteShiftStaffTail(score, tx, staff, target.tick, target.tick + clip->len);
            }
        }
        std::vector<PasteChordNotes> before;
        if (chordMode) {
            before = pasteChordNotesIn(score, target, clip->staves, clip->len);
        }

        Segment* dst = ok ? pasteSegmentAt(score, target.tick) : nullptr;
        XmlReader xml(clip->data);
        ok = dst && Paste::pasteStaff(tx, score, xml, dst, target.staff);
        if (!ok) {
            break;
        }
        pasteRestoreChordNotes(score, tx, before);
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

    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(TranslatableString("undoableAction", "Cut"));
    Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
    bool ok = true;
    for (staff_idx_t staff = staffStart; ok && staff < staffEnd; ++staff) {
        ok = pasteShiftStaffTail(score, tx, staff, to, from);
    }
    if (!ok) {
        undoStack->rollbackChanges();
        return true;
    }
    undoStack->commitChanges();

    if (ChordRest* cr = score->findCR(from, staff2track(staffStart))) {
        notation->interaction()->select({ cr });
    }
    return true;
}
