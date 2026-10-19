<!-- markdownlint-disable -->
# 35 — Popover: note tools (Shift-I)

Status: done · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_note_tools_*_r.html`
- [M] `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_transformations/write_mode_note_tools_popover_r.html`
- [M] `.../write_mode/write_mode_note_input/write_mode_notes_adding_above_below_t.html`
- [M] `.../write_mode/write_mode_transposing_tools/write_mode_notes_transposing_existing_popover_t.html`

Reference: [popovers](../reference/dorico-popovers.md) (B2 "Note tools") · [editing](../reference/dorico-editing.md) (sections 2.5, 8.2, 8.4, 8.5) · [internals](../reference/musescore-internals.md) (sections 2 and 3).

Names such as `parse`, `Result` and `apply` follow the contract in spec 20; rename to match its final API.

## Goal
Shift-I opens a popover that works on the selected notes (also during note input). The first version supports two tools: adding notes at an interval (`3`, `-4`, `5 8 st`) and transposing (`t3`, `t-min6`). Invert, reverse, rotate, repeat and the mappings are out of scope for now.

## Dorico behaviour

Works only with notes selected, including during note input. In Setup mode Shift-I is the instrument picker, which is outside this spec.

**Interval syntax**

| Entry | Meaning |
|---|---|
| `1`–`15` | Interval degree |
| `M`, `maj`, `major` | Major (`M` is case-sensitive) |
| `m`, `min`, `minor` | Minor (`m` is case-sensitive) |
| `perf`, `per`, `p` | Perfect |
| `diminished`, `dim`, `d` | Diminished |
| `augmented`, `aug`, `a` | Augmented |
| `diatonic`, `diat` | Diatonic |
| `8 div` (`divisions`, `div`, `d`) | Octave divisions |
| `8 st` (`semitones`, `st`, `s`) | Half-steps |
| `8 qt` (`quartertones`, `qt`, `q`) | Quarter tones (24-EDO) |

Without a quality, the interval is diatonic in the current key: `3` above D in C major gives F♮.

**Add notes**

| Entry | Meaning |
|---|---|
| `3`, `3rd` | Add above |
| `-4`, `-4th` | Add below |
| `5 8 div`, `5 8 st`, `5 8 qt` | Add by octave divisions, spelled as the given degree |
| `-6 9 div` and so on | Same, below |
| `3,6`, `-3,3,4` | Several notes |
| `top`, `bottom` | Apply only to the top / bottom note of chords, e.g. `-M2,m3 bottom` |

**Transpose**

| Entry | Meaning |
|---|---|
| `t3` | Up a third |
| `t-6`, `t-min6` | Down |
| `t 5 8 div`, `t -6 9 qt` | By octave divisions |
| `t top -8` | Top notes of chords down an octave |
| `t bottom 8` | Bottom notes of chords up an octave |

**Controls**: Return applies and closes; Esc cancels; no Opt-Return variant. There is no existing item to reopen, so there is no edit-existing behaviour.

## MuseScore today
- Add interval: `add-interval-*` commands (Shift+Alt+digit; research section 1) add a diatonic interval above the current note in the key. This is the right engine for an interval with no quality. The `NoteAddingMode` / interval code lives in `notationactioncontroller.cpp` (grep `add-interval`). Downward intervals: confirm whether MuseScore has a separate command.
- Transpose:
  - Diatonic stepping: `pitch-up-diatonic` / `pitch-down-diatonic` → `movePitchDiatonic` (`notationinteraction.cpp`).
  - Semitone / octave: `pitch-up` / `pitch-down` → `movePitch` (`notationinteraction.cpp:4000`).
  - Interval transposition of a selection: the Transpose dialog uses an engraving-level function (grep `transpose(` in `src/engraving/editing/` and `src/notation/internal/`). Confirm its signature; it takes a mode, a direction, an interval or key, and flags.
- `Interval` (diatonic + chromatic steps) exists in the engraving DOM (look for `interval.h`); confirm.
- No 24-EDO support, so quarter-tone intervals cannot be represented.

## Design
One engine for both tools: pick the target notes, then move each one. `Note::transpose(Interval)` and `Note::transposeDiatonic(steps, keepAlterations)` (`note.cpp`) already do the move, set tpc1/tpc2 for transposing instruments, record an undo step, and return false when the pitch leaves the range. The `add-interval-*` and `pitch-up-diatonic` commands are not used: they act on the whole selection, so they cannot do `top` / `bottom`, compound intervals, or one undo step for `3,6`. `Transpose::transpose` is not used either: `Interval::allIntervals` stops at one octave.

- **New files** (all in the module, no upstream edits):
  - `popover/parsers/notetoolsparser.h/.cpp`: pure `parseNoteTools(text) -> NoteToolsParse { status, transpose, scope (All/Top/Bottom), intervals }` and `toSteps(IntervalSpec) -> {diatonic, semitones}`.
  - `popover/handlers/notetoolshandler.h/.cpp`: `noteToolsPopover()`, registered in `popoverregistry.cpp` in place of the placeholder.
  - `tests/notetoolsparser_tests.cpp`: the parser and the interval math (they share one file).
- `IntervalSpec{ degree (signed), quality (None/Major/Minor/Perfect/Diminished/Augmented), semitones (optional count of the unit form) }`. `followsKey()` is true with no quality and no unit.
- **Parser:**
  1. A leading `t` followed by a digit, `-` or a space means transpose.
  2. `top` / `bottom` may stand anywhere; the start of one waits (Incomplete).
  3. Items are split on commas: `[-][quality]degree[ordinal][ count unit]`. Only `M` / `m` are case-sensitive. A quality that does not fit the degree (`p3`, `M5`) is Invalid.
  4. Invert / reverse / rotate / repeat, `=` (pitch mapping), `to` / `snap` / `spell` (scale mapping) and `qt` are Unsupported. The start of an unsupported keyword is Incomplete.
  5. `div` is `st` (12-EDO). A transpose takes one interval. A plain unison (`1`, `-1`) on add is Incomplete: it adds nothing and may become `12`.
- **Degree and quality to semitones:** major / perfect size from `{0,2,4,5,7,9,11}` plus 12 per octave; minor and diminished (perfect intervals) one less; diminished (major / minor intervals) two less; augmented one more. A unit entry uses its own count, spelled by the degree.
- **Apply:**
  1. Targets are `selection().noteList()` (list and range; during note input the just-entered note is selected). Skip drum staves; with `top` / `bottom` keep each chord's `upNote()` / `downNote()`; one note per tie chain (`firstTiedNote()`).
  2. Moving a note: with a quality or unit, `transpose(Interval(diatonic, semitones))`. Without one, `transposeDiatonic(degree -+ 1, keepAlterations = steps % 7 == 0)`: a third follows the key, an octave keeps the accidental (as `Score::addInterval` does).
  3. Transpose moves every note of each tie chain. Add creates a unison copy in every chord of the chain (`Factory::createNote`, `setNval(source->noteVal())`, `undoAddElement`), moves the copies, then ties them as `Score::addInterval` does. Each interval is measured from the selected note, so `3,6` is a third and a sixth. If the chord already has the pitch, nothing is added.
  4. One transaction for the entry. If any note leaves the pitch range, the transaction is rolled back (our choice: Dorico documents nothing) and nothing changes.
  5. The selection is left as it is (our choice).

## Deferred / unsupported
- Invert (`inv`, `invert`, `mirror`, central-pitch forms).
- Reverse / retrograde (`ret`, `rev`, `pitch`, `rhythm`, `player`, `voice`).
- Rotate (`rot`, step counts, backwards).
- Repeat pitches (`repeat 2 pitches`).
- Pitch mapping (`c4=eb4,…`) and scale mapping (`c M to c m`, `snap`, `spell`).
- Quarter-tone and other non-12-EDO units (`qt`, and `div` outside 12).
- A "top/bottom" qualifier on mixed chord/single-note selections beyond the simple highest/lowest rule.
- Add-notes during real-time input (MIDI).
- Each of the deferred forms returns `Unsupported{reason}`, shown through the spec 20 error state, so the user sees why nothing happened.

## Acceptance criteria
- [x] Select D in C major, Shift-I, `3`, Return: F♮ is added above D (a chord D-F).
- [x] `-4` adds a note a fourth below.
- [x] `M3` and `m3` produce a major and minor third respectively, regardless of key.
- [x] `3,6` adds two notes in one undo step.
- [x] `5 8 st` on C adds G♯ (spelled as a fifth, 8 semitones above).
- [x] `t3` moves the selection up a diatonic third; `t-min6` moves it down a minor sixth.
- [x] `t top -8` moves only the top note of each chord down an octave.
- [x] `inv` shows an "unsupported" message and changes nothing.
- [x] `qt` shows an "unsupported" message.
- [x] During note input with a note selected, the popover works the same way.

## Tests
GoogleTest, `src/dorico/tests/notetoolsparser_tests.cpp` (parser and interval math). The apply path was checked by hand in the UI (all acceptance items, in C major). The parser table:

| Input | Expected result |
|---|---|
| `3` | `AddNotes[{degree=3}], scope=All` |
| `-4` | `AddNotes[{degree=-4}]` |
| `3rd` | `AddNotes[{degree=3}]` |
| `M3` | `AddNotes[{degree=3, quality=Major}]` |
| `m3` | `AddNotes[{degree=3, quality=Minor}]` |
| `perf5` / `p5` | `AddNotes[{5, Perfect}]` |
| `aug4` | `AddNotes[{4, Augmented}]` |
| `3,6` | two items |
| `-M2,m3 bottom` | `AddNotes[{-2, Major}, {3, Minor}], scope=Bottom` |
| `5 8 st` | `AddNotes[{5, unit=semitones, units=8}]` |
| `5 8 div` | same as the `st` case |
| `5 8 qt` | `Unsupported` |
| `t3` | `Transpose{3}` |
| `t-6` | `Transpose{-6}` |
| `t-min6` | `Transpose{-6, Minor}` |
| `t top -8` | `Transpose{-8}, scope=Top` |
| `t bottom 8` | `Transpose{8}, scope=Bottom` |
| `inv` / `rev` / `rot` / `rep 2 pit` | `Unsupported` |
| `c4=eb4` | `Unsupported` |
| `16` | `Invalid` (degree above 15) |
| `0` | `Invalid` |
| `banana` | `Invalid` |

Interval math:

| Spec | Semitones | Diatonic steps |
|---|---|---|
| Major 3rd | 4 | 2 |
| Minor 3rd | 3 | 2 |
| Perfect 5th | 7 | 4 |
| Diminished 5th | 6 | 4 |
| Augmented 4th | 6 | 3 |
| Major 9th | 14 | 8 |
| Perfect 8th | 12 | 7 |
| Degree 3, 8 semitones (unit form) | 8 | 2 |
