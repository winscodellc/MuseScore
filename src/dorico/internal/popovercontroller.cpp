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

#include "popovercontroller.h"

#include <algorithm>

#include "engraving/dom/measure.h"
#include "engraving/dom/system.h"

#include "notation/inotation.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"
#include "notationscene/notationcommands.h"

#include "../doricocommands.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;
using muse::RectF;

void PopoverController::init()
{
    for (const muse::rcommand::Command& command : DORICO_POPOVER_COMMANDS) {
        const PopoverDef* def = m_registry.find(command.pathSegments().back());
        dispatcher()->onRequest(this, command, [this, def]() {
            if (canOpen()) {
                open(def, std::string());
            }
            return muse::make_ok();
        });
    }

    // Return on a selected text item (dynamic, tempo, ...) never reaches command://dorico/return: the score view claims
    // the key and dispatches screen-edit-text. A popover owns that item, so reopen the popover instead of the text editor.
    dispatcher()->preDispatch().onReceive(this, [this](const muse::rcommand::Command& command, bool* allowDispatch) {
        if (command == mu::notation::SCREEN_EDIT_TEXT_COMMAND && openForSelection()) {
            *allowDispatch = false;
        }
    });

    globalContext()->currentNotationChanged().onNotify(this, [this]() { cancel(); });
    modes()->modeChanged().onNotify(this, [this]() { cancel(); });
}

const PopoverDef* PopoverController::current() const
{
    return m_current;
}

std::string PopoverController::initialText() const
{
    return m_initialText;
}

RectF PopoverController::anchorRect() const
{
    return m_anchor;
}

muse::async::Notification PopoverController::stateChanged() const
{
    return m_stateChanged;
}

// Dorico: Write mode only, during note input or with something selected.
bool PopoverController::canOpen() const
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation || modes()->mode() != mu::dorico::Mode::Write || notationController()->isTextEditing()
        || notationController()->isLyricsEditing()) {
        return false;
    }
    return notation->interaction()->noteInput()->isNoteInputMode() || !notation->interaction()->selection()->isNone();
}

bool PopoverController::openForSelection()
{
    if (!canOpen()) {
        return false;
    }
    INotationInteractionPtr interaction = globalContext()->currentNotation()->interaction();
    const EngravingItem* item = interaction->selection()->element();
    if (!item || interaction->noteInput()->isNoteInputMode()) {
        return false;
    }
    const PopoverDef* def = m_registry.findByType(item->type());
    if (!def) {
        return false;
    }
    open(def, def->prefill(item));
    return true;
}

void PopoverController::open(const PopoverDef* def, const std::string& text)
{
    m_current = def; // replaces any open popover: Dorico shows one at a time
    m_initialText = text;
    m_anchor = computeAnchor();
    m_stateChanged.notify();
}

// The caret, or the earliest selected item, with the top raised to its staff's top line so the box sits above the staff.
RectF PopoverController::computeAnchor() const
{
    INotationInteractionPtr interaction = globalContext()->currentNotation()->interaction();
    if (interaction->noteInput()->isNoteInputMode()) {
        return interaction->noteInput()->cursorRect();
    }

    INotationSelectionPtr selection = interaction->selection();
    const EngravingItem* first = nullptr;
    for (const EngravingItem* e : selection->elements()) {
        if (!first || e->staffIdx() < first->staffIdx() || (e->staffIdx() == first->staffIdx() && e->tick() < first->tick())) {
            first = e;
        }
    }

    RectF rect = selection->canvasBoundingRect();
    if (rect.isEmpty() && first) { // a list of several items: anchor at the earliest one
        rect = first->canvasBoundingRect();
    }
    const Measure* measure = first ? first->findMeasure() : nullptr;
    const System* system = measure ? measure->system() : nullptr;
    if (system && system->staff(first->staffIdx())) {
        rect.setTop(std::min(rect.top(), system->staffCanvasYpage(first->staffIdx())));
    }
    return rect;
}

ParseStatus PopoverController::validate(const std::string& text) const
{
    return m_current ? m_current->validate(text) : ParseStatus::Invalid;
}

bool PopoverController::accept(const std::string& text, ApplyScope scope)
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!m_current || !notation || text.empty()) { // our choice: Return in an empty popover closes it
        cancel();
        return true;
    }
    if (m_current->validate(text) != ParseStatus::Ok) {
        return false;
    }

    // One undo step per Return, however many transactions the handler makes.
    INotationUndoStackPtr undoStack = notation->undoStack();
    const size_t start = undoStack->currentStateIndex();
    if (!m_current->apply(text, m_current->supportsLocal ? scope : ApplyScope::Default, notation)) {
        return false;
    }
    if (undoStack->currentStateIndex() > start + 1) {
        undoStack->mergeTransactions(start);
    }

    cancel();
    return true;
}

void PopoverController::cancel()
{
    if (m_current) {
        m_current = nullptr;
        m_stateChanged.notify();
    }
}
