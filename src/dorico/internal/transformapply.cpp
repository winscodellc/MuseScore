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

#include "transformapply.h"

#include <algorithm>
#include <map>

#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/interval.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/pitchspelling.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/tie.h"
#include "engraving/dom/tuplet.h"
#include "engraving/editing/editnote.h"
#include "engraving/editing/noteinput.h"
#include "engraving/editing/transpose.h"

#include "transact.h"
#include "transform.h"

#include "log.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;

// Named, not anonymous: the unity build compiles the internal files together.
namespace transformapply {
//! One event of a track as it is in the score before the change: the data of `transform::Event` plus what is needed to
//! write it somewhere else.
struct Source {
    transform::Event ev;
    Fraction tick;
    std::vector<Note*> heads; // the notes of the first chord, ascending by pitch
    DirectionV stem = DirectionV::AUTO;
    std::vector<Articulation*> articulations; // clones, owned until they are written
};

//! The music of one track the transformation works on.
struct Span {
    track_idx_t track = 0;
    Fraction from;
    Fraction to;
    bool tuplets = false; // tuplets or grace notes: a rhythm operation cannot handle these
    std::vector<Source> sources;
};

constexpr int SCALE[7] = { 0, 2, 4, 5, 7, 9, 11 };

// The spans of the selection: every track of a range, or the tracks of the selected notes.
std::vector<Span> selectedSpans(const Score* score)
{
    std::vector<Span> spans;
    const Selection& sel = score->selection();
    if (sel.isRange()) {
        for (staff_idx_t staff = sel.staffStart(); staff < sel.staffEnd(); ++staff) {
            if (score->staff(staff)->isDrumStaff(sel.tickStart())) {
                continue;
            }
            for (voice_idx_t v = 0; v < VOICES; ++v) {
                Span span;
                span.track = staff2track(staff, v);
                span.from = sel.tickStart();
                span.to = sel.tickEnd();
                spans.push_back(span);
            }
        }
        return spans;
    }
    std::map<track_idx_t, std::pair<Fraction, Fraction> > ranges; // from the first to the end of the last selected chord
    for (Note* note : sel.noteList()) {
        const Chord* chord = note->chord();
        if (note->staff()->isDrumStaff(note->tick()) || chord->isGrace()) {
            continue;
        }
        const auto [it, added] = ranges.try_emplace(chord->track(), chord->tick(), chord->tick() + chord->ticks());
        if (!added) {
            it->second.first = std::min(it->second.first, chord->tick());
            it->second.second = std::max(it->second.second, chord->tick() + chord->ticks());
        }
    }
    for (const auto& [track, range] : ranges) {
        Span span;
        span.track = track;
        span.from = range.first;
        span.to = range.second;
        spans.push_back(span);
    }
    return spans;
}

// Reads the events of the span; `clones` also keeps a copy of each chord's articulations.
void collect(Score* score, Span& span, bool clones)
{
    for (Segment* s = score->tick2segment(span.from, true, SegmentType::ChordRest); s && s->tick() < span.to;
         s = s->next1(SegmentType::ChordRest)) {
        ChordRest* cr = toChordRest(s->element(span.track));
        if (!cr) {
            continue;
        }
        span.tuplets = span.tuplets || cr->tuplet();
        if (cr->isRest()) {
            Source src;
            src.tick = cr->tick();
            src.ev.len = cr->ticks();
            src.ev.id = span.sources.size();
            span.sources.push_back(src);
            continue;
        }
        Chord* chord = toChord(cr);
        span.tuplets = span.tuplets || !chord->graceNotes().empty();
        if (!span.sources.empty() && !span.sources.back().ev.isRest() && chord->notes().front()->tieBack()) {
            span.sources.back().ev.len += chord->ticks(); // the tail of a tied note
            continue;
        }
        Source src;
        src.tick = chord->tick();
        src.ev.len = chord->ticks();
        src.ev.id = span.sources.size();
        src.heads = chord->notes();
        std::sort(src.heads.begin(), src.heads.end(), [](const Note* a, const Note* b) { return a->pitch() < b->pitch(); });
        for (const Note* n : src.heads) {
            src.ev.notes.push_back(n->noteVal());
        }
        src.stem = chord->stemDirection();
        if (clones) {
            for (const Articulation* a : chord->articulations()) {
                src.articulations.push_back(a->clone());
            }
        }
        span.sources.push_back(src);
    }
    if (!span.sources.empty()) {
        span.to = std::max(span.to, span.sources.back().tick + span.sources.back().ev.len);
    }
}

void dropClones(std::vector<Span>& spans)
{
    for (Span& span : spans) {
        for (Source& src : span.sources) {
            for (Articulation* a : src.articulations) {
                delete a;
            }
            src.articulations.clear();
        }
    }
}

// What the inversion mirrors around, from the whole selection. False if there are no notes to take it from.
bool invertContext(const Score* score, const TransformSpec& spec, const std::vector<Span>& spans, transform::InvertContext& ctx)
{
    const Source* first = nullptr;
    const Span* firstSpan = nullptr;
    int lo = 128;
    int hi = -1;
    for (const Span& span : spans) {
        for (const Source& src : span.sources) {
            if (src.ev.isRest()) {
                continue;
            }
            lo = std::min(lo, src.ev.notes.front().pitch);
            hi = std::max(hi, src.ev.notes.back().pitch);
            if (!first || src.tick < first->tick || (src.tick == first->tick && span.track < firstSpan->track)) {
                first = &src;
                firstSpan = &span;
            }
        }
    }
    if (!first) {
        return false;
    }

    // ponytail: one key for the whole selection (the key at its first note), and concert pitch for transposing instruments
    const Key key = score->staff(track2staff(firstSpan->track))->concertKey(first->tick);
    ctx.key = static_cast<int>(key);
    ctx.diatonic = spec.diatonic || spec.centre == InvertCentre::Outermost;

    int pitch = 0;
    switch (spec.centre) {
    case InvertCentre::Outermost:
        ctx.stepSum = transform::stepOf(lo, ctx.key, pitch) + transform::stepOf(hi, ctx.key, pitch);
        ctx.twiceCentre = lo + hi;
        return true;
    case InvertCentre::Pitch:
        pitch = (spec.octave + 1) * 12 + SCALE[spec.letter] + spec.alter;
        ctx.stepSum = 2 * ((spec.octave + 1) * 7 + spec.letter);
        ctx.twiceCentre = 2 * pitch;
        return true;
    case InvertCentre::Root: {
        const int tonic = ((tpc2pitch(key2Tpc(key)) % 12) + 12) % 12;
        const int near = first->ev.notes.back().pitch;
        pitch = tonic + 12 * ((near - tonic + 6 + 120) / 12 - 10); // the tonic nearest the first note
        break;
    }
    case InvertCentre::First:
    case InvertCentre::Top:
        pitch = first->ev.notes.back().pitch;
        break;
    case InvertCentre::Bottom:
        pitch = first->ev.notes.front().pitch;
        break;
    }
    int alter = 0;
    ctx.stepSum = 2 * transform::stepOf(pitch, ctx.key, alter);
    ctx.twiceCentre = 2 * pitch;
    return true;
}

// Fills in the spelling a note value lacks (an inverted pitch), as Note::setNval does.
void spell(const Note* note, NoteVal& nv)
{
    if (nv.tpc1 != Tpc::TPC_INVALID && nv.tpc2 != Tpc::TPC_INVALID) {
        return;
    }
    const Fraction tick = note->tick();
    nv.tpc1 = pitch2tpc(nv.pitch, note->staff()->concertKey(tick), Prefer::NEAREST);
    Interval v = note->staff()->transpose(tick);
    if (v.isZero()) {
        nv.tpc2 = nv.tpc1;
    } else {
        v.flip();
        nv.tpc2 = Transpose::transposeTpc(nv.tpc1, v, true);
    }
}

void removeNote(Score* score, Note* note)
{
    if (Tie* tie = note->tieFor()) {
        score->undoRemoveElement(tie);
    }
    if (Tie* tie = note->tieBack()) {
        score->undoRemoveElement(tie);
    }
    score->undoRemoveElement(note);
}

// Pitches only: the chords stay where they are, their notes get the new pitches.
bool rewritePitches(Score* score, Transaction& tx, const Span& span, const std::vector<transform::Event>& out)
{
    for (size_t i = 0; i < span.sources.size(); ++i) {
        const Source& src = span.sources[i];
        if (src.ev.isRest()) {
            continue;
        }
        const std::vector<NoteVal>& notes = out[i].notes;
        const size_t kept = std::min(src.heads.size(), notes.size());
        for (size_t j = 0; j < kept; ++j) {
            for (Note* note : src.heads[j]->tiedNotes()) {
                NoteVal nv = notes[j];
                spell(note, nv);
                EditNote::undoChangePitch(score, note, nv.pitch, nv.tpc1, nv.tpc2);
            }
        }
        for (size_t j = kept; j < notes.size(); ++j) { // a bigger chord moved here
            NoteVal nv = notes[j];
            spell(src.heads.front(), nv);
            NoteInput::addNoteToTiedChord(tx, score, src.heads.front()->chord(), nv, false, {});
        }
        for (size_t j = kept; j < src.heads.size(); ++j) { // a smaller one
            for (Note* note : src.heads[j]->tiedNotes()) {
                removeNote(score, note);
            }
        }
    }
    return true;
}

// The chords of `track` that start in [from, to), in order.
std::vector<Chord*> chordsBetween(Score* score, track_idx_t track, const Fraction& from, const Fraction& to)
{
    std::vector<Chord*> chords;
    for (Segment* s = score->tick2segment(from, true, SegmentType::ChordRest); s && s->tick() < to;
         s = s->next1(SegmentType::ChordRest)) {
        if (EngravingItem* e = s->element(track); e && e->isChord()) {
            chords.push_back(toChord(e));
        }
    }
    return chords;
}

void tieNotes(Score* score, Note* n1, Note* n2)
{
    if (n1->tieFor()) {
        return;
    }
    Tie* tie = Factory::createTie(n1);
    tie->setStartNote(n1);
    tie->setEndNote(n2);
    tie->setTick(n1->tick());
    tie->setTick2(n2->tick());
    tie->setTrack(n1->track());
    score->undoAddElement(tie);
}

// Rhythms (and pitches with them): the span is written again, event by event, in the new order.
bool rewriteRhythm(Score* score, Span& span, const std::vector<transform::Event>& out)
{
    Fraction tick = span.from;
    for (const transform::Event& e : out) {
        Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
        if (!seg) {
            return false;
        }
        const Fraction end = tick + e.len;
        if (e.isRest()) {
            score->setNoteRest(seg, span.track, NoteVal(), e.len);
            tick = end;
            continue;
        }
        Source& origin = span.sources[e.id];
        score->setNoteRest(seg, span.track, e.notes.front(), e.len, origin.stem);
        const std::vector<Chord*> chain = chordsBetween(score, span.track, tick, end);
        if (chain.empty()) {
            return false;
        }
        for (Chord* chord : chain) {
            for (size_t n = 1; n < e.notes.size(); ++n) {
                Note* note = Factory::createNote(chord);
                note->setTrack(span.track);
                note->setNval(e.notes[n], chord->tick());
                score->undoAddElement(note);
            }
        }
        for (size_t c = 0; c + 1 < chain.size(); ++c) { // setNoteRest tied the first note along the chain
            for (size_t n = 1; n < chain[c]->notes().size() && n < chain[c + 1]->notes().size(); ++n) {
                tieNotes(score, chain[c]->notes()[n], chain[c + 1]->notes()[n]);
            }
        }
        for (Articulation* a : origin.articulations) {
            a->setOwnershipParent(chain.front());
            a->setTrack(span.track);
            score->undoAddElement(a);
        }
        origin.articulations.clear();
        tick = end;
    }
    return true;
}

bool run(const INotationPtr& notation, const TransformSpec& spec)
{
    Score* score = notation->elements()->msScore();
    const bool rhythm = spec.rhythms && spec.op != TransformOp::Invert;

    std::vector<Span> spans = selectedSpans(score);
    for (Span& span : spans) {
        collect(score, span, rhythm);
    }
    spans.erase(std::remove_if(spans.begin(), spans.end(), [](const Span& s) {
        return std::none_of(s.sources.begin(), s.sources.end(), [](const Source& e) { return !e.ev.isRest(); });
    }), spans.end());
    if (spans.empty()) {
        return false;
    }
    if (rhythm && std::any_of(spans.begin(), spans.end(), [](const Span& s) { return s.tuplets; })) {
        LOGW() << "Reversing or rotating rhythms is not supported over tuplets or grace notes";
        dropClones(spans);
        return false;
    }

    transform::InvertContext ctx;
    if (spec.invert && !invertContext(score, spec, spans, ctx)) {
        dropClones(spans);
        return false;
    }
    std::vector<std::vector<transform::Event> > results;
    for (const Span& span : spans) {
        std::vector<transform::Event> events;
        for (const Source& src : span.sources) {
            events.push_back(src.ev);
        }
        results.push_back(transform::apply(events, spec, ctx));
        for (const transform::Event& e : results.back()) {
            for (const NoteVal& n : e.notes) {
                if (n.pitch < 0 || n.pitch > 127) {
                    LOGW() << "The transformation would leave the range of pitches";
                    dropClones(spans);
                    return false;
                }
            }
        }
    }

    const Selection& sel = score->selection();
    const bool wasRange = sel.isRange();
    const Fraction rangeFrom = wasRange ? sel.tickStart() : Fraction();
    const Fraction rangeTo = wasRange ? sel.tickEnd() : Fraction();
    const staff_idx_t staffFrom = wasRange ? sel.staffStart() : track2staff(spans.front().track);
    const staff_idx_t staffTo = wasRange ? sel.staffEnd() : track2staff(spans.back().track) + 1;
    Fraction from = spans.front().from;
    Fraction to = spans.front().to;
    for (const Span& span : spans) {
        from = std::min(from, span.from);
        to = std::max(to, span.to);
    }

    const char* title = spec.op == TransformOp::Invert ? "Invert" : spec.op == TransformOp::Reverse ? "Reverse" : "Rotate";
    const bool ok = transact(notation, muse::TranslatableString("undoableAction", title), [&](Score* sc, Transaction& tx) {
        for (size_t i = 0; i < spans.size(); ++i) {
            if (!(rhythm ? rewriteRhythm(sc, spans[i], results[i]) : rewritePitches(sc, tx, spans[i], results[i]))) {
                return false;
            }
        }
        if (rhythm) { // setNoteRest left a single note selected: take back the range
            sc->deselectAll();
            sc->selection().setRangeTicks(wasRange ? rangeFrom : from, wasRange ? rangeTo : to, staffFrom, staffTo);
        }
        return true;
    });
    dropClones(spans);
    return ok;
}
}

bool mu::dorico::applyTransform(const INotationPtr& notation, const TransformSpec& spec)
{
    return transformapply::run(notation, spec);
}
