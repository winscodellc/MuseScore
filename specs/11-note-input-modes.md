# 11 - Note-input modes (Dorico's note-input model on MuseScore's)
Status: done  -  Depends on: 00, 10 (land with 10)  -  Dorico refs: see list in Dorico behaviour; full text in [reference/dorico-key-commands.md](reference/dorico-key-commands.md) sections A2-A5

## Goal
Make note input *feel* like Dorico: Shift-N / Return / Esc start and stop, letters only mean pitches during input, `Q` chord mode, `I` insert mode, `K` pitch-before-duration, `L` lock duration, `,`/`Y` rests, digits with double-tap dotting, forced octave above/below, voices, latching articulations. Gives back what spec 10 took away (`Shift+letter` add-to-chord is now Chord mode).

## Dorico behaviour
Sources (manual base `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/`): `write_mode/write_mode_caret/write_mode_caret_activating_deactivating_t.html`, `.../write_mode_note_input/write_mode_note_input_inputting_notes_t.html`, `..._inputting_chords_t.html`, `..._chord_mode_c.html`, `..._inputting_insert_mode_t.html`, `..._pitch_before_duration_t.html`, `write_mode/write_mode_transposing_tools/write_mode_notes_pitch_changing_rhythm_locked_t.html`, `write_mode/write_mode_durations/write_mode_duration_selecting_t.html`, `write_mode/write_mode_notations_input/write_mode_articulations_inputting_methods_key_commands_r.html`.

| Key | Behaviour |
|---|---|
| Shift-N | Start note input at the selected item (none selected: earliest position on top staff in view). Again = stop. |
| Return | Note/rest selected: start note input. Notation item selected (dynamic...): open its popover for editing (spec 20). During input: stop. |
| Esc | Stop note input |
| A-G | Input a pitch **only during note input**; octave closest to the previous note; key signature applied. Outside input: nothing (except `F` = flip, spec 10 table). |
| Shift-Opt-A..G | Force the note **above** the previous note (repeat in Pitch Before Duration: next octave up) |
| Ctrl(physical)-A..G | Force the note **below** the previous note; in Chord mode: below the lowest note at the caret |
| `-` `=` `0` | Flat / sharp / natural for the **next** note only; on a selection they change its accidental; pressing the same key again removes it |
| 1-9 | Duration (see 10). Pressing the same digit twice quickly = dotted (6 6 = dotted quarter). With notes selected, changes their duration |
| `.` / Opt-. | Toggle rhythm dot / cycle 1, 2, 3, 4 dots |
| `,` | Toggle rest input; while on, `Y` or any letter A-G enters a rest. `Y` also works as "rest now" |
| Space | Advance the caret by the selected duration; implicit rests fill the gap, existing music is not overwritten |
| Q | Chord mode on/off: caret does not advance, new notes stack **above the highest** note (a repeated letter goes an octave up). Cannot be on together with Insert |
| I | Insert mode on/off (later music is pushed along; delete pulls it back). Cannot be on together with Chord |
| K | Pitch Before Duration: pick pitch (+ accidental, dot, articulation) first; a duration key then commits it (shadow notehead shown) |
| L | Lock Duration: repitch existing notes keeping rhythm; caret jumps note to note, turns off at the last note |
| V / Shift-V | Next existing voice / create a new voice (repeat for more). With notes selected: V cycles their voice, Shift-V moves them to a new voice |
| `{` `}` `"` `\|` | Stressed / staccatissimo / unstressed / tenuto+staccato. Articulations **latch** during input (stay on until toggled off); on a selection they toggle on those notes; incompatible pairs (staccato + staccatissimo) are refused |
| T | During input: tie next note to the previous same-pitch note. On a selection: tie to the next same-pitch note |
| `/` | Toggle grace-note input |
| Preference | "Specify accidental, rhythm dot and articulations: before / after inputting note" (Preferences > Note Input and Editing) |

Mode badges on the caret (orange line): Chord = `+` at top-left; Insert = V and inverted-V marks top and bottom; Lock Duration = dashed caret; Pitch Before Duration = shadow notehead.

## MuseScore today
- **Methods**: `NoteEntryMethod { UNKNOWN, BY_NOTE_NAME, BY_DURATION, REPITCH, RHYTHM, REALTIME_AUTO, REALTIME_MANUAL, TIMEWISE }` (`src/engraving/dom/input.h:55`), exposed as `NoteInputMethod` (`src/notation/types/noteinputtypes.h`). One method at a time.
- **Pitch before duration already exists** as `BY_DURATION`: letters only set the pending pitch (`NotationActionController::doAddNote` -> `setInputNote`, `src/notationscene/internal/notationactioncontroller.cpp:1431-1458`), Up/Down change it (`moveInputNotes`, :2182), the duration key places it. Command `notation/toggle-note-input-by-duration` (registered at :199).
- **Lock duration ~ REPITCH**: `notation/toggle-note-input-repitch` (:201).
- **Insert mode = TIMEWISE**: `notation/toggle-insert-mode` -> `toggleNoteInputInsert` (:205, impl ~:1383).
- **Chord mode** = `NoteAddingMode::CurrentChord` via `add-note-a..g` (previously Shift+letter), `NoteAddingMode { CurrentChord, NextChord, InsertChord }` (`noteinputtypes.h:40`). `INotationNoteInput::addNote(const NoteInputParams&, NoteAddingMode)`.
- **Interface** `src/notation/inotationnoteinput.h:36-83`: `isNoteInputMode`, `state()`, `startNoteInput(method, focus)`, `endNoteInput`, `usingNoteInputMethod`, `setNoteInputMethod`, `addNote`, `setDuration`, `toggleRest`, `toggleDots`, `setRestMode`, `setAccidental`, `setArticulation(SymId)`, `setCurrentVoice`, `setInputNote`, `moveInputNotes`, `noteInputStarted()/noteInputEnded()/stateChanged()`.
- **Enabled state**: note commands are enabled when `isNoteInputActionAllowed()` (:3293). Letters A-G currently auto-start note input (`doAddNote` -> `startNoteInput`).
- **No** "force duration", no implicit-rest advance, no double-tap dot, no double-method (K+I) combos.

## Design

### New code (all under `src/dorico/`)
1. `internal/doricomodestate.h/.cpp` + interface `idoricomodestate.h`: `class IDoricoModeState : MODULE_CONTEXT_INTERFACE { bool isChordMode(); bool isInsertMode(); void setChordMode(bool); void setInsertMode(bool); Notification modesChanged(); }`. Registered per window in `DoricoContext::registerExports` so specs 40 and 43 can read it (`ContextInject<IDoricoModeState>`). Rules: setting Chord clears Insert and vice versa. Flags persist across note-input sessions (like Dorico's toolbox toggles). While note input is active, Insert=true keeps method `TIMEWISE` (resync on `noteInputStarted()` and on toggle); Chord=true makes letters use `NoteAddingMode::CurrentChord`.
2. `internal/doriconoteinputcontroller.h/.cpp`: handlers for the commands below. Dispatch to existing commands through `commandDispatcher()->dispatch(...)` (verify the call name in `muse/framework/rcommand/icommanddispatcher.h`) rather than copying MuseScore logic.
3. `internal/pitchmath.h` (pure, no Qt): `int lineAbove(int prevLine, int stepA2G)` and `int lineBelow(...)`, where line = octave*7 + diatonic step (C=0). Returns the nearest line strictly above/below `prevLine` with the given step. Used for forced octave.

### New commands (`doricocommands.h`; build the 21 letter commands in a loop in `DoricoCommandsRegister`)
| Command | Handler |
|---|---|
| `dorico/return` | Note input active: `endNoteInput()`. Else if selection is a single note/rest: `startNoteInput(BY_NOTE_NAME)`. Else if a popover exists for the selection: call the spec-20 hook (`IDoricoPopoverController::openForSelection()`, optional until 20 lands; no-op before). |
| `dorico/space` | Note input active: if the input position holds a chord, dispatch `notation/move-right` (advance without overwriting); else dispatch `notation/enter-rest` (rest of the selected duration). Not in note input: dispatch `playback/play-toggle`. Scope `PLAYBACK` (so Space still plays when the score is not focused). |
| `dorico/pitch-a..g` | Note input active: rest mode on -> `notation/enter-rest`; chord mode -> `notation/add-note-x`; else `notation/enter-note-x`. Not in note input: `f` -> `notation/flip` (selection), others no-op. |
| `dorico/pitch-above-a..g`, `dorico/pitch-below-a..g` | Note input only. Compute target line with `pitchmath` from `state().lastNote` (previous input note), build `NoteInputParams` with that pitch/tpc, `addNote(params, chord ? CurrentChord : NextChord)`. In Chord mode "above" means above the highest note of the current chord and "below" below the lowest. In BY_DURATION method use `setInputNote(params)`; a repeated press raises/lowers by an octave (re-run with the pending note as `prev`). |
| `dorico/rest` (Y) | Note input only: `notation/enter-rest`. |
| `dorico/toggle-chord-mode` (Q) | `setChordMode(!isChordMode())`, which clears Insert (and sets method back from TIMEWISE to BY_NOTE_NAME). |
| `dorico/toggle-insert-mode` (I) | `setInsertMode(!...)` then dispatch `notation/toggle-insert-mode` only if the method needs to change. |
| `dorico/set-duration-1..9` | Dispatch the matching `notation/set-duration-*` (10's mapping). If the same digit was pressed < `DOT_TAP_MS` (400) ago, instead dispatch `notation/toggle-dot`. Tap state is a tiny struct `{int lastDigit; steady_clock::time_point at;}`; reset when selection/mode changes. Keep `DOT_TAP_MS` in a settings key `dorico/dotTapMs` (tuning knob). |
| `dorico/cycle-dots` (Opt-.) | Read dots from `state().duration()` (note input) or the selected ChordRest; dispatch `notation/toggle-dot`, `-dot2`, `-dot3`, `-dot4` or clear so that 0 -> 1 -> 2 -> 3 -> 4 -> 0. |
| `dorico/next-voice` (V) | Note input: `setCurrentVoice((v+1) % 4)` over voices that exist on the staff (fall back to the next index). Selection: dispatch `notation/use-voice-N` for the next voice. |
| `dorico/new-voice` (Shift-V) | First voice index (0-3) with no content on the staff, `setCurrentVoice` (note input) or `notation/use-voice-N` (selection). Over 4 voices: no-op + toast. |
| `dorico/toggle-articulation-stressed/staccatissimo/unstressed/tenuto-staccato` | Map to `SymId::articStressAbove`, `articStaccatissimoAbove`, `articUnstressAbove`, `articTenutoStaccatoAbove`. Note input: `setArticulation(sym)` toggle in `state().articulationIds`; else `INotationInteraction::toggleArticulation(sym)` (:8367). Refuse staccato + staccatissimo pairs (just let engraving's own rule decide; verify what `toggleArticulation` does). Mirror how `TOGGLE_STACCATO_COMMAND` is registered in `notationactioncontroller.cpp` for the latch behaviour. |

### Reused as-is (bound in this spec, no new handler)
`K` -> `notation/toggle-note-input-by-duration`; `L` -> `notation/toggle-note-input-repitch`; Shift-N, `,`, `T`, `S`, `.` unchanged from 10.

### Adds to `shortcuts_dorico.json` (and updates `dorico_keys.json`)
```
NOTATION: Return,Enter -> dorico/return;  A..G (+H for B) -> dorico/pitch-a..g;
          Alt+Shift+A..G -> dorico/pitch-above-a..g;  Meta+A..G -> dorico/pitch-below-a..g;
          Y -> dorico/rest;  Q -> dorico/toggle-chord-mode;  I -> dorico/toggle-insert-mode;
          K -> notation/toggle-note-input-by-duration;  L -> notation/toggle-note-input-repitch;
          V -> dorico/next-voice;  Shift+V -> dorico/new-voice;
          1..9 and Num+1..9 -> dorico/set-duration-1..9   (replaces 10's rows for the 9 notation/set-duration-* commands: list those commands with sequences [])
          Alt+. -> dorico/cycle-dots;
          { -> dorico/toggle-articulation-stressed;  } -> ...-staccatissimo;  " -> ...-unstressed;  | -> ...-tenuto-staccato
          notation/enter-note-a..g -> []  (now reached only through dorico/pitch-*)
          notation/append-measures -> []  (Alt+Shift+B conflicts with note-above B)
PLAYBACK: Space -> dorico/space
```
Verify while writing: shortcut JSON has no per-entry parameters (hence the many small commands), and `"` / `|` literal sequences parse (else use `Shift+'` / `Shift+\\`).

### Upstream edits
None in this spec (all in `src/dorico/` and `shortcuts_dorico.json`).

### As built (corrections to the design above)
- `ICommandDispatcher::dispatch` is **asynchronous** (queued through `Async::call`), so a handler must not read state that a command it just dispatched changes. Each handler dispatches at most one command and reads state *before* dispatching. Direct `INotationNoteInput` calls are synchronous.
- Dots, voice, rest mode and "is text being edited" come from `INotationCommandsController` (`src/notationscene/inotationcommandscontroller.h`: `currentDotCount()`, `currentVoice()`, `isNoteOrRestSelected()`, `isTextEditing()`), not from `ICommandsState`. All `dorico/*` commands are **disabled while text or lyrics are being edited** (`DoricoCommandsState`), so Space, Return and the letters reach the text editor.
- Chord mode: the first letter after a reset is `enter-note-x`, further letters are `add-note-x` (stack onto the selected chord). MuseScore advances the input position after every note, so the `noteAdded` hook puts the caret back on the chord (`holdCaretOnChord`). `Space`, a duration key and toggling Q/I move the caret past the chord (`closeChord`: `InputState::moveToNextInputPos`); ending input just forgets it. A mouse click elsewhere does not reset it.
- Insert mode: `IDoricoModeState::isInsertMode()` is kept equal to "method is TIMEWISE" while input is active (`DoricoNoteInputController::syncInsertModeFromInput`); starting input with the flag on switches to TIMEWISE.
- Insert mode pushes music into the next bars: MuseScore's TIMEWISE lengthens the bar, so the `noteAdded` hook re-bars from that bar up to the next time signature with `EditTimeSig::rewriteMeasures` (what a time signature change uses), merged into the note's undo step. All staves are pushed (Dorico's Global scope rather than Voice).
- Rests: `,` is `dorico/toggle-rest-input` (only flips rest input, like Dorico); letters and `Y` then enter rests. MuseScore's `toggle-rest` also turned the selected note into a rest, so it is unbound.
- macOS key strings are physical keys: the articulation keys are `Shift+[`, `Shift+]`, `Shift+'`, `Shift+\\` (a bare `}` fired staccato's `]`). `check_keymap.py` now normalises shifted characters the same way; it also exposed upstream's `(` add-parentheses firing on `9`, now unbound.
- The `}` / `{` / `"` / `|` latch during note input did nothing: engraving's `updateArticulations` (only used by `NotationNoteInput::setArticulation`) ignored every articulation but accent, marcato, tenuto and staccato. Its default case now toggles the symbol (upstream edit, marked `Dorico fork:`, tested in `noteinput_tests.cpp`).
- Return on a selected dynamic (or other item) opens MuseScore's text edit for it until spec 20 adds popovers.
- `dorico/toggle-insert-mode` outside note input only flips the flag (it does not start input, unlike `notation/toggle-insert-mode`).
- Forced octave: reference step is the pending note (K mode), the chord's top/bottom note (chord mode with a pending chord), or the selected note; with none, engraving's nearest-octave choice is kept. These notes do not play a preview sound (the preview helper is private to `NotationActionController`).
- `O` (Force Duration) stays reserved/unbound.
- Tests are in one file, `tests/noteinput_tests.cpp` (step math, dot tap, mode state).

## Deferred / unsupported
- **Force Duration (O)**: MuseScore never auto-merges rests/ties; no equivalent. Key stays reserved.
- **Grace-note input mode (`/`)**: `/` keeps `notation/add-acciaccatura` (adds a grace note to the selection). Toggle mode later.
- **K + I / K + Q combinations**: one `NoteEntryMethod` at a time. Turning K on while Insert is on turns Insert off and vice versa (document in the toast). Insert + Chord stays mutually exclusive as in Dorico.
- **Lock Duration auto-off at the last note** and "max rests skipped" preference: whatever REPITCH does today.
- **Opt-I scope cycling, Shift-Opt-I stop position**: README global deferred list.
- **Pitch before duration with accidental/dot/articulation chosen first** is MuseScore's own behaviour; the "before / after inputting note" preference is not added.
- **Caret badges** (Chord `+`, Insert V marks, dashed Lock Duration, shadow notehead): need the overlay loader from spec 20; `IDoricoModeState::modesChanged()` is the hook. Not part of this spec's acceptance.
- Double-click on a staff to start note input at that position; Shift-held chromatic mouse shadow note.
- Slash voices (Shift-Opt-V).
- Implicit rests: Space writes explicit rests when the position is empty.
- No playback preview for forced-octave notes (`Shift-Opt-A..G`, `Ctrl-A..G`).
- Return on a selected notation item (popover) is a no-op until spec 20.

## Acceptance criteria
- [x] Select a note, press Return: note input starts at it. Return again stops it. Shift-N toggles. Esc stops.
- [x] Select a dynamic, press Return: opens it for text editing (MuseScore's edit) until spec 20 adds its popover.
- [x] During input, `C D E F` enters four notes at the selected duration; with no note input, pressing `C` does nothing and `F` flips the stem of a selected note.
- [x] Shift-Opt-C after a C4 enters C5 (nearest C **above**); Ctrl-C (physical Control) after a C4 enters C3; pressing Shift-Opt-C twice in a row in K mode goes to C6.
- [x] `Q` on: `C` `E` `G` produce a C-E-G chord without advancing; second `C` goes up an octave; the caret advances only on a duration key/Space. `I` while Q is on turns Q off, and vice versa.
- [x] `I` on: entering a note in a full bar pushes later notes along, re-barred into the following bars. (Pulling back on delete belongs to spec 43.)
- [x] `K` on: `C` shows a pending note, `6` commits a quarter note; `L` on: letters change pitches of existing notes, keeping rhythm.
- [x] `6` then `6` within 400 ms yields a dotted quarter; waiting 1 s between presses does not.
- [x] Opt-`.` cycles 0-4 dots.
- [x] `,` then `C` enters a rest; `Y` enters a rest.
- [x] `-` `=` `0` apply to the next note only and clear after it.
- [x] `}` latches staccatissimo for subsequent notes; pressing it again unlatches. With notes selected it toggles on them.
- [x] `V` moves between voices 1-2 in input; `Shift-V` creates voice 2 when only voice 1 exists.
- [x] Space during input advances without deleting existing notes; outside input it starts/stops playback even when the mixer has focus.
- [x] `check_keymap.py` still exits 0 (all rows updated).
- [x] Unit tests pass.

## Tests
GoogleTest in `src/dorico/tests/noteinput_tests.cpp` (one file; pure helpers live in `internal/noteinpututils.h`):
- step math (was `pitchmath_tests.cpp`): `lineAbove/lineBelow` over all 7 steps and octave boundaries (prev B4 + C -> C5 above, C4 below; prev C4 + C above -> C5 not C4).
- mode state: toggling Chord clears Insert and the reverse; `modesChanged` fires once per change.
- dot tap: injectable clock; same digit within 400 ms -> dot, different digit -> no dot, beyond 400 ms -> no dot.
Manual: the acceptance list above on a blank 4/4 score.
