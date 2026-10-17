<!-- markdownlint-disable -->
# 30 — Popover: repeats and tremolos (Shift-R)

Status: done · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_repeats_*_r.html`
- [M] `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_input/write_mode_repeats_popover_r.html`
- [M] `.../write_mode_notations_input/write_mode_repeat_markers_inputting_popover_t.html`

Reference: [popovers](../reference/dorico-popovers.md) (B1, B2 "Repeats and tremolos") · [internals](../reference/musescore-internals.md) (section 3).

Names such as `parse`, `Result` and `apply` follow the contract in spec 20; rename to match its final API.

## Goal
Shift-R opens a popover. Typing a marker name, ending, tremolo or bar-repeat token and pressing Return inserts that item at the selection.

## Dorico behaviour

**General** (reference B1):
- Needs a selection or an active caret.
- The popover opens above the top staff, at the earliest selected item.
- Return applies. Esc cancels. There is no Opt-Return variant for this popover.
- With a repeat marker, jump or ending already selected, Return reopens the popover pre-filled.

**Endings**

| Entry | Effect |
|---|---|
| `end`, `ending` | Whole ending. The first segment covers the selected bars. A second segment and an end-repeat barline are created automatically. |
| `add` | Add another segment to the selected ending. |

**Markers and jumps**

| Entry | Item |
|---|---|
| `da capo`, `dc`, `D.C.` | D.C. |
| `DC al Fine`, `dcalf`, `D.C. al Fine` | D.C. al Fine |
| `DC al Coda`, `dcalc`, `D.C. al Coda` | D.C. al Coda |
| `dal segno`, `ds`, `D.S.` | D.S. |
| `DS al Fine`, `dsalf`, `D.S. al Fine` | D.S. al Fine |
| `DS al Coda`, `dsalc`, `D.S. al Coda` | D.S. al Coda |
| `to coda`, `toc`, `tc` | to Coda |
| `segno`, `seg`, `s`, `$` | Segno (`$` comes from the procedure page, [M] `write_mode_repeat_markers_inputting_popover_t.html`) |
| `fine`, `fin`, `f` | Fine |
| `coda`, `co`, `c` | Coda |

End-of-section markers (Fine, D.C. al Coda, and so on) right-align to the position.

**Single-note tremolos**

| Entry | Meaning |
|---|---|
| `/`, `\`, `1` | 1 stroke |
| `//`, `\\`, `2` | 2 strokes |
| `///`, `\\\`, `3` | 3 strokes |
| `////`, `\\\\`, `4` | 4 strokes |
| `z`, `zonstem` | Buzz roll (Z on stem) |
| `//rel`, `//r`, `2rel`, `2r` | Tremolo with release |
| `//att`, `//a`, `2att`, `2a` | Tremolo with attack |
| `0`, `clear` | Remove all tremolos |

**Multi-note tremolos** (two chords)

| Entry | Meaning |
|---|---|
| `/2`, `\2`, `12` | 1 stroke |
| `//2`, `\\2`, `22` | 2 strokes |
| `///2`, `\\\2`, `32` | 3 strokes |
| `////2`, `\\\\2`, `42` | 4 strokes |
| `0`, `clear` | Remove all tremolos |

**Regions**

| Entry | Region |
|---|---|
| `slash` | Slash region |
| `number bars`, `numbars`, `num`, `nb` | Numbered bar region |
| `%1`, `%` | Repeat last bar |
| `%2`, `%4` | Repeat last 2 / 4 bars |
| `%1,2`, `%1,4` | Repeat last bar, grouped in 2 / 4 |
| `%2,4` | Repeat last 2 bars, grouped in 4 |

**Selection and caret**:
- A range selection puts an ending over the range.
- Per Dorico's general rule (reference B1), `|:` with a range adds the matching `:|`. That rule belongs to the barlines popover (spec 25), not here.
- Tremolos apply to the selected chords. A multi-note tremolo needs exactly two adjacent chords.

## MuseScore today
- **Hook:** `NotationInteraction::applyPaletteElement(EngravingItem*, modifiers)` (`src/notation/internal/notationinteraction.cpp:2224`) accepts any element built with `Factory::create*`. The palette calls it at `src/palette/internal/paletteprovider.cpp:596`. There is no dedicated API for repeats or markers.
- **Elements to confirm in the DOM:**
  - `Marker` (segno, coda, fine, to coda).
  - `Jump` (D.C., D.S. and their al Fine / al Coda variants).
  - `Volta`, a spanner. Check how the palette applies it to a range.
  - `MeasureRepeat` with its number of measures.
  - The tremolo classes (single-chord and two-chord).
  - Grep the DOM for names and enums. They are not verified here.
- Single-chord tremolo types in MuseScore: strokes 1–5 plus buzz roll. Check whether buzz roll exists in this 5.0-dev tree.
- Clearing a tremolo: remove it with an undoable `undoRemoveElement` (confirm the function name).
- No popover or text parser exists for any of this.

## Design
- **New files** (as built): `src/dorico/popover/parsers/repeatsparser.h/.cpp` (pure `parseRepeats(text) -> RepeatsParse`, `repeatsText(entry)` for prefill) and `src/dorico/popover/handlers/repeatshandler.h/.cpp` (`repeatsPopover()`).
- **Result variant:**
  - `Marker{type}`
  - `Jump{type}`
  - `Ending{add: bool}`
  - `Tremolo{strokes, twoChord, variant}`
  - `ClearTremolo`
  - `MeasureRepeat{n}`
  - `Slash`
- **Parser:** lower-case and trim the input, then match against a static token table. Keep the original case where Dorico distinguishes it (`DC al Fine` is case-insensitive in practice). Match the longest token first, so `dcalf` is not read as `dc`.
- **Apply:** `INotationInteraction::startEdit()` / `apply()` wrap the undo transaction. Then, per kind:
  - Marker or Jump: create it with `Factory`, then `applyPaletteElement` on the selected measure.
  - Ending: create a `Volta` and `applyPaletteElement` on the range. Add end-repeat barlines on the last bar, reusing whatever spec 25 exposes. If spec 25 is not done, only set the volta and list the barline as a known gap.
  - Tremolo: apply to the selected chords. For a two-chord tremolo, require two chords, otherwise report an error through the spec 20 error state.
  - `%N`: apply a `MeasureRepeat` to the selected measure(s).
  - `slash`: dispatch the existing fill-with-slashes command. Confirm its command id in `notationcommands.h`.
- **Register** the popover with spec 20's registry: key Shift-R, icon from the existing palette icons.
- **Upstream edits:** none. The shortcut itself is in spec 10's JSON. Shift-R collides with an existing base binding that spec 10 unbinds.

## Deferred / unsupported
- Numbered bar regions: `number bars`, `numbars`, `num`, `nb`.
- Grouped bar repeats: `%1,2`, `%1,4`, `%2,4`.
- Right-aligning end-of-section markers is not deferred after all: MuseScore's Fine, To Coda and every jump use the right-aligned `REPEAT_RIGHT` text style.
- Ending-segment editing beyond `add`.
- Tremolo variants with release or attack (`//rel`, `//att`): not available in MuseScore.
- Any `%N` with N other than 1, 2, 4: MuseScore supports only those.
- If buzz roll does not exist in this tree, `z` and `zonstem` are rejected as unknown.
- Unsupported tokens go through the spec 20 invalid-input state.
- `%N` empties the bars before adding the repeat: on bars with notes MuseScore's `makeMeasureRepeatGroup` otherwise shows a modal "Current contents of measures will be replaced" box, which stole the popover's focus and cancelled the entry. Dorico replaces without asking.
- At the note-input caret: single-chord tremolos go on the next note input (shared `PendingAtCaret`, as for ornaments); two-chord tremolos, `0`/`clear` and `slash` need a selection (Return does nothing at the caret).
- `slash` runs MuseScore's Fill With Slashes over the selection; Dorico's slash *region* (a spanner-like region) has no MuseScore model.
- `add` needs a selected ending segment. It closes the ending's last segment with an end repeat and adds an open segment numbered one higher on the next bar.

## Acceptance criteria
- [x] Shift-R opens the popover at the selection; Esc closes it without changes.
- [x] `segno`, `s` and `$` insert a Segno on the selected measure.
- [x] `dc`, `D.C. al Coda` and `dsalf` insert the matching jump.
- [x] `end` on a bar range creates a volta over the range with an end-repeat barline; `add` adds a segment.
- [x] `2` on a selected chord gives a 2-stroke tremolo; `22` on two adjacent chords gives a 2-stroke two-chord tremolo; `0` removes it.
- [x] `%2` on a measure inserts a two-measure repeat.
- [x] Return on a selected marker or jump reopens the popover pre-filled with its text.
- [x] An unknown token (`xyz`) shows the error state and changes nothing.
- [x] One Cmd-Z undoes a whole insertion.

## Tests
GoogleTest, in `src/dorico/tests/repeatsparser_tests.cpp` (follow the test setup defined in spec 20). Pure parser, no score needed:

| Input | Expected result |
|---|---|
| `segno` / `s` / `$` | `Marker{Segno}` |
| `to coda` / `toc` | `Marker{ToCoda}` |
| `DC al Fine` / `dcalf` | `Jump{DcAlFine}` |
| `D.S. al Coda` | `Jump{DsAlCoda}` |
| `dc` | `Jump{Dc}` (not a prefix match of something longer) |
| `end` | `Ending{add=false}` |
| `add` | `Ending{add=true}` |
| `/` / `1` | `Tremolo{strokes=1, twoChord=false}` |
| `\\` / `2` | `Tremolo{strokes=2}` |
| `////2` / `42` | `Tremolo{strokes=4, twoChord=true}` |
| `z` | `Tremolo{buzz}` |
| `0` / `clear` | `ClearTremolo` |
| `%` / `%1` | `MeasureRepeat{1}` |
| `%4` | `MeasureRepeat{4}` |
| `%1,2` | `Unsupported` |
| `banana` | `Invalid` |
| `""` | `Incomplete` (spec 20 convention: an empty popover has a neutral border) |

Manual: the acceptance list above, on a four-bar score with two staves.
