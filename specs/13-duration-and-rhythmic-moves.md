<!-- markdownlint-disable -->
# 13 - Duration editing and rhythmic moves
Status: todo  -  Depends on: 00, 10, 12 (grid service)  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_durations/write_mode_notes_duration_changing_t.html`, `.../write_mode_durations/write_mode_notes_splitting_by_duration_t.html`, `.../write_mode_arranging_tools/write_mode_notes_items_moving_rhythmically_t.html`, `.../write_mode_notations_input/write_mode_ties_inputting_t.html`, `.../write_mode_notations_input/write_mode_dynamics_inputting_popover_t.html` (hairpin keys)

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

### Commands (all in `src/dorico/internal/dorico_durations.cpp`, registered by `DoricoCommandsRegister`)
| Command | Key | Behaviour |
|---|---|---|
| `dorico/lengthen-by-grid`, `dorico/shorten-by-grid` | Alt+Shift+Right / Left | For every selected ChordRest (notes map to their chord; process later ticks first when lengthening): `score->changeCRlen(cr, cr->ticks() +/- gridTicks)`; shortening clamps at one grid unit; one undo step. For selected Spanners: end target from `closerTarget(...)` (below), applied with `undoChangeProperty(Pid::SPANNER_TICKS, newTicks)`. Mixed selections do both. |
| `dorico/double-duration`, `dorico/halve-duration` | Ctrl+Alt+Shift+Right / Left | ChordRests: dispatch `notation/set-double-duration` / `set-halve-duration`. Spanners: end tick = end of the next / previous ChordRest in the spanner's track. |
| `dorico/move-right`, `dorico/move-left` | Alt+Right / Left | ChordRest selection: `swapChordRest(Right|Left)` (approximation: swap with the neighbour keeps both durations; Dorico moves by the nearest note position). Annotation (single item): re-parent to the next / previous ChordRest `Segment` of the same track (`Segment::nextCR(track)` / `prevCR`): `undoRemoveElement` + clone with new parent + `undoAddElement`; keeps the selection on the clone. Anything else: no-op, status message "cannot move". |
| `dorico/scissors` | U | Selection of notes/ties: for each note with `tieFor()`, `undoRemoveElement(tie)`; a selected Tie is removed directly. Note input active: no-op (split at caret deferred). |

Pure helper (unit-tested) in `internal/gridmath.h`:
```cpp
// For a spanner end at `end`, the next chordrest end at `nextCrEnd`, grid length `grid`, direction (+1/-1):
// returns the new end: whichever of (end + dir*grid) and nextCrEnd is closer to `end` (never past 0 / the score end).
engraving::Fraction closerTarget(Fraction end, Fraction nextCrEnd, Fraction grid, int dir);
```

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
None.

## Deferred / unsupported
- **Move by grid (Cmd-Opt-Right/Left)**: needs splitting rests at arbitrary ticks (MuseScore has segments only at existing ChordRests). Reserved.
- **Opt-U split by grid and U split at the caret**: needs "split a chord into tied pieces" which MuseScore only does while entering notes. Reserved.
- Moving by the *nearest note position* with Chord mode overlap / Insert mode push; moving multiple items or non-annotation items; moving lyrics, pedal lines and cues; Dorico's restore-deleted-items-on-undo detail.
- Extend to End of Selection, Shorten to Next Note (menu-only in Dorico).
- Opt-/ slashed grace notes; grace-note duration edits.
- Shift-Opt-, / Shift-Opt-. messa di voce hairpins.

## Acceptance criteria
- [ ] Grid = eighth (spec 12). Quarter note selected, then rests follow: Shift-Opt-Right makes it a dotted quarter, again a half; the rest after it shrinks accordingly. Shift-Opt-Left reverses. One undo per press.
- [ ] Three quarter notes selected: Shift-Opt-Right lengthens each (later ones first, no corruption); with no following room it clamps at the measure end without losing notes.
- [ ] Hairpin selected, ending at beat 2 with the next notehead at beat 4, grid eighth: Shift-Opt-Right moves its end to beat 2.5. If the next notehead is only a sixteenth away, it snaps to that notehead instead.
- [ ] Cmd-Shift-Opt-Right doubles a quarter to a half, Cmd-Shift-Opt-Left halves it; the input duration changes too during note input.
- [ ] Quarter note followed by a half note, Opt-Right swaps their order; undo restores.
- [ ] Dynamic selected under a note, Opt-Right moves it to the next ChordRest of the same staff/track; at the last note nothing happens; undo restores.
- [ ] U on two tied notes removes the tie; on untied notes nothing happens; during note input nothing happens.
- [ ] `<` with a two-note range creates a crescendo over it.
- [ ] `check_keymap.py` exits 0.
- [ ] Unit tests pass.

## Tests
GoogleTest in `src/dorico/tests/gridmath_tests.cpp`:
- `closerTarget`: grid closer, notehead closer, equal distance prefers the notehead, dir -1, clamp at 0.
Score-level tests: use the engraving `ScoreRW`/`ScoreComp` helpers (see `src/engraving/tests/`) to build a small `.mscx` reference for lengthen/shorten/scissors if feasible; otherwise manual with the acceptance list (note in `## Results` which ones were manual).
