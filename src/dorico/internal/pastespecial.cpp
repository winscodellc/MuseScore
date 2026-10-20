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

#include "pastespecial.h"

#include <algorithm>
#include <functional>
#include <map>
#include <vector>

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

#include "engraving/dom/chord.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/selectionfilter.h"
#include "engraving/dom/slur.h"
#include "engraving/dom/spanner.h"
#include "engraving/dom/tuplet.h"
#include "engraving/editing/exchangevoices.h"
#include "engraving/editing/paste.h"
#include "engraving/editing/transaction/transaction.h"
#include "engraving/rw/iwriter.h"
#include "engraving/rw/rwregister.h"
#include "engraving/rw/xmlreader.h"

#include "notation/inotationelements.h"
#include "notation/inotationundostack.h"

#include "pasteplanner.h"

#include "log.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;
using namespace muse;

std::optional<PasteClip> mu::dorico::pasteClipFrom(const ByteArray& data)
{
    PasteClip clip;
    clip.data = data;
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

std::optional<PasteClip> mu::dorico::pasteClipFromClipboard()
{
    const QMimeData* md = QGuiApplication::clipboard()->mimeData();
    const QString format = QString::fromLatin1(mimeStaffListFormat);
    if (!md || !md->hasFormat(format)) {
        return std::nullopt;
    }
    return pasteClipFrom(ByteArray::fromQByteArray(md->data(format)));
}

Segment* mu::dorico::pasteSegmentAt(Score* score, const Fraction& tick)
{
    Measure* m = score->tick2measure(tick);
    return m ? m->undoGetSegment(SegmentType::ChordRest, tick) : nullptr;
}

std::set<int> mu::dorico::pasteClipVoices(const ByteArray& data)
{
    std::set<int> voices;
    XmlReader xml(data);
    if (!xml.readNextStartElement() || xml.name() != "StaffList") {
        return voices;
    }
    while (xml.readNextStartElement()) {
        if (xml.name() != "Staff") {
            xml.skipCurrentElement();
            continue;
        }
        int voice = 0;
        while (xml.readNextStartElement()) {
            if (xml.name() == "location") {
                while (xml.readNextStartElement()) {
                    if (xml.name() == "voices") {
                        voice += xml.readInt();
                    } else {
                        xml.skipCurrentElement();
                    }
                }
                continue;
            }
            if (xml.name() == "Chord") {
                voices.insert(voice);
            }
            xml.skipCurrentElement();
        }
    }
    return voices;
}

namespace mu::dorico::pastespecial {
//! What Duplicate / Move / Swap act on: whole staves (range) or the voices of the selected notes (list), for a tick span.
struct Source {
    staff_idx_t staffStart = 0;
    staff_idx_t staffEnd = 0;
    Fraction from;
    Fraction to;
    std::set<int> voices;

    bool valid() const { return !voices.empty() && staffEnd > staffStart && to > from; }
};

const ChordRest* selectedChordRest(const EngravingItem* e)
{
    const ChordRest* cr = e->isNote() ? static_cast<const ChordRest*>(toNote(e)->chord()) : (e->isChordRest() ? toChordRest(e) : nullptr);
    return cr && !(cr->isChord() && toChord(cr)->isGrace()) ? cr : nullptr;
}

//! List selection: the span of the selected chord-rests, widened to whole tuplets (Dorico keeps tuplets together when a note
//! inside one is moved). ponytail: the whole span is taken, also chords of those voices between two far-apart selected notes.
Source sourceOf(const Score* score)
{
    Source src;
    const Selection& sel = score->selection();
    if (sel.isRange()) {
        src.staffStart = sel.staffStart();
        src.staffEnd = sel.staffEnd();
        src.from = sel.tickStart();
        src.to = sel.tickEnd();
        src.voices = { 0, 1, 2, 3 };
        return src;
    }

    bool first = true;
    for (const EngravingItem* e : sel.elements()) {
        const ChordRest* cr = selectedChordRest(e);
        if (!cr) {
            continue;
        }
        const DurationElement* top = cr->topTuplet() ? static_cast<const DurationElement*>(cr->topTuplet()) : cr;
        if (first) {
            src.staffStart = cr->staffIdx();
            src.staffEnd = cr->staffIdx() + 1;
            src.from = top->tick();
            src.to = top->endTick();
            first = false;
        } else {
            src.staffStart = std::min(src.staffStart, cr->staffIdx());
            src.staffEnd = std::max(src.staffEnd, cr->staffIdx() + 1);
            src.from = std::min(src.from, top->tick());
            src.to = std::max(src.to, top->endTick());
        }
        src.voices.insert(static_cast<int>(cr->voice()));
    }
    return src;
}

bool canGo(const Score* score, const Source& src, int dir)
{
    return src.valid() && (dir < 0 ? src.staffStart > 0 : src.staffEnd < score->nstaves());
}

//! Only these voices are copied and deleted
SelectionFilter voiceFilter(const std::set<int>& voices)
{
    SelectionFilter filter;
    filter.setFiltered(VoicesSelectionFilterTypes::ALL, false);
    for (int v : voices) {
        filter.setFiltered(static_cast<VoicesSelectionFilterTypes>(1u << v), true);
    }
    return filter;
}

//! A clipboard-format copy of a span, kept in memory: the system clipboard is not touched. Same recipe as
//! pasteShiftStaffTail in doricopastecontroller.cpp (the writer's filter reads the live selection, and needs an end segment).
ByteArray sliceOf(Score* score, staff_idx_t staffStart, staff_idx_t staffEnd, const Fraction& from, const Fraction& to,
                  const SelectionFilter& filter)
{
    Segment* start = score->tick2rightSegment(from);
    Segment* end = score->tick2rightSegment(to);
    if (!start) {
        return ByteArray();
    }
    score->selection().setRange(start, end, staffStart, staffEnd);
    return rw::RWRegister::writer()->writeStaffSelection(score, filter, staffStart, staffEnd, from, to, start,
                                                         end ? end : score->lastMeasure()->last());
}

void deleteSpan(Score* score, const Source& src, const SelectionFilter& filter)
{
    Segment* start = score->tick2rightSegment(src.from);
    Segment* end = score->tick2rightSegment(src.to);
    if (start) {
        score->deleteRange(start, end, staff2track(src.staffStart), staff2track(src.staffEnd), filter, true);
    }
    score->deselectAll(); // it pointed at segments that deleteRange rebuilt
}

bool pasteAt(Score* score, Transaction& tx, const ByteArray& data, staff_idx_t staff, const Fraction& tick)
{
    Segment* dst = pasteSegmentAt(score, tick);
    if (!dst || data.empty()) {
        return false;
    }
    XmlReader xml(data);
    return Paste::pasteStaff(tx, score, xml, dst, staff);
}

//! MuseScore's paste keeps the voice the music was copied from. To land it in another voice, swap the two voices over the
//! span's bars, paste (into the source voice, which now holds the destination's old music), and swap back: outside the
//! pasted span the two swaps cancel, inside it the destination voice holds the clip and the source voice is as it was.
bool pasteAtInVoice(Score* score, Transaction& tx, const ByteArray& data, staff_idx_t staff, const Fraction& tick, const Fraction& len,
                    int srcVoice, int dstVoice)
{
    if (srcVoice == dstVoice) {
        return pasteAt(score, tx, data, staff, tick);
    }
    if (score->excerpt()) {
        LOGW() << "Paste special: voices cannot be changed in a part";
        return false;
    }

    // setRangeTicks only plans a range (tickStart() stays -1 until the next update), the exchange needs real segments
    const auto exchange = [&]() {
        Segment* start = score->tick2rightSegment(tick);
        score->selection().setRange(start, score->tick2rightSegment(tick + len), staff, staff + 1);
        ExchangeVoices::exchangeVoicesInSelection(score, srcVoice, dstVoice);
    };
    exchange();
    if (!pasteAt(score, tx, data, staff, tick)) {
        return false;
    }
    exchange();
    return true;
}

bool transact(const INotationPtr& notation, const TranslatableString& title, const std::function<bool(Score*, Transaction&)>& body)
{
    Score* score = notation->elements()->msScore();
    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(title);
    Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
    if (!body(score, tx)) {
        undoStack->rollbackChanges();
        LOGW() << "Paste special: no room or nothing to do at the destination";
        return false;
    }
    undoStack->commitChanges();
    return true;
}

bool duplicateOrMove(const INotationPtr& notation, int dir, bool move)
{
    const Score* constScore = notation->elements()->msScore();
    const Source src = sourceOf(constScore);
    if (!canGo(constScore, src, dir)) {
        return false;
    }

    const staff_idx_t dstStaff = dir < 0 ? src.staffStart - 1 : src.staffStart + 1;
    const SelectionFilter filter = voiceFilter(src.voices);
    // Dorico: moved notes go into the first voice of the staff they land on
    const bool toFirstVoice = move && src.voices.size() == 1 && *src.voices.begin() != 0 && src.staffEnd - src.staffStart == 1;

    return transact(notation, move ? TranslatableString("undoableAction", "Move to staff")
                    : TranslatableString("undoableAction", "Duplicate to staff"), [&](Score* score, Transaction& tx) {
        const ByteArray data = sliceOf(score, src.staffStart, src.staffEnd, src.from, src.to, filter);
        if (data.empty()) {
            return false;
        }
        if (move) {
            deleteSpan(score, src, filter);
        }
        if (toFirstVoice) {
            return pasteAtInVoice(score, tx, data, dstStaff, src.from, src.to - src.from, *src.voices.begin(), 0);
        }
        return pasteAt(score, tx, data, dstStaff, src.from);
    });
}

//! Where a paste with nothing but a selection (or the caret) lands: the top selected staff at its earliest tick
std::optional<PasteTarget> anchorOf(const Score* score, size_t clipStaves)
{
    if (score->noteEntryMode()) {
        const InputState& is = score->inputState();
        return PasteTarget { track2staff(is.track()), is.tick() };
    }

    const Selection& sel = score->selection();
    if (sel.isRange()) {
        return PasteTarget { sel.staffStart(), sel.tickStart() };
    }

    std::vector<PasteTarget> items;
    for (const EngravingItem* e : sel.elements()) {
        if (const ChordRest* cr = selectedChordRest(e)) {
            items.push_back({ cr->staffIdx(), cr->tick() });
        }
    }
    const std::vector<PasteTarget> targets = planListPaste(items, clipStaves);
    return targets.empty() ? std::nullopt : std::optional<PasteTarget>(targets.front());
}

//! The first voice with no chord starting in [from, from + len) on the staff, -1 when all four have one
int firstEmptyVoice(const Score* score, staff_idx_t staff, const Fraction& from, const Fraction& len)
{
    const Fraction to = from + len;
    for (int v = 0; v < static_cast<int>(VOICES); ++v) {
        bool used = false;
        for (Measure* m = score->tick2measure(from); m && !used && m->tick() < to; m = m->nextMeasure()) {
            for (Segment* s = m->first(SegmentType::ChordRest); s && !used; s = s->next(SegmentType::ChordRest)) {
                const EngravingItem* e = s->tick() >= from && s->tick() < to ? s->element(staff2track(staff) + v) : nullptr;
                used = e && e->isChord();
            }
        }
        if (!used) {
            return v;
        }
    }
    return -1;
}

//! What a copied run carries besides its notes: per chord the articulations, and slurs as chord-index pairs
struct Markings {
    std::vector<std::set<SymId> > articulations;
    std::vector<std::pair<size_t, size_t> > slurs;
};

//! Reads the markings of the chords of `track` that start in [from, to). The slurs come from the score's spanner map,
//! so this is meant to be called right after pasting the clipboard there.
Markings markingsIn(const Score* score, track_idx_t track, const Fraction& from, const Fraction& to)
{
    Markings result;
    std::map<Fraction, size_t> index;
    for (Measure* m = score->tick2measure(from); m && m->tick() < to; m = m->nextMeasure()) {
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            const EngravingItem* e = s->tick() >= from && s->tick() < to ? s->element(track) : nullptr;
            if (e && e->isChord() && !toChord(e)->isGrace()) {
                index[s->tick()] = result.articulations.size();
                result.articulations.push_back(toChord(e)->articulationSymbolIds());
            }
        }
    }
    for (const auto& interval : score->spannerMap().findContained(from.ticks(), to.ticks())) {
        const Spanner* sp = interval.value;
        const auto a = sp->isSlur() && sp->track() == track ? index.find(sp->tick()) : index.end();
        const auto b = sp->isSlur() ? index.find(sp->tick2()) : index.end();
        if (a != index.end() && b != index.end()) {
            result.slurs.push_back({ a->second, b->second });
        }
    }
    std::sort(result.slurs.begin(), result.slurs.end());
    return result;
}

const ChordRest* firstSelectedChordRest(const Score* score)
{
    const Selection& sel = score->selection();
    if (sel.isRange()) {
        return sel.firstChordRest();
    }
    const ChordRest* first = nullptr;
    for (const EngravingItem* e : sel.elements()) {
        const ChordRest* cr = selectedChordRest(e);
        if (cr && (!first || cr->staffIdx() < first->staffIdx() || (cr->staffIdx() == first->staffIdx() && cr->tick() < first->tick()))) {
            first = cr;
        }
    }
    return first;
}
}

using namespace mu::dorico::pastespecial;

bool mu::dorico::pasteSpecialCanMove(const Score* score, int dir)
{
    return canGo(score, sourceOf(score), dir);
}

bool mu::dorico::pasteSpecialCanSwap(const Score* score)
{
    const Selection& sel = score->selection();
    return sel.isRange() && sel.staffEnd() - sel.staffStart() == 2;
}

bool mu::dorico::pasteSpecialHasTarget(const Score* score)
{
    return score->noteEntryMode() || !score->selection().isNone();
}

bool mu::dorico::pasteSpecialDuplicate(const INotationPtr& notation, int dir)
{
    return duplicateOrMove(notation, dir, false);
}

bool mu::dorico::pasteSpecialMove(const INotationPtr& notation, int dir)
{
    return duplicateOrMove(notation, dir, true);
}

bool mu::dorico::pasteSpecialIntoVoice(const INotationPtr& notation, int voice)
{
    const std::optional<PasteClip> clip = pasteClipFromClipboard();
    if (!clip || clip->staves != 1) {
        LOGW() << "Paste Into Voice: the clipboard must hold music from one staff";
        return false;
    }
    const std::set<int> clipVoices = pasteClipVoices(clip->data);
    if (clipVoices.size() != 1) {
        LOGW() << "Paste Into Voice: the clipboard must hold exactly one voice";
        return false;
    }

    const Score* constScore = notation->elements()->msScore();
    const std::optional<PasteTarget> anchor = anchorOf(constScore, clip->staves);
    if (!anchor) {
        return false;
    }
    const int dstVoice = voice >= 0 ? voice : firstEmptyVoice(constScore, anchor->staff, anchor->tick, clip->len);
    if (dstVoice < 0) {
        LOGW() << "Paste Into Voice: all four voices are in use there";
        return false;
    }

    return transact(notation, TranslatableString("undoableAction", "Paste into voice"), [&](Score* score, Transaction& tx) {
        return pasteAtInVoice(score, tx, clip->data, anchor->staff, anchor->tick, clip->len, *clipVoices.begin(), dstVoice);
    });
}

bool mu::dorico::pasteSpecialSwap(const INotationPtr& notation)
{
    const Score* constScore = notation->elements()->msScore();
    if (!pasteSpecialCanSwap(constScore)) {
        return false;
    }
    const Selection& sel = constScore->selection();
    const staff_idx_t top = sel.staffStart();
    const Fraction from = sel.tickStart();
    const Fraction to = sel.tickEnd();
    const Source src { top, top + 2, from, to, { 0, 1, 2, 3 } };

    const bool ok = transact(notation, TranslatableString("undoableAction", "Swap staves"), [&](Score* score, Transaction& tx) {
        const SelectionFilter all;
        const ByteArray upper = sliceOf(score, top, top + 1, from, to, all);
        const ByteArray lower = sliceOf(score, top + 1, top + 2, from, to, all);
        if (upper.empty() || lower.empty()) {
            return false;
        }
        deleteSpan(score, src, all);
        return pasteAt(score, tx, lower, top, from) && pasteAt(score, tx, upper, top + 1, from);
    });
    if (ok) {
        Score* score = notation->elements()->msScore();
        score->selection().setRangeTicks(from, to, top, top + 2);
        score->update();
    }
    return ok;
}

//! ponytail: the markings are read by pasting the clipboard at the target and rolling that back, which reuses MuseScore's
//! own reader (slurs live in a separate tick-based map in the payload). If the trial paste does not fit, nothing is applied.
bool mu::dorico::pasteSpecialArticulations(const INotationPtr& notation)
{
    const std::optional<PasteClip> clip = pasteClipFromClipboard();
    const std::set<int> clipVoices = clip ? pasteClipVoices(clip->data) : std::set<int>();
    Score* score = notation->elements()->msScore();
    const ChordRest* target = clip && !clipVoices.empty() ? firstSelectedChordRest(score) : nullptr;
    if (!target) {
        return false;
    }
    const staff_idx_t staff = target->staffIdx();
    const Fraction tick = target->tick();
    const track_idx_t targetTrack = target->track();

    // 1. Trial paste (undone below) to read the markings
    Markings markings;
    {
        INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(TranslatableString("undoableAction", "Paste articulations"));
        Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
        if (pasteAt(score, tx, clip->data, staff, tick)) {
            markings = markingsIn(score, staff2track(staff) + *clipVoices.begin(), tick, tick + clip->len);
        }
        undoStack->rollbackChanges();
    }
    if (markings.articulations.empty()) {
        return false;
    }

    // 2. Apply them to the chords that follow the selected one, in the selected one's voice
    return transact(notation, TranslatableString("undoableAction", "Paste articulations"), [&](Score* sc, Transaction&) {
        std::vector<Chord*> chords;
        const ChordRest* first = sc->findCR(tick, targetTrack);
        for (Segment* s = first ? first->segment() : nullptr; s && chords.size() < markings.articulations.size();
             s = s->next1(SegmentType::ChordRest)) {
            EngravingItem* e = s->element(targetTrack);
            if (e && e->isChord() && !toChord(e)->isGrace()) {
                chords.push_back(toChord(e));
            }
        }
        if (chords.empty()) {
            return false;
        }
        for (size_t i = 0; i < chords.size(); ++i) {
            if (!markings.articulations[i].empty()) {
                chords[i]->updateArticulations(markings.articulations[i]);
            }
        }
        for (const auto& [a, b] : markings.slurs) {
            if (b < chords.size()) {
                sc->addSlur(chords[a], chords[b], nullptr);
            }
        }
        return true;
    });
}
