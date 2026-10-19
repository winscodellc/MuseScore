<!-- markdownlint-disable -->
# 36 — Popover: figured bass (Shift-G)

Status: done · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_figured_bass_figures_r.html`
- [P] `.../dorico_popovers_figured_bass_alterations_r.html`
- [P] `.../dorico_popovers_navigation_r.html`

Reference: [popovers](../reference/dorico-popovers.md) (B1, B2 "Figured bass") · [internals](../reference/musescore-internals.md) (section 3).

Names such as `parse`, `Result` and `apply` follow the contract in spec 20; rename to match its final API.

## Goal
Shift-G opens a popover. Typing `6`, `#6`, `6#42` or `6,4,2` and pressing Return puts figured bass under the selected bass note. Space moves to the next beat.

## Dorico behaviour

**Figures and suspensions**

| Entry | Meaning |
|---|---|
| `1`–`19` | Figures, auto-sorted |
| `6#42`, `6,#4,2` | Multiple figures; use commas to avoid ambiguity |
| `(#643)`, `#64(3)`, `(#)643` | Bracketed figures or accidentals |
| `<3>`, `{3}` | Hidden figures |
| Chord-symbol syntax (`Em7`, `Amaj7`, `G/B`) | Converted to figures automatically |
| `tasto`, `ts` | tasto solo |
| `4->3`, `4_3`, `4~3` | Suspension with resolution |
| `r=2`, `r=n` / `r=note`, `r=1/2n` | Suspension duration: quarter notes / bass-note length / fraction of the bass note |
| `d=2`, `d=n`, `d=1/2n` | Hold duration, same forms |
| `R`, `r`, `V`, `v` or `?` at the start | Follow Engraving Options for this figure |
| `O`, `o` or `!` at the start | Follow input literally |

**Accidentals and alterations**

| Entry | Meaning |
|---|---|
| `#`, `s` | Sharp; the figure can be omitted for a raised 3rd |
| `b`, `f` | Flat |
| `n`, `N` | Natural |
| `x`, `##`, `ds` | Double sharp |
| `x#`, `#x`, `###`, `ts` | Triple sharp |
| `bb`, `db` | Double flat |
| `bbb`, `tb` | Triple flat |
| `+` / `-` | Raise / lower a half-step |
| `d` | Diminished |
| `u` | Unaltered |

**Controls**:
- Local/global indicator is shown. Return uses the type shown in the popover; Opt-Return uses the opposite for this one entry. Opt-L / Opt-G set local / global.
- Space: next beat. Shift-Space: previous beat. Tab: start of the next bar. Shift-Tab: start of the current bar if mid-bar, otherwise the previous bar. Right / Left: next / previous note, rest, grid position or character. Cmd-Right / Cmd-Left: next / previous existing figure.
- Esc cancels.
- Editing existing: Return on a selected figured bass reopens the popover with its entry.

## MuseScore today
- Element: `FiguredBass` in the engraving DOM (`src/engraving/dom/figuredbass.h`; confirm). It stores a list of figure lines, each with optional prefix accidental, digit, suffix and continuation line.
- Entry path: `addTextToItem(TextStyleType::FIGURED_BASS, bassNote)` (`notationinteraction.cpp:6204`) → `Score::addText` → `startEditText`. The user types text; MuseScore parses it into items when editing ends. Ctrl+G is the existing shortcut (confirm).
- MuseScore's figured-bass text format is its own (one figure per line; accidental prefix, digit, optional suffix or continuation). Space inside the FB editor moves to the next note; confirm.
- Selection filter already has `FIGURED_BASS` (`selectionfilter.h`).
- No local/global distinction, no suspension durations, no hidden figures.

## Design
Built (as the chord symbols popover, spec 32): a navigating popover (`applyAt`) on a `FiguredBass` segment annotation.
- **Parser** `popover/parsers/figuredbassparser.h/.cpp`: pure `parseFiguredBass(text) -> { status, figures, text }`. `figures` are in typed order (`Figure{ accidental, number, alteration, bracketed, accidentalBracketed }`); `text` is the same figures as MuseScore lines, largest number on top (Dorico "ascending" read bottom-up), `\n`-joined: `n` -> `h`, `+` stays a suffix, `d` (diminished) -> `/` suffix.
  - A leading mode flag (`R r V v ? O o !`) is stripped and ignored.
  - With a comma, each piece is one figure (1-19); without, each digit is a figure, except `1` + digit = 10-19.
  - `(` `)` bracket the parts inside; `(#)6` brackets only the accidental.
  - A lone accidental is a figure with no number (raised third).
  - Empty text, a trailing comma or an unclosed bracket is `Incomplete`.
- **Handler** `popover/handlers/figuredbasshandler.h/.cpp`: `figuredBassPopover()`, registered in `popoverregistry.cpp` (replaced the placeholder). `applyAt` removes the staff's figured bass at the position, creates one with `FiguredBass::addFiguredBassToSegment`, builds its lines like the MusicXML import (see `reference/musescore-internals.md`) and selects it, so Return reopens it. `prefill` turns `plainText()` back into popover syntax.
- **Upstream edits:** none. One fork edit: `PopoverController`'s `singleSelectedItem` maps a clicked figured bass line (`FiguredBassItem`, what a click selects) to its `FiguredBass`.

## Deferred / unsupported
- Suspensions with resolution (`4->3`, `4_3`, `4~3`) and their duration (`r=`, `d=`).
- Hidden figures (`<3>`, `{3}`).
- Chord-symbol-to-figures conversion.
- `tasto` / `ts`.
- Engraving-option flags (`R`, `V`, `O`, `?`, `!`); the first character is accepted and ignored.
- Local vs global figured bass.
- Figure-line stacking beyond what MuseScore draws.
- Triple sharp/flat and `u` (unaltered) unless MuseScore's text format accepts them.
- Figures beyond 9 if MuseScore's editor cannot display two-digit figures.
- Anything unsupported is `Unsupported` (red border, "Not supported yet"), also: lowered figures (`-`), unaltered (`u`), and an accidental together with `+` / `d` on one figure (MuseScore allows a prefix or a suffix per line, not both).

## Acceptance criteria
- [x] Select a bass note, Shift-G, `6`, Return: a "6" figured bass appears under the note.
- [x] `#6` shows a sharp before the 6.
- [x] `6,4,2` stacks three figures.
- [x] `6#42` is read as 6, #4, 2.
- [x] `(#643)` produces bracketed figures, or shows the unsupported message if MuseScore cannot bracket.
- [x] Space applies and moves to the next bass note; Tab moves to the next bar.
- [x] Return on an existing figured bass reopens it pre-filled; changing it replaces the old one.
- [x] `4->3` shows the "Not supported yet" state and changes nothing.
- [x] One undo per entry.

## Tests
GoogleTest, `src/dorico/tests/figuredbassparser_tests.cpp` (also covers the MuseScore text, spellings, brackets and flags). A figure is written `[prefix]number`, and lists are shown in the order the parser returns them before the final sort:

| Input | Expected result |
|---|---|
| `6` | `[6]` |
| `#6` | `[#6]` |
| `#` | `[#, number=0]` |
| `6,4` | `[6, 4]` |
| `6,#4,2` | `[6, #4, 2]` |
| `6#42` | `[6, #4, 2]` |
| `64` | `[6, 4]` |
| `643` | `[6, 4, 3]` |
| `b7` | `[b7]` |
| `n3` | `[n3]` |
| `x4` | `[##4]` |
| `(#643)` | `[#6, 4, 3]` all bracketed |
| `10` | `[10]` (with no comma, two digits 10–19 read as one figure) |
| `4->3` | `Unsupported` |
| `<3>` | `Unsupported` |
| `ts` | `Unsupported` |
| `Em7` | `Unsupported` |
| `""` | `Incomplete` (framework convention: empty is a valid prefix) |
| `6,,4` | `Invalid` |
| `20` | `Invalid` |
