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
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "engraving/dom/chord.h"
#include "engraving/dom/clef.h"
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
#include "engraving/editing/implodeexplode.h"
#include "engraving/editing/paste.h"
#include "engraving/editing/transaction/transaction.h"
#include "engraving/rw/iwriter.h"
#include "engraving/rw/rwregister.h"
#include "engraving/rw/xmlreader.h"

#include "notation/inotationelements.h"
#include "notation/inotationundostack.h"

#include "pasteplanner.h"
#include "transact.h"

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

std::vector<size_t> mu::dorico::reduceGroups(size_t nSources, size_t nDest)
{
    std::vector<size_t> sizes;
    for (size_t i = 0; nDest > 0 && i < nDest; ++i) {
        sizes.push_back(nSources / nDest + (i < nSources % nDest ? 1 : 0));
    }
    return sizes;
}

std::vector<std::vector<size_t> > mu::dorico::explodeNotes(size_t nNotes, size_t nStaves)
{
    std::vector<std::vector<size_t> > result(nStaves);
    if (nNotes == 0 || nStaves == 0) {
        return result;
    }
    if (nNotes >= nStaves) {
        size_t next = 0;
        for (size_t i = 0; i < nStaves; ++i) {
            for (size_t n = reduceGroups(nNotes, nStaves)[i]; n > 0; --n) {
                result[i].push_back(next++);
            }
        }
    } else {
        const size_t perNote = std::max<size_t>(nStaves / nNotes, 1);
        for (size_t i = 0; i < nStaves; ++i) {
            result[i].push_back(std::min(i / perNote, nNotes - 1));
        }
    }
    return result;
}

namespace {
//! Copies the element the reader is on (and what is inside it) to `out`. `track` receives the text of a direct <track> child.
//! `firstTick`: the element is a <Staff> that has to start at that tick. A clip reads its staves one after the other, so a
//! staff after the first begins with a <location> that rewinds from where the previous one ended (<fractions>-1/1</fractions>);
//! on its own that lands at the wrong tick. The leading <location> is replaced by one that goes to `firstTick`, as the first
//! staff of a clip has.
void copyElementTo(QXmlStreamReader& in, QXmlStreamWriter& out, int* track = nullptr, const QString* firstTick = nullptr)
{
    int depth = 0;
    bool startPlaced = !firstTick;
    do {
        switch (in.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (!startPlaced && depth == 1) {
                startPlaced = true;
                const bool hadLocation = in.name() == QLatin1String("location");
                out.writeStartElement("location");
                out.writeTextElement("fractions", *firstTick);
                while (hadLocation && in.readNextStartElement()) { // keep the other steps (a voice change) as they were
                    const QString name = in.name().toString();
                    const QString text = in.readElementText();
                    if (name != QLatin1String("fractions")) {
                        out.writeTextElement(name, text);
                    }
                }
                out.writeEndElement();
                if (hadLocation) {
                    in.readNext(); // past the </location> the loop stopped on
                    continue;
                }
            }
            if (track && depth == 1 && in.name() == QLatin1String("track")) {
                *track = in.readElementText().toInt();
                out.writeTextElement("track", QString::number(*track));
                in.readNext(); // readElementText() stopped on the end tag, which is already written
                continue;
            }
            ++depth;
            out.writeStartElement(in.name().toString());
            out.writeAttributes(in.attributes());
            break;
        case QXmlStreamReader::EndElement:
            out.writeEndElement();
            if (--depth == 0) {
                return; // stay on the end tag, like skipCurrentElement(), so the caller's next read is the next sibling
            }
            break;
        case QXmlStreamReader::Characters:
            out.writeCharacters(in.text().toString());
            break;
        default:
            break;
        }
        in.readNext();
    } while (!in.atEnd());
}
}

ByteArray mu::dorico::staffClipOf(const ByteArray& data, size_t index)
{
    QXmlStreamReader in(data.toQByteArrayNoCopy());
    if (!in.readNextStartElement() || in.name() != QLatin1String("StaffList")) {
        return ByteArray();
    }

    const QXmlStreamAttributes rootAttrs = in.attributes();
    std::vector<std::pair<int, QByteArray> > spanners; // track, xml
    QByteArray staffXml;
    int staffId = -1;
    size_t seen = 0;
    while (in.readNextStartElement()) {
        if (in.name() == QLatin1String("Staff")) {
            if (seen++ == index) {
                staffId = in.attributes().value("id").toInt();
                QXmlStreamWriter w(&staffXml);
                const QString firstTick = rootAttrs.value("tick").toString();
                copyElementTo(in, w, nullptr, &firstTick);
            } else {
                in.skipCurrentElement();
            }
        } else if (in.name() == QLatin1String("SpannerMap")) {
            while (in.readNextStartElement()) {
                int track = -1;
                QByteArray xml;
                QXmlStreamWriter w(&xml);
                copyElementTo(in, w, &track);
                spanners.push_back({ track, xml });
            }
        } else {
            in.skipCurrentElement();
        }
    }
    if (staffId < 0) {
        return ByteArray();
    }

    QByteArray result = "<StaffList";
    for (const QXmlStreamAttribute& a : rootAttrs) {
        const QString name = a.name().toString();
        const QString value = name == "staff" ? QString::number(staffId) : name == "staves" ? QString("1") : a.value().toString();
        result += " " + name.toUtf8() + "=\"" + value.toHtmlEscaped().toUtf8() + "\"";
    }
    result += ">" + staffXml;
    bool any = false;
    for (const auto& [track, xml] : spanners) {
        if (track / static_cast<int>(VOICES) == staffId) {
            result += (any ? "" : "<SpannerMap>") + xml;
            any = true;
        }
    }
    result += any ? "</SpannerMap></StaffList>" : "</StaffList>";
    return ByteArray::fromQByteArray(result);
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

//! The chords of the staff that start in [from, to), grace chords included
std::vector<Chord*> chordsIn(const Score* score, staff_idx_t staff, const Fraction& from, const Fraction& to)
{
    std::vector<Chord*> chords;
    for (Measure* m = score->tick2measure(from); m && m->tick() < to; m = m->nextMeasure()) {
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            if (s->tick() < from || s->tick() >= to) {
                continue;
            }
            for (voice_idx_t v = 0; v < VOICES; ++v) {
                EngravingItem* e = s->element(staff2track(staff) + v);
                if (e && e->isChord()) {
                    chords.push_back(toChord(e));
                    chords.insert(chords.end(), toChord(e)->graceNotes().begin(), toChord(e)->graceNotes().end());
                }
            }
        }
    }
    return chords;
}

//! Where the chords of a voice start and how long they are: two voices with the same list can be merged chord by chord
using Rhythm = std::vector<std::pair<Fraction, Fraction> >;

Rhythm rhythmOf(const Score* score, track_idx_t track, const Fraction& from, const Fraction& to)
{
    Rhythm rhythm;
    for (Measure* m = score->tick2measure(from); m && m->tick() < to; m = m->nextMeasure()) {
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            const EngravingItem* e = s->tick() >= from && s->tick() < to ? s->element(track) : nullptr;
            if (e && e->isChord()) {
                rhythm.push_back({ s->tick(), toChord(e)->ticks() });
            }
        }
    }
    return rhythm;
}

//! Dorico's Reduce drops the clef changes and octave lines of the music it merges
void removeClefsAndOttavas(Score* score, staff_idx_t staff, const Fraction& from, const Fraction& to)
{
    std::vector<EngravingItem*> doomed;
    for (Measure* m = score->tick2measure(from); m && m->tick() < to; m = m->nextMeasure()) {
        for (Segment* s = m->first(SegmentType::Clef); s; s = s->next(SegmentType::Clef)) {
            EngravingItem* e = s->tick() >= from && s->tick() < to ? s->element(staff2track(staff)) : nullptr;
            if (e && e->isClef()) {
                doomed.push_back(e);
            }
        }
    }
    for (const auto& interval : score->spannerMap().findOverlapping(from.ticks(), to.ticks())) {
        Spanner* sp = interval.value;
        if (sp->isOttava() && sp->staffIdx() == staff && sp->tick() >= from && sp->tick() < to) {
            doomed.push_back(sp);
        }
    }
    for (EngravingItem* e : doomed) {
        score->undoRemoveElement(e);
    }
}

//! The clip goes to every staff, then each staff keeps only its share of every chord
bool explodeInto(Score* score, Transaction& tx, const ByteArray& data, staff_idx_t top, size_t nStaves, const Fraction& from,
                 const Fraction& len)
{
    for (size_t i = 0; i < nStaves; ++i) {
        if (!pasteAt(score, tx, data, top + i, from)) {
            return false;
        }
    }
    for (size_t i = 0; i < nStaves; ++i) {
        for (Chord* chord : chordsIn(score, top + i, from, from + len)) {
            const std::vector<Note*> notes = chord->notes(); // bottom to top
            const std::vector<size_t> keep = explodeNotes(notes.size(), nStaves)[i];
            for (size_t j = 0; j < notes.size(); ++j) {
                if (std::find(keep.begin(), keep.end(), notes.size() - 1 - j) == keep.end()) {
                    score->undoRemoveElement(notes[j]);
                }
            }
        }
    }
    return true;
}

//! Each destination staff takes its group of the copied staves, one per voice; groups whose voices all have the same rhythm
//! are then merged into chords of the first voice (ImplodeExplode drops the unisons).
bool reduceInto(Score* score, Transaction& tx, const PasteClip& clip, staff_idx_t top, size_t nStaves, const Fraction& from)
{
    const Fraction to = from + clip.len;
    const std::vector<size_t> groups = reduceGroups(clip.staves, nStaves);
    if (*std::max_element(groups.begin(), groups.end()) > VOICES) {
        LOGW() << "Reduce: a staff would need more than four voices";
        return false;
    }

    size_t source = 0;
    for (size_t g = 0; g < nStaves; ++g) {
        for (size_t j = 0; j < groups[g]; ++j, ++source) {
            const ByteArray one = staffClipOf(clip.data, source);
            const std::set<int> voices = pasteClipVoices(one);
            if (voices.size() > 1) {
                LOGW() << "Reduce: a copied staff holds more than one voice";
                return false;
            }
            if (!voices.empty() && !pasteAtInVoice(score, tx, one, top + g, from, clip.len, *voices.begin(), static_cast<int>(j))) {
                return false;
            }
        }
    }

    for (size_t g = 0; g < nStaves; ++g) {
        const staff_idx_t staff = top + g;
        const Rhythm first = rhythmOf(score, staff2track(staff), from, to);
        bool same = true;
        for (size_t j = 1; j < groups[g] && same; ++j) {
            same = rhythmOf(score, staff2track(staff) + j, from, to) == first;
        }
        if (!same || groups[g] < 2) {
            continue;
        }
        score->selection().setRange(score->tick2rightSegment(from), score->tick2rightSegment(to), staff, staff + 1);
        if (!ImplodeExplode::implode(score)) {
            return false;
        }
        removeClefsAndOttavas(score, staff, from, to);
    }
    return true;
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

bool mu::dorico::pasteSpecialCanExplode(const Score* score)
{
    const Selection& sel = score->selection();
    const std::optional<PasteClip> clip = pasteClipFromClipboard();
    return clip && clip->staves == 1 && sel.isRange() && sel.staffEnd() - sel.staffStart() >= 2;
}

bool mu::dorico::pasteSpecialCanReduce(const Score* score)
{
    const Selection& sel = score->selection();
    const std::optional<PasteClip> clip = pasteClipFromClipboard();
    return clip && sel.isRange() && clip->staves > sel.staffEnd() - sel.staffStart();
}

bool mu::dorico::pasteSpecialExplode(const INotationPtr& notation)
{
    const Score* constScore = notation->elements()->msScore();
    if (!pasteSpecialCanExplode(constScore)) {
        return false;
    }
    const std::optional<PasteClip> clip = pasteClipFromClipboard();
    const Selection& sel = constScore->selection();
    const staff_idx_t top = sel.staffStart();
    const size_t nStaves = sel.staffEnd() - top;
    const Fraction from = sel.tickStart();

    const bool ok = transact(notation, TranslatableString("undoableAction", "Explode"), [&](Score* score, Transaction& tx) {
        return explodeInto(score, tx, clip->data, top, nStaves, from, clip->len);
    });
    if (ok) {
        Score* score = notation->elements()->msScore();
        score->selection().setRangeTicks(from, from + clip->len, top, top + nStaves);
        score->update();
    }
    return ok;
}

bool mu::dorico::pasteSpecialReduce(const INotationPtr& notation)
{
    const Score* constScore = notation->elements()->msScore();
    if (!pasteSpecialCanReduce(constScore)) {
        return false;
    }
    const std::optional<PasteClip> clip = pasteClipFromClipboard();
    const Selection& sel = constScore->selection();
    const staff_idx_t top = sel.staffStart();
    const size_t nStaves = sel.staffEnd() - top;
    const Fraction from = sel.tickStart();

    const bool ok = transact(notation, TranslatableString("undoableAction", "Reduce"), [&](Score* score, Transaction& tx) {
        return reduceInto(score, tx, *clip, top, nStaves, from);
    });
    if (ok) {
        Score* score = notation->elements()->msScore();
        score->selection().setRangeTicks(from, from + clip->len, top, top + nStaves);
        score->update();
    }
    return ok;
}
