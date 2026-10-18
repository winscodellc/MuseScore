<!-- markdownlint-disable -->
# 31 — Popover: tuplets (`;` and `:`)

Status: done · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_tuplets_r.html`
- [P] `.../dorico_popovers_beat_units_r.html`
- [M] `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_durations/write_mode_tuplets_inputting_t.html`

Reference: [popovers](../reference/dorico-popovers.md) (B1, B2 "Tuplets", "Beat units") · [key commands](../reference/dorico-key-commands.md) (A3) · [internals](../reference/musescore-internals.md) (sections 2 and 3).

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

**Editing** (manual "Turning existing notes into tuplets", checked in Dorico 6 on 2026-10-17):
- Notes selected and no caret: the popover is pre-filled with a suggested ratio (`3:2` for three eighths, `5:4` for five eighths, `3:2` for one quarter); Return turns them into tuplets of that ratio, as many as needed (six eighths + `3:2` = two triplets).
- The freed time becomes rests after the tuplets (three eighths -> triplet + eighth rest; the following notes do not move).
- Without a beat unit the tuplet uses the selected notes' value. When the selection does not fill the last tuplet, Dorico fills it with the following notes: one quarter + `3` gives a quarter triplet of that quarter and the next two.
- A note crossing a tuplet's end is split and tied: one quarter + `3:2e` gives a triplet of the quarter and the first half of the next quarter, then the other half (tied), then an eighth rest.

**Note input** (checked in Dorico 6): the popover is pre-filled with `3`. Return shows the empty tuplet (rests under a bracket) at once; the next tuplet appears when a note is input after the first is full.

**Opt-Return**: no variant. Return applies. Esc cancels.

## MuseScore today
- `NotationInteraction::addTupletToSelectedChordRests(TupletOptions)` turns *each* selected chord/rest into its own tuplet (a quarter becomes an eighth triplet inside that quarter): not Dorico's behaviour, so the handler does not use it.
- `Score::addTuplet(cr, ratio, ...)` turns one chord/rest into a tuplet that spans that chord/rest's length, filled with rests of `length / normal`. MuseScore's tuplet completes when it is full; there is no "stay in the tuplet until stopped".

## Design (as built)
- **Files**: `src/dorico/popover/parsers/tupletparser.h/.cpp` (pure `parseTuplet(text) -> TupletParse{status, TupletEntry{actual, normal, unit?}}`, `defaultTupletNormal`, `tupletText`; the beat-unit table is the tempo parser's, exposed as `parseBeatUnit`), `src/dorico/popover/handlers/tupletshandler.h/.cpp`.
- **Parser**: ratio = digits, then `:` / `/` / a space before digits, then digits; the rest is the beat unit: a letter token may be attached (`3:2e`), any token may follow a space or hyphen (`5:4-6.`). `5:46` reads as the ratio 5:46 (digits are greedy), which then fails to apply.
- **Default normal count** (only the actual count typed): 2->3, 3->2, 4->3, 8->6, else the largest power of two below N (5->4, 6->4, 7->4, 9->8). Only `3` -> 3:2 is confirmed by the manual.
- **Outside note input** (`wrapSelection`): one voice, no existing tuplets, one bar. Unit = typed unit, else the shortest selected value. The selection is extended with following notes to fill whole tuplets; the region is cleared (`makeGap`, annotations kept), `count` rests of `normal x unit` are each turned into a tuplet (`Score::addTuplet`), the rest of the region becomes rests, and every note is written back with `setNoteRest` at its squeezed position, split at tuplet ends and tied. Pitches, spelling, extra chord notes, articulations and ties between the notes are kept.
- **Note input**: Return pushes a level `{entry, unit}` on a small tuplet-input state, sets the input duration to the unit and creates the empty tuplet at the caret. `DoricoNoteInputController::enterAtCaret` calls `startTupletAtCaret` before each note/rest: for each level whose tuplet the caret is not inside (matched by tick, track, ratio and base length), a new one is made at the caret (nested inside the previous level's), in an undo step merged with the entry. An inner tuplet that no longer fits in the outer one stops (Dorico); an outermost one that does not fit before the barline stops too.
- **`:`** = `command://dorico/stop-tuplet` (Shift-;): pops the innermost level and moves the caret past that tuplet (its unfilled slots stay rests). Arrow-key caret moves (`onCaretStep`, `onVertical`) and the end of note input clear all levels.
- **Framework addition**: `PopoverDef::selectionPrefill(notation)` gives the popover's initial text when it is not opened on an existing item.
- The Notes toolbox `x:y` button opens this popover.
- **Upstream edits:** none.

## Deferred / unsupported
- Wrapping selections that span voices, barlines, or existing tuplets: Return does nothing.
- Lyrics, slurs and other spanners on wrapped notes are not carried over; dynamics and other annotations stay at their old positions.
- A selected note that does not start on the tuplet's grid after squeezing: the whole entry is refused (nothing changes) rather than lose the note.
- Tuplets crossing barlines (Dorico's "allow tuplets to span barlines" option).
- Tuplet input on several staves at once (extended caret).
- The default ratios for counts other than 3 are assumptions (a check in Dorico on typing `5` alone was inconclusive).
- Return on an existing tuplet does not reopen the popover to edit its ratio.
- Tuplet-input state is one per app (tied to the score it started in).
- Cosmetic: right after a tuplet is created in note input, a faint orange highlight can stay on its second slot.

## Acceptance criteria
- [x] `;` with three eighth notes selected opens the popover pre-filled with `3:2`; Return makes them a triplet.
- [x] During note input, `3` then three notes gives a triplet and the tuplet is complete.
- [x] `5:4x` creates a 16th-note quintuplet regardless of the selected duration.
- [x] `5:4q.` creates a quintuplet whose base unit is a dotted quarter.
- [x] `3 2`, `3/2` and `3:2` give identical results.
- [x] `abc` and `0:2` show the error state and change nothing.
- [x] One undo reverts the tuplet.

## Tests
GoogleTest, `src/dorico/tests/tupletparser_tests.cpp`: ratios in every form, default normal counts, letter/digit/dotted beat units (`3:2e`, `5:4x`, `5:4q.`, `5:4-6.`, `5:4 6.`, `3:2 2w`, `3:2-9`), incomplete (`""`, `3:`, `5:4-`), invalid (`abc`, `x`, `0:2`, `3:0`, `:2`, `3:2k`, `3:2.`, `3:2-6x`, `3:2-1`), and `tupletText`.
