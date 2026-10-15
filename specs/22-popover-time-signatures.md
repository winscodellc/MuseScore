# 22 — Time signatures popover (Shift-M)
Status: todo · Depends on: 20 · Dorico refs: [time signatures](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_time_signatures_time_sigs_r.html), [pick-up bars](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_time_signatures_pick_up_bars_r.html)

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
- `INotationInteraction::addTimeSignature(Measure*, staffIdx, TimeSignature*)` — `src/notation/internal/notationinteraction.cpp:6359`; engraving side `EditTimeSig::addTimeSig` — `src/engraving/editing/edittimesig.h:37`.
- `applyPaletteElement` can also drop a `TimeSig` on a measure (what the palette does), including selection/range handling.
- `TimeSig` supports: type (`TimeSigType::NORMAL / FOUR_FOUR / ALLA_BREVE`), numerator/denominator strings (`setNumeratorString` for additive display — **verify** names in `src/engraving/dom/timesig.h`), beam groups (`Groups`), and a staff-local setting for the per-staff variant (**verify** how the palette's Ctrl-drop local time signature is represented).
- Pick-up measures: measure actual duration differs from nominal (`Measure::ticks` vs `timesig`); **locate** the existing operation behind "Measure properties → actual duration" (`Measure::adjustToLen` or an `EditMeasure` entry under `src/engraving/editing/`).
- Gaps: open meter, alternating/aggregate/interchangeable signatures, hidden-grouping display.

## Design
- Parser: `popover/parsers/timesigparser.{h,cpp}`.
  ```cpp
  struct TimeSigEntry {
      enum class Kind { Simple, Common, Cut, OpenX, OpenBlank, Additive, Alternating, Aggregate, Interchangeable };
      std::vector<int> numeratorGroups;   // [3,2,2]
      int denominator;
      bool groupingShown;
      std::optional<PickUp> pickUp;       // fraction of a whole note
  };
  ```
  Grammar: split off `,<n>` suffix (decimal) first; strip surrounding `[ ]` on the numerator part; `a+b+c/d` additive; denominator must be a power of two in {1,2,4,8,16,32,64}; numerator 1–99 (match MuseScore limit; verify).
- Handler: `handlers/timesighandler.cpp`.
  - Simple/Additive/Common/Cut → `Factory::createTimeSig`, set sig fraction, `TimeSigType`, numerator string for additive-shown (`3+2+2`); grouping-hidden form sets `Groups` only.
  - Insert via `applyPaletteElement(timesig, mods)` so replacement of an existing time signature works; Opt-Return passes `Qt::ControlModifier` (or the repo's local-timesig flag — **verify** `TimeSig::isLocal`/staff-specific mode) → selected staves only.
  - Pick-up: add the time signature, then set the first measure's actual length to `n × (1/denominator)` using the actual-duration operation found above; mark `setIrregular(true)` so it is excluded from measure numbering. Do not add beats unless Insert mode is on (`INotationNoteInput`/insert flag from spec 11).
- `prefill`: format a `TimeSig` as `n/d`, `c`, `cut`, or additive.
- `handlesItem`: `TimeSig`.
- Key binding: `Shift+M` → `command://dorico/popover/timesig`.

## Deferred / unsupported
- `x`, `X`, `open` (open meter): no MuseScore equivalent → `Unsupported`.
- Alternating `6/8 + 3/4`, aggregate `2/4|6/8`, `2/4:6/8`, interchangeable forms: `Unsupported` (parser recognises and reports).
- Pick-up bar beat fill only when Insert mode is on (v1: do not fill, create the short measure only).
- Per-staff independent time signatures beyond the local flag.
- `¢` glyph input depends on keyboard; accept `cut`/`cutc` as the main forms.

## Acceptance criteria
- [ ] Shift-M, `3/4`, Return on a bar in 4/4 changes that bar and following bars to 3/4 on every staff of a 2-staff score; Cmd-Z restores.
- [ ] `c` gives the common-time symbol (4/4), `cut`/`cutc` gives cut time (2/2).
- [ ] `3+2+2/8` shows an additive numerator string `3+2+2` over 8; `[2+3+2]/8` shows `7/8` with beams grouped 2+3+2.
- [ ] Opt-Return with only staff 1 selected changes only staff 1.
- [ ] Selecting an existing time signature and applying a new one replaces it.
- [ ] `4/4,1` creates a first bar of one quarter beat; `6/8,2`, `3/4,0.75` correct durations; measure numbering starts at 1 on the following bar.
- [ ] `x`, `open`, `6/8 + 3/4`, `2/4|6/8` show red + "not supported yet".
- [ ] Return on a selected time signature reopens the popover pre-filled.
- [ ] All parser tests pass.

## Tests
`src/dorico/tests/timesigparser_tests.cpp`:

| Input | Expected |
|---|---|
| `3/4` | Ok, Simple, [3]/4 |
| `6/8` | Ok, Simple |
| `2/2` | Ok |
| `c` | Ok, Common |
| `cut`, `cutc`, `¢` | Ok, Cut |
| `x`, `X` | Unsupported (OpenX) |
| `open` | Unsupported (OpenBlank) |
| `3+2+2/8` | Ok, Additive, groups [3,2,2], shown |
| `[2+3+2]/8` | Ok, Additive, groups [2,3,2], hidden |
| `6/8 + 3/4` | Unsupported (Alternating) |
| `2/4\|6/8` | Unsupported (Aggregate dashed) |
| `2/4:6/8` | Unsupported (Aggregate) |
| `2/4 (6/8)`, `2/4 = 6/8`, `2/4 / 6/8` | Unsupported (Interchangeable) |
| `4/4,1` | Ok, 4/4, pick-up 1/4 |
| `4/4,1.5` | Ok, pick-up 3/8 |
| `4/4,0.25` | Ok, pick-up 1/16 |
| `3/4,0.75` | Ok, pick-up 3/32 |
| `6/8,2` | Ok, pick-up 2/8 |
| `2/2,0.5` | Ok, pick-up 1/4 |
| `3/` | Incomplete |
| `3/0`, `3/3`, `0/4`, `abc` | Invalid |
