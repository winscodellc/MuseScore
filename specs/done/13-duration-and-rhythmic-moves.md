<!-- markdownlint-disable -->
# 13 - Duration editing and rhythmic moves
Status: done  -  Depends on: 00, 10, 12 (grid service)  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_durations/write_mode_notes_duration_changing_t.html`, `.../write_mode_durations/write_mode_notes_splitting_by_duration_t.html`, `.../write_mode_arranging_tools/write_mode_notes_items_moving_rhythmically_t.html`, `.../write_mode_notations_input/write_mode_ties_inputting_t.html`, `.../write_mode_notations_input/write_mode_dynamics_inputting_popover_t.html` (hairpin keys)

## Goal
Lengthen / shorten / double / halve selected notes and lines, move notes and annotations rhythmically, scissors on ties, and the direct hairpin keys, with Dorico's keys.

## Dorico behaviour
| Key | Notes / rests selected | Items selected (hairpins, lines, pedal lines, ...) |
|---|---|---|
| Shift-Opt-Right | Lengthen by the **grid** value | Lengthen to the grid position **or** the next notehead, whichever is closer |
| Shift-Opt-Left | Shorten by the grid value | Same, shortening |
| Cmd-Shift-Opt-Right | Double the duration | Extend to the next notehead |
| Cmd-Shift-Opt-Left | Halve the duration | Shorten to the previous notehead |
| Opt-Right / Opt-Left | Move rhythmically by the **nearest note or position** | A single dynamic / lyric / pedal line jumps to adjacent noteheads, bars or barlines |
| Cmd-Opt-Right / Left | Move rhythmically by the **grid** | Multiple items move together by the grid |
| 1-9 on a selection | Change the duration of existing notes (spec 10/11) | |
| U | Scissors: during input split at the caret; otherwise delete ties in tie chains | |
| Opt-U | Split selected notes by the rhythmic grid resolution (or at the caret when the caret spans staves) | |
| `<` / `>` | Crescendo / diminuendo hairpin directly, no popover. With a range: spans it. During input: opens at the caret and extends as the caret advances (spec 21) | |

Details:
- Extended notes **fill rests** after them. Chord mode (Q) lets moved notes overlap existing ones; Insert mode (I) pushes material and can change durations of notes passed through (state in `IDoricoModeState`, spec 11).
- Only one tempo mark / clef / similar item can exist per position: moving over one deletes or shortens it. Undo restores them only if the move used the keyboard.
- Items attached to noteheads (fingerings, glissandi, tremolos) cannot be moved; arpeggio signs are deleted if moved onto a rest.
- Write > Edit Duration menu also has Extend to Next Note, Extend to End of Selection, Shorten to Next Note (menu only; no default key).

## MuseScore today
- `notation/set-double-duration`, `set-halve-duration`, `...-dotted` -> `increaseDecreaseDuration(-1|+1, stepByDots)` (`notationactioncontroller.cpp:223-226`; `INotationInteraction::increaseDecreaseDuration`, `inotationinteraction.h:220`). They act on the selection and on the input duration. After spec 10 they have no key.
- `notation/extend-to-next-note` (Alt+W kept) -> `extendToNextNote` (`inotationinteraction.h:267`).
- Swap with the neighbour: `notation/swap-left` / `swap-right` -> `INotationInteraction::swapChordRest(MoveDirection)` (`notationactioncontroller.cpp:442-443`; formerly Shift+Left/Right in note-input scope, unbound by spec 10).
- `engraving::Score::changeCRlen(ChordRest*, const Fraction&, bool fillWithRest = true)` (`src/engraving/dom/score.h:410`): sets a new length, filling with rests.
- Ties: `INotationInteraction::toggleTieForSelection()`; `Note::tieFor()` / `tieBack()` share one `Tie`.
- Spanners (hairpins, lines): `Spanner::tick()`, `ticks()`, property `Pid::SPANNER_TICKS`; change with `item->undoChangeProperty(Pid::SPANNER_TICKS, Fraction)`.
- Annotations (Dynamic, StaffText, Tempo text, RehearsalMark, Harmony) are children of a `Segment`; there is no "move to the next notehead" command.
- Hairpins: `notation/add-hairpin` / `add-hairpin-reverse` (`<` `>`, mac diff uses `Shift+,` / `Shift+.`), implemented by `addHairpinsToSelection` (`notationinteraction.cpp:5467`).
- Edit pattern: `INotation::undoStack()->prepareChanges(TranslatableString)` ... `commitChanges()` (`src/notation/inotationundostack.h:53-55`); get the `Score*` from a selected item (`item->score()`).
- Grid: `IDoricoGrid::resolution()` from spec 12.

## Design

### Commands (all in `src/dorico/internal/doricodurationscontroller.cpp`, registered by `DoricoCommandsRegister`)
| Command | Key | Behaviour |
|---|---|---|
| `dorico/lengthen-by-grid`, `dorico/shorten-by-grid` | Alt+Shift+Right / Left | For every selected tie chain (a note stands for its whole tie chain, as in Dorico; later chains first when lengthening; rests, grace notes and tuplets are skipped). Lengthening: `score->changeCRlen(last, last->ticks() + grid)` on the chain's last chord, which only grows into rests: it stops at the next chord of the voice or the barline (changeCRlen adds tied pieces when one note value is not enough). Shortening cuts from the end of the chain (`changeCRlen`, or `Score::deleteItem` for a piece that is cut off completely) and stops at one grid unit. One undo step. For selected segment-anchored lines / hairpins: the new end is the next / previous grid position or note on the staff, whichever is closer (`nextCaretTick` / `prevCaretTick` from spec 12), applied with `undoChangeProperty(Pid::SPANNER_TICKS, newTicks)`. Mixed selections do both. The grid comes from `IDoricoCaret::gridDenominator()`. |
| `dorico/double-duration`, `dorico/halve-duration` | Ctrl+Alt+Shift+Right / Left | ChordRests: dispatch `notation/set-double-duration` / `set-halve-duration`. Spanners: end tick = end of the next / previous ChordRest in the spanner's track. |
| `dorico/move-right`, `dorico/move-left` | Alt+Right / Left | One chord or rest selected: it swaps places with its neighbour in the same bar (approximation: Dorico moves by the nearest note position). MuseScore's `swapChordRest` only swaps equal durations, so both are removed and clones are added the other way round (`undoRemoveElement` + `undoAddCR`). Refused (no-op) for tuplets, grace notes, ties, two-note tremolos, full-bar rests, bar repeats and chords with slurs / glissandi (hairpins and other lines on rhythmic positions do not block it: layout recomputes their anchors). Annotation (one dynamic, text, tempo mark ...): to the next / previous chord or rest on its staff with MuseScore's `MoveElementAnchors::moveSegment` (keeps snapped hairpins with it). Anything else: no-op. |
| `dorico/scissors` | U | Selection of notes / ties: removes every tie of the tie chains it touches (Dorico selects whole chains); laissez vibrer and partial ties are left alone. Note input active: no-op (split at caret deferred). The Notes toolbox Scissors button (spec 52) now uses it. |

No new pure helper: "next grid position or note, whichever is closer" is exactly the caret step of spec 12 (`nextCaretTick` / `prevCaretTick` in `internal/noteinpututils.h`), so the hairpin case reuses it and is unit-tested there.

### Reused unchanged
`<` / `>` keep `notation/add-hairpin` / `add-hairpin-reverse` (bound in 10 through the base + mac diff). Open-ended hairpins during input arrive with spec 21. `Alt+W` extend-to-next-note stays.

### Adds to `shortcuts_dorico.json`
```
NOTATION: Alt+Shift+Right -> dorico/lengthen-by-grid;  Alt+Shift+Left -> dorico/shorten-by-grid;
          Ctrl+Alt+Shift+Right -> dorico/double-duration;  Ctrl+Alt+Shift+Left -> dorico/halve-duration;
          Alt+Right -> dorico/move-right;  Alt+Left -> dorico/move-left;
          U -> dorico/scissors
```
`Ctrl+Alt+Right/Left` (move by grid) and `Alt+U` (split by grid) stay unbound/reserved.

### Upstream edits
- `notationscene/qml/MuseScore/NotationScene/notationviewinputcontroller.cpp` `shortcutOverrideEvent`: (Cmd-)Shift-Opt-Left/Right are not taken by the score view (MuseScore uses Shift+arrows on a selected line for grip editing and on text for anchor moves, which swallowed the key before the shortcut fired). Not while text is being edited. Marked `// Dorico fork (spec 13)`.

## Deferred / unsupported
- **Move by grid (Cmd-Opt-Right/Left)**: needs splitting rests at arbitrary ticks (MuseScore has segments only at existing ChordRests). Reserved.
- **Opt-U split by grid and U split at the caret**: needs "split a chord into tied pieces" which MuseScore only does while entering notes. Reserved.
- Moving by the *nearest note position* with Chord mode overlap / Insert mode push; moving multiple items or non-annotation items; moving lyrics, pedal lines and cues; Dorico's restore-deleted-items-on-undo detail.
- Extend to End of Selection, Shorten to Next Note (menu-only in Dorico).
- Opt-/ slashed grace notes; grace-note duration edits.
- Shift-Opt-, / Shift-Opt-. messa di voce hairpins.
- Lengthening over following notes (Dorico overwrites them) and across barlines: notes only grow into rests up to the barline (our choice, so nothing is lost).
- Lengthening / shortening rests; a length that needs two note values is written as note + rest when shortening (MuseScore's `changeCRlen`), Dorico writes tied notes.
- Moving lines and hairpins with Opt-Left/Right; the "cannot move" status message (no-op instead).

## Acceptance criteria
- [x] Grid = eighth (spec 12). Quarter note selected, then rests follow: Shift-Opt-Right makes it a dotted quarter, again a half; the rest after it shrinks accordingly. Shift-Opt-Left reverses. One undo per press.
- [x] Three quarter notes selected: Shift-Opt-Right lengthens each (later ones first, no corruption); with no following room it clamps at the measure end without losing notes.
- [x] Hairpin selected, ending at beat 2 with the next notehead at beat 4, grid eighth: Shift-Opt-Right moves its end to beat 2.5. If the next notehead is only a sixteenth away, it snaps to that notehead instead.
- [x] Cmd-Shift-Opt-Right doubles a quarter to a half, Cmd-Shift-Opt-Left halves it; the input duration changes too during note input.
- [x] Quarter note followed by a half note, Opt-Right swaps their order; undo restores.
- [x] Dynamic selected under a note, Opt-Right moves it to the next ChordRest of the same staff/track; at the last note nothing happens; undo restores.
- [x] U on two tied notes removes the tie; on untied notes nothing happens; during note input nothing happens.
- [x] `<` with a two-note range creates a crescendo over it.
- [x] `check_keymap.py` exits 0.
- [x] Unit tests pass.

## Results
Unit tests: `dorico_tests` 63/63 pass. `check_keymap.py` exits 0. The acceptance list was run by hand in the installed build, driven with ClaudeUIControl on a temp copy of `engraving/tests/copypaste_data/copypaste25-ref.mscx` (2026-10-16). The hairpin "sixteenth away" case is covered by the unit test; the others were checked on screen and, for hairpin lengths, in the saved file (`<ticks>` 1/4 -> 3/8 -> 1/2, then 3/8, then 7/4 with Cmd-Shift-Opt-Right).

Found during the manual run and fixed:
- After a lengthen splits a note into tied pieces (eighth + half), the next press did nothing, because only the first piece was edited. Notes are now edited as whole tie chains.
- Opt-Left/Right refused to move a note under a hairpin, because the hairpin's start element is that note. Segment-anchored lines are now ignored by that check (slurs still block it, verified).

Not fixed (cosmetic): after shortening, the freed time is filled with separate eighth rests, not merged into larger rests (MuseScore's `changeCRlen` fill).

## Tests
GoogleTest `DoricoCaretMath.HairpinEndGoesToTheCloserOfGridAndNote` in `src/dorico/tests/noteinput_tests.cpp`: grid closer, note closer, equal distance, shortening by grid or to a note.
Score-level tests: use the engraving `ScoreRW`/`ScoreComp` helpers (see `src/engraving/tests/`) to build a small `.mscx` reference for lengthen/shorten/scissors if feasible; otherwise manual with the acceptance list (note in `## Results` which ones were manual).
