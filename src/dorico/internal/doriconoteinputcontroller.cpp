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

#include "doriconoteinputcontroller.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <optional>
#include <set>

#include "settings.h"

#include "engraving/dom/chord.h"
#include "engraving/dom/input.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/instrument.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/pitchspelling.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/editing/edittimesig.h"
#include "engraving/editing/noteinput.h"
#include "engraving/editing/transaction/transaction.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"
#include "notationscene/notationcommands.h"
#include "playback/playbackcommands.h"

#include "../doricocommands.h"

#include "log.h"

using namespace mu::dorico;
using namespace mu::notation;
using namespace muse;
using namespace muse::rcommand;

namespace {
const Settings::Key DOT_TAP_MS_KEY("dorico", "dorico/dotTapMs");

// Index = digit - 1.
const std::array<Command, 9> NOTATION_DURATION_COMMANDS = {
    SET_DURATION_128TH_COMMAND, SET_DURATION_64TH_COMMAND, SET_DURATION_32ND_COMMAND, SET_DURATION_16TH_COMMAND,
    SET_DURATION_EIGHTH_COMMAND, SET_DURATION_QUARTER_COMMAND, SET_DURATION_HALF_COMMAND, SET_DURATION_WHOLE_COMMAND,
    SET_DURATION_BREVE_COMMAND
};

const std::array<Command, 4> USE_VOICE_COMMANDS = {
    USE_VOICE_1_COMMAND, USE_VOICE_2_COMMAND, USE_VOICE_3_COMMAND, USE_VOICE_4_COMMAND
};

const std::array<Command, 7> ENTER_NOTE_COMMANDS = {
    ENTER_NOTE_C_COMMAND, ENTER_NOTE_D_COMMAND, ENTER_NOTE_E_COMMAND, ENTER_NOTE_F_COMMAND,
    ENTER_NOTE_G_COMMAND, ENTER_NOTE_A_COMMAND, ENTER_NOTE_B_COMMAND
};

const std::array<Command, 7> ADD_NOTE_COMMANDS = {
    ADD_NOTE_C_COMMAND, ADD_NOTE_D_COMMAND, ADD_NOTE_E_COMMAND, ADD_NOTE_F_COMMAND,
    ADD_NOTE_G_COMMAND, ADD_NOTE_A_COMMAND, ADD_NOTE_B_COMMAND
};

constexpr int NOTE_F = 3;
constexpr size_t VOICE_COUNT = 4;

int64_t nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// Same convention as engraving's NoteInput::resolveNoteInputParams: octave = pitch / 12 (middle C = 5).
int stepOf(int epitch, int tpc)
{
    return ((epitch - static_cast<int>(mu::engraving::tpc2alter(tpc))) / 12) * 7 + mu::engraving::tpc2step(tpc);
}
}

void DoricoNoteInputController::init()
{
    settings()->setDefaultValue(DOT_TAP_MS_KEY, Val(400));

    reg(DORICO_RETURN_COMMAND, [this]() { onReturn(); });
    reg(DORICO_SPACE_COMMAND, [this]() { onSpace(); });
    reg(DORICO_REST_COMMAND, [this]() { onRest(); });
    reg(DORICO_TOGGLE_REST_INPUT_COMMAND, [this]() { onToggleRestInput(); });
    reg(DORICO_TOGGLE_CHORD_MODE_COMMAND, [this]() { onToggleChordMode(); });
    reg(DORICO_TOGGLE_INSERT_MODE_COMMAND, [this]() { onToggleInsertMode(); });
    reg(DORICO_CYCLE_DOTS_COMMAND, [this]() { onCycleDots(); });
    reg(DORICO_NEXT_VOICE_COMMAND, [this]() { onNextVoice(); });
    reg(DORICO_NEW_VOICE_COMMAND, [this]() { onNewVoice(); });
    reg(DORICO_CARET_LEFT_COMMAND, [this]() { onCaretStep(false); });
    reg(DORICO_CARET_RIGHT_COMMAND, [this]() { onCaretStep(true); });
    reg(DORICO_GRID_FINER_COMMAND, [this]() { caret()->finerGrid(); });
    reg(DORICO_GRID_COARSER_COMMAND, [this]() { caret()->coarserGrid(); });
    reg(DORICO_SELECT_UP_COMMAND, [this]() { onVertical(true, false); });
    reg(DORICO_SELECT_DOWN_COMMAND, [this]() { onVertical(false, false); });
    reg(DORICO_SELECT_TOP_STAFF_COMMAND, [this]() { onVertical(true, true); });
    reg(DORICO_SELECT_BOTTOM_STAFF_COMMAND, [this]() { onVertical(false, true); });
    reg(DORICO_RESPELL_ABOVE_COMMAND, [this]() { onRespell(true); });
    reg(DORICO_RESPELL_BELOW_COMMAND, [this]() { onRespell(false); });

    for (int i = 0; i < 7; ++i) {
        reg(DORICO_PITCH_COMMANDS[i], [this, i]() { onPitch(i); });
        reg(DORICO_PITCH_ABOVE_COMMANDS[i], [this, i]() { onForcedOctave(i, true); });
        reg(DORICO_PITCH_BELOW_COMMANDS[i], [this, i]() { onForcedOctave(i, false); });
    }
    for (int i = 0; i < 9; ++i) {
        reg(DORICO_SET_DURATION_COMMANDS[i], [this, i]() { onSetDuration(i + 1); });
    }

    using mu::engraving::SymId;
    reg(DORICO_TOGGLE_STRESSED_COMMAND, [this]() { onToggleArticulation(SymId::articStressAbove); });
    reg(DORICO_TOGGLE_STACCATISSIMO_COMMAND, [this]() { onToggleArticulation(SymId::articStaccatissimoAbove); });
    reg(DORICO_TOGGLE_UNSTRESSED_COMMAND, [this]() { onToggleArticulation(SymId::articUnstressAbove); });
    reg(DORICO_TOGGLE_TENUTO_STACCATO_COMMAND, [this]() { onToggleArticulation(SymId::articTenutoStaccatoAbove); });

    globalContext()->currentNotationChanged().onNotify(this, [this]() { onNotationChanged(); });
    onNotationChanged();
}

void DoricoNoteInputController::reg(const Command& command, std::function<void()> handler)
{
    dispatcher()->onRequest(this, command, [handler]() {
        handler();
        return muse::make_ok();
    });
}

INotationPtr DoricoNoteInputController::currentNotation() const
{
    return globalContext()->currentNotation();
}

INotationNoteInputPtr DoricoNoteInputController::noteInput() const
{
    INotationPtr notation = currentNotation();
    return notation ? notation->interaction()->noteInput() : nullptr;
}

bool DoricoNoteInputController::inNoteInput() const
{
    INotationNoteInputPtr ni = noteInput();
    return ni && ni->isNoteInputMode();
}

// ---- Insert-mode flag <-> MuseScore's TIMEWISE method ----

void DoricoNoteInputController::onNotationChanged()
{
    if (m_subscribedInput) {
        m_subscribedInput->noteInputStarted().disconnect(this);
        m_subscribedInput->noteInputEnded().disconnect(this);
        m_subscribedInput->stateChanged().disconnect(this);
        m_subscribedInput->noteAdded().disconnect(this);
    }

    m_subscribedInput = noteInput();
    m_chordPending = false;
    m_dotTap.reset();
    if (!m_subscribedInput) {
        return;
    }

    m_subscribedInput->noteInputStarted().onReceive(this, [this](bool) {
        // The Insert toggle survives across note-input sessions, like Dorico's toolbox toggle.
        INotationNoteInputPtr ni = noteInput();
        if (ni && modes()->isInsertMode() && !ni->usingNoteInputMethod(NoteInputMethod::TIMEWISE)) {
            ni->setNoteInputMethod(NoteInputMethod::TIMEWISE);
        }
    });
    m_subscribedInput->noteInputEnded().onNotify(this, [this]() {
        m_chordPending = false;
        m_dotTap.reset();
    });
    m_subscribedInput->stateChanged().onNotify(this, [this]() { syncInsertModeFromInput(); });
    m_subscribedInput->noteAdded().onNotify(this, [this]() { onNoteAdded(); });
}

// ---- After a note is entered (synchronous, before MuseScore redraws the caret) ----

void DoricoNoteInputController::onNoteAdded()
{
    INotationNoteInputPtr ni = noteInput();
    if (!ni || !ni->isNoteInputMode()) {
        return;
    }

    if (ni->usingNoteInputMethod(NoteInputMethod::TIMEWISE)) {
        rebarAfterInsert();
    } else if (modes()->isChordMode() && m_chordPending && !ni->usingNoteInputMethod(NoteInputMethod::BY_DURATION)) {
        holdCaretOnChord();
    }
}

void DoricoNoteInputController::holdCaretOnChord()
{
    // MuseScore moved the input position past the new note; Dorico's caret stays on the chord until Space.
    mu::engraving::Score* score = currentNotation()->elements()->msScore();
    const mu::engraving::EngravingItem* el = score->selection().element();
    if (el && el->isNote()) {
        score->inputState().setSegment(mu::engraving::toNote(el)->chord()->segment());
    }
}

void DoricoNoteInputController::closeChord()
{
    if (!m_chordPending) {
        return;
    }
    m_chordPending = false;

    INotationNoteInputPtr ni = noteInput();
    if (!ni || !ni->isNoteInputMode()) {
        return;
    }

    mu::engraving::InputState& is = currentNotation()->elements()->msScore()->inputState();
    is.moveToNextInputPos();
    ni->setCurrentTrack(is.track()); // no-op change that makes MuseScore redraw the caret
}

void DoricoNoteInputController::rebarAfterInsert()
{
    // MuseScore's Insert (TIMEWISE) lengthens the bar; Dorico pushes the music on into the next bars. Re-bar from the
    // lengthened bar up to the next time signature, like a time signature change does, as part of the same undo step.
    // ponytail: every staff is pushed (Dorico's Global scope, not Voice), and a pick-up bar that grows past its time
    // signature gets re-barred too.
    INotationPtr notation = currentNotation();
    mu::engraving::Score* score = notation->elements()->msScore();
    const mu::engraving::EngravingItem* el = score->selection().element();
    mu::engraving::Measure* m = el ? score->tick2measure(el->tick()) : nullptr;
    if (!m || m->ticks() <= m->timesig()) {
        return;
    }

    INotationUndoStackPtr undoStack = notation->undoStack();
    const size_t beforeInsert = undoStack->currentStateIndex() - 1; // the note entry was one transaction
    const mu::engraving::track_idx_t track = score->inputState().track();

    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Insert note"));
    if (!mu::engraving::EditTimeSig::rewriteMeasures(score->transactionManager()->currentOrDummyTransaction(), score, m,
                                                     m->timesig(), muse::nidx)) {
        undoStack->rollbackChanges(); // e.g. a local time signature: keep MuseScore's longer bar
        return;
    }
    undoStack->commitChanges();
    undoStack->mergeTransactions(beforeInsert);

    // The entered note was rebuilt: select the note or rest before the caret again, as after any note entry.
    const mu::engraving::Fraction caret = score->inputState().tick();
    if (mu::engraving::ChordRest* cr = score->findCR(caret - mu::engraving::Fraction::eps(), track)) {
        mu::engraving::EngravingItem* sel = cr->isChord() ? static_cast<mu::engraving::EngravingItem*>(mu::engraving::toChord(cr)->upNote()) : cr;
        notation->interaction()->select({ sel });
    }
}

void DoricoNoteInputController::syncInsertModeFromInput()
{
    // K, L, ... replaced TIMEWISE (or the user switched it on from the menu): keep the flag truthful.
    INotationNoteInputPtr ni = noteInput();
    if (ni && ni->isNoteInputMode()) {
        modes()->setInsertMode(ni->usingNoteInputMethod(NoteInputMethod::TIMEWISE));
    }
}

// ---- Handlers ----

void DoricoNoteInputController::onReturn()
{
    INotationNoteInputPtr ni = noteInput();
    if (!ni) {
        return;
    }

    if (ni->isNoteInputMode()) {
        ni->endNoteInput();
    } else if (notationController()->isNoteOrRestSelected()) {
        dispatcher()->dispatch(TOGGLE_NOTE_INPUT_COMMAND);
    } else {
        popovers()->openForSelection(); // e.g. a selected clef or key signature
    }
}

void DoricoNoteInputController::onSpace()
{
    INotationNoteInputPtr ni = noteInput();
    if (!ni || !ni->isNoteInputMode()) {
        dispatcher()->dispatch(mu::playback::PLAY_TOGGLE_COMMAND);
        return;
    }

    m_dotTap.reset();
    if (m_chordPending) {
        closeChord(); // advances past the chord
        return;
    }

    enterAtCaret([this, ni]() {
        mu::engraving::ChordRest* cr = ni->state().cr();
        dispatcher()->dispatch(cr && cr->isChord() ? MOVE_RIGHT_COMMAND : ENTER_REST_COMMAND);
    });
}

void DoricoNoteInputController::onPitch(int note)
{
    INotationNoteInputPtr ni = noteInput();
    if (!ni) {
        return;
    }

    if (!ni->isNoteInputMode()) {
        if (note == NOTE_F) {
            dispatcher()->dispatch(FLIP_COMMAND);
        }
        return;
    }

    m_dotTap.reset();

    enterAtCaret([this, ni, note]() {
        const bool byDuration = ni->usingNoteInputMethod(NoteInputMethod::BY_DURATION);
        if (ni->state().rest() && !byDuration) {
            dispatcher()->dispatch(ENTER_REST_COMMAND);
            return;
        }

        if (modes()->isChordMode() && !byDuration) {
            dispatcher()->dispatch(m_chordPending ? ADD_NOTE_COMMANDS[note] : ENTER_NOTE_COMMANDS[note]);
            m_chordPending = true;
            return;
        }

        dispatcher()->dispatch(ENTER_NOTE_COMMANDS[note]);
    });
}

std::optional<int> DoricoNoteInputController::referenceStep(bool above) const
{
    INotationNoteInputPtr ni = noteInput();

    if (ni->usingNoteInputMethod(NoteInputMethod::BY_DURATION) && !ni->state().notes().empty()) {
        const mu::engraving::NoteVal& nv = ni->state().notes().front();
        if (nv.tpc1 != mu::engraving::Tpc::TPC_INVALID) {
            return stepOf(nv.pitch, nv.tpc1);
        }
        return std::nullopt;
    }

    mu::engraving::EngravingItem* el = currentNotation()->interaction()->selection()->element();
    if (!el || !el->isNote()) {
        return std::nullopt;
    }

    const mu::engraving::Note* note = mu::engraving::toNote(el);
    if (modes()->isChordMode() && m_chordPending) {
        const mu::engraving::Chord* chord = note->chord();
        note = above ? chord->upNote() : chord->downNote();
    }
    return stepOf(note->epitch(), note->tpc());
}

void DoricoNoteInputController::onForcedOctave(int note, bool above)
{
    INotationNoteInputPtr ni = noteInput();
    if (!ni || !ni->isNoteInputMode()) {
        return;
    }

    m_dotTap.reset();

    NoteInputParams params;
    if (!mu::engraving::NoteInput::resolveNoteInputParams(currentNotation()->elements()->msScore(), note, false, params)) {
        return;
    }

    // No previous note: keep engraving's nearest-octave choice.
    if (std::optional<int> ref = referenceStep(above)) {
        params.step = above ? stepAbove(*ref, note) : stepBelow(*ref, note);
    }

    if (ni->usingNoteInputMethod(NoteInputMethod::BY_DURATION)) {
        ni->setRestMode(false);
        ni->setInputNote(params);
        return;
    }

    enterAtCaret([this, ni, params]() {
        // Set before addNote: its noteAdded notification holds the caret on the chord.
        const bool addToChord = modes()->isChordMode() && m_chordPending;
        m_chordPending = modes()->isChordMode();
        ni->addNote(params, addToChord ? NoteAddingMode::CurrentChord : NoteAddingMode::NextChord);
    });
}

void DoricoNoteInputController::onRest()
{
    if (inNoteInput()) {
        m_dotTap.reset();
        enterAtCaret([this]() { dispatcher()->dispatch(ENTER_REST_COMMAND); });
    }
}

void DoricoNoteInputController::onToggleRestInput()
{
    // Dorico's comma only switches rest input; letters and Y then enter rests (onPitch / onRest).
    INotationNoteInputPtr ni = noteInput();
    if (ni && ni->isNoteInputMode()) {
        ni->setRestMode(!ni->state().rest());
    }
}

void DoricoNoteInputController::onToggleChordMode()
{
    closeChord();
    modes()->setChordMode(!modes()->isChordMode());

    // Chord excludes Insert, so leave TIMEWISE if it was on.
    INotationNoteInputPtr ni = noteInput();
    if (ni && ni->isNoteInputMode() && modes()->isChordMode() && ni->usingNoteInputMethod(NoteInputMethod::TIMEWISE)) {
        ni->setNoteInputMethod(NoteInputMethod::BY_NOTE_NAME);
    }
}

void DoricoNoteInputController::onToggleInsertMode()
{
    closeChord();
    modes()->setInsertMode(!modes()->isInsertMode());

    INotationNoteInputPtr ni = noteInput();
    if (ni && ni->isNoteInputMode()) {
        ni->setNoteInputMethod(modes()->isInsertMode() ? NoteInputMethod::TIMEWISE : NoteInputMethod::BY_NOTE_NAME);
        // setNoteInputMethod refuses unavailable methods; make the flag match what really happened.
        modes()->setInsertMode(ni->usingNoteInputMethod(NoteInputMethod::TIMEWISE));
    }
}

void DoricoNoteInputController::onSetDuration(int digit)
{
    closeChord();

    const bool dot = m_dotTap.press(digit, nowMs(), settings()->value(DOT_TAP_MS_KEY).toInt());
    dispatcher()->dispatch(dot ? TOGGLE_DOT_COMMAND : NOTATION_DURATION_COMMANDS[digit - 1]);
}

void DoricoNoteInputController::onCycleDots()
{
    const int dots = notationController()->currentDotCount();

    // toggle-dotN sets N dots; at 4 the same command toggles them off again.
    static const std::array<Command, 4> DOT_COMMANDS = { TOGGLE_DOT_COMMAND, TOGGLE_DOT2_COMMAND, TOGGLE_DOT3_COMMAND, TOGGLE_DOT4_COMMAND };
    dispatcher()->dispatch(DOT_COMMANDS[std::min(dots, 3)]);
}

bool DoricoNoteInputController::voiceHasContent(size_t staffIdx, size_t voice) const
{
    if (voice == 0) {
        return true;
    }

    // ponytail: scans the whole score per key press; fine for typical scores, cache per staff if it ever lags.
    const mu::engraving::Score* score = currentNotation()->elements()->msScore();
    const mu::engraving::track_idx_t track = staffIdx * VOICE_COUNT + voice;
    for (const mu::engraving::Segment* s = score->firstSegment(mu::engraving::SegmentType::ChordRest); s;
         s = s->next1(mu::engraving::SegmentType::ChordRest)) {
        if (s->element(track)) {
            return true;
        }
    }
    return false;
}

void DoricoNoteInputController::switchToVoice(size_t voice)
{
    dispatcher()->dispatch(USE_VOICE_COMMANDS[voice]);
}

void DoricoNoteInputController::onNextVoice()
{
    INotationNoteInputPtr ni = noteInput();
    if (!ni) {
        return;
    }

    size_t staffIdx = 0;
    if (ni->isNoteInputMode()) {
        staffIdx = ni->state().staffIdx();
    } else if (mu::engraving::EngravingItem* el = currentNotation()->interaction()->selection()->element()) {
        staffIdx = el->staffIdx();
    } else {
        return;
    }

    const size_t current = notationController()->currentVoice();
    for (size_t i = 1; i <= VOICE_COUNT; ++i) {
        const size_t v = (current + i) % VOICE_COUNT;
        if (voiceHasContent(staffIdx, v)) {
            switchToVoice(v);
            return;
        }
    }
}

void DoricoNoteInputController::onNewVoice()
{
    INotationNoteInputPtr ni = noteInput();
    if (!ni) {
        return;
    }

    size_t staffIdx = 0;
    if (ni->isNoteInputMode()) {
        staffIdx = ni->state().staffIdx();
    } else if (mu::engraving::EngravingItem* el = currentNotation()->interaction()->selection()->element()) {
        staffIdx = el->staffIdx();
    } else {
        return;
    }

    for (size_t v = 1; v < VOICE_COUNT; ++v) {
        if (!voiceHasContent(staffIdx, v)) {
            switchToVoice(v);
            return;
        }
    }
    toastService()->showInfo("Voices", "All four voices on this staff are in use");
}

void DoricoNoteInputController::onToggleArticulation(mu::engraving::SymId sym)
{
    INotationPtr notation = currentNotation();
    if (!notation) {
        return;
    }

    INotationInteractionPtr interaction = notation->interaction();
    INotationNoteInputPtr ni = interaction->noteInput();

    // Mirrors NotationActionController::toggleArticulation: latch for the next notes during input,
    // otherwise toggle on the selection. With nothing selected and no input we do nothing.
    if (ni->isNoteInputMode() && notationConfiguration()->addAccidentalDotsArticulationsToNextNoteEntered()) {
        ni->setArticulation(sym);
    } else if (!interaction->selection()->isNone()) {
        interaction->toggleArticulationForSelection(sym);
    }
}

void DoricoNoteInputController::onCaretStep(bool right)
{
    // Moving the caret ends a chord in progress, like Space.
    m_chordPending = false;
    m_dotTap.reset();
    if (!caret()->step(right)) {
        dispatcher()->dispatch(right ? MOVE_RIGHT_COMMAND : MOVE_LEFT_COMMAND);
    }
}

void DoricoNoteInputController::onVertical(bool up, bool toEnd)
{
    // Moving the caret ends a chord in progress, like Space.
    m_chordPending = false;
    m_dotTap.reset();
    caret()->navigateVertically(up, toEnd);
}

void DoricoNoteInputController::onRespell(bool above)
{
    INotationPtr notation = currentNotation();
    if (!notation) {
        return;
    }

    using namespace mu::engraving;
    const std::vector<Note*> notes = notation->interaction()->selection()->notes();
    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(TranslatableString("undoableAction", "Respell notes"));
    bool changed = false;
    std::set<const Note*> done; // a tie chain is respelled once, even when several of its notes are selected
    for (Note* note : notes) {
        if (done.count(note) || note->staff()->isTabStaff(note->tick()) || note->part()->instrument(note->tick())->useDrumset()) {
            continue;
        }
        const int tpc = respellTpc(note->tpc(), above);
        if (tpc == note->tpc()) {
            continue;
        }
        // Set the shown spelling and keep the other one (concert / written) consistent, like MuseScore's enharmonic
        // change does; tied notes follow.
        std::vector<Note*> tied = note->tiedNotes();
        if (tied.empty()) {
            tied.push_back(note);
        }
        for (Note* n : tied) {
            done.insert(n);
            n->undoSetTpc(tpc);
            n->undoChangeProperty(n->concertPitch() ? Pid::TPC2 : Pid::TPC1, n->transposeTpc(tpc));
        }
        changed = true;
    }
    if (changed) {
        undoStack->commitChanges();
        notation->notationChanged().send(muse::RectF());
    } else {
        undoStack->rollbackChanges();
    }
}

void DoricoNoteInputController::enterAtCaret(const std::function<void()>& enter)
{
    INotationUndoStackPtr undoStack = currentNotation()->undoStack();
    const size_t before = undoStack->currentStateIndex();
    const bool cut = caret()->materialize();
    enter();
    if (cut && undoStack->currentStateIndex() > before + 1) {
        undoStack->mergeTransactions(before);
    }
}
