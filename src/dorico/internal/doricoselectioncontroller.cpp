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


#include "doricoselectioncontroller.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>

#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/lyrics.h"
#include "engraving/dom/pitchspelling.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/spanner.h"
#include "engraving/dom/system.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"

#include "../doricocommands.h"
#include "filters.h"
#include "filtersettings.h"
#include "selectmore.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;

namespace {
//! A spanner is one item however many systems it crosses: its first segment stands for it
EngravingItem* canonical(EngravingItem* e)
{
    if (e->isSpannerSegment()) {
        Spanner* sp = toSpannerSegment(e)->spanner();
        return sp && !sp->spannerSegments().empty() ? sp->frontSegment() : nullptr;
    }
    return e;
}

Fraction startTick(const EngravingItem* e)
{
    return e->isSpannerSegment() ? toSpannerSegment(e)->spanner()->tick() : e->tick();
}

//! Items with a place on a staff; bars, systems and the like are not selected one by one
std::optional<SelectMoreItem> describe(const EngravingItem* e)
{
    if (e->track() == muse::nidx || e->isMeasureBase() || e->isSegment() || e->isSystem() || e->isPage()) {
        return std::nullopt;
    }
    const Fraction tick = startTick(e);
    if (!tick.isValid() || tick < Fraction(0, 1)) {
        return std::nullopt;
    }
    SelectKey key;
    key.type = static_cast<int>(e->type());
    key.staff = static_cast<int>(e->staffIdx());
    key.voice = static_cast<int>(e->voice());
    if (e->isLyrics()) {
        key.variant = toLyrics(e)->verse() * 2 + (e->placeAbove() ? 0 : 1);
    } else if (e->isNote() && toNote(e)->chord()->isGrace()) {
        key.variant = 1;
    }
    return SelectMoreItem { key, tick.ticks() };
}

std::set<const EngravingItem*> asSet(const std::vector<EngravingItem*>& items)
{
    return std::set<const EngravingItem*>(items.begin(), items.end());
}

//! First or last chord / rest of the bar in a voice
ChordRest* barEdge(const Measure* m, track_idx_t track, bool end)
{
    for (Segment* s = end ? m->last(SegmentType::ChordRest) : m->first(SegmentType::ChordRest); s;
         s = end ? s->prev(SegmentType::ChordRest) : s->next(SegmentType::ChordRest)) {
        if (s->element(track)) {
            return s->cr(track);
        }
    }
    return nullptr;
}

FilterKind filterKindOf(ElementType type)
{
    switch (type) {
    case ElementType::NOTE: return FilterKind::Note;
    case ElementType::REST:
    case ElementType::MMREST: return FilterKind::Rest;
    case ElementType::STEM:
    case ElementType::HOOK:
    case ElementType::BEAM:
    case ElementType::NOTEDOT:
    case ElementType::ACCIDENTAL:
    case ElementType::LEDGER_LINE:
    case ElementType::STEM_SLASH:
    case ElementType::NOTEHEAD:
    case ElementType::CHORD_BRACKET:
    case ElementType::PARENTHESIS:
    case ElementType::LYRICSLINE:
    case ElementType::LYRICSLINE_SEGMENT: return FilterKind::Part;
    case ElementType::ARTICULATION:
    case ElementType::FINGERING:
    case ElementType::CHORDLINE:
    case ElementType::TREMOLO_SINGLECHORD:
    case ElementType::TREMOLO_TWOCHORD: return FilterKind::Attached;
    case ElementType::DYNAMIC: return FilterKind::Dynamic;
    case ElementType::HAIRPIN:
    case ElementType::HAIRPIN_SEGMENT: return FilterKind::Hairpin;
    case ElementType::TEMPO_TEXT: return FilterKind::Tempo;
    case ElementType::GRADUAL_TEMPO_CHANGE:
    case ElementType::GRADUAL_TEMPO_CHANGE_SEGMENT: return FilterKind::GradualTempo;
    case ElementType::LYRICS: return FilterKind::Lyrics;
    case ElementType::TEXT:
    case ElementType::STAFF_TEXT:
    case ElementType::SYSTEM_TEXT:
    case ElementType::EXPRESSION:
    case ElementType::STICKING:
    case ElementType::MARKER:
    case ElementType::JUMP:
    case ElementType::PLAY_COUNT_TEXT: return FilterKind::Text;
    case ElementType::HARMONY: return FilterKind::ChordSymbol;
    case ElementType::FIGURED_BASS: return FilterKind::FiguredBass;
    case ElementType::SLUR:
    case ElementType::SLUR_SEGMENT: return FilterKind::Slur;
    case ElementType::TIE:
    case ElementType::TIE_SEGMENT:
    case ElementType::LAISSEZ_VIB:
    case ElementType::LAISSEZ_VIB_SEGMENT:
    case ElementType::PARTIAL_TIE:
    case ElementType::PARTIAL_TIE_SEGMENT: return FilterKind::Tie;
    case ElementType::ORNAMENT: return FilterKind::Ornament;
    case ElementType::ARPEGGIO: return FilterKind::Arpeggio;
    case ElementType::GLISSANDO:
    case ElementType::GLISSANDO_SEGMENT: return FilterKind::Glissando;
    case ElementType::OTTAVA:
    case ElementType::OTTAVA_SEGMENT: return FilterKind::Ottava;
    case ElementType::PEDAL:
    case ElementType::PEDAL_SEGMENT: return FilterKind::Pedal;
    case ElementType::TEXTLINE:
    case ElementType::TEXTLINE_SEGMENT:
    case ElementType::TRILL:
    case ElementType::TRILL_SEGMENT:
    case ElementType::VOLTA:
    case ElementType::VOLTA_SEGMENT:
    case ElementType::LET_RING:
    case ElementType::LET_RING_SEGMENT:
    case ElementType::VIBRATO:
    case ElementType::VIBRATO_SEGMENT:
    case ElementType::PALM_MUTE:
    case ElementType::PALM_MUTE_SEGMENT:
    case ElementType::NOTELINE:
    case ElementType::NOTELINE_SEGMENT: return FilterKind::OtherLine;
    case ElementType::TUPLET: return FilterKind::Tuplet;
    case ElementType::FERMATA:
    case ElementType::BREATH: return FilterKind::Hold;
    case ElementType::REHEARSAL_MARK: return FilterKind::RehearsalMark;
    case ElementType::PLAYTECH_ANNOTATION: return FilterKind::Technique;
    case ElementType::CLEF: return FilterKind::Clef;
    default: return FilterKind::None;
    }
}

FilterPitch filterPitchOf(int pitch, int tpc)
{
    return { tpc2step(tpc), static_cast<int>(tpc2alter(tpc)), playingOctave(pitch, tpc) };
}

//! `groups` numbers the chords, so that a note and what is attached to it share a group
FilterItem filterItemOf(const EngravingItem* e, std::map<const Chord*, int>& groups)
{
    FilterItem item;
    item.kind = filterKindOf(e->type());
    item.voice = static_cast<int>(e->voice());
    if (e->isLyrics()) {
        item.verse = toLyrics(e)->verse();
    }
    const EngravingItem* chordItem = e->isNote() || item.kind == FilterKind::Attached ? e->findAncestor(ElementType::CHORD) : nullptr;
    if (const Chord* chord = chordItem ? toChord(chordItem) : nullptr) {
        item.group = groups.try_emplace(chord, static_cast<int>(groups.size())).first->second;
        item.grace = chord->isGrace();
        item.stemUp = chord->up();
        if (e->isNote()) {
            const Note* n = toNote(e);
            const std::vector<Note*>& notes = chord->notes();
            item.chordSize = static_cast<int>(notes.size());
            item.indexFromTop = static_cast<int>(notes.size()) - 1 - static_cast<int>(std::find(notes.begin(), notes.end(), n) - notes.begin());
            item.written = filterPitchOf(n->pitch() - n->transposition(), n->tpc2());
            item.sounding = filterPitchOf(n->pitch(), n->tpc1());
        }
    }
    return item;
}

struct CycleEntry {
    EngravingItem* item = nullptr;
    const void* group = nullptr; //!< the notes of a chord, or the segments of a spanner, are one stop
};

const void* groupOf(const EngravingItem* e)
{
    if (e->isNote()) {
        return toNote(e)->chord();
    }
    return e->isSpannerSegment() ? static_cast<const void*>(toSpannerSegment(e)->spanner()) : e;
}

//! Everything Tab visits at one rhythmic position on a staff: chords and rests of each voice, then their articulations and
//! lyrics, then the staff's text and other annotations, then the spanners that start there. System-attached items are skipped.
std::vector<CycleEntry> itemsAt(const Score* score, const Fraction& tick, staff_idx_t staff)
{
    std::vector<CycleEntry> entries;
    Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
    if (!seg) {
        return entries;
    }
    const auto add = [&entries](EngravingItem* e) { entries.push_back({ e, groupOf(e) }); };

    std::vector<ChordRest*> crs;
    for (track_idx_t t = staff2track(staff); t < staff2track(staff + 1); ++t) {
        if (ChordRest* cr = seg->cr(t)) {
            crs.push_back(cr);
            add(cr->isChord() ? static_cast<EngravingItem*>(toChord(cr)->upNote()) : cr);
        }
    }
    for (ChordRest* cr : crs) {
        if (cr->isChord()) {
            for (Articulation* a : toChord(cr)->articulations()) {
                add(a);
            }
        }
        for (Lyrics* l : cr->lyrics()) {
            add(l);
        }
    }
    for (EngravingItem* a : seg->annotations()) {
        if (a->staffIdx() == staff && !a->systemFlag()) {
            add(a);
        }
    }
    for (const auto& interval : score->spannerMap().findOverlapping(tick.ticks(), tick.ticks())) {
        Spanner* sp = interval.value;
        if (sp->tick() == tick && sp->staffIdx() == staff && !sp->systemFlag() && !sp->spannerSegments().empty()) {
            add(sp->frontSegment());
        }
    }
    return entries;
}
}

void DoricoSelectionController::init()
{
    const auto onNotation = [this](const muse::rcommand::Command& command, std::function<void()> run) {
        dispatcher()->onRequest(this, command, [this, run]() {
            if (globalContext()->currentNotation()) {
                run();
            }
            return muse::make_ok();
        });
    };
    onNotation(DORICO_EXTEND_TO_BAR_START_COMMAND, [this]() { extendToBar(false); });
    onNotation(DORICO_EXTEND_TO_BAR_END_COMMAND, [this]() { extendToBar(true); });
    onNotation(DORICO_SELECT_MORE_COMMAND, [this]() { selectMore(); });
    onNotation(DORICO_DESELECT_ALL_COMMAND, [this]() { globalContext()->currentNotation()->interaction()->clearSelection(); });
    onNotation(DORICO_NEXT_ITEM_COMMAND, [this]() { cycleItems(true); });
    onNotation(DORICO_PREV_ITEM_COMMAND, [this]() { cycleItems(false); });
    onNotation(DORICO_SELECT_SAME_PITCH_ONWARDS_COMMAND, [this]() { selectSamePitchOnwards(); });

    muse::settings()->setDefaultValue(FILTER_MODE_KEY, muse::Val("select"));
    muse::settings()->setDefaultValue(FILTER_PITCHES_KEY, muse::Val(""));
    muse::settings()->setDefaultValue(FILTER_PITCHES_SOUNDING_KEY, muse::Val(false));
    for (const FilterDef& def : filterDefs()) {
        onNotation(doricoFilterCommand(def.id), [this, &def]() { applyCategoryFilter(def.matches); });
    }
    onNotation(DORICO_FILTER_NOTES_BY_PITCH_COMMAND, [this]() { filterNotesByPitch(); });
    dispatcher()->onRequest(this, DORICO_FILTER_SELECT_ONLY_COMMAND, []() {
        muse::settings()->setSharedValue(FILTER_MODE_KEY, muse::Val("select"));
        return muse::make_ok();
    });
    dispatcher()->onRequest(this, DORICO_FILTER_DESELECT_ONLY_COMMAND, []() {
        muse::settings()->setSharedValue(FILTER_MODE_KEY, muse::Val("deselect"));
        return muse::make_ok();
    });
}

void DoricoSelectionController::extendToBar(bool end)
{
    INotationPtr notation = globalContext()->currentNotation();
    Score* score = notation->elements()->msScore();
    if (score->noteEntryMode()) {
        return;
    }
    const Selection& sel = score->selection();
    ChordRest* cr = sel.activeCR();
    if (!cr) {
        cr = end ? sel.lastChordRest() : sel.firstChordRest();
    }
    Measure* m = cr ? cr->findMeasure() : nullptr;
    if (!m) {
        return;
    }
    ChordRest* target = barEdge(m, cr->track(), end);
    if (target == cr) {
        // Already at the edge: go on to the next bar's
        m = end ? m->nextMeasure() : m->prevMeasure();
        target = m ? barEdge(m, cr->track(), end) : nullptr;
    }
    if (target) {
        notation->interaction()->select({ target }, SelectType::RANGE, target->staffIdx());
    }
}

void DoricoSelectionController::selectMore()
{
    INotationPtr notation = globalContext()->currentNotation();
    INotationInteractionPtr interaction = notation->interaction();
    Score* score = notation->elements()->msScore();

    // The selected items, one per spanner, with the key they expand by
    std::vector<EngravingItem*> selectedItems;
    std::set<SelectKey> keys;
    for (EngravingItem* e : score->selection().elements()) {
        EngravingItem* item = canonical(e);
        const std::optional<SelectMoreItem> info = item ? describe(item) : std::nullopt;
        if (info && std::find(selectedItems.begin(), selectedItems.end(), item) == selectedItems.end()) {
            selectedItems.push_back(item);
            keys.insert(info->key);
        }
    }
    if (selectedItems.empty()) {
        return;
    }

    std::vector<EngravingItem*> items;
    std::vector<SelectMoreItem> all;
    std::set<EngravingItem*> seen;
    score->scanElements([&](EngravingItem* e) {
        const std::optional<SelectMoreItem> info = describe(e);
        if (info && canonical(e) == e && keys.count(info->key) && seen.insert(e).second) {
            items.push_back(e);
            all.push_back(*info);
        }
    });
    std::vector<size_t> selected;
    for (EngravingItem* item : selectedItems) {
        const auto it = std::find(items.begin(), items.end(), item);
        if (it != items.end()) {
            selected.push_back(static_cast<size_t>(it - items.begin()));
        }
    }

    // A fresh selection starts at the first press
    const std::set<const EngravingItem*> current = asSet(score->selection().elements());
    const bool carriesOn = current == std::set<const EngravingItem*>(m_lastItems.begin(), m_lastItems.end());

    const auto barOf = [score](int tick) {
        const Measure* m = score->tick2measure(Fraction::fromTicks(tick));
        return m ? SelectMoreRange { m->tick().ticks(), m->endTick().ticks() } : SelectMoreRange { tick, tick + 1 };
    };
    SelectMoreContext ctx;
    ctx.bar = barOf;
    ctx.system = [score, barOf](int tick) {
        const Measure* m = score->tick2measure(Fraction::fromTicks(tick));
        const System* sys = m ? m->system() : nullptr;
        if (!sys || !sys->firstMeasure() || !sys->lastMeasure()) {
            return barOf(tick);
        }
        return SelectMoreRange { sys->firstMeasure()->tick().ticks(), sys->lastMeasure()->endTick().ticks() };
    };
    ctx.flow = { 0, score->endTick().ticks() };
    ctx.galley = score->linearMode();

    const SelectMoreResult result = mu::dorico::selectMore(all, selected, carriesOn ? m_lastStage : 0, ctx);

    std::vector<EngravingItem*> toSelect;
    for (size_t i : result.indices) {
        toSelect.push_back(items[i]);
    }
    interaction->clearSelection();
    interaction->select(toSelect, SelectType::ADD);

    // What the selection really holds now, so that an untouched selection counts as "the same"
    const std::vector<EngravingItem*>& now = score->selection().elements();
    m_lastItems.assign(now.begin(), now.end());
    m_lastStage = result.stage;
}

void DoricoSelectionController::cycleItems(bool forward)
{
    INotationPtr notation = globalContext()->currentNotation();
    Score* score = notation->elements()->msScore();
    EngravingItem* cur = score->selection().element();
    if (!cur || score->noteEntryMode() || !describe(cur)) {
        return;
    }
    const std::vector<CycleEntry> entries = itemsAt(score, startTick(cur), cur->staffIdx());
    const void* group = groupOf(cur);
    const auto it = std::find_if(entries.begin(), entries.end(), [group](const CycleEntry& e) { return e.group == group; });
    if (it == entries.end() || entries.size() < 2) {
        return;
    }
    const size_t n = entries.size();
    const size_t next = (static_cast<size_t>(it - entries.begin()) + (forward ? 1 : n - 1)) % n;
    notation->interaction()->select({ entries[next].item }, SelectType::SINGLE);
}

void DoricoSelectionController::selectSamePitchOnwards()
{
    INotationPtr notation = globalContext()->currentNotation();
    Score* score = notation->elements()->msScore();
    EngravingItem* cur = score->selection().element();
    if (!cur || !cur->isNote()) {
        return;
    }
    const Note* note = toNote(cur);
    const track_idx_t track = note->track();

    std::vector<EngravingItem*> notes;
    for (Segment* s = note->chord()->segment(); s; s = s->next1(SegmentType::ChordRest)) {
        if (s->element(track) && s->element(track)->isChord()) {
            for (Note* n : toChord(s->element(track))->notes()) {
                if (n->pitch() == note->pitch() && (s != note->chord()->segment() || n == note)) {
                    notes.push_back(n);
                }
            }
        }
    }
    INotationInteractionPtr interaction = notation->interaction();
    interaction->clearSelection();
    interaction->select(notes, SelectType::ADD);
}

//! Edit > Filter: keeps (Select Only) or drops (Deselect Only) one category of the selected items. A selection is not an undo step.
void DoricoSelectionController::applyCategoryFilter(const FilterPredicate& matches)
{
    INotationPtr notation = globalContext()->currentNotation();
    Score* score = notation->elements()->msScore();
    if (score->noteEntryMode()) {
        return;
    }

    std::vector<EngravingItem*> items;
    for (EngravingItem* e : score->selection().elements()) {
        EngravingItem* item = canonical(e);
        if (item && std::find(items.begin(), items.end(), item) == items.end()) {
            items.push_back(item);
        }
    }
    if (items.empty()) {
        return;
    }

    std::map<const Chord*, int> groups;
    std::vector<FilterItem> described;
    for (const EngravingItem* item : items) {
        described.push_back(filterItemOf(item, groups));
    }
    std::vector<EngravingItem*> kept;
    for (size_t i : applyFilter(described, matches, filterIsDeselectOnly())) {
        kept.push_back(items[i]);
    }

    INotationInteractionPtr interaction = notation->interaction();
    interaction->clearSelection();
    interaction->select(kept, SelectType::ADD);
}

void DoricoSelectionController::filterNotesByPitch()
{
    muse::UriQuery query("musescore://dorico/filternotesbypitch");
    query.addParam("pitches", muse::Val(muse::settings()->value(FILTER_PITCHES_KEY).toString()));
    query.addParam("sounding", muse::Val(muse::settings()->value(FILTER_PITCHES_SOUNDING_KEY).toBool()));
    interactive()->open(query).onResolve(this, [this](const muse::Val& result) {
        // The dialog answers "w:" or "s:" (written / sounding) and the pitch list
        const std::string text = result.toString();
        const std::optional<std::vector<PitchToken> > tokens = text.size() >= 2 ? parsePitchTokens(text.substr(2)) : std::nullopt;
        if (!tokens || tokens->empty() || !globalContext()->currentNotation()) {
            return;
        }
        const bool sounding = text[0] == 's';
        muse::settings()->setSharedValue(FILTER_PITCHES_KEY, muse::Val(pitchTokensToString(*tokens)));
        muse::settings()->setSharedValue(FILTER_PITCHES_SOUNDING_KEY, muse::Val(sounding));
        applyCategoryFilter([tokens, sounding](const FilterItem& i) {
            return i.kind == FilterKind::Note && matchesPitch(*tokens, sounding ? i.sounding : i.written);
        });
    });
}
