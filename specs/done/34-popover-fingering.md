<!-- markdownlint-disable -->
# 34 — Popover: fingering (Shift-F)

Status: done · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_fingering_non_fretted_r.html`
- [P] `.../dorico_popovers_fingering_fretted_r.html`
- [P] `.../dorico_popovers_navigation_r.html`

Reference: [popovers](../reference/dorico-popovers.md) (B1, B2 "Fingerings") · [internals](../reference/musescore-internals.md) (section 3, `addTextToItem`).

Names such as `parse`, `Result` and `apply` follow the contract in spec 20; rename to match its final API.

## Goal
Shift-F opens a popover. Typing `1`, `1,3,5` or `L2` and pressing Return puts fingering text on the selected note or chord. Space moves to the next note so a passage can be fingered quickly.

## Dorico behaviour

**Non-fretted instruments**

| Entry | Meaning |
|---|---|
| `1` … `7` | Single fingering (also valve numbers and trombone slide positions) |
| `12` | Valves 1 and 2 |
| `1,3,5` | One fingering per chord note. Keyboard order is automatic: right hand for the upper staff, left hand for the lower. |
| `L`, `G`, `S`, `I` or `H` + n | Left hand, for example `L2` |
| `R`, `D` or `M` + n | Right hand, for example `R5` |
| `T` | Thumb |
| `2343` | Sequence on one note (for ornaments) |
| `1,1` | Same finger on two notes |
| `2(3)` | Alternative fingering |
| `[4]` | Editorial fingering |
| `1-3` | Finger substitution |
| `2/` | String shift indicator |

**Fretted instruments**

| Entry | Meaning |
|---|---|
| `0`–`5` | Left-hand fingers |
| `t` | Left-hand thumb |
| `1`–`5` or `p i m a e` | Right-hand fingers |
| `p`, `t`, `1` | Right-hand thumb |
| `e`, `x`, `c`, `o`, `5` | Right-hand pinky |

**Controls**:
- Fretted popover: Up switches to the left hand and Down to the right hand.
- Space: next note or chord. Shift-Space: previous note or chord.
- Right / Left: next / previous note, rest, grid position or character.
- Return closes. Esc cancels. No Opt-Return variant.
- Editing existing: Return on a selected fingering reopens it.

**Limits**: one rhythmic position at a time; at most one fingering per note at that position; one voice only (the top voice if several are selected).

## MuseScore today
- Fingering is a text element attached to a note. Text styles: `TextStyleType::FINGERING`, `LH_GUITAR_FINGERING`, `RH_GUITAR_FINGERING`, `STRING_NUMBER` (confirm in `src/engraving/types/types.h`).
- Entry path: `addTextToItem(style, note)` (`notationinteraction.cpp:6204`) → `Score::addText` (`edit.cpp:640`), or the palette's fingering cells via `applyPaletteElement`.
- Shortcuts: Ctrl+F for fingering text (the exact command id is in `notationcommands.h`).
- MuseScore stores fingering as plain text on a note. It has no model of hand, finger substitution or alternatives; those are just characters in the text.
- Staff type tells fretted (TAB / guitar) from non-fretted (`Staff::isTabStaff`, or the instrument family; confirm).

## Design (as built)
- **Parser** `popover/parsers/fingeringparser.{h,cpp}`: `parseFingering(text, fretted) -> FingeringParse{status, items}`, one `FingeringItem{text, style}` per chord note, lowest first. Split on commas; each item keeps its text as typed (`2(3)`, `[4]`, `1-3`, `2343`, `2/`). `L G S I H` + digits gives `LH_GUITAR_FINGERING`, `R D M` + digits gives `RH_GUITAR_FINGERING` (the letter is dropped from the text), `T` is plain, a lone `p i m a e x c o` is right hand and `t` is left hand. `fretted` only decides a bare digit (left hand on a fretted staff). The status never depends on `fretted`, so `validate` (which only gets the text) is exact. Empty text, `1,` and an open `(`/`[` are `Incomplete`; any other character or an empty item (`1,,3`) is `Invalid`.
- **Navigating popover** (`applyAt`, like chord symbols) with the new `PopoverDef::stepsByChord`: the position keeps the note's voice (not voice 1), Space / Shift-Space go to the next / previous chord of that voice (rests skipped; `moveChord`, `chordAt` in `popovernavigation.cpp`), Tab and Cmd-arrows do nothing, and the item at a position is its chord, whose `prefill` joins the notes' fingerings (a hand digit keeps its letter, `L2`).
- **Apply** (`handlers/fingeringhandler.cpp`): item *i* goes to the *i*-th note counted from the lowest; more items than notes fails, fewer leaves the upper notes alone. Each target note's existing fingerings are removed first (replace), then `Factory::createFingering(note, style)` + `setPlainText` + `undoAddElement`. A TAB staff whose staff type has no `showTabFingering` gets nothing, as in `Note::drop`. One transaction per entry.
- **Undo**: each Space entry and the final Return are separate undo steps (as for chord symbols); one Return is one step.
- **Upstream edits:** none.

## Deferred / unsupported
- Semantic rendering: finger substitution `1-3`, alternatives `2(3)`, editorial `[4]`, string-shift `2/` and sequences `2343` are stored as literal text. MuseScore draws them as typed.
- Automatic hand assignment on keyboard staves.
- Fretted-instrument switching with Up/Down (v1 infers left/right hand from the character).
- On a TAB staff without `showTabFingering` the popover accepts the entry and adds nothing (MuseScore hides fingering there), which looks like a no-op.
- Fingering several voices at once.
- Fingering a position that has more than one note in different voices.

## Acceptance criteria
- [x] Shift-F on a note, `3`, Return: a fingering "3" appears above the note.
- [x] On a three-note chord, `1,3,5`: 1 on the lowest note, 3 on the middle, 5 on the top.
- [x] Space applies the fingering and moves to the next chord, so `1 2 3 4` fingers four notes in order.
- [x] On a guitar staff, `p` and `i` create right-hand fingerings; `0`–`4` create left-hand fingerings.
- [x] Return on a selected fingering reopens the popover pre-filled; editing replaces the old text.
- [x] `zzz` shows the error state and changes nothing.
- [x] One undo per Return.

## Tests
GoogleTest, `src/dorico/tests/fingeringparser_tests.cpp`:

| Input | fretted | Expected |
|---|---|---|
| `1` | no | `[{"1", FINGERING}]` |
| `12` | no | `[{"12", FINGERING}]` |
| `1,3,5` | no | `[{"1"}, {"3"}, {"5"}]`, all `FINGERING` |
| `L2` | no | `[{"2", LH_GUITAR_FINGERING}]` |
| `R5` | no | `[{"5", RH_GUITAR_FINGERING}]` |
| `T` | no | `[{"T", FINGERING}]` |
| `2(3)` | no | `[{"2(3)", FINGERING}]` |
| `[4]` | no | `[{"[4]", FINGERING}]` |
| `1-3` | no | `[{"1-3", FINGERING}]` |
| `2343` | no | `[{"2343", FINGERING}]` |
| `p` | yes | `[{"p", RH_GUITAR_FINGERING}]` |
| `i,m,a` | yes | three `RH_GUITAR_FINGERING` items |
| `3` | yes | `[{"3", LH_GUITAR_FINGERING}]` |
| `t` | yes | `[{"t", LH_GUITAR_FINGERING}]` |
| `zzz` | no | `Invalid` |
| `""` | no | `Incomplete` (the framework treats empty text so) |
| `1,,3` | no | `Invalid` |
