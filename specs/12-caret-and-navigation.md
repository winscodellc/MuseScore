# 12 - Caret and navigation
Status: pending review  -  Depends on: 00, 10, 11  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_navigation/write_mode_navigation_items_music_area_t.html`, `.../write_mode_caret/write_mode_caret_moving_manually_t.html`, `.../write_mode_rhythmic_grid/write_mode_rhythmic_grid_c.html`, `.../write_mode_rhythmic_grid/write_mode_rhythmic_grid_resolution_changing_t.html`, `.../write_mode_transposing_tools/write_mode_notes_pitch_changing_t.html`, `.../write_mode_transposing_tools/write_mode_notes_respelling_t.html`

## Goal
Arrow keys, Cmd+arrows, Space and the pitch-by-step keys behave as in Dorico, both with a selection and during note input; add the rhythmic grid state (Opt-[ / Opt-]) and directional respelling (Opt-= / Opt--).

## Dorico behaviour
**Selection navigation (Write mode, caret off)**

| Key | Result |
|---|---|
| any arrow, nothing selected | selects the first note/rest/item on the top staff |
| Right / Left | next / previous item in the **same voice** (same item type) |
| Up | closest note above: other notes in the chord first, then the **lowest** note/rest on the staff above. A selected whole chord keeps its bottom note selected |
| Down | mirror: closest note below, then the **highest** note/rest on the staff below. A whole chord keeps its top note |
| Cmd-Right / Cmd-Left | note/rest at the start of the next / previous bar |
| Cmd-Up / Cmd-Down | top / bottom staff of the system |
| Tab / Shift-Tab | cycle item types at the same rhythmic position (deferred) |

**Caret movement (note input)**

| Key | Result |
|---|---|
| Right / Left | by the grid resolution **or** to the next/previous note/rest, whichever is closer |
| Space | advance by the selected note duration (spec 11) |
| Cmd-Right / Cmd-Left | next / previous bar |
| Up / Down | caret to the staff above / below |
| Cmd-Up / Cmd-Down | top / bottom staff of the system |
| Shift-Up / Shift-Down | extend the caret over the staff above / below (deferred) |

**Rhythmic grid**: default eighth note; controls caret step, lengthen/shorten step, move step, split length. `Opt-]` = decrease resolution (a shorter value, e.g. eighth -> 16th). `Opt-[` = increase (longer value, eighth -> quarter). Shown in the status bar and as a ruler above the caret.

**Pitch by step (already bound in spec 10)**: Opt-Up/Down = one staff position (diatonic), Shift-Opt-Up/Down = one octave division (semitone in 12-EDO), Cmd-Opt-Up/Down = octave.

**Respell**: Opt-= respells using the note name **above**, Opt-- the name **below**. Cycle example from G-sharp: up -> A-flat -> B-triple-flat; down -> F-triple-sharp. Triple accidentals are the limit. (Page: `write_mode_notes_respelling_t.html`.)

## MuseScore today
- Left/Right/Up/Down in `NotationActionController::moveWithRet` (`src/notationscene/internal/notationactioncontroller.cpp:2071`): Up/Down are bound to `pitch-up` / `pitch-down` and branch inside (lyrics, text nudge, grips, pending pitch in BY_DURATION, string on TAB, otherwise `movePitch`). Left/Right are `move-left` / `move-right` -> `moveSelectionDeprecated(Chord|Measure)`. Spec 10 removed Up/Down from pitch editing.
- Selection commands (`notation/select` with a `SelectionTarget` param; `src/notation/notationtypes.h:113`): `FirstItem, LastItem, NextItem, PrevItem, NextChord, PrevChord, NextMeasure, PrevMeasure, NextTrack, PrevTrack, AboveStaff, BelowStaff, UpNoteInChord, DownNoteInChord, TopNoteInChord, BottomNoteInChord, TopStaff, ...`. Aliases: `notation/goto-upnote-in-chord`, `goto-downnote-in-chord`, `goto-topnote-in-chord`, `goto-bottomnote-in-chord`, `goto-top-staff`, `goto-first-element` (`notationactioncontroller.cpp:136-140`, implementation `src/notation/internal/notationselection.cpp:190-290`).
- Note input position: `INotationNoteInput::setCurrentTrack(track_idx_t)`, `state()` (`NoteInputState` = `engraving::InputState`, `src/engraving/dom/input.h:63`: segment, track, voice, duration).
- Enharmonics: `notation/enharmonic-spell-current` / `enharmonic-spell-both` toggle between two spellings; `notation/pitch-spell*` exist. No directional respell.
- Spelling helpers: `tpc2step`, `step2tpc`, `tpc2pitch` (`src/engraving/dom/pitchspelling.h`, verify exact names).
- No grid concept anywhere.

## Design

### 1. Selection navigation (new commands, all `dorico/`)
| Command | Key | Behaviour |
|---|---|---|
| `dorico/select-up` | Up | Not in note input: nothing selected -> `notation/goto-first-element`. Else dispatch `notation/goto-upnote-in-chord`. **First check** whether `UpNoteInChord` already crosses to the staff above when the selection is the top note (MuseScore's old `Alt+Up` "go to higher pitched note" does in v3/v4). If not, fall back to `select` with target `AboveStaff` and then `BottomNoteInChord` of the result. In note input: move the input to the staff above (below). |
| `dorico/select-down` | Down | Mirror, `goto-downnote-in-chord`, `BelowStaff`, `TopNoteInChord`. |
| `dorico/select-top-staff` | Cmd-Up | `notation/goto-top-staff` (note input: set the input track to the first visible staff, same voice index). |
| `dorico/select-bottom-staff` | Cmd-Down | Repeat `select` target `BelowStaff` until the selection stops changing (bounded by staff count). Same in note input via `setCurrentTrack`. |
Left, Right, Cmd-Left, Cmd-Right stay on `move-left/right(-quickly)`; add a thin `dorico/select-left|right` wrapper **only if** testing shows MuseScore does nothing with an empty selection (then: empty selection -> `goto-first-element`, else forward to the MuseScore command).

Note-input variant of Up/Down: `track = state().track(); staff = track / VOICES; track' = (staff -/+ 1) * VOICES + voice; noteInput->setCurrentTrack(track')`. Verify `setCurrentTrack` re-seats the input segment; if not, also call `startNoteInput()` at the corresponding segment of the new staff (same tick).

### 2. Rhythmic grid
`internal/doricogrid.h/.cpp` + `idoricogrid.h` (context service like 11's mode state):
```cpp
class IDoricoGrid : MODULE_CONTEXT_INTERFACE {
    virtual engraving::DurationType resolution() const = 0;   // default V_EIGHTH
    virtual void finer() = 0;      // eighth -> 16th ... clamps at V_128TH
    virtual void coarser() = 0;    // eighth -> quarter ... clamps at V_WHOLE
    virtual muse::async::Notification changed() const = 0;
};
```
Commands: `dorico/grid-finer` (Opt-]), `dorico/grid-coarser` (Opt-[). Persist in settings `dorico/gridResolution`. Show it: log line now; the status bar/ruler is deferred (needs the overlay from spec 20). Spec 13 consumes `resolution()` for lengthen/shorten/move/split.

### 3. Directional respell (`dorico/respell-above`, `dorico/respell-below`; Opt-= / Opt--)
Pure function in `internal/respell.h` (no Qt):
```cpp
// returns the tpc spelling pitch (mod 12) using the note name one step above/below, or the input tpc if impossible (|alter| > 3)
int respellTpc(int tpc, bool above);
```
Algorithm: `step = tpc2step(tpc)` (C=0..B=6), `pcClass = tpc2pitch(tpc) mod 12`, `newStep = (step +/- 1) mod 7`, `alter = (pcClass - naturalPitchOf(newStep))` normalised into [-6, 6]; if `|alter| > 3` return `tpc`; else `step2tpc(newStep, alter)`. Check against the manual example: G-sharp (pc 8) up -> A, alter -1 -> A-flat; A-flat up -> B, alter -3 -> B-triple-flat; G-sharp down -> F, alter +3 -> F-triple-sharp.
Apply to every selected note with `Note::undoChangeProperty(Pid::TPC1, ...)` and `Pid::TPC2` computed the same way for the transposed spelling. Copy the exact pattern (setting TPC1 and TPC2 together inside `startEdit/apply`) from `changeEnharmonicSpelling` (grep in `src/engraving/editing/` and `src/notation/internal/notationinteraction.cpp`). One undo step for the whole selection.

### 4. Adds to `shortcuts_dorico.json`
```
NOTATION: Up -> dorico/select-up;  Down -> dorico/select-down;
          Ctrl+Up -> dorico/select-top-staff;  Ctrl+Down -> dorico/select-bottom-staff;
          Alt+] -> dorico/grid-finer;  Alt+[ -> dorico/grid-coarser;
          Alt+= -> dorico/respell-above;  Alt+- -> dorico/respell-below
          (if needed) Left/Right -> dorico/select-left|right
```
Spec 10 had `Ctrl+Up/Down` and `Up/Down` unbound; check the dead-key results table in 10 for `Alt+[`, `Alt+]`, `Alt+=`, `Alt+-` and pick fallbacks if those fail.

### Upstream edits
None.

## Deferred / unsupported
- Shift-Up/Down caret extension over several staves (README global list).
- Tab / Shift-Tab item cycling; selection of hidden items behind others (Shift-Opt-click).
- Grid shown in the status bar.
- Caret stepping inside tuplets (Right/Left fall back to MuseScore's next-chord/rest move); caret ruler on TAB staves and in input-by-duration mode (MuseScore's own caret/ruler is drawn).
- Respell in score vs part scoping; reset/propagate spellings; Write > Respell Notes Automatically dialog.
- Quarter-tone / octave-division arithmetic (12-EDO only).

## As built
- `IDoricoCaret` (`src/dorico/idoricocaret.h`, impl `internal/doricocaret.cpp`) keeps a virtual caret tick inside the chord/rest at MuseScore's input position. `dorico/caret-left|right` (Left / Right) step by the grid or to the next/previous note/rest (`nextCaretTick` / `prevCaretTick` in `noteinpututils.h`); outside note input they forward to `notation/move-left|right` (which already selects the first item when nothing is selected, so no `select-left|right` wrapper).
- Entering a note/rest (letters, Shift-Opt / Ctrl letters, Y, Space) with the caret inside a note/rest first cuts it there with `Score::changeCRlen`, merged into the same undo step as the entry. A cut note becomes note + rests (Dorico would tie).
- Ruler + caret line are painted by the Dorico module through one optional hook in `AbstractNotationPaintView::paint` (upstream edit). Grid: `dorico/grid-finer|coarser` (Opt-] / Opt-[), persisted as `dorico/gridResolution` (denominator, 1..128). The grid is a denominator inside `DoricoCaret`, not a separate `IDoricoGrid` service; no log line, the ruler shows it.
- Up / Down / Cmd-Up / Cmd-Down: `dorico/select-up|down|top-staff|bottom-staff` -> `IDoricoCaret::navigateVertically`. Selection: nothing selected -> `goto-first-element`; else `goto-upnote|downnote-in-chord`, which already crosses into the staff above (its lowest note) / below (highest). A whole chord starts from its top (Up) / bottom (Down) note. Cmd-Up/Down go to the top / bottom visible staff at the same tick (lowest note on the top staff, highest on the bottom one). Note input: the caret moves to the visible staff above / below (or top / bottom), same caret tick, same voice when that voice has music there, else the first voice that has. Target staff: `verticalStaff` in `noteinpututils.h`.
- Respell: `dorico/respell-above|below` (Opt-= / Opt--), `respellTpc` in `noteinpututils.h` (not a separate `respell.h`). Sets the shown spelling with `Note::undoSetTpc` and the other one (concert / written) with `transposeTpc`, for the whole tie chain; TAB and drum staves are skipped. One undo step.

## Acceptance criteria
- [ ] Nothing selected, press Right: first item of the top staff is selected.
- [ ] Two-note chord on staff 2, bottom note selected: Up selects the top note; Up again selects the lowest note/rest of staff 1 at that tick; Down walks back.
- [ ] Whole chord selected on a middle staff: Up keeps going to the staff above and selects its bottom note.
- [ ] Cmd-Up selects the first staff's item at the same tick; Cmd-Down the last visible staff's.
- [ ] During note input on staff 2, Up moves the caret to staff 1, Down back; the entered note lands on the right staff.
- [ ] Cmd-Right / Cmd-Left move to the start of the next / previous bar, with and without note input.
- [ ] Opt-] then Opt-[ changes the grid (log shows 1/16 then 1/8); the value survives restart; clamps at 128th and whole.
- [ ] G-sharp selected: Opt-= gives A-flat, again gives B-triple-flat, again does nothing; Opt-- from G-sharp gives F-triple-sharp. Undo restores in one step. Works on a multi-note selection and on a transposing instrument (concert and written spellings stay consistent).
- [x] `check_keymap.py` exits 0.
- [x] Unit tests pass.

## Tests
GoogleTest in `src/dorico/tests/`:
(As built: all in `noteinput_tests.cpp`: `DoricoRespell.*`, `DoricoVerticalStaff.*`, `DoricoCaretMath.*`. No grid persistence test: the settings round trip is the framework's.)
- `respell_tests.cpp`: the three manual examples; wrap-around (B-sharp / C-flat, pitch class 0 and 11); `|alter| > 3` returns the input unchanged; naturals (C up -> D double-flat).
- `grid_tests.cpp`: finer/coarser order, clamping, persistence round trip (use the settings test helper used by other modules' tests).
Manual: the acceptance list on a two-staff piano score with a 3-note chord.
