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
#include <optional>
#include <set>

#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/lyrics.h"
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
