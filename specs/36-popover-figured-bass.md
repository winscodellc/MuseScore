# 36 — Popover: figured bass (Shift-G)

Status: todo · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_figured_bass_figures_r.html`
- [P] `.../dorico_popovers_figured_bass_alterations_r.html`
- [P] `.../dorico_popovers_navigation_r.html`

Reference: [popovers](reference/dorico-popovers.md) (B1, B2 "Figured bass") · [internals](reference/musescore-internals.md) (section 3).

Names such as `parse`, `Result` and `apply` follow the contract in spec 20; rename to match its final API.

## Goal
Shift-G opens a popover. Typing `6`, `#6`, `6#42` or `4-3` and pressing Return puts figured bass under the selected bass note. Space moves to the next beat.

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
- **New files** in `src/dorico/internal/popovers/figuredbass/`:
  - `figuredbassparser.h/.cpp`: pure `parse(const std::string&) -> FbResult`.
- **Result:** `Figures{ std::vector<Figure> }` where `Figure{ prefix (accidental or none), number (0 if none), bracketed }`, or `Invalid` / `Unsupported{reason}`.
- **Parser steps:**
  1. Strip a leading mode flag (`R r V v ? O o !`) and ignore it.
  2. If the text contains a comma, split on commas; each piece is one figure.
  3. Otherwise scan left to right: an accidental run (`#`, `b`, `n`, `x`, `s`, `f` and the doubled forms) followed by digits forms one figure. A number greater than 9 is read as one figure only if it is 10–19 and no comma was used.
  4. Parentheses around one or more figures or accidentals set `bracketed`.
  5. A lone accidental with no digit is a figure with `number = 0` (raised third).
  6. `+` / `-` suffixes become a suffix on the figure.
- **Order:** emit figures from the largest number at the top to the smallest at the bottom, the usual engraving order. The Dorico table says "auto-sorted ascending"; check Dorico's rendered order before finalising and flip the sort if it is the other way around.
- **Render:** build MuseScore figured-bass text, one figure per line (`\n` between lines), prefix accidental as `#`, `b` or `n` before the digit, double sharp/flat as the two-character forms MuseScore accepts (confirm). Put that text into the figured bass element within a single edit transaction.
- **Space:** apply, then move to the next chord-rest of the bass voice and reopen. Reuse the re-anchoring helper from spec 32.
- **Opt-Return, Opt-L, Opt-G:** accepted and ignored (no local/global).
- **Upstream edits:** none.

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
- Anything unsupported returns `Unsupported{reason}` through the spec 20 error state.

## Acceptance criteria
- [ ] Select a bass note, Shift-G, `6`, Return: a "6" figured bass appears under the note.
- [ ] `#6` shows a sharp before the 6.
- [ ] `6,4,2` stacks three figures.
- [ ] `6#42` is read as 6, #4, 2.
- [ ] `(#643)` produces bracketed figures, or shows the unsupported message if MuseScore cannot bracket.
- [ ] Space applies and moves to the next bass note; Tab moves to the next bar.
- [ ] Return on an existing figured bass reopens it pre-filled; changing it replaces the old one.
- [ ] `4->3` shows an "unsupported" message and changes nothing.
- [ ] One undo per entry.

## Tests
GoogleTest, `src/dorico/tests/figuredbassparser_tests.cpp`. A figure is written `[prefix]number`, and lists are shown in the order the parser returns them before the final sort:

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
| `""` | `Invalid` |
| `6,,4` | `Invalid` |
| `20` | `Invalid` |
