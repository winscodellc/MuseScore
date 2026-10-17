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

#include "pendingatcaret.h"

#include "async/async.h"

#include "engraving/dom/chordrest.h"
#include "engraving/dom/input.h"
#include "engraving/dom/score.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

using namespace mu::dorico;
using namespace mu::engraving;

PendingAtCaret::PendingAtCaret(const muse::TranslatableString& undoName)
    : m_undoName(undoName)
{
}

bool PendingAtCaret::add(AddItem item, mu::notation::INotationPtr notation)
{
    const InputState& is = notation->elements()->msScore()->inputState();
    if (!is.segment() || is.beyondScore()) {
        return false;
    }
    if (m_notation != notation || is.tick() != m_tick || is.track() != m_track) {
        cancel();
        m_notation = notation;
        m_tick = is.tick();
        m_track = is.track();
        auto later = [this]() { muse::async::Async::call(this, [this]() { check(); }); };
        notation->notationChanged().onReceive(this, [later](const muse::RectF&) { later(); }); // also rests (Space)
        notation->interaction()->selectionChanged().onNotify(this, later);
        notation->interaction()->noteInput()->stateChanged().onNotify(this, later);
    }
    m_items.push_back(std::move(item));
    return true;
}

void PendingAtCaret::cancel()
{
    if (m_notation) {
        m_notation->notationChanged().disconnect(this);
        m_notation->interaction()->selectionChanged().disconnect(this);
        m_notation->interaction()->noteInput()->stateChanged().disconnect(this);
    }
    m_notation = nullptr;
    m_items.clear();
}

void PendingAtCaret::check()
{
    if (!m_notation || m_items.empty()) {
        return;
    }
    if (!m_notation->interaction()->noteInput()->isNoteInputMode()) {
        cancel();
        return;
    }
    Score* score = m_notation->elements()->msScore();
    const Fraction caret = score->inputState().tick();
    if (caret == m_tick) {
        return; // nothing input yet (or a chord still being built in chord mode)
    }
    ChordRest* cr = score->findCR(m_tick, m_track);
    if (caret < m_tick || !cr || cr->tick() != m_tick) {
        cancel(); // our choice: the caret went back, or nothing was input there
        return;
    }

    const std::vector<AddItem> items = m_items;
    mu::notation::INotationPtr notation = m_notation;
    cancel();
    mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(m_undoName);
    bool changed = false;
    for (const AddItem& item : items) {
        changed = item(score, cr, caret) || changed;
    }
    if (changed) {
        undoStack->commitChanges();
    } else {
        undoStack->rollbackChanges();
    }
}
