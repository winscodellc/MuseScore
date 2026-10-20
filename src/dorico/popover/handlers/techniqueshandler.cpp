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

#include "techniqueshandler.h"

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/fingering.h"
#include "engraving/dom/harmonicmark.h"
#include "engraving/dom/input.h"
#include "engraving/dom/note.h"
#include "engraving/dom/pedal.h"
#include "engraving/dom/playtechannotation.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/textline.h"
#include "engraving/editing/editchord.h"
#include "engraving/types/symnames.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/techniquesparser.h"
#include "pendingatcaret.h"

using namespace mu::dorico;
using namespace mu::engraving;
using muse::String;
using PedalAction = TechniquesEntry::Pedal;

namespace {
const muse::TranslatableString ADD_TECHNIQUE("undoableAction", "Add playing technique");

// Dorico: techniques are voice-specific, above up-stem voices and below down-stem ones (voices 2 and 4).
PlacementV techniquePlacement(track_idx_t track)
{
    return track & 1 ? PlacementV::BELOW : PlacementV::ABOVE;
}

Fraction techniqueEnd(const ChordRest* cr)
{
    return cr->isChord() ? toChord(cr)->endTickIncludingTied() : cr->endTick();
}

String techniqueText(const TechniqueItem& item)
{
    const Technique& t = *item.technique;
    const String text = t.text.empty() ? u"<sym>" + String::fromAscii(SymNames::nameForSymId(t.sym).ascii()) + u"</sym>"
                        : String::fromStdString(t.text);
    return item.parenthesised ? u"(" + text + u")" : text;
}

// An item this popover makes. Selected on its own, a new entry replaces it (Return-to-edit).
bool isTechniqueItem(const EngravingItem* e)
{
    return e->isPlayTechAnnotation() || e->isPedalSegment();
}

// --- Adding one item (inside an open transaction) ---

EngravingItem* addAnnotation(Score* score, const TechniqueItem& item, const Fraction& tick, track_idx_t track)
{
    Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
    if (!seg) {
        return nullptr;
    }
    PlayTechAnnotation* a = Factory::createPlayTechAnnotation(seg, item.technique->type, TextStyleType::STAFF);
    a->setXmlText(techniqueText(item));
    a->setTrack(track);
    a->setVisible(!item.hidden); // a hidden technique still plays
    score->undoAddElement(a);
    // As the Properties panel does: unstyled, with the offset for that side. setPlacement() alone is reset by the style.
    a->undoChangeProperty(Pid::PLACEMENT, techniquePlacement(track));
    return a;
}

// Bowing, o, +: a symbol on the chord; the same one again removes it (toggle). Null when removed or on a rest.
EngravingItem* addTechniqueSymbol(Score* score, ChordRest* cr, const TechniqueItem& item)
{
    if (!cr->isChord()) {
        return nullptr;
    }
    Articulation* a = Factory::createArticulation(score->dummy());
    a->setSymId(item.technique->sym);
    a->setVisible(!item.hidden);
    if (!EditChord::toggleArticulation(score, cr, a)) {
        delete a;
        return nullptr;
    }
    return a;
}

EngravingItem* addTechnique(Score* score, const TechniqueItem& item, ChordRest* cr, const Fraction& tick, track_idx_t track)
{
    if (item.technique->sym != SymId::noSym) {
        return cr ? addTechniqueSymbol(score, cr, item) : nullptr;
    }
    return addAnnotation(score, item, tick, track);
}

// vibrato->: the technique with a duration line, a solid line with a hook at the end (Dorico's default).
// ponytail: the line is a TextLine, so a technique with duration does not change playback.
EngravingItem* addDurationLine(Score* score, const TechniqueItem& item, const Fraction& from, const Fraction& to,
                               track_idx_t track)
{
    if (to <= from) {
        return nullptr;
    }
    const String text = techniqueText(item);
    const bool below = techniquePlacement(track) == PlacementV::BELOW;
    TextLine* l = Factory::createTextLine(score->dummy());
    l->setBeginText(text);
    l->setContinueText(u"(" + text + u")");
    l->setEndHookType(HookType::HOOK_90);
    l->setEndHookHeight(Spatium(below ? -1.5 : 1.5));
    l->setVisible(!item.hidden);
    l->setTrack(track);
    l->setTrack2(track);
    l->setTick(from);
    l->setTick2(to);
    score->undoAddElement(l);
    l->undoChangeProperty(Pid::PLACEMENT, techniquePlacement(track), PropertyFlags::UNSTYLED);
    return l;
}

// A sustain pedal line: "Ped." and a line ending in a hook. A retake joins two lines with 45° hooks.
EngravingItem* addPedal(Score* score, const Fraction& from, const Fraction& to, staff_idx_t staff, bool retakeStart,
                        bool retakeEnd)
{
    if (to <= from) {
        return nullptr;
    }
    mu::engraving::Pedal* p = Factory::createPedal(score->dummy());
    p->setBeginHookType(retakeStart ? HookType::HOOK_45 : HookType::NONE);
    p->setEndHookType(retakeEnd ? HookType::HOOK_45 : HookType::HOOK_90);
    if (!retakeStart) {
        p->setBeginText(p->propertyDefault(Pid::BEGIN_TEXT).value<String>());
    }
    p->setContinueText(p->propertyDefault(Pid::CONTINUE_TEXT).value<String>());
    p->setEndText(p->propertyDefault(Pid::END_TEXT).value<String>());
    p->setTrack(staff * VOICES);
    p->setTrack2(staff * VOICES);
    p->setTick(from);
    p->setTick2(to);
    score->undoAddElement(p);
    return p;
}

std::vector<mu::engraving::Pedal*> pedalsAt(Score* score, const Fraction& tick, staff_idx_t staff)
{
    std::vector<mu::engraving::Pedal*> result;
    for (const auto& i : score->spannerMap().findOverlapping(tick.ticks(), tick.ticks())) {
        if (i.value->isPedal() && i.value->staffIdx() == staff) {
            result.push_back(toPedal(i.value));
        }
    }
    return result;
}

// ^ on a note under a pedal line splits the line there with a retake.
bool addRetake(Score* score, const Fraction& tick, staff_idx_t staff)
{
    for (mu::engraving::Pedal* p : pedalsAt(score, tick, staff)) {
        if (p->tick() < tick && tick < p->tick2()) {
            const bool retakeStart = p->beginHookType() == HookType::HOOK_45;
            const bool retakeEnd = p->endHookType() == HookType::HOOK_45;
            const Fraction from = p->tick();
            const Fraction to = p->tick2();
            score->undoRemoveElement(p);
            addPedal(score, from, tick, staff, retakeStart, true);
            addPedal(score, tick, to, staff, true, retakeEnd);
            return true;
        }
    }
    return false;
}

// nonotch on the note of a retake joins the two lines again.
bool removeRetake(Score* score, const Fraction& tick, staff_idx_t staff)
{
    mu::engraving::Pedal* before = nullptr;
    mu::engraving::Pedal* after = nullptr;
    for (mu::engraving::Pedal* p : pedalsAt(score, tick, staff)) {
        if (p->tick2() == tick && p->endHookType() == HookType::HOOK_45) {
            before = p;
        } else if (p->tick() == tick && p->beginHookType() == HookType::HOOK_45) {
            after = p;
        }
    }
    if (!before || !after) {
        return false;
    }
    const bool retakeStart = before->beginHookType() == HookType::HOOK_45;
    const bool retakeEnd = after->endHookType() == HookType::HOOK_45;
    const Fraction from = before->tick();
    const Fraction to = after->tick2();
    score->undoRemoveElement(before);
    score->undoRemoveElement(after);
    addPedal(score, from, to, staff, retakeStart, retakeEnd);
    return true;
}

// --- Harmonics and string numbers (spec 61): on the selected notes, or the last input note, which is selected ---

std::vector<Note*> selectedNotes(const std::vector<EngravingItem*>& items)
{
    std::vector<Note*> notes;
    for (EngravingItem* e : items) {
        if (e->isNote() && std::find(notes.begin(), notes.end(), toNote(e)) == notes.end()) {
            notes.push_back(toNote(e));
        }
    }
    return notes;
}

Articulation* harmonicCircle(const Chord* chord)
{
    for (Articulation* a : chord->articulations()) {
        if (a->symId() == SymId::stringsHarmonic) {
            return a;
        }
    }
    return nullptr;
}

// A natural harmonic: a diamond head, or `circle`, the o above the staff.
void setNaturalHarmonic(Score* score, Note* note, bool circle)
{
    if (!circle) {
        note->undoChangeProperty(Pid::HEAD_GROUP, NoteHeadGroup::HEAD_DIAMOND);
    } else if (note->headGroup() == NoteHeadGroup::HEAD_DIAMOND) {
        note->undoChangeProperty(Pid::HEAD_GROUP, NoteHeadGroup::HEAD_NORMAL);
    }
    Chord* chord = note->chord();
    if (circle != (harmonicCircle(chord) != nullptr)) {
        Articulation* a = Factory::createArticulation(score->dummy());
        a->setSymId(SymId::stringsHarmonic);
        if (!EditChord::toggleArticulation(score, chord, a)) {
            delete a;
        }
    }
}

std::vector<HarmonicMark*> harmonicMarksIn(Score* score, const Fraction& from, const Fraction& to, staff_idx_t staff)
{
    std::vector<HarmonicMark*> marks;
    for (const auto& i : score->spannerMap().findOverlapping(from.ticks(), to.ticks() - 1)) {
        if (i.value->isHarmonicMark() && i.value->staffIdx() == staff) {
            marks.push_back(toHarmonicMark(i.value));
        }
    }
    return marks;
}

// noharm: the harmonic look and the marks over the notes' span go.
void removeHarmonics(Score* score, const std::vector<Note*>& notes)
{
    std::map<staff_idx_t, std::pair<Fraction, Fraction> > spans;
    for (Note* n : notes) {
        if (n->headGroup() == NoteHeadGroup::HEAD_DIAMOND) {
            n->undoChangeProperty(Pid::HEAD_GROUP, NoteHeadGroup::HEAD_NORMAL);
        }
        if (Articulation* o = harmonicCircle(n->chord())) {
            score->undoRemoveElement(o);
        }
        auto& span = spans.try_emplace(n->staffIdx(), n->tick(), n->tick()).first->second;
        span = { std::min(span.first, n->tick()), std::max(span.second, techniqueEnd(n->chord())) };
    }
    for (const auto& [staff, span] : spans) {
        for (HarmonicMark* h : harmonicMarksIn(score, span.first, span.second, staff)) {
            score->undoRemoveElement(h);
        }
    }
}

// An artificial or pinch harmonic: AH / PH over the notes of each staff, replacing the marks there.
std::vector<EngravingItem*> addHarmonicMarks(Score* score, const std::vector<Note*>& notes, bool pinch)
{
    std::map<staff_idx_t, Chord*> firsts;
    std::map<staff_idx_t, Fraction> ends;
    for (Note* n : notes) {
        Chord*& first = firsts[n->staffIdx()];
        if (!first || n->tick() < first->tick()) {
            first = n->chord();
        }
        Fraction& end = ends[n->staffIdx()];
        end = std::max(end, techniqueEnd(n->chord()));
    }
    std::vector<EngravingItem*> added;
    for (const auto& [staff, first] : firsts) {
        for (HarmonicMark* old : harmonicMarksIn(score, first->tick(), ends[staff], staff)) {
            score->undoRemoveElement(old);
        }
        HarmonicMark* h = Factory::createHarmonicMark(score->dummy());
        h->setTrack(first->track());
        h->setTrack2(first->track());
        h->setTick(first->tick());
        h->setTick2(ends[staff]);
        const String text = pinch ? u"PH" : u"AH";
        h->setBeginText(text);
        h->setContinueText(text);
        score->undoAddElement(h);
        added.push_back(h);
    }
    return added;
}

// string 3: a circled number on the note; it replaces the string number the note has.
EngravingItem* addStringNumber(Score* score, Note* note, int number)
{
    const ElementList elements = note->el(); // a copy: removing changes the list
    for (EngravingItem* e : elements) {
        if (e->isFingering() && toFingering(e)->textStyleType() == TextStyleType::STRING_NUMBER) {
            score->undoRemoveElement(e);
        }
    }
    Fingering* f = Factory::createFingering(note, TextStyleType::STRING_NUMBER);
    f->setTrack(note->track());
    f->setPlainText(String::number(number));
    score->undoAddElement(f);
    return f;
}

bool applyNoteLevel(const TechniquesEntry& entry, Score* score, mu::notation::INotationPtr notation)
{
    const std::vector<Note*> notes = selectedNotes(score->selection().elements());
    if (notes.empty()) {
        return false;
    }
    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(ADD_TECHNIQUE);
    std::vector<EngravingItem*> added;
    if (entry.stringNumber) {
        // The top note of each selected chord (a single note when only one is selected).
        std::vector<Chord*> chords;
        for (Note* n : notes) {
            if (std::find(chords.begin(), chords.end(), n->chord()) == chords.end()) {
                chords.push_back(n->chord());
                added.push_back(addStringNumber(score, notes.size() == 1 ? n : n->chord()->upNote(), entry.stringNumber));
            }
        }
    } else {
        switch (entry.harmonic->kind) {
        case HarmonicEntry::Kind::Natural:
            for (Note* n : notes) {
                setNaturalHarmonic(score, n, entry.harmonic->circle);
            }
            break;
        case HarmonicEntry::Kind::Artificial:
        case HarmonicEntry::Kind::Pinch:
            added = addHarmonicMarks(score, notes, entry.harmonic->kind == HarmonicEntry::Kind::Pinch);
            break;
        case HarmonicEntry::Kind::Remove:
            removeHarmonics(score, notes);
            break;
        }
    }
    undoStack->commitChanges();
    return true;
}

// --- Selection ---

// Where an entry goes on one staff: the selected chords/rests in order, from start to end.
struct TechniqueSpot {
    track_idx_t track = 0;
    Fraction start;
    Fraction end;
    std::vector<ChordRest*> crs;
};

void addSpotCr(TechniqueSpot& spot, ChordRest* cr)
{
    if (std::none_of(spot.crs.begin(), spot.crs.end(), [&](const ChordRest* c) { return c->tick() == cr->tick(); })) {
        spot.crs.push_back(cr);
    }
}

std::map<staff_idx_t, TechniqueSpot> selectionSpots(Score* score, const std::vector<EngravingItem*>& items)
{
    std::map<staff_idx_t, TechniqueSpot> spots;
    const Selection& sel = score->selection();
    if (sel.isRange()) {
        for (staff_idx_t staff = sel.staffStart(); staff < sel.staffEnd(); ++staff) {
            TechniqueSpot& spot = spots[staff];
            spot = { staff * VOICES, sel.tickStart(), sel.tickEnd(), {} };
            for (Segment* s = score->tick2segment(sel.tickStart(), true, SegmentType::ChordRest);
                 s && s->tick() < sel.tickEnd(); s = s->next1(SegmentType::ChordRest)) {
                if (s->element(spot.track) && s->element(spot.track)->isChordRest()) {
                    addSpotCr(spot, toChordRest(s->element(spot.track)));
                }
            }
        }
        return spots;
    }

    for (EngravingItem* e : items) {
        Fraction start = e->tick();
        Fraction end = e->tick();
        ChordRest* cr = nullptr;
        if (e->isSpannerSegment()) {
            start = toSpannerSegment(e)->spanner()->tick();
            end = toSpannerSegment(e)->spanner()->tick2();
        } else if (e->isNote() || e->isChordRest()) {
            cr = e->isNote() ? toNote(e)->chord() : toChordRest(e);
        } else if (e->isArticulation()) {
            cr = toArticulation(e)->chordRest();
        } else {
            cr = score->findCR(e->tick(), e->track()); // an annotation: the chord/rest at its position
            if (cr && cr->tick() != e->tick()) {
                cr = nullptr;
            }
        }
        if (cr) {
            end = techniqueEnd(cr);
        }

        const bool first = !spots.count(e->staffIdx());
        TechniqueSpot& spot = spots[e->staffIdx()];
        if (first || start < spot.start) {
            spot.start = start;
            spot.track = cr ? cr->track() : e->track();
        }
        spot.end = first ? end : std::max(spot.end, end);
        if (cr) {
            addSpotCr(spot, cr);
        }
    }
    for (auto& [staff, spot] : spots) {
        std::sort(spot.crs.begin(), spot.crs.end(), [](const ChordRest* a, const ChordRest* b) { return a->tick() < b->tick(); });
    }
    return spots;
}

// The chord/rest after cr in its voice.
ChordRest* nextTechniqueCr(const ChordRest* cr)
{
    Segment* next = cr->segment()->next1(SegmentType::ChordRest);
    return next ? next->nextChordRest(cr->track()) : nullptr;
}

std::vector<EngravingItem*> applyToSpot(const TechniquesEntry& entry, Score* score, const TechniqueSpot& spot, bool& changed)
{
    std::vector<EngravingItem*> added;
    const staff_idx_t staff = track2staff(spot.track);
    switch (entry.pedal) {
    case PedalAction::Start:
        added.push_back(addPedal(score, spot.start, spot.end, staff, false, false));
        return added;
    case PedalAction::Retake:
    case PedalAction::RemoveRetake:
        for (const ChordRest* cr : spot.crs) {
            changed = (entry.pedal == PedalAction::Retake ? addRetake(score, cr->tick(), staff)
                       : removeRetake(score, cr->tick(), staff)) || changed;
        }
        return added;
    case PedalAction::Stop: // only during note input
    case PedalAction::None:
        break;
    }

    if (entry.withDuration) {
        added.push_back(addDurationLine(score, entry.items.front(), spot.start, spot.end, spot.track));
        return added;
    }
    // One technique at the start; A->B puts each one on the next selected note, or the next note in the voice.
    ChordRest* cr = spot.crs.empty() ? nullptr : spot.crs.front();
    for (size_t i = 0; i < entry.items.size(); ++i) {
        if (i > 0) {
            cr = i < spot.crs.size() ? spot.crs[i] : cr ? nextTechniqueCr(cr) : nullptr;
            if (!cr) {
                break;
            }
        }
        const Fraction tick = cr ? cr->tick() : spot.start;
        const track_idx_t track = cr ? cr->track() : spot.track;
        added.push_back(addTechnique(score, entry.items[i], cr, tick, track));
        changed = changed || entry.items[i].technique->sym != SymId::noSym; // a symbol entered again was toggled off
    }
    return added;
}

bool applyTechniqueSelection(const TechniquesEntry& entry, Score* score, mu::notation::INotationPtr notation)
{
    if (entry.stop || entry.pedal == PedalAction::Stop) {
        return false; // only during note input
    }
    const Selection& sel = score->selection();
    const std::vector<EngravingItem*> items = sel.elements(); // a copy: removing items changes the selection
    const std::map<staff_idx_t, TechniqueSpot> spots = selectionSpots(score, items);

    // Return on a selected technique reopened the popover with only that item selected: the entry replaces it.
    std::vector<EngravingItem*> edited;
    const bool replaces = !entry.items.empty() || entry.pedal == PedalAction::Start;
    if (!sel.isRange() && replaces) {
        std::copy_if(items.begin(), items.end(), std::back_inserter(edited), isTechniqueItem);
    }

    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(ADD_TECHNIQUE);
    for (EngravingItem* e : edited) {
        score->undoRemoveElement(e->isSpannerSegment() ? toSpannerSegment(e)->spanner() : e);
    }
    bool changed = !edited.empty();
    std::vector<EngravingItem*> added;
    for (const auto& [staff, spot] : spots) {
        for (EngravingItem* a : applyToSpot(entry, score, spot, changed)) {
            added.push_back(a);
        }
    }
    std::erase(added, nullptr);
    if (!changed && added.empty()) {
        undoStack->rollbackChanges();
        return false;
    }
    undoStack->commitChanges();

    // The new item is selected, so Return reopens it for editing.
    if (!added.empty()) {
        EngravingItem* first = added.front();
        if (first->isSpanner()) {
            first = toSpanner(first)->frontSegment();
        }
        if (first) {
            notation->interaction()->select({ first });
        }
    }
    return true;
}

// --- Note input ---

// An open-ended line entered at the caret (vibrato->, ped): created when stopped (?, *, a new technique, the end of
// note input), like p< in the dynamics popover. ponytail: the line appears when stopped, it does not grow while notes
// are entered.
struct OpenTechniqueLine {
    bool active = false;
    const Score* score = nullptr;
    Fraction start;
    track_idx_t track = 0;
    TechniqueItem item;       // the technique; unused for a pedal
    bool retakeStart = false; // a pedal continuing after a retake
};

struct OpenTechniqueLines {
    OpenTechniqueLine technique;
    OpenTechniqueLine pedal;
    std::shared_ptr<PendingAtCaret> symbols = std::make_shared<PendingAtCaret>(ADD_TECHNIQUE);
};

bool finishTechniqueLine(OpenTechniqueLine& l, Score* score, const Fraction& end)
{
    const bool add = l.active && l.score == score;
    l.active = false;
    return add && addDurationLine(score, l.item, l.start, end, l.track);
}

bool finishPedalLine(OpenTechniqueLine& l, Score* score, const Fraction& end, bool retakeEnd)
{
    const bool add = l.active && l.score == score;
    l.active = false;
    return add && addPedal(score, l.start, end, track2staff(l.track), l.retakeStart, retakeEnd);
}

bool applyTechniqueCaret(const TechniquesEntry& entry, Score* score, OpenTechniqueLines& open, mu::notation::INotationPtr notation)
{
    const InputState& is = score->inputState();
    if (!is.segment() || is.beyondScore() || entry.items.size() > 1 || entry.pedal == PedalAction::RemoveRetake) {
        return false; // Dorico: one technique per entry during note input
    }
    const Fraction caret = is.tick();
    const track_idx_t track = is.track();

    // A technique with a symbol waits for the note at the caret.
    if (entry.items.size() == 1 && !entry.withDuration && entry.items.front().technique->sym != SymId::noSym) {
        const TechniqueItem item = entry.items.front();
        open.symbols->add([item](Score* s, ChordRest* cr, const Fraction&) { return addTechniqueSymbol(s, cr, item); },
                          notation);
        return true;
    }

    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(ADD_TECHNIQUE);
    bool changed = false;
    switch (entry.pedal) {
    case PedalAction::Start:
        changed = finishPedalLine(open.pedal, score, caret, false);
        open.pedal = { true, score, caret, track, {}, false };
        break;
    case PedalAction::Stop:
        changed = finishPedalLine(open.pedal, score, caret, false);
        break;
    case PedalAction::Retake:
        if (open.pedal.active) {
            changed = finishPedalLine(open.pedal, score, caret, true);
            open.pedal = { true, score, caret, track, {}, true };
        }
        break;
    case PedalAction::RemoveRetake:
    case PedalAction::None:
        break;
    }
    if (entry.stop || !entry.items.empty()) { // a new technique ends the open one
        changed = finishTechniqueLine(open.technique, score, caret) || changed;
    }
    if (entry.withDuration) {
        open.technique = { true, score, caret, track, entry.items.front(), false };
    } else if (!entry.items.empty()) {
        changed = addAnnotation(score, entry.items.front(), caret, track) || changed;
    }
    if (changed) {
        undoStack->commitChanges();
    } else {
        undoStack->rollbackChanges();
    }
    return true;
}

std::string techniquePrefill(const EngravingItem* item)
{
    if (item->isPedalSegment()) {
        return "ped";
    }
    if (!item->isPlayTechAnnotation()) {
        return std::string();
    }
    const std::string entry = techniqueEntryForText(toPlayTechAnnotation(item)->plainText().toStdString());
    return item->visible() || entry.empty() || entry.front() == '(' ? entry : "[" + entry + "]";
}
}

PopoverDef mu::dorico::techniquesPopover()
{
    auto open = std::make_shared<OpenTechniqueLines>();

    PopoverDef def;
    def.id = "techniques";
    def.title = "Playing Techniques";
    def.icon = muse::ui::IconCode::Code::ARTICULATION;
    def.supportsLocal = false; // Dorico: Return only
    def.itemTypes = { ElementType::PLAYTECH_ANNOTATION, ElementType::PEDAL_SEGMENT };
    def.editsAttached = false; // Dorico stacks techniques: Shift-P on a note with pizz. adds another one
    def.validate = [](const std::string& text) { return parseTechniques(text).status; };
    def.prefill = techniquePrefill;
    def.suggest = techniqueSuggestions;

    def.apply = [open](const std::string& text, ApplyScope, mu::notation::INotationPtr notation) {
        const TechniquesParse parsed = parseTechniques(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        Score* score = notation->elements()->msScore();
        if (parsed.entry.harmonic || parsed.entry.stringNumber) {
            return applyNoteLevel(parsed.entry, score, notation);
        }
        if (notation->interaction()->noteInput()->isNoteInputMode()) {
            return applyTechniqueCaret(parsed.entry, score, *open, notation);
        }
        return applyTechniqueSelection(parsed.entry, score, notation);
    };

    def.noteInputEnded = [open](mu::notation::INotationPtr notation) {
        open->symbols->cancel();
        if (!open->technique.active && !open->pedal.active) {
            return;
        }
        Score* score = notation->elements()->msScore();
        const Fraction caret = score->inputState().tick();
        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(ADD_TECHNIQUE);
        const bool line = finishTechniqueLine(open->technique, score, caret);
        if (finishPedalLine(open->pedal, score, caret, false) || line) {
            undoStack->commitChanges();
        } else {
            undoStack->rollbackChanges();
        }
    };
    return def;
}
