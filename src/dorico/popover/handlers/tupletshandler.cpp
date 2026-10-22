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

#include "tupletshandler.h"

#include <algorithm>
#include <vector>

#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/input.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include "engraving/dom/tuplet.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/tupletparser.h"

using namespace mu::dorico;
using namespace mu::engraving;

namespace {
const muse::TranslatableString ADD_TUPLET("undoableAction", "Add tuplet");

// One active level of tuplet input: the entry, its beat unit, and where its current tuplet starts (none yet: -1).
struct TupletLevel {
    TupletEntry entry;
    TDuration unit;
    Fraction tick = Fraction(-1, 1);
    track_idx_t track = 0;
};

// ponytail: one tuplet input for the app, tied to the score it started in; per-notation state if several windows
// ever need tuplet input at once.
struct TupletInput {
    const Score* score = nullptr;
    std::vector<TupletLevel> levels; // outermost first
};

TupletInput& tupletInput()
{
    static TupletInput input;
    return input;
}

TDuration toDuration(const BeatUnit& unit)
{
    TDuration d(unit.type);
    d.setDots(unit.dots);
    return d;
}

// `count` beat units as one note value (a tuplet's written span), or invalid (5 eighths).
TDuration spanOf(const TDuration& unit, int count)
{
    const Fraction f = unit.fraction() * Fraction(count, 1);
    const TDuration d(f);
    return d.isValid() && d.fraction() == f ? d : TDuration();
}

// How many whole tuplets come before `offset` (written time).
int groupIndex(const Fraction& offset, const Fraction& group)
{
    const Fraction g = (offset / group).reduced();
    return g.numerator() / g.denominator();
}

Tuplet* addTupletOn(Score* score, Rest* rest, const TupletEntry& entry)
{
    return score->addTuplet(rest, Fraction(entry.actual, entry.normal),
                            TupletNumberType(score->style().styleI(Sid::tupletNumberType)),
                            TupletBracketType(score->style().styleI(Sid::tupletBracketType)));
}

// Replaces the music from `cr` on with an empty tuplet (rests) inside `cr`'s own tuplet, if any. Null, changing
// nothing, when the tuplet does not fit before the end of the bar or of that tuplet.
Tuplet* makeTupletAt(Score* score, ChordRest* cr, const TupletEntry& entry, const TDuration& unit)
{
    const TDuration span = spanOf(unit, entry.normal);
    Tuplet* outer = cr->tuplet();
    if (!span.isValid()) {
        return nullptr;
    }
    Fraction actual = span.fraction();
    for (const Tuplet* t = outer; t; t = t->tuplet()) {
        actual /= t->ratio();
    }
    const Fraction limit = outer ? outer->endTick() : cr->measure()->endTick();
    if (cr->tick() + actual > limit) {
        return nullptr;
    }

    const Fraction tick = cr->tick();
    const track_idx_t track = cr->track();
    score->makeGap(cr->segment(), track, span.fraction(), outer, false, false);
    return addTupletOn(score, score->addRest(tick, track, span, outer), entry);
}

// The selected notes and rests in score order, grace notes left out.
std::vector<ChordRest*> selectedChordRests(const Score* score)
{
    std::vector<ChordRest*> crs;
    for (ChordRest* cr : score->getSelectedChordRests()) {
        if (!cr->isGrace() && !(cr->isChord() && toChord(cr)->isTrillCueNote())) {
            crs.push_back(cr);
        }
    }
    std::sort(crs.begin(), crs.end(), [](const ChordRest* a, const ChordRest* b) {
        return a->track() != b->track() ? a->track() < b->track() : a->tick() < b->tick();
    });
    return crs;
}

Fraction shortestOf(const std::vector<ChordRest*>& crs)
{
    Fraction shortest = crs.front()->ticks();
    for (const ChordRest* cr : crs) {
        shortest = std::min(shortest, cr->ticks());
    }
    return shortest;
}

// Pre-fill (Dorico: "a suggested ratio based on your selection"): as many notes as the selection holds of its
// shortest value, in the time of the usual count.
std::string tupletSuggestion(mu::notation::INotationPtr notation)
{
    if (notation->interaction()->noteInput()->isNoteInputMode()) {
        return "3"; // Dorico
    }
    const std::vector<ChordRest*> crs = selectedChordRests(notation->elements()->msScore());
    if (crs.empty()) {
        return std::string();
    }
    Fraction written;
    for (const ChordRest* cr : crs) {
        written += cr->ticks();
    }
    const Fraction count = (written / shortestOf(crs)).reduced();
    const int actual = count.denominator() == 1 && count.numerator() > 1 ? count.numerator() : 3;
    return tupletText({ actual, defaultTupletNormal(actual), std::nullopt });
}

// The chord or rest after `cr` in its voice, or null.
ChordRest* nextInTrack(const ChordRest* cr)
{
    for (Segment* s = cr->segment()->next1(SegmentType::ChordRest); s; s = s->next1(SegmentType::ChordRest)) {
        if (EngravingItem* e = s->element(cr->track())) {
            return toChordRest(e);
        }
    }
    return nullptr;
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

// A chord or rest of the wrapped music, kept to be written again. A chord crossing a tuplet's end is cut into pieces
// tied together.
struct SavedChordRest {
    std::vector<NoteVal> notes;                     // empty: a rest (the new tuplets start as rests)
    std::vector<Articulation*> articulations;       // clones, for the first piece
    std::vector<std::pair<size_t, size_t> > ties;   // note index here -> note index in the next item
    DirectionV stem = DirectionV::AUTO;
    std::vector<std::pair<Fraction, Fraction> > pieces; // written offset from the start, written length
    std::vector<Chord*> first, last;                   // per piece, once written
};

// Dorico: the selected notes become tuplets of the entered ratio, as many as they need ("5:4" on five eighths gives
// quintuplet eighths). The last tuplet is filled with the music after the selection (one selected quarter and "3"
// takes the next two quarters too); a note crossing a tuplet's end is split and tied. The time the tuplets free
// becomes rests. ponytail: one voice, no existing tuplets, inside one bar. Notes keep pitches, spelling,
// articulations and ties between them; lyrics and slurs are not carried over.
bool wrapSelection(Score* score, const TupletEntry& entry, mu::notation::INotationPtr notation)
{
    std::vector<ChordRest*> crs = selectedChordRests(score);
    if (crs.empty()) {
        return false;
    }
    const ChordRest* first = crs.front();
    const Measure* measure = first->measure();
    auto usable = [&](const ChordRest* cr) {
        return cr->track() == first->track() && !cr->tuplet() && cr->measure() == measure && !cr->isMeasureRepeat();
    };
    Fraction written;
    for (size_t i = 0; i < crs.size(); ++i) {
        if (!usable(crs[i]) || (i > 0 && crs[i]->tick() != crs[i - 1]->endTick())) {
            return false;
        }
        written += crs[i]->ticks();
    }

    const TDuration unit = entry.unit ? toDuration(*entry.unit) : TDuration(shortestOf(crs));
    const TDuration span = spanOf(unit, entry.normal);
    if (!unit.isValid() || !span.isValid()) {
        return false;
    }
    const Fraction group = unit.fraction() * Fraction(entry.actual, 1); // written time one tuplet holds
    const Fraction groups = (written / group).reduced();
    const int count = (groups.numerator() + groups.denominator() - 1) / groups.denominator();
    const Fraction consumed = group * Fraction(count, 1);
    const Fraction newLen = span.fraction() * Fraction(count, 1);
    while (written < consumed) {
        ChordRest* next = nextInTrack(crs.back());
        if (!next || !usable(next)) {
            return false;
        }
        crs.push_back(next);
        written += next->ticks();
    }
    const Fraction start = first->tick();
    const Fraction leftover = written - consumed; // the end of the last note, written after the tuplets
    const Fraction region = std::max(written, newLen + leftover);
    if (start + region > measure->endTick()) {
        return false;
    }

    std::vector<SavedChordRest> saved(crs.size());
    Fraction offset;
    for (size_t i = 0; i < crs.size(); ++i) {
        SavedChordRest& s = saved[i];
        const ChordRest* cr = crs[i];
        for (Fraction from = offset, end = offset + cr->ticks(); from < end;) {
            Fraction to = from < consumed ? group * Fraction(groupIndex(from, group) + 1, 1) : end;
            to = std::min(to, end);
            s.pieces.emplace_back(from, to - from);
            from = to;
        }
        offset += cr->ticks();
        if (!cr->isChord()) {
            continue;
        }
        const Chord* chord = toChord(cr);
        s.stem = chord->stemDirection();
        const Chord* next = i + 1 < crs.size() && crs[i + 1]->isChord() ? toChord(crs[i + 1]) : nullptr;
        for (size_t n = 0; n < chord->notes().size(); ++n) {
            const Note* note = chord->notes()[n];
            s.notes.push_back(note->noteVal());
            const Tie* tie = note->tieFor();
            if (next && tie && tie->endNote() && tie->endNote()->chord() == next) {
                const auto& nextNotes = next->notes();
                s.ties.emplace_back(n, std::find(nextNotes.begin(), nextNotes.end(), tie->endNote()) - nextNotes.begin());
            }
        }
        for (const Articulation* a : chord->articulations()) {
            s.articulations.push_back(a->clone());
        }
    }
    auto dropClones = [&saved]() {
        for (SavedChordRest& s : saved) {
            for (Articulation* a : s.articulations) {
                delete a;
            }
            s.articulations.clear();
        }
    };

    const track_idx_t track = first->track();
    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(ADD_TUPLET);
    score->makeGap(first->segment(), track, region, nullptr, false, false);
    for (int i = 0; i < count; ++i) {
        if (!addTupletOn(score, score->addRest(start + span.fraction() * Fraction(i, 1), track, span, nullptr), entry)) {
            undoStack->rollbackChanges();
            dropClones();
            return false;
        }
    }
    if (region > newLen) {
        score->setRests(start + newLen, track, region - newLen, true, nullptr);
    }

    // Write the notes back: squeezed into the tuplets, or after them (the leftover).
    const Fraction squeeze(entry.normal, entry.actual);
    std::vector<EngravingItem*> select;
    for (SavedChordRest& s : saved) {
        for (const auto& [pieceOffset, len] : s.pieces) {
            Fraction tick, end;
            if (pieceOffset < consumed) {
                const int index = groupIndex(pieceOffset, group);
                tick = start + span.fraction() * Fraction(index, 1) + (pieceOffset - group * Fraction(index, 1)) * squeeze;
                end = tick + len * squeeze;
            } else {
                tick = start + newLen + pieceOffset - consumed;
                end = tick + len;
            }
            Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
            if (!seg || !seg->element(track)) {
                undoStack->rollbackChanges(); // ponytail: a note off the tuplet's grid; refuse rather than lose it
                dropClones();
                return false;
            }
            if (s.notes.empty()) {
                if (pieceOffset < consumed) {
                    select.push_back(seg->element(track));
                }
                continue;
            }
            score->setNoteRest(seg, track, s.notes.front(), len, s.stem);
            const std::vector<Chord*> chain = chordsBetween(score, track, tick, end);
            if (chain.empty()) {
                undoStack->rollbackChanges();
                dropClones();
                return false;
            }
            for (Chord* chord : chain) {
                for (size_t n = 1; n < s.notes.size(); ++n) {
                    Note* note = Factory::createNote(chord);
                    note->setTrack(track);
                    note->setNval(s.notes[n], chord->tick());
                    score->undoAddElement(note);
                }
            }
            for (size_t c = 0; c + 1 < chain.size(); ++c) { // setNoteRest tied the first note along the chain
                for (size_t n = 1; n < chain[c]->notes().size() && n < chain[c + 1]->notes().size(); ++n) {
                    tieNotes(score, chain[c]->notes()[n], chain[c + 1]->notes()[n]);
                }
            }
            s.first.push_back(chain.front());
            s.last.push_back(chain.back());
            if (pieceOffset < consumed) {
                select.insert(select.end(), chain.begin(), chain.end());
            }
        }
        if (!s.first.empty()) {
            for (Articulation* a : s.articulations) {
                a->setOwnershipParent(s.first.front());
                a->setTrack(track);
                score->undoAddElement(a);
            }
            s.articulations.clear();
        }
    }

    // Ties: between the pieces of one note, and the original ones between notes.
    auto tieChords = [score](Chord* a, Chord* b, size_t from, size_t to) {
        if (a && b && from < a->notes().size() && to < b->notes().size()) {
            tieNotes(score, a->notes()[from], b->notes()[to]);
        }
    };
    for (size_t i = 0; i < saved.size(); ++i) {
        const SavedChordRest& s = saved[i];
        for (size_t p = 0; p + 1 < s.first.size(); ++p) {
            for (size_t n = 0; n < s.notes.size(); ++n) {
                tieChords(s.last[p], s.first[p + 1], n, n);
            }
        }
        if (i + 1 < saved.size() && !s.last.empty() && !saved[i + 1].first.empty()) {
            for (const auto& [from, to] : s.ties) {
                tieChords(s.last.back(), saved[i + 1].first.front(), from, to);
            }
        }
    }
    dropClones();
    undoStack->commitChanges();
    notation->interaction()->select(select, SelectType::RANGE);
    return true;
}
}

PopoverDef mu::dorico::tupletsPopover()
{
    PopoverDef def;
    def.id = "tuplets";
    def.title = "Tuplets";
    def.glyph = "\uE1F0\uE1F7\uE1FF\uE1F0\uE1F7\uE1F0";
    def.validate = [](const std::string& text) { return parseTuplet(text).status; };
    def.prefill = [](const EngravingItem*) { return std::string(); };
    def.selectionPrefill = tupletSuggestion;
    def.noteInputEnded = [](mu::notation::INotationPtr notation) { stopTupletInput(notation, true); };

    def.apply = [](const std::string& text, ApplyScope, mu::notation::INotationPtr notation) {
        const TupletParse parsed = parseTuplet(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        Score* score = notation->elements()->msScore();
        if (!notation->interaction()->noteInput()->isNoteInputMode()) {
            return wrapSelection(score, parsed.entry, notation);
        }

        // Note input: an empty tuplet at the caret, then a new one with each note after it is full (startTupletAtCaret).
        // Its beat unit becomes the input duration, as in Dorico; notes of other values can still go inside.
        InputState& is = score->inputState();
        const TDuration unit = parsed.entry.unit ? toDuration(*parsed.entry.unit) : is.duration();
        if (!unit.isValid() || unit.isMeasure() || !spanOf(unit, parsed.entry.normal).isValid()) {
            return false;
        }
        TupletInput& input = tupletInput();
        if (input.score != score) {
            input = { score, {} };
        }
        input.levels.push_back({ parsed.entry, unit });
        const size_t levels = input.levels.size();
        is.setDuration(unit);
        startTupletAtCaret(notation);
        return input.levels.size() == levels; // dropped: it does not fit before the barline
    };
    return def;
}

bool mu::dorico::startTupletAtCaret(mu::notation::INotationPtr notation)
{
    TupletInput& input = tupletInput();
    Score* score = notation ? notation->elements()->msScore() : nullptr;
    if (!score || input.score != score || input.levels.empty()) {
        return false;
    }
    InputState& is = score->inputState();
    ChordRest* cr = is.cr();
    if (!cr || is.beyondScore()) {
        return false;
    }

    const TDuration duration = is.duration();
    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    bool opened = false;
    bool changed = false;
    for (size_t i = 0; i < input.levels.size(); ++i) {
        TupletLevel& level = input.levels[i];
        const Fraction ratio(level.entry.actual, level.entry.normal);
        bool inside = false;
        for (const Tuplet* t = cr->tuplet(); t && !inside; t = t->tuplet()) {
            inside = t->tick() == level.tick && t->track() == level.track && t->ratio() == ratio && t->baseLen() == level.unit;
        }
        if (inside) {
            continue;
        }
        if (!opened) {
            undoStack->prepareChanges(ADD_TUPLET);
            opened = true;
        }
        Tuplet* tuplet = makeTupletAt(score, cr, level.entry, level.unit);
        if (!tuplet) {
            // Dorico: an inner tuplet that no longer fits in the outer one stops; the outer one goes on.
            // ponytail: an outermost tuplet that does not fit before the barline stops too.
            input.levels.resize(i);
            break;
        }
        changed = true;
        level.tick = tuplet->tick();
        level.track = tuplet->track();
        cr = toChordRest(tuplet->elements().front());
    }
    if (!changed) {
        if (opened) {
            undoStack->rollbackChanges();
        }
        return false;
    }
    undoStack->commitChanges();
    // Set after the commit: ending the command may move the input position to the selection.
    is.setSegment(cr->segment());
    is.setDuration(duration); // addTuplet sets the input duration to the beat unit; keep the one chosen
    return true;
}

void mu::dorico::stopTupletInput(mu::notation::INotationPtr notation, bool all)
{
    TupletInput& input = tupletInput();
    if (all || input.levels.empty()) {
        input.levels.clear();
        return;
    }
    const TupletLevel level = input.levels.back();
    input.levels.pop_back();

    // `:` returns to normal notes: the caret leaves the tuplet it is in, whose unfilled slots stay rests.
    Score* score = notation ? notation->elements()->msScore() : nullptr;
    if (!score || score != input.score || !score->inputState().cr()) {
        return;
    }
    InputState& is = score->inputState();
    for (const Tuplet* t = is.cr()->tuplet(); t; t = t->tuplet()) {
        if (t->tick() == level.tick && t->track() == level.track && t->baseLen() == level.unit) {
            const DurationElement* last = t->elements().back();
            while (last->isTuplet()) {
                last = toTuplet(last)->elements().back();
            }
            if (ChordRest* next = nextInTrack(toChordRest(last))) {
                is.setSegment(next->segment());
                notation->interaction()->noteInput()->setCurrentTrack(is.track()); // no-op that redraws the caret
            }
            break;
        }
    }
}
