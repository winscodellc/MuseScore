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

#include "engraving/dom/chordrest.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/segment.h"
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

namespace {
// The one item the selection stands for. Clicking a time or key signature selects it on every staff: that list of
// same-type items at one position counts as one item (the top staff's).
EngravingItem* singleSelectedItem(const INotationSelectionPtr& selection)
{
    if (EngravingItem* e = selection->element()) {
        return e;
    }
    EngravingItem* first = nullptr;
    for (EngravingItem* e : selection->elements()) {
        if (first && (e->type() != first->type() || e->tick() != first->tick())) {
            return nullptr;
        }
        if (!first || e->staffIdx() < first->staffIdx()) {
            first = e;
        }
    }
    return first;
}
}

void PopoverController::init()
{
    for (const muse::rcommand::Command& command : DORICO_POPOVER_COMMANDS) {
        const PopoverDef* def = m_registry.find(command.pathSegments().back());
        dispatcher()->onRequest(this, command, [this, def]() {
            if (canOpen()) {
                open(def, existingItem(def));
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

    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        cancel();
        subscribeNoteInput();
    });
    subscribeNoteInput();
    modes()->modeChanged().onNotify(this, [this]() { cancel(); });
}

// Open-ended items (p< at the caret) are closed when note input stops.
void PopoverController::subscribeNoteInput()
{
    if (m_noteInput) {
        m_noteInput->noteInputEnded().disconnect(this);
    }
    INotationPtr notation = globalContext()->currentNotation();
    m_noteInput = notation ? notation->interaction()->noteInput() : nullptr;
    if (!m_noteInput) {
        return;
    }
    m_noteInput->noteInputEnded().onNotify(this, [this]() {
        INotationPtr notation = globalContext()->currentNotation();
        for (const PopoverDef& def : m_registry.defs()) {
            if (notation && def.noteInputEnded) {
                def.noteInputEnded(notation);
            }
        }
    });
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
    EngravingItem* item = singleSelectedItem(interaction->selection());
    if (!item || interaction->noteInput()->isNoteInputMode()) {
        return false;
    }
    const PopoverDef* def = m_registry.findByType(item->type());
    if (!def) {
        return false;
    }
    open(def, item);
    return true;
}

// The item the popover would edit: the selected item itself, or one of its type already attached to the selected
// note/rest (e.g. Shift-D on a note that has a dynamic). Entering text then replaces it instead of adding a second one.
// ponytail: only looks at the segment's annotations (dynamics, tempo, chord symbols, text); specs whose items live
// elsewhere (clefs, key signatures, fingering) extend this when they need it.
EngravingItem* PopoverController::existingItem(const PopoverDef* def) const
{
    INotationInteractionPtr interaction = globalContext()->currentNotation()->interaction();
    EngravingItem* selected = singleSelectedItem(interaction->selection());
    if (!def || !selected || interaction->noteInput()->isNoteInputMode()) {
        return nullptr;
    }

    auto owned = [def](const EngravingItem* e) {
        return std::find(def->itemTypes.begin(), def->itemTypes.end(), e->type()) != def->itemTypes.end();
    };
    if (owned(selected)) {
        return selected;
    }
    if (!def->editsAttached) {
        return nullptr;
    }

    const EngravingItem* cr = selected->isNote() ? toNote(selected)->chord() : selected;
    if (!cr->isChordRest()) {
        return nullptr;
    }
    for (EngravingItem* e : toChordRest(cr)->segment()->annotations()) {
        if (e->staffIdx() == cr->staffIdx() && owned(e)) {
            return e;
        }
    }
    return nullptr;
}

void PopoverController::open(const PopoverDef* def, EngravingItem* editing)
{
    m_current = def; // replaces any open popover: Dorico shows one at a time
    m_editing = editing;
    m_initialText = def && editing ? def->prefill(editing) : std::string();
    if (def && !editing && def->selectionPrefill) {
        m_initialText = def->selectionPrefill(globalContext()->currentNotation()); // tuplets: a ratio for the selected notes
    }
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
    // Handlers act on the selection; selecting the edited item makes the entry replace it.
    if (m_editing && notation->interaction()->selection()->element() != m_editing) {
        notation->interaction()->select({ m_editing });
    }

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
    m_editing = nullptr;
    if (m_current) {
        m_current = nullptr;
        m_stateChanged.notify();
    }
}
