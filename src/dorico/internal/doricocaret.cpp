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

#include "doricocaret.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "settings.h"
#include "draw/painter.h"

#include "engraving/dom/chordrest.h"
#include "engraving/dom/input.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/sig.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafftype.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "noteinpututils.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;
using namespace muse;

static const Settings::Key GRID_KEY("dorico", "dorico/gridResolution");

static int gridTicks(int denominator)
{
    return Fraction(1, denominator).ticks();
}

void DoricoCaret::init()
{
    settings()->setDefaultValue(GRID_KEY, Val(8));
    m_gridDenominator = std::clamp(settings()->value(GRID_KEY).toInt(), 1, 128);
}

bool DoricoCaret::insideChordRest() const
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return false;
    }

    const InputState& is = notation->elements()->msScore()->inputState();
    const ChordRest* cr = is.cr();
    return cr && is.segment() == m_anchor && cr->tick() < m_tick && m_tick < cr->endTick();
}

Fraction DoricoCaret::caretTick() const
{
    if (insideChordRest()) {
        return m_tick;
    }
    INotationPtr notation = globalContext()->currentNotation();
    return notation ? notation->elements()->msScore()->inputState().tick() : Fraction();
}

bool DoricoCaret::step(bool right)
{
    INotationPtr notation = globalContext()->currentNotation();
    INotationNoteInputPtr ni = notation ? notation->interaction()->noteInput() : nullptr;
    if (!ni || !ni->isNoteInputMode() || ni->usingNoteInputMethod(NoteInputMethod::BY_DURATION)) {
        return false;
    }

    Score* score = notation->elements()->msScore();
    InputState& is = score->inputState();
    ChordRest* cr = is.cr();
    if (!cr || cr->tuplet() || is.beyondScore()) {
        return false;
    }

    // Grid positions count from the start of the bar.
    const int cur = caretTick().ticks();
    const int grid = gridTicks(m_gridDenominator);
    ChordRest* target = cr; // the chord/rest that will contain the caret
    int to = 0;

    if (right) {
        to = nextCaretTick(cur, cr->measure()->tick().ticks(), grid, cr->endTick().ticks());
        if (to == cr->endTick().ticks()) {
            target = score->findCR(cr->endTick(), is.track());
            if (!target || target->tick().ticks() != to) {
                return false; // end of the score: MuseScore's move handles it
            }
        }
    } else {
        if (cur == cr->tick().ticks()) {
            target = score->findCR(cr->tick() - Fraction::eps(), is.track());
            if (!target || target->tuplet()) {
                return false;
            }
        }
        to = prevCaretTick(cur, target->measure()->tick().ticks(), grid, target->tick().ticks());
    }

    is.setSegment(target->segment());
    m_anchor = target->segment();
    m_tick = Fraction::fromTicks(to);
    ni->setCurrentTrack(is.track()); // no-op change that makes MuseScore redraw the caret
    return true;
}

bool DoricoCaret::materialize()
{
    if (!insideChordRest()) {
        return false;
    }

    // ponytail: a note cut this way becomes note + rests, not tied notes (Dorico ties); good enough for rests.
    INotationPtr notation = globalContext()->currentNotation();
    Score* score = notation->elements()->msScore();
    ChordRest* cr = score->inputState().cr();
    const Fraction at = m_tick;
    m_anchor = nullptr;

    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(TranslatableString("undoableAction", "Enter note"));
    score->changeCRlen(cr, at - cr->tick());
    undoStack->commitChanges();

    // Set after the commit: ending the command may move the input position to the selection.
    if (Segment* s = score->tick2segment(at, true, SegmentType::ChordRest)) {
        score->inputState().setSegment(s);
    }
    return true;
}

void DoricoCaret::setGrid(int denominator)
{
    m_gridDenominator = std::clamp(denominator, 1, 128); // whole ... 128th
    settings()->setSharedValue(GRID_KEY, Val(m_gridDenominator));

    INotationPtr notation = globalContext()->currentNotation();
    INotationNoteInputPtr ni = notation ? notation->interaction()->noteInput() : nullptr;
    if (ni && ni->isNoteInputMode()) {
        ni->setCurrentTrack(ni->state().track()); // redraw the ruler
    }
}

void DoricoCaret::finerGrid()
{
    setGrid(m_gridDenominator * 2);
}

void DoricoCaret::coarserGrid()
{
    setGrid(m_gridDenominator / 2);
}

bool DoricoCaret::paint(muse::draw::Painter* painter)
{
    INotationPtr notation = globalContext()->currentNotation();
    INotationNoteInputPtr ni = notation ? notation->interaction()->noteInput() : nullptr;
    if (!ni || !ni->isNoteInputMode() || ni->usingNoteInputMethod(NoteInputMethod::BY_DURATION)) {
        return false; // input by duration has MuseScore's own ruler
    }

    const NoteInputState& state = ni->state();
    if (!state.isValid() || state.beyondScore() || !state.staff()) {
        return false;
    }
    const StaffType* staffType = state.staff()->staffType(state.tick());
    const Measure* m = state.segment()->measure();
    const RectF cursor = ni->cursorRect();
    if (staffType->isTabStaff() || !m->system() || cursor.isNull()) {
        return false; // ponytail: TAB keeps MuseScore's caret with its string marks
    }

    // tick -> x: linear between the bar's chord/rest positions; same page coordinates as cursorRect().
    std::vector<std::pair<int, double> > xs;
    for (const Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
        xs.emplace_back(s->tick().ticks(), s->pagePos().x());
    }
    xs.emplace_back(m->endTick().ticks(), m->pagePos().x() + m->width());
    auto xAt = [&xs](int t) {
        auto b = std::lower_bound(xs.begin(), xs.end(), t, [](const auto& p, int v) { return p.first < v; });
        if (b == xs.begin() || b == xs.end() || b->first == t) {
            return b == xs.end() ? xs.back().second : b->second;
        }
        auto a = std::prev(b);
        return a->second + (b->second - a->second) * (t - a->first) / double(b->first - a->first);
    };

    const double sp = staffType->spatium();
    const double nudge = 0.3 * sp; // the caret sits just left of the noteheads
    const double rulerBottom = cursor.top() - sp;
    const Color color = notationConfiguration()->selectionColor(state.voice());

    // Ruler: one tick per grid position, taller on the beats.
    const int barStart = m->tick().ticks();
    const int beat = ticks_beat(m->timesig().denominator());
    const int grid = gridTicks(m_gridDenominator);
    for (int t = barStart; t < m->endTick().ticks(); t += grid) {
        const double h = (t - barStart) % beat == 0 ? sp : 0.5 * sp;
        painter->fillRect(RectF(xAt(t) - nudge, rulerBottom - h, sp / 6, h), color);
    }

    // Caret: a line from above the ruler down through the staff.
    const double top = rulerBottom - 1.5 * sp;
    painter->fillRect(RectF(xAt(caretTick().ticks()) - nudge, top, sp / 4, cursor.bottom() - top), color);
    return true;
}
