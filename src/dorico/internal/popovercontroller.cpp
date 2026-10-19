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
#include "engraving/dom/figuredbass.h"
#include "engraving/dom/input.h"
#include "engraving/dom/lyrics.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/system.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"
#include "notationscene/notationcommands.h"

#include "../doricocommands.h"
#include "../popover/popovernavigation.h"

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
        // Clicking a figured bass selects one of its lines; the popover edits the whole figured bass.
        return e->isFiguredBassItem() ? const_cast<FiguredBass*>(toFiguredBassItem(e)->figuredBass()) : e;
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

// The top staff's earliest selected item.
const EngravingItem* earliestSelected(const INotationSelectionPtr& selection)
{
    const EngravingItem* first = nullptr;
    for (const EngravingItem* e : selection->elements()) {
        if (!first || e->staffIdx() < first->staffIdx() || (e->staffIdx() == first->staffIdx() && e->tick() < first->tick())) {
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
            if (!canOpen()) {
                return muse::make_ok();
            }
            if (def->openInline) {
                cancel();
                def->openInline(globalContext()->currentNotation());
            } else {
                open(def, existingItem(def));
            }
            return muse::make_ok();
        });
    }

    // Lyrics (spec 33): Return closes the lyric editor; Shift-Up / Shift-Down put the lyric, and the ones typed after
    // it (MuseScore copies the placement on), above or below the staff.
    dispatcher()->onRequest(this, DORICO_LYRICS_CLOSE_COMMAND, [this]() {
        globalContext()->currentNotation()->interaction()->endEditElement();
        return muse::make_ok();
    });
    // Down: MuseScore's next-verse only visits existing lines and its add-verse always adds one; Dorico's goes to the
    // next line, adding it when there is none.
    dispatcher()->onRequest(this, DORICO_LYRICS_NEXT_LINE_COMMAND, [this]() {
        INotationInteractionPtr interaction = globalContext()->currentNotation()->interaction();
        const EngravingItem* e = interaction->selection()->element();
        if (e && e->isLyrics()) {
            const Lyrics* lyric = toLyrics(e);
            if (lyric->verse() < lyric->chordRest()->lastVerse(lyric->placement())) {
                interaction->navigateToLyricsVerse(MoveDirection::Down);
            } else {
                interaction->addLyricsVerse();
            }
        }
        return muse::make_ok();
    });
    for (const auto& [command, placement] : { std::pair { DORICO_LYRICS_ABOVE_COMMAND, PlacementV::ABOVE },
                                              std::pair { DORICO_LYRICS_BELOW_COMMAND, PlacementV::BELOW } }) {
        dispatcher()->onRequest(this, command, [this, placement]() {
            INotationPtr notation = globalContext()->currentNotation();
            EngravingItem* lyric = notation->interaction()->selection()->element();
            if (lyric && lyric->isLyrics() && lyric->placement() != placement) {
                notation->undoStack()->prepareChanges(muse::TranslatableString("undoableAction", "Change lyrics placement"));
                // Unstyled, or the next lyric (which copies the flag) snaps back to the style's placement
                lyric->undoChangeProperty(Pid::PLACEMENT, placement, PropertyFlags::UNSTYLED);
                notation->undoStack()->commitChanges();
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

// The item a navigating popover edits at the position: its chord for popovers that step by chord (fingering), else the
// item of its types.
static EngravingItem* itemOrChordAt(const Score* score, const PopoverPosition& pos, const PopoverDef& def)
{
    return def.stepsByChord ? static_cast<EngravingItem*>(chordAt(score, pos)) : itemAt(score, pos, def.itemTypes);
}

void PopoverController::open(const PopoverDef* def, EngravingItem* editing)
{
    m_current = def; // replaces any open popover: Dorico shows one at a time
    m_editing = editing;
    m_position = def && def->applyAt ? startPosition(editing) : std::nullopt;
    if (m_position) {
        // Dorico: the chord symbols popover stops note input when it opens.
        INotationPtr notation = globalContext()->currentNotation();
        if (notation->interaction()->noteInput()->isNoteInputMode()) {
            notation->interaction()->noteInput()->endNoteInput();
        }
        if (!m_editing) {
            m_editing = itemOrChordAt(notation->elements()->msScore(), *m_position, *def);
        }
    }
    m_initialText = def && m_editing ? def->prefill(m_editing) : std::string();
    if (def && !editing && def->selectionPrefill) {
        m_initialText = def->selectionPrefill(globalContext()->currentNotation()); // tuplets: a ratio for the selected notes
    }
    m_anchor = computeAnchor();
    m_stateChanged.notify();
}

// Where a navigating popover starts: the item it edits, the caret, or the earliest selected item; on voice 1 of its staff.
std::optional<PopoverPosition> PopoverController::startPosition(EngravingItem* editing) const
{
    INotationPtr notation = globalContext()->currentNotation();
    const Score* score = notation->elements()->msScore();
    if (!editing && notation->interaction()->noteInput()->isNoteInputMode()) {
        const InputState& is = score->inputState();
        if (!is.segment() || is.beyondScore()) {
            return std::nullopt;
        }
        return PopoverPosition { is.tick(), m_current->stepsByChord ? is.track() : trackZeroVoice(is.track()) };
    }
    const EngravingItem* e = editing ? editing : earliestSelected(notation->interaction()->selection());
    if (!e) {
        return std::nullopt;
    }
    return PopoverPosition { e->tick(), m_current->stepsByChord ? e->track() : trackZeroVoice(e->track()) };
}

// The caret, or the earliest selected item, with the top raised to its staff's top line so the box sits above the staff.
RectF PopoverController::computeAnchor() const
{
    INotationPtr notation = globalContext()->currentNotation();
    if (m_position) {
        return m_editing ? m_editing->canvasBoundingRect() : positionRect(notation->elements()->msScore(), *m_position);
    }
    INotationInteractionPtr interaction = notation->interaction();
    if (interaction->noteInput()->isNoteInputMode()) {
        return interaction->noteInput()->cursorRect();
    }

    INotationSelectionPtr selection = interaction->selection();
    const EngravingItem* first = earliestSelected(selection);

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
    if (!applyEntry(text, scope)) {
        return false;
    }
    cancel();
    return true;
}

// Space / Tab / Cmd-arrows: an entry that leaves the existing item as it was adds no undo step.
bool PopoverController::move(const std::string& text, PopoverMove move)
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!m_current || !m_position || !notation) {
        return false;
    }
    const bool byChord = m_current->stepsByChord;
    if (byChord && move != PopoverMove::NextBeat && move != PopoverMove::PrevBeat) {
        return false;
    }
    const bool unchanged = text.empty() || (m_editing && text == m_initialText);
    if (!unchanged && !applyEntry(text, ApplyScope::Default)) {
        return false;
    }

    const Score* score = notation->elements()->msScore();
    const std::optional<PopoverPosition> next = byChord ? moveChord(score, *m_position, move == PopoverMove::NextBeat)
                                                        : movePosition(score, *m_position, move, m_current->itemTypes);
    if (next) {
        m_position = next;
    }
    m_editing = itemOrChordAt(score, *m_position, *m_current); // at the end of the score: the entry just made
    m_initialText = m_editing ? m_current->prefill(m_editing) : std::string();
    m_anchor = computeAnchor();
    m_stateChanged.notify();
    return true;
}

bool PopoverController::applyEntry(const std::string& text, ApplyScope scope)
{
    INotationPtr notation = globalContext()->currentNotation();
    if (m_current->validate(text) != ParseStatus::Ok) {
        return false;
    }

    // One undo step per Return, however many transactions the handler makes.
    // Handlers act on the selection; selecting the edited item makes the entry replace it.
    if (!m_position && m_editing && notation->interaction()->selection()->element() != m_editing) {
        notation->interaction()->select({ m_editing });
    }

    INotationUndoStackPtr undoStack = notation->undoStack();
    const size_t start = undoStack->currentStateIndex();
    const bool applied = m_current->applyAt
                         ? m_position && m_current->applyAt(text, *m_position, notation)
                         : m_current->apply(text, m_current->supportsLocal ? scope : ApplyScope::Default, notation);
    if (!applied) {
        return false;
    }
    if (undoStack->currentStateIndex() > start + 1) {
        undoStack->mergeTransactions(start);
    }
    return true;
}

void PopoverController::cancel()
{
    m_editing = nullptr;
    m_position.reset();
    if (m_current) {
        m_current = nullptr;
        m_stateChanged.notify();
    }
}
