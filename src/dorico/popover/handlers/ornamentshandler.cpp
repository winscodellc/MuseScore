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

#include "ornamentshandler.h"

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "engraving/dom/arpeggio.h"
#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/chordline.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/glissando.h"
#include "engraving/dom/guitarbend.h"
#include "engraving/dom/hammeronpulloff.h"
#include "engraving/dom/note.h"
#include "engraving/dom/ornament.h"
#include "engraving/dom/score.h"
#include "engraving/dom/select.h"
#include "engraving/dom/tapping.h"
#include "engraving/dom/trill.h"
#include "engraving/dom/whammybar.h"
#include "engraving/editing/editchord.h"
#include "engraving/editing/editdata.h"
#include "engraving/editing/transaction/transaction.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/ornamentsparser.h"
#include "pendingatcaret.h"

using namespace mu::dorico;
using namespace mu::engraving;
using OKind = OrnamentEntry::Kind;
using GTech = OrnamentEntry::GuitarTech;

namespace {
const muse::TranslatableString ADD_ORNAMENT("undoableAction", "Add ornament");

bool isJazzOrnament(const EngravingItem* e)
{
    if (!e->isArticulation()) {
        return false;
    }
    const SymId sym = toArticulation(e)->symId();
    return sym == SymId::brassBend || sym == SymId::brassFlip || sym == SymId::brassJazzTurn || sym == SymId::brassSmear;
}

// An item this popover makes. Selected on its own, a new entry replaces it (Return-to-edit).
bool isOrnamentItem(const EngravingItem* e)
{
    return e->isOrnament() || isJazzOrnament(e) || e->isArpeggio() || e->isChordLine() || e->isTrillSegment()
           || e->isGlissandoSegment() || e->isGuitarBendSegment() || e->isHammerOnPullOffSegment() || e->isTapping()
           || e->isWhammyBarSegment();
}

Fraction endIncludingTies(const ChordRest* cr)
{
    return cr->isChord() ? toChord(cr)->endTickIncludingTied() : cr->endTick();
}

// The chord a selected item belongs to: a note's, or the one an ornament, arpeggio, jazz line, trill or glissando is on.
Chord* ornamentChordOf(EngravingItem* e)
{
    EngravingItem* owner = e;
    if (e->isNote()) {
        return toNote(e)->chord();
    } else if (e->isOrnament() || e->isArticulation()) {
        owner = toArticulation(e)->chordRest();
    } else if (e->isArpeggio()) {
        owner = toArpeggio(e)->chord();
    } else if (e->isChordLine()) {
        owner = toChordLine(e)->chord();
    } else if (e->isTrillSegment()) {
        owner = toTrillSegment(e)->trill()->startElement();
    } else if (e->isGlissandoSegment()) {
        EngravingItem* start = toGlissandoSegment(e)->glissando()->startElement();
        owner = start && start->isNote() ? toNote(start)->chord() : nullptr;
    } else if (e->isTapping()) {
        owner = toTapping(e)->chordRest();
    } else if (e->isGuitarBendSegment()) {
        const Note* start = toGuitarBendSegment(e)->guitarBend()->startNote();
        owner = start ? start->chord() : nullptr;
    } else if (e->isHammerOnPullOffSegment()) {
        owner = toHammerOnPullOffSegment(e)->hammerOnPullOff()->startChord();
    } else if (e->isWhammyBarSegment()) {
        owner = toWhammyBarSegment(e)->whammyBar()->startChord();
    }
    return owner && owner->isChord() ? toChord(owner) : nullptr;
}

// --- Adding one item (inside an open transaction) ---

// Ornament or jazz ornament symbol on the chord; the same symbol again removes it (toggle). Null when removed.
EngravingItem* addSymbol(Score* score, Chord* chord, const OrnamentEntry& entry)
{
    Articulation* a = entry.kind == OKind::Ornament ? Factory::createOrnament(score->dummy())
                      : Factory::createArticulation(score->dummy());
    a->setSymId(entry.sym);
    if (!EditChord::toggleArticulation(score, chord, a)) {
        delete a;
        return nullptr;
    }
    return a;
}

// A trill line from the chord/rest to end; a trill already starting there in that voice is replaced.
EngravingItem* addTrill(Score* score, ChordRest* cr, const Fraction& end, const OrnamentEntry& entry)
{
    if (end <= cr->tick()) {
        return nullptr;
    }
    std::vector<Spanner*> old;
    const auto range = score->spanner().equal_range(cr->tick().ticks());
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second->isTrill() && it->second->track() == cr->track()) {
            old.push_back(it->second);
        }
    }
    for (Spanner* s : old) {
        score->undoRemoveElement(s);
    }

    Trill* t = Factory::createTrill(score->dummy());
    t->setTrillType(TrillType::TRILL_LINE);
    t->styleChanged();
    t->setTrack(cr->track());
    t->setTrack2(cr->track());
    t->setTick(cr->tick());
    t->setTick2(end);
    if (entry.interval) {
        t->ornament()->setIntervalAbove(*entry.interval);
    }
    score->undoAddElement(t);
    return t;
}

// An arpeggio sign on the chord, replacing the one it has (as MuseScore's palette drop does).
EngravingItem* addArpeggio(Score* score, Chord* chord, ArpeggioType type)
{
    if (chord->arpeggio()) {
        score->undoRemoveElement(chord->arpeggio());
    }
    Arpeggio* a = Factory::createArpeggio(chord);
    a->setArpeggioType(type);
    a->setTrack(chord->track());
    a->setOwnershipParent(chord);
    score->undoAddElement(a);
    return a;
}

// A jazz articulation on the note. A note has one on each side: the new one replaces the one on its side, and the
// same one again removes it (toggle). Null when removed.
EngravingItem* addJazzArticulation(Score* score, Note* note, const OrnamentEntry& entry)
{
    Chord* chord = note->chord();
    ChordLine* cl = Factory::createChordLine(chord);
    cl->setChordLineType(entry.chordLine);
    cl->setStraight(entry.smooth);
    cl->setNote(note);

    bool same = false;
    const ElementList elements = chord->el(); // a copy: removing changes the list
    for (EngravingItem* e : elements) {
        if (e->isChordLine() && toChordLine(e)->note() == note && toChordLine(e)->isToTheLeft() == cl->isToTheLeft()) {
            same = same || (toChordLine(e)->chordLineType() == entry.chordLine && toChordLine(e)->isStraight() == entry.smooth);
            score->undoRemoveElement(e);
        }
    }
    if (same) {
        delete cl;
        return nullptr;
    }
    cl->setTrack(chord->track());
    cl->setOwnershipParent(chord);
    score->undoAddElement(cl);
    return cl;
}

// A glissando from the note to `to`, or to the next note in its voice when `to` is null.
EngravingItem* addGlissando(Score* score, Note* from, Note* to, const OrnamentEntry& entry)
{
    Glissando* g = Factory::createGlissando(score->dummy());
    if (entry.glissando) {
        g->setGlissandoType(*entry.glissando);
    }
    g->setEndElement(to);
    EditData ed;
    ed.dropElement = g;
    if (!from->acceptDrop(ed)) { // the note already starts a glissando
        delete g;
        return nullptr;
    }
    from->drop(score->transactionManager()->currentOrDummyTransaction(), ed); // deletes g if no note follows
    for (Spanner* s : from->spannerFor()) { // drop() returns nothing for a glissando
        if (s->isGlissando()) {
            return s;
        }
    }
    return nullptr;
}

// --- Guitar (spec 61) ---

// A vibrato bar line from the chord/rest to end.
EngravingItem* addWhammyBar(Score* score, ChordRest* cr, const Fraction& end)
{
    if (end <= cr->tick()) {
        return nullptr;
    }
    WhammyBar* w = Factory::createWhammyBar(score->dummy());
    w->setTrack(cr->track());
    w->setTrack2(cr->track());
    w->setTick(cr->tick());
    w->setTick2(end);
    score->undoAddElement(w);
    return w;
}

// Tapping on the chord; it replaces the one it has, and the same hand again removes it. Null when removed.
EngravingItem* addTapping(Score* score, Chord* chord, TappingHand hand)
{
    Tapping* old = chord->tapping();
    const bool same = old && old->hand() == hand;
    if (old) {
        score->undoRemoveElement(old);
    }
    if (same) {
        return nullptr;
    }
    Tapping* t = Factory::createTapping(chord);
    t->setHand(hand);
    t->setTrack(chord->track());
    t->setOwnershipParent(chord);
    score->undoAddElement(t);
    return t;
}

// A hammer-on / pull-off slur from the first to the last chord, or to the next chord of the voice when they are the
// same. MuseScore writes H or P (or both, for a ligado) from the pitches. Null when the chord already starts one.
EngravingItem* addHammerPull(Score* score, Chord* first, Chord* last)
{
    const auto range = score->spanner().equal_range(first->tick().ticks());
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second->isHammerOnPullOff() && it->second->track() == first->track()) {
            return nullptr;
        }
    }
    HammerOnPullOff* tmpl = Factory::createHammerOnPullOff(score->dummy());
    Slur* slur = score->addSlur(first, last == first ? nullptr : last, tmpl);
    delete tmpl;
    return slur;
}

// --- Selection ---

// Where an ornament or a trill goes on one staff: its earliest selected note, and the end of its latest one.
struct StaffSpot {
    Chord* chord = nullptr;
    Fraction end;
};

// The notes a glissando or a bend joins: two notes are joined, and from more notes each one but the last joins the
// next note in its voice (a single note too, `to` null). A selected glissando or bend keeps its two notes.
std::vector<std::pair<Note*, Note*> > notePairs(const std::vector<EngravingItem*>& items)
{
    std::vector<std::pair<Note*, Note*> > pairs;
    std::vector<Note*> notes;
    for (EngravingItem* e : items) {
        if (e->isNote()) {
            notes.push_back(toNote(e));
        } else if (e->isGlissandoSegment()) {
            const Glissando* g = toGlissandoSegment(e)->glissando();
            if (g->startElement() && g->startElement()->isNote() && g->endElement() && g->endElement()->isNote()) {
                pairs.push_back({ toNote(g->startElement()), toNote(g->endElement()) });
            }
        } else if (e->isGuitarBendSegment()) {
            const GuitarBend* b = toGuitarBendSegment(e)->guitarBend();
            if (b->startNote() && b->endNote() && b->endNote() != b->startNote()) {
                pairs.push_back({ b->startNote(), b->endNote() });
            }
        }
    }
    std::sort(notes.begin(), notes.end(), [](const Note* a, const Note* b) { return a->tick() < b->tick(); });
    if (notes.size() == 2 && notes[0]->tick() != notes[1]->tick()) {
        pairs.push_back({ notes[0], notes[1] });
    } else {
        for (Note* n : notes) {
            if (notes.size() == 1 || n->tick() != notes.back()->tick()) {
                pairs.push_back({ n, nullptr });
            }
        }
    }
    return pairs;
}

struct ChordSpan {
    Chord* first = nullptr;
    Chord* last = nullptr;
};

// The first and last selected chord of each voice.
std::map<track_idx_t, ChordSpan> chordSpans(const std::vector<EngravingItem*>& items)
{
    std::map<track_idx_t, ChordSpan> spans;
    for (EngravingItem* e : items) {
        Chord* c = ornamentChordOf(e);
        if (!c) {
            continue;
        }
        ChordSpan& span = spans[c->track()];
        if (!span.first || c->tick() < span.first->tick()) {
            span.first = c;
        }
        if (!span.last || c->tick() > span.last->tick()) {
            span.last = c;
        }
    }
    return spans;
}

void applyGuitar(GTech tech, Score* score, const std::vector<EngravingItem*>& items, bool& changed,
                 std::vector<EngravingItem*>& added)
{
    switch (tech) {
    case GTech::Bend:
    case GTech::Dive:
        for (const auto& [from, to] : notePairs(items)) {
            added.push_back(score->addGuitarBend(tech == GTech::Bend ? GuitarBendType::BEND : GuitarBendType::DIVE, from, to));
        }
        break;
    case GTech::Scoop:
    case GTech::Dip: {
        // A scoop goes on every selected note, a dip on the earliest one of each staff.
        std::map<staff_idx_t, Note*> earliest;
        std::vector<Note*> notes;
        for (EngravingItem* e : items) {
            Note* n = e->isNote() ? toNote(e) : e->isGuitarBendSegment() ? toGuitarBendSegment(e)->guitarBend()->startNote() : nullptr;
            if (n && std::find(notes.begin(), notes.end(), n) == notes.end()) {
                notes.push_back(n);
                Note*& first = earliest[n->staffIdx()];
                if (!first || n->tick() < first->tick()) {
                    first = n;
                }
            }
        }
        if (tech == GTech::Scoop) {
            for (Note* n : notes) {
                added.push_back(score->addGuitarBend(GuitarBendType::SCOOP, n));
            }
        } else {
            for (const auto& [staff, n] : earliest) {
                added.push_back(score->addGuitarBend(GuitarBendType::DIP, n));
            }
        }
        break;
    }
    case GTech::WhammyBar: {
        // On each selected staff, from its earliest selected chord to the end of its latest.
        std::map<staff_idx_t, StaffSpot> spots;
        for (EngravingItem* e : items) {
            Chord* c = ornamentChordOf(e);
            if (!c) {
                continue;
            }
            const Fraction end = e->isWhammyBarSegment() ? toWhammyBarSegment(e)->whammyBar()->tick2() : endIncludingTies(c);
            StaffSpot& spot = spots[c->staffIdx()];
            if (!spot.chord || c->tick() < spot.chord->tick()) {
                spot.chord = c;
            }
            spot.end = std::max(spot.end, end);
        }
        for (const auto& [staff, spot] : spots) {
            added.push_back(addWhammyBar(score, spot.chord, spot.end));
        }
        break;
    }
    case GTech::HammerPull:
        for (const auto& [track, span] : chordSpans(items)) {
            added.push_back(addHammerPull(score, span.first, span.last));
        }
        break;
    case GTech::Tap:
    case GTech::LeftTap: {
        std::vector<Chord*> chords;
        for (EngravingItem* e : items) {
            Chord* c = ornamentChordOf(e);
            if (c && std::find(chords.begin(), chords.end(), c) == chords.end()) {
                chords.push_back(c);
                changed = true; // the same hand again removed it
                added.push_back(addTapping(score, c, tech == GTech::Tap ? TappingHand::RIGHT : TappingHand::LEFT));
            }
        }
        break;
    }
    case GTech::TapPull:
    case GTech::LeftTapPull:
        for (const auto& [track, span] : chordSpans(items)) {
            added.push_back(addTapping(score, span.first, tech == GTech::TapPull ? TappingHand::RIGHT : TappingHand::LEFT));
            added.push_back(addHammerPull(score, span.first, span.last));
        }
        break;
    }
}

std::vector<EngravingItem*> applyToSelection(const OrnamentEntry& entry, Score* score,
                                             const std::vector<EngravingItem*>& items, bool& changed)
{
    std::vector<EngravingItem*> added;
    switch (entry.kind) {
    case OKind::Ornament:
    case OKind::JazzOrnament:
    case OKind::Trill: {
        // Dorico: on each selected staff, at the earliest selected note; a trill spans to the end of the latest one.
        std::map<staff_idx_t, StaffSpot> spots;
        for (EngravingItem* e : items) {
            Chord* c = ornamentChordOf(e);
            if (!c) {
                continue;
            }
            const Fraction end = e->isTrillSegment() ? toTrillSegment(e)->trill()->tick2() : endIncludingTies(c);
            StaffSpot& spot = spots[c->staffIdx()];
            if (!spot.chord || c->tick() < spot.chord->tick()) {
                spot.chord = c;
            }
            spot.end = std::max(spot.end, end);
        }
        for (const auto& [staff, spot] : spots) {
            EngravingItem* a = entry.kind == OKind::Trill ? addTrill(score, spot.chord, spot.end, entry)
                               : addSymbol(score, spot.chord, entry);
            changed = true; // a symbol entered again was toggled off
            added.push_back(a);
        }
        break;
    }
    case OKind::Arpeggio: {
        std::vector<Chord*> chords;
        for (EngravingItem* e : items) {
            Chord* c = ornamentChordOf(e);
            if (c && std::find(chords.begin(), chords.end(), c) == chords.end()) {
                chords.push_back(c);
                added.push_back(addArpeggio(score, c, entry.arpeggio));
            }
        }
        break;
    }
    case OKind::JazzArticulation: {
        std::vector<Note*> notes;
        for (EngravingItem* e : items) {
            Note* n = e->isNote() ? toNote(e) : e->isChordLine() ? toChordLine(e)->note() : nullptr;
            if (n && std::find(notes.begin(), notes.end(), n) == notes.end()) {
                notes.push_back(n);
                changed = true;
                added.push_back(addJazzArticulation(score, n, entry));
            }
        }
        break;
    }
    case OKind::Glissando: {
        for (const auto& [from, to] : notePairs(items)) {
            added.push_back(addGlissando(score, from, to, entry));
        }
        break;
    }
    case OKind::Guitar:
        applyGuitar(entry.guitar, score, items, changed, added);
        break;
    }
    std::erase(added, nullptr);
    return added;
}

bool applySelection(const OrnamentEntry& entry, Score* score, mu::notation::INotationPtr notation)
{
    const Selection& sel = score->selection();
    const std::vector<EngravingItem*> items = sel.elements(); // a copy: removing items changes the selection
    // Return on a selected ornament reopened the popover with only that item selected: the entry replaces it.
    std::vector<EngravingItem*> edited;
    if (!sel.isRange()) {
        std::copy_if(items.begin(), items.end(), std::back_inserter(edited), isOrnamentItem);
    }

    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(ADD_ORNAMENT);
    for (EngravingItem* e : edited) {
        if (e->isSpannerSegment()) {
            score->undoRemoveElement(toSpannerSegment(e)->spanner());
        } else {
            score->undoRemoveElement(e);
        }
    }
    bool changed = !edited.empty();
    const std::vector<EngravingItem*> added = applyToSelection(entry, score, items, changed);
    if (!changed && added.empty()) {
        undoStack->rollbackChanges();
        return false;
    }
    undoStack->commitChanges();

    // The new item is selected, so Return reopens it for editing.
    if (!added.empty() && !notation->interaction()->noteInput()->isNoteInputMode()) {
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

// Ornaments, trills and arpeggios entered at the caret wait for the note there; a trill spans the caret's advance.
bool addOrnamentAtCaret(const OrnamentEntry& entry, Score* score, ChordRest* cr, const Fraction& caret)
{
    if (entry.kind == OKind::Trill) {
        return addTrill(score, cr, caret, entry);
    }
    if (entry.kind == OKind::Guitar && entry.guitar == GTech::WhammyBar) {
        return addWhammyBar(score, cr, caret);
    }
    if (!cr->isChord()) {
        return false;
    }
    if (entry.kind == OKind::Arpeggio) {
        return addArpeggio(score, toChord(cr), entry.arpeggio);
    }
    addSymbol(score, toChord(cr), entry);
    return true; // a symbol entered again was toggled off
}

std::string ornamentPrefill(const EngravingItem* item)
{
    switch (item->type()) {
    case ElementType::ORNAMENT:
        return ornamentText(toOrnament(item)->symId());
    case ElementType::TRILL_SEGMENT: {
        const Trill* t = toTrillSegment(item)->trill();
        if (t->trillType() != TrillType::TRILL_LINE) {
            return std::string(); // prall lines have no entry
        }
        return trillText(t->ornament() ? t->ornament()->intervalAbove() : OrnamentInterval());
    }
    case ElementType::ARPEGGIO:
        return arpeggioText(toArpeggio(item)->arpeggioType());
    case ElementType::GLISSANDO_SEGMENT:
        return glissandoText(toGlissandoSegment(item)->glissando()->glissandoType());
    case ElementType::CHORDLINE:
        return jazzArticulationText(toChordLine(item)->chordLineType(), toChordLine(item)->isStraight());
    case ElementType::GUITAR_BEND_SEGMENT:
        switch (toGuitarBendSegment(item)->guitarBend()->bendType()) {
        case GuitarBendType::BEND: return guitarText(GTech::Bend);
        case GuitarBendType::DIVE: return guitarText(GTech::Dive);
        case GuitarBendType::SCOOP: return guitarText(GTech::Scoop);
        case GuitarBendType::DIP: return guitarText(GTech::Dip);
        default: return std::string(); // pre-bends, grace-note and slight bends have no entry
        }
    case ElementType::HAMMER_ON_PULL_OFF_SEGMENT:
        return guitarText(GTech::HammerPull);
    case ElementType::TAPPING:
        return guitarText(toTapping(item)->hand() == TappingHand::LEFT ? GTech::LeftTap : GTech::Tap);
    case ElementType::WHAMMY_BAR_SEGMENT:
        return guitarText(GTech::WhammyBar);
    default:
        return std::string();
    }
}
}

PopoverDef mu::dorico::ornamentsPopover()
{
    auto pending = std::make_shared<PendingAtCaret>(ADD_ORNAMENT);

    PopoverDef def;
    def.id = "ornaments";
    def.title = "Ornaments";
    def.icon = muse::ui::IconCode::Code::ORNAMENT;
    def.supportsLocal = false; // Dorico: Return only
    def.itemTypes = { ElementType::ORNAMENT, ElementType::TRILL_SEGMENT, ElementType::ARPEGGIO,
                      ElementType::GLISSANDO_SEGMENT, ElementType::CHORDLINE, ElementType::GUITAR_BEND_SEGMENT,
                      ElementType::HAMMER_ON_PULL_OFF_SEGMENT, ElementType::TAPPING, ElementType::WHAMMY_BAR_SEGMENT };
    def.validate = [](const std::string& text) { return parseOrnament(text).status; };
    def.prefill = ornamentPrefill;

    def.apply = [pending](const std::string& text, ApplyScope, mu::notation::INotationPtr notation) {
        const OrnamentParse parsed = parseOrnament(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        Score* score = notation->elements()->msScore();
        const OKind kind = parsed.entry.kind;
        // During note input jazz articulations, glissandos and the guitar items on a note go on the last input note,
        // which is selected; the rest wait at the caret for the next note. A bend needs a second note (Dorico: not
        // in note input).
        const bool inInput = notation->interaction()->noteInput()->isNoteInputMode();
        if (inInput && kind == OKind::Guitar && (parsed.entry.guitar == GTech::Bend || parsed.entry.guitar == GTech::Dive)) {
            return false;
        }
        const bool onLastNote = kind == OKind::JazzArticulation || kind == OKind::Glissando
                                || (kind == OKind::Guitar && parsed.entry.guitar != GTech::WhammyBar);
        if (inInput && !onLastNote) {
            return pending->add([entry = parsed.entry](Score* score, ChordRest* cr, const Fraction& caret) {
                return addOrnamentAtCaret(entry, score, cr, caret);
            }, notation);
        }
        return applySelection(parsed.entry, score, notation);
    };

    def.noteInputEnded = [pending](mu::notation::INotationPtr) { pending->cancel(); };
    return def;
}
