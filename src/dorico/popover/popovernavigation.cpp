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

#include "popovernavigation.h"

#include <algorithm>

#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/system.h"

using namespace mu::dorico;
using namespace mu::engraving;

Fraction mu::dorico::beatLength(const Fraction& timeSig)
{
    const bool compound = timeSig.numerator() > 3 && timeSig.numerator() % 3 == 0 && timeSig.denominator() > 4;
    return Fraction(compound ? 3 : 1, timeSig.denominator());
}

Fraction mu::dorico::beatStart(const Fraction& tickInBar, const Fraction& timeSig)
{
    const int beat = beatLength(timeSig).ticks();
    return Fraction::fromTicks(tickInBar.ticks() / beat * beat);
}

static bool hasItem(const Segment* segment, staff_idx_t staff, const std::vector<ElementType>& itemTypes)
{
    return std::any_of(segment->annotations().begin(), segment->annotations().end(), [&](const EngravingItem* e) {
        return e->staffIdx() == staff && std::find(itemTypes.begin(), itemTypes.end(), e->type()) != itemTypes.end();
    });
}

std::optional<PopoverPosition> mu::dorico::movePosition(const Score* score, const PopoverPosition& from, PopoverMove move,
                                                        const std::vector<ElementType>& itemTypes)
{
    const Measure* m = score->tick2measure(from.tick);
    if (!m) {
        return std::nullopt;
    }
    std::optional<Fraction> tick;
    switch (move) {
    case PopoverMove::NextBeat: {
        const Fraction next = m->tick() + beatStart(from.tick - m->tick(), m->timesig()) + beatLength(m->timesig());
        if (next < m->endTick()) {
            tick = next;
        } else if (const Measure* nm = m->nextMeasure()) {
            tick = nm->tick();
        }
        break;
    }
    case PopoverMove::PrevBeat:
        if (from.tick > Fraction(0, 1)) {
            const Fraction before = from.tick - Fraction::fromTicks(1);
            const Measure* pm = score->tick2measure(before);
            tick = pm->tick() + beatStart(before - pm->tick(), pm->timesig());
        }
        break;
    case PopoverMove::NextBar:
        if (const Measure* nm = m->nextMeasure()) {
            tick = nm->tick();
        }
        break;
    case PopoverMove::PrevBar:
        if (from.tick > m->tick()) {
            tick = m->tick();
        } else if (const Measure* pm = m->prevMeasure()) {
            tick = pm->tick();
        }
        break;
    case PopoverMove::NextItem:
    case PopoverMove::PrevItem: {
        const bool next = move == PopoverMove::NextItem;
        const staff_idx_t staff = track2staff(from.track);
        for (const Segment* s = score->firstSegment(SegmentType::ChordRest); s; s = s->next1(SegmentType::ChordRest)) {
            if ((next ? s->tick() > from.tick : s->tick() < from.tick) && hasItem(s, staff, itemTypes)) {
                tick = s->tick();
                if (next) {
                    break;
                }
            }
        }
        break;
    }
    }
    if (!tick) {
        return std::nullopt;
    }
    return PopoverPosition { *tick, from.track };
}

EngravingItem* mu::dorico::itemAt(const Score* score, const PopoverPosition& pos, const std::vector<ElementType>& itemTypes)
{
    const staff_idx_t staff = track2staff(pos.track);
    for (Segment* s = score->tick2segment(pos.tick, true, SegmentType::ChordRest); s && s->tick() == pos.tick;
         s = s->next1(SegmentType::ChordRest)) {
        for (EngravingItem* e : s->annotations()) {
            if (e->staffIdx() == staff && std::find(itemTypes.begin(), itemTypes.end(), e->type()) != itemTypes.end()) {
                return e;
            }
        }
    }
    return nullptr;
}

muse::RectF mu::dorico::positionRect(const Score* score, const PopoverPosition& pos)
{
    const Measure* m = score->tick2measure(pos.tick);
    const System* system = m ? m->system() : nullptr;
    const staff_idx_t staff = track2staff(pos.track);
    if (!system || !system->staff(staff)) {
        return muse::RectF();
    }

    // Between the chords/rests either side of the tick, in proportion: a beat inside a half note sits mid-note.
    const Segment* before = nullptr;
    const Segment* after = nullptr;
    for (const Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
        if (s->tick() <= pos.tick) {
            before = s;
        } else {
            after = s;
            break;
        }
    }
    double x = m->canvasPos().x();
    if (before) {
        const double x1 = before->canvasPos().x();
        const double x2 = after ? after->canvasPos().x() : m->canvasPos().x() + m->width();
        const Fraction t2 = after ? after->tick() : m->endTick();
        x = x1 + (x2 - x1) * ((pos.tick - before->tick()) / (t2 - before->tick())).toDouble();
    }
    return muse::RectF(x, system->staffCanvasYpage(staff), score->style().spatium(), score->style().spatium() * 4);
}
