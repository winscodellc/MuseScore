<!-- markdownlint-disable -->
# 22 — Time signatures popover (Shift-M)
Status: done · Depends on: 20 · Dorico refs: [time signatures](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_time_signatures_time_sigs_r.html), [pick-up bars](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_time_signatures_pick_up_bars_r.html)

Reference: `specs/reference/dorico-popovers.md` (Time signatures), `specs/reference/musescore-internals.md` (§3).

## Goal
Shift-M, type `3/4`, Return → time signature at the selected bar (or caret bar) on all staves; `4/4,1` creates a one-beat pick-up bar.

## Dorico behaviour

### Entry syntax
| Entry | Meaning |
|---|---|
| `2/2`, `3/4`, `6/8`, `5/4` … | Simple |
| `c` | Common time |
| `cutc`, `cut`, `¢` | Cut time |
| `x`, `X` | Open meter shown as X |
| `open` | Open meter, no indication |
| `3+2+2/8` | Additive, grouping shown |
| `[2+3+2]/8` | Grouping set but not shown |
| `6/8 + 3/4` | Alternating (spaces required) |
| `2/4\|6/8` | Aggregate with dashed barlines |
| `2/4:6/8` | Aggregate without dashed barlines |
| `2/4 (6/8)`, `2/4 / 6/8`, `2/4 = 6/8`, `2/4 – 6/8` | Interchangeable; spaces required around the separators and before `(` |

### Pick-up bars
Syntax `[timesig],[n]` where n is a multiple of the denominator unit:

| Entry | Pick-up |
|---|---|
| `4/4,1` | Quarter |
| `4/4,1.5` | Dotted quarter |
| `4/4,0.25` | 16th |
| `3/4,0.75` | Dotted eighth |
| `6/8,2` | Two eighths |
| `2/2,0.5` | Quarter |

- Beats are only added to fill bars if **Insert mode (I)** is on.

### Placement and variants
| | Return | Opt-Return |
|---|---|---|
| Staves | All staves | Only the selected staves, or the staves the caret spans |

- Entering a time signature with an existing one selected **replaces** it.
- Time signature goes at the start of the bar containing the earliest selected item or caret; with Insert mode on, existing music is re-barred accordingly (MuseScore behaviour is the default re-bar with the time sig).
- Return on a selected time signature reopens the popover with its entry text.

## MuseScore today
- `EditTimeSig::addTimeSig(tx, score, measure, staffIdx, ts, local)` (`src/engraving/editing/edittimesig.cpp:453`) adds or replaces the time signature at a bar and re-bars the music up to the next one; it takes ownership of `ts`. `local = true` is MuseScore's per-staff time signature (`TimeSig::isLocal()` = stretch != 1); it only works over bars holding measure rests (`CANNOT_CHANGE_LOCAL_TIMESIG_MEASURE_NOT_EMPTY`). On failure it unwinds the transaction and returns without telling the caller.
- `INotationInteraction::addTimeSignature` (`notationinteraction.cpp:6359`) always passes `local = true`; `applyPaletteElement` on a range restores the old time signature at the range end (`applyPaletteElementToRange`), which Dorico does not do. Neither is used.
- `TimeSig` (`src/engraving/dom/timesig.h`): `setSig(Fraction, TimeSigType::FOUR_FOUR / ALLA_BREVE)`, `setNumeratorString` (needs `setDenominatorString` too, or the denominator disappears), `setGroups(Groups)`: beam groups as `GroupNode { pos in 32nds, action }`, `0x111` = new beam at every level.
- Pick-up: `Measure::adjustToLen(len)` (what Measure Properties > Actual duration and the New Score pick-up use) plus `Pid::EXCLUDE_FROM_NUMBERING`. Shortening deletes whatever lies past the new length. `addTimeSig` keeps an existing pick-up on the first bar.
- Clicking a time signature selects it on every staff (a list, so `selection()->element()` is null).
- Gaps: open meter, alternating/aggregate/interchangeable signatures, mid-bar time signatures.

## Design (as built)
- Parser `src/dorico/popover/parsers/timesigparser.{h,cpp}`: `parseTimeSig(text) -> { status, TimeSigEntry { kind Normal/Common/Cut, groups [3,2,2], denominator, groupingShown, pickup (Fraction) } }`. Order: compound separators (` +`, `|`, `:`, ` (`, ` /`, ` =`, ` –`, ` -`) → Unsupported when both sides are valid, Incomplete while the right side is being typed; then `,<n>` pick-up (decimal count of denominator units, must be a plain note value shorter than the bar, denominator ≤ 128); then `c`, `cut`/`cutc`/`¢`, `x`/`X`/`open` (Unsupported), `[a+b]/d` (hidden grouping), `a+b+c/d`. Denominators 1–64, numerator total 1–99. Prefixes (`3`, `3/`, `3/3` on the way to `3/32`, `cu`, `ope`, `[2+3`) are Incomplete.
- Handler `src/dorico/popover/handlers/timesighandler.{h,cpp}`, `timeSigPopover()`:
  - Bar: the caret's bar, else the bar of the earliest selected item (range: its start bar). Staves for Opt-Return: the caret's staff, the selected items' staves, or the range's staves.
  - Calls `EditTimeSig::addTimeSig` directly inside one undo step: global = staff 0, `local = false`; Opt-Return = a copy per staff with `local = true`. Then checks the staff's time signature at that tick and rolls back if `addTimeSig` silently failed.
  - Additive shown: numerator string `3+2+2` + denominator string; hidden and shown both set beam groups.
  - Pick-up: refused when local (MuseScore bars span every staff) or when notes / attached items lie past the pick-up in that bar (see Deferred); otherwise `EXCLUDE_FROM_NUMBERING` + `adjustToLen(pickup)`. In note input the caret is re-seated on the rewritten segment.
  - Outside note input the new time signature is selected, so Return reopens it.
  - `prefill`: `c`, `cut`, `3+2+2/8`, `[2+3+2]/8` (from beam groups), `n/d`, plus `,n` when the bar is an excluded short bar.
- `PopoverController` (ours, spec 20): `singleSelectedItem()` treats a list of same-type items at one tick as one item (top staff), so Return on a clicked time signature reopens the popover. Key signatures (spec 23) get this for free.
- Key binding `Shift+M` → `command://dorico/popover/timesig` was already in `shortcuts_dorico.json` (spec 20). No upstream edits.

## Deferred / unsupported
- `x`, `X`, `open` (open meter): no MuseScore equivalent → "Not supported yet".
- Alternating `6/8 + 3/4`, aggregate `2/4|6/8`, `2/4:6/8`, interchangeable `2/4 (6/8)`, `2/4 / 6/8`, `2/4 = 6/8`, `2/4 – 6/8`: recognised, "Not supported yet".
- **Mid-bar time signatures**: Dorico inputs at the caret / selected item even mid-bar; we always use the start of that bar (MuseScore time signatures live at bar starts).
- **Pick-up re-barring**: Dorico keeps music at its rhythmic positions and moves barlines. MuseScore's pick-up shortens the bar, deleting what lies past it, so we refuse (popover stays open, no message) when the cut-off part holds notes or attached items. Re-barring existing music into a pick-up needs a range rewrite (`ScoreRange` read / remove measures / insert / write, like `EditTimeSig::rewriteMeasures`).
- Pick-up with Opt-Return: refused (MuseScore bars are global).
- Local (Opt-Return) time signatures only over bars with measure rests (MuseScore restriction); otherwise nothing happens and the popover stays open.
- Insert-mode beat filling at the end of the affected region: MuseScore always fills the last re-barred bar with rests.
- A refused Return gives no message (the popover framework has no apply-error state).
- `¢` works only if the keyboard can type it; `cut`/`cutc` are the main forms.

## Acceptance criteria
- [x] Shift-M, `3/4`, Return on a bar in 4/4 changes that bar and following bars to 3/4 on every staff of a 2-staff score; Cmd-Z restores.
- [x] `c` gives the common-time symbol (4/4), `cut`/`cutc` gives cut time (2/2).
- [x] `3+2+2/8` shows an additive numerator string `3+2+2` over 8; `[2+3+2]/8` shows `7/8` with beams grouped 2+3+2.
- [x] Opt-Return with only staff 1 selected changes only staff 1.
- [x] Selecting an existing time signature and applying a new one replaces it.
- [x] `4/4,1` creates a first bar of one quarter beat; `6/8,2`, `3/4,0.75` correct durations; measure numbering starts at 1 on the following bar.
- [x] `x`, `open`, `6/8 + 3/4`, `2/4|6/8` show red + "not supported yet".
- [x] Return on a selected time signature reopens the popover pre-filled.
- [x] All parser tests pass.

## Tests
`src/dorico/tests/timesigparser_tests.cpp` (6 tests, all pass; full `dorico_tests` 69/69):

| Input | Expected |
|---|---|
| `3/4`, `6/8`, `2/2`, ` 5/4 `, `12/16` | Ok, Normal, one group |
| `c` | Ok, Common, 4/4 |
| `cut`, `cutc`, `¢` | Ok, Cut, 2/2 |
| `x`, `X`, `open` | Unsupported |
| `3+2+2/8` | Ok, groups [3,2,2], shown |
| `[2+3+2]/8` | Ok, groups [2,3,2], hidden, 7/8 |
| `6/8 + 3/4`, `2/4\|6/8`, `2/4:6/8`, `2/4 (6/8)`, `2/4 = 6/8`, `2/4 / 6/8`, `2/4 – 6/8` | Unsupported |
| `6/8 +`, `6/8 + 3/` | Incomplete |
| `4/4,1` / `4/4,1.5` / `4/4,0.25` / `3/4,0.75` / `6/8,2` / `2/2,0.5` / `c,1` | pick-up 1/4, 3/8, 1/16, **3/16** (dotted eighth; this table said 3/32 before), 2/8, 1/4, 1/4 |
| `4/4,`, `4/4,1.` | Incomplete |
| `4/4,4` (whole bar), `4/4,0`, `4/4,0.3`, `4/4,a` | Invalid |
| `""`, `3`, `3+`, `3/`, `[2+3`, `[2+3]`, `3/3`, `3/6`, `o`, `ope` | Incomplete (`3/3` is the start of `3/32`; this table said Invalid before) |
| `3/0`, `3/5`, `0/4`, `abc`, `3+/8`, `+3/4`, `100/4`, `[2+3/8`, `3/4/`, `3/,1` | Invalid |

### Manual test (2026-10-16, ClaudeUIControl on our build, temp 2-staff Violin/Viola score, 6 empty 4/4 bars)
| Step | Result |
|---|---|
| Bar 2 selected, Shift-M `3/4` Return | 3/4 on both staves from bar 2, music re-barred, new time signature selected |
| Return on it | popover reopens with `3/4` selected; `c` → common time, `cut` → ¢, `3+2+2/8` → 3+2+2 over 8, `[2+3+2]/8` → 7/8 |
| 7 eighths entered in the `[2+3+2]/8` bar | beamed 2+3+2 |
| Cmd-Z ×6 | back to the original, one step per Return |
| Only Violin bar 3, `3/4` Opt-Return | 3/4 on Violin only |
| Bar 1, `4/4,1` | quarter-rest pick-up; log reports bar 2 as "Measure 2, Displayed measure 1" |
| Bar 1, `6/8,2` / `3/4,0.75` | two eighth rests / 16th + 8th rest |
| `4/4,1` on the bar holding the eighths | refused, popover stays open, notes intact |
| `x`, `open`, `6/8 + 3/4`, `2/4\|6/8`, `2/4 = 6/8` | red border + "Not supported yet" |
| Clicked time signature (selects both staves), Return | reopens pre-filled (needed `singleSelectedItem`) |
| Note input, caret in a bar, Shift-M `2/4` Return | 2/4 at the caret's bar, note input continues in the new meter |
