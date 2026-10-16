<!-- markdownlint-disable -->
# 31 — Popover: tuplets (`;` and `:`)

Status: todo · Depends on: [20-popover-framework](done/20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_tuplets_r.html`
- [P] `.../dorico_popovers_beat_units_r.html`
- [M] `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_durations/write_mode_tuplets_inputting_t.html`

Reference: [popovers](reference/dorico-popovers.md) (B1, B2 "Tuplets", "Beat units") · [key commands](reference/dorico-key-commands.md) (A3) · [internals](reference/musescore-internals.md) (sections 2 and 3).

Names such as `parse`, `Result` and `apply` follow the contract in spec 20; rename to match its final API.

## Goal
`;` opens a small popover. Typing a ratio such as `3`, `5:4` or `3:2e` and pressing Return starts tuplet input (during note input) or wraps the selected notes in a tuplet (outside note input).

## Dorico behaviour

**Syntax** (reference B2):

| Entry | Tuplet |
|---|---|
| `3`, `3:2`, `3/2`, `3 2` | Triplet (3 in the time of 2) |
| `3:4`, `3/4`, `3 4` | 3 in the time of 4 |
| `4:3`, `4/3`, `4 3` | Quadruplet |
| `5:2` / `5:4` / `5:6` (also `/` and space forms) | Quintuplets |
| `6:4` | Sextuplet |
| `7:4` | Septuplet |
| `9:8` | Nonuplet |
| `2:3` | Duplet |
| `5:4q.`, `5/4q.`, `5 4q.`, `5:4-6.`, `5:4 6.` | Quintuplet of dotted quarters |

- A trailing beat unit sets the tuplet's base duration: `3:2e` = eighth-note triplets, `5:4x` = 16th-note quintuplets.
- Without a unit, the currently selected duration is used.
- A numeric beat unit must be separated from the ratio by a space or a hyphen (`5:4-6.`, `5:4 6.`). A letter unit can be attached.
- Beat-unit tokens (reference B2): `z`/`2` = 64th, `y`/`3` = 32nd, `x`/`4` = 16th, `e`/`5` = eighth, `q`/`6` = quarter, `h`/`7` = half, `w`/`8` = whole, `2w`/`9` = double whole. `.` adds a dot.

**Note input**:
- The tuplet stays active, so every note entered is inside it, until `:` (Shift-;), an arrow-key caret move, or the end of note input.
- `:` once stops the inner tuplet of a nested pair; twice stops both.
- Opening another tuplet without stopping the first creates a nested tuplet.

**Editing**:
- Notes selected and no caret: the popover wraps them in the entered tuplet.
- When opened on existing notes, the popover is pre-filled with a suggested ratio.

**Opt-Return**: no variant. Return applies. Esc cancels.

## MuseScore today
- `NotationInteraction::addTupletToSelectedChordRests(TupletOptions)` (`src/notation/internal/notationinteraction.cpp:5687`) wraps selected chord/rests. `TupletOptions` carries the ratio, number type, bracket type and an auto base length (confirm the fields in `notationtypes.h`).
- `INotationNoteInput::addTuplet(...)` (`src/notation/inotationnoteinput.h`) starts a tuplet during note input. MuseScore's tuplet completes when it is full; there is no "stay in the tuplet until stopped".
- Existing shortcuts Ctrl+2…9 create fixed-ratio tuplets. Spec 10 unbinds them if the keys are needed.

## Design
- **New files** in `src/dorico/internal/popovers/tuplets/`:
  - `tupletparser.h/.cpp`: pure `parse(const std::string&, Duration selected) -> TupletResult`.
- **Result:** `Tuplet{actual, normal, baseUnit (optional), dots}` or `Invalid`.
- **Parser steps:**
  1. Trim the input.
  2. Split off the ratio: digits, then an optional separator (`:`, `/` or a single space), then optional digits.
  3. The remainder is the beat unit: letter token or number token, optionally preceded by a space or hyphen, plus trailing dots.
  4. If the denominator is missing, use the default table below.
- **Default normal count when only the actual count is typed** (assumption, flag to check against Dorico's behaviour): 2→3, 3→2, 4→3, 5→4, 6→4, 7→4, 8→6, 9→8; any larger N → the largest power of two below N.
- **Apply:**
  - Not in note input and a chord/rest selection exists: build `TupletOptions` from the result and call `addTupletToSelectedChordRests`.
  - In note input: call `INotationNoteInput::addTuplet` with the ratio and base length.
  - `baseUnit` sets the base length; absent means the current input duration.
- **`:` (stop tuplet input)**: register `command://dorico/stop-tuplet`. MuseScore's tuplet ends by itself when full, so this command only ends an early partial tuplet by filling the remainder with rests. See Deferred.
- **Pre-fill on existing notes**: derive the suggested ratio from the selection's total duration against its note count (for example three equal notes in a half-note span → `3:2`). Keep it to equal-length notes.
- **Upstream edits:** none.

## Deferred / unsupported
- Staying inside a tuplet until `:` is pressed (MuseScore completes the tuplet when it is full). `:` is mapped to "fill the remaining slots with rests", which differs from Dorico.
- Nested tuplets typed as two successive entries during note input (MuseScore supports nesting only through its own palette flow; check before promising it).
- Dotted or double-dotted beat units when the selected notes do not divide evenly.
- Arrow-key caret movement ending tuplet input.
- Beat-unit `2w` / `9` (double whole) if MuseScore's duration enum rejects it for tuplets.

## Acceptance criteria
- [ ] `;` with three eighth notes selected opens the popover pre-filled with `3:2`; Return makes them a triplet.
- [ ] During note input, `3` then three notes gives a triplet and the tuplet is complete.
- [ ] `5:4x` creates a 16th-note quintuplet regardless of the selected duration.
- [ ] `5:4q.` creates a quintuplet whose base unit is a dotted quarter.
- [ ] `3 2`, `3/2` and `3:2` give identical results.
- [ ] `abc` and `0:2` show the error state and change nothing.
- [ ] One undo reverts the tuplet.

## Tests
GoogleTest, `src/dorico/tests/tupletparser_tests.cpp`. `selected` is the current input duration passed in (quarter unless stated).

| Input | Expected result |
|---|---|
| `3` | `{actual=3, normal=2, unit=selected}` |
| `3:2` / `3/2` / `3 2` | `{3, 2, selected}` |
| `3:4` | `{3, 4, selected}` |
| `4` | `{4, 3, selected}` |
| `5` | `{5, 4, selected}` |
| `9` | `{9, 8, selected}` |
| `2` | `{2, 3, selected}` |
| `3:2e` | `{3, 2, unit=eighth}` |
| `5:4x` | `{5, 4, unit=16th}` |
| `5:4q.` | `{5, 4, unit=quarter, dots=1}` |
| `5:4-6.` | `{5, 4, unit=quarter, dots=1}` |
| `5:4 6.` | `{5, 4, unit=quarter, dots=1}` |
| `5:46` | `Invalid` (a numeric unit needs a separator) |
| `0:2` | `Invalid` |
| `3:0` | `Invalid` |
| `x` | `Invalid` |
| `""` | `Invalid` |
