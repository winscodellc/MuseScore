<!-- markdownlint-disable -->
# 34 — Popover: fingering (Shift-F)

Status: todo · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_fingering_non_fretted_r.html`
- [P] `.../dorico_popovers_fingering_fretted_r.html`
- [P] `.../dorico_popovers_navigation_r.html`

Reference: [popovers](reference/dorico-popovers.md) (B1, B2 "Fingerings") · [internals](reference/musescore-internals.md) (section 3, `addTextToItem`).

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

## Design
- **New files** in `src/dorico/internal/popovers/fingering/`:
  - `fingeringparser.h/.cpp`: pure `parse(const std::string&, bool fretted) -> FingeringResult`.
- **Result:** `Fingering{ std::vector<Item> }`, one `Item{ text, style }` per chord note, ordered bottom to top.
- **Parser:**
  - Non-fretted: split on commas into one item per note. Each item keeps its text as typed (so `2(3)`, `[4]`, `1-3`, `2343`, `2/` survive as text). A leading `L/G/S/I/H` + digit becomes `LH_GUITAR_FINGERING` with only the digit; `R/D/M` + digit becomes `RH_GUITAR_FINGERING`; `T` stays `T` on the plain fingering style.
  - Fretted: digits `0`–`5` and `t` are left-hand (`LH_GUITAR_FINGERING`); letters `p i m a e x c o` are right-hand (`RH_GUITAR_FINGERING`). Up/Down toggle which style a bare digit means.
  - Reject any other character.
- **Apply:** for a chord, give the i-th item to the i-th note counted from the lowest. If there are more items than notes, report an error; if fewer, leave the upper notes alone. Create each fingering with `addTextToItem`, set the text, then `endEditText`; all inside one `startEdit()` / `apply()`.
- **Replace** an existing fingering on the same note rather than adding a second.
- **Space / Shift-Space:** apply, then re-anchor to the next/previous chord-rest in the same voice (reuse the spec 32 re-anchoring helper if present; otherwise the cursor primitives in `INotationInteraction`).
- **Keyboard staff hands:** the automatic right-hand-upper / left-hand-lower rule is deferred, and all digits use the plain `FINGERING` style.
- **Upstream edits:** none.

## Deferred / unsupported
- Semantic rendering: finger substitution `1-3`, alternatives `2(3)`, editorial `[4]`, string-shift `2/` and sequences `2343` are stored as literal text. MuseScore draws them as typed.
- Automatic hand assignment on keyboard staves.
- Fretted-instrument switching with Up/Down (v1 infers left/right hand from the character).
- Fingering several voices at once.
- Fingering a position that has more than one note in different voices.

## Acceptance criteria
- [ ] Shift-F on a note, `3`, Return: a fingering "3" appears above the note.
- [ ] On a three-note chord, `1,3,5`: 1 on the lowest note, 3 on the middle, 5 on the top.
- [ ] Space applies the fingering and moves to the next chord, so `1 2 3 4` fingers four notes in order.
- [ ] On a guitar staff, `p` and `i` create right-hand fingerings; `0`–`4` create left-hand fingerings.
- [ ] Return on a selected fingering reopens the popover pre-filled; editing replaces the old text.
- [ ] `zzz` shows the error state and changes nothing.
- [ ] One undo per Return.

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
| `""` | no | `Invalid` |
| `1,,3` | no | `Invalid` |
