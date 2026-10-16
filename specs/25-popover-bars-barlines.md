<!-- markdownlint-disable -->
# 25 — Bars and barlines popover (Shift-B)
Status: todo · Depends on: 20 · Dorico refs: [bars](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_bars_barlines_bars_r.html), [beats](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_bars_barlines_beats_r.html), [barlines](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_bars_barlines_barlines_r.html), [barline procedure](https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_input/write_mode_barlines_inputting_popover_t.html), [deleting bars](https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/notation_reference/notation_reference_bars/notation_reference_bars_deleting_t.html)

Reference: `specs/reference/dorico-popovers.md` (Bars and barlines), `specs/reference/dorico-editing.md` (§3.4), `specs/reference/musescore-internals.md` (§3).

## Goal
Shift-B, `+4` adds four bars, `-2` deletes two, `rest` makes a bar rest, `:|` sets an end-repeat barline.

## Dorico behaviour

### Bars
| Entry | Effect |
|---|---|
| `+2` or `2` | Add bars |
| `-6` | Delete bars (starting with the selected bar) |
| `rest` | Add a bar rest |
| `trim` | Delete empty bars at the end of the flow |

### Beats
| Entry | Effect |
|---|---|
| `+2q`, `+2-6`, `+2 6`, `+2/4` | Add beats |
| `-2q`, `-2-6`, `-2 6`, `-2/4` | Delete beats |
| `trim` | Delete empty beats at the end of the flow |

(Beat unit: `z`/`2` 64th, `y`/`3` 32nd, `x`/`4` 16th, `e`/`5` eighth, `q`/`6` quarter, `h`/`7` half, `w`/`8` whole, `2w`/`9` double whole, `.` dot.)

### Barlines
| Barline | Entries |
|---|---|
| Single | `\|`, `single`, `normal` |
| Double | `\|\|`, `double` |
| Triple (Pro) | `\|\|\|`, `triple` |
| Dashed | `:`, `dash`, `dashed` |
| Final | `\|]`, `fin`, `final` |
| Tick (top) (Pro) | `'`, `tick` |
| Tick (bottom) (Pro) | `tickbottom` |
| Short (center) (Pro) | `,`, `short` |
| Short (top) (Pro) | `shorttop` |
| Thick (Pro) | `thick` |
| Double thick | `doublethick`, `thickdouble` |
| Start repeat | `\|:`, `start` |
| End repeat | `:\|`, `end` |
| End/start repeat | `:\|:`, `:\|\|:`, `end-start`, `endstart` |

### Behaviour
- Return: all staves. Opt-Return: only the selected staves (barlines on single staves only work if those staves have an independent time signature).
- A repeat barline entered where one already exists **combines** into end/start.
- A final barline at the end of the flow can trim the flow.
- Deleting bars: music to the right moves left as in Insert mode; signposts inside the range are deleted.
- Local/Global indicator is shown in the popover (barlines).
- Return on a selected barline reopens the popover pre-filled; entering a barline over an existing one replaces it.
- `|:` with a range selection adds a matching `:|` at the range end.

## MuseScore today
- Add bars: `INotationInteraction::addBoxes(BoxType::Measure, count, AddBoxesTarget)` — `notationinteraction.cpp:4916`; delete: `removeSelectedMeasures` (:6423), `removeSelectedRange` (:6477). Empty-bars trimming is not an existing action.
- Barlines: no dedicated API; drop a `BarLine` element (`Factory::createBarLine`, `BarLine::setBarLineType(BarLineType)`) via `applyPaletteElement` (:2224). `BarLineType` includes `NORMAL`, `DOUBLE`, `START_REPEAT`, `END_REPEAT`, `BROKEN`, `END`, `END_START_REPEAT`, `DOTTED`, `REVERSE_END`, `HEAVY`, `DOUBLE_HEAVY` (**verify** in `src/engraving/dom/barline.h`).
- Tick/short barlines exist in MuseScore as span presets (`Pid::BARLINE_SPAN`, `BARLINE_SPAN_FROM`, `BARLINE_SPAN_TO`) — **verify**; otherwise Deferred.
- Bar rest = a measure containing only a whole-measure rest: "delete contents" on the measure (`Score::cmdDeleteSelection` for a measure range) produces it.
- No "insert beats" (shortening/lengthening a bar) action; changing a bar's length means measure properties.

## Design
- Parser: `popover/parsers/barsparser.{h,cpp}`.
  ```cpp
  struct BarsEntry {
      enum class Kind { AddBars, DeleteBars, BarRest, Trim, AddBeats, DeleteBeats, Barline };
      int count = 0;
      std::optional<BeatUnit> unit;      // for beats
      std::string barlineId;             // canonical id, e.g. "end_repeat"
  };
  ParseResult<BarsEntry> parseBars(std::string_view);
  ```
  Barline aliases live in one `static const std::unordered_map<std::string, std::string>` (alias → canonical id). Check the barline table (exact match after trim) **before** numbers, so `:` (dashed) and `|` are not mistaken for operators. Beat grammar: `^[+-]\d+(?:[ \-/]?(unit))?$`; `+2` without a unit is **bars**, not beats; a unit makes it beats (matching the manual's examples).
- Handler: `handlers/barshandler.cpp`.
  - `AddBars` / `+N` / `N`: `addBoxes(BoxType::Measure, N, AddBoxesTarget::AfterSelection)`; no selection + note input → after the caret measure. (Dorico inserts after the selected bar; **verify** in-app before finalising.)
  - `DeleteBars`: extend the selection to N measures from the selected one, then `removeSelectedMeasures()`.
  - `BarRest`: clear the selected measures' contents (leaves a bar rest).
  - `Trim`: find the last measure with any `ChordRest` that is not a rest/with any element; select the empty measures after it; `removeSelectedMeasures()`.
  - `Barline`: create a `BarLine` of the mapped type and `applyPaletteElement`. If the target barline is already `END_REPEAT` and a `START_REPEAT` is requested (or vice versa), use `END_START_REPEAT`. Range selection with `start`: also drop `END_REPEAT` at the end measure.
  - Opt-Return (local): apply only to the selected staves via the palette modifier (**verify** barline local drop semantic = staff-only `BarLine` span).
- `prefill`: barline → canonical alias (`end`, `double`…).
- `handlesItem`: `BarLine`.
- Key binding: `Shift+B` → `command://dorico/popover/bars`.

## Deferred / unsupported
- Beats: `+2q`, `-2q`, `+2-6`, `+2 6`, `+2/4`, `trim` for beats → `Unsupported` (no beat-granular insert/delete in MuseScore).
- Triple barline `|||`, tick/short barlines (`'`, `tick`, `tickbottom`, `,`, `short`, `shorttop`) unless the span presets are verified usable → `Unsupported`.
- Final barline trimming the flow.
- Independent per-staff barlines when staves lack an independent time signature.
- Signpost handling (no signposts in MuseScore).

## Acceptance criteria
- [ ] Shift-B, `+4`, Return with bar 3 selected → 4 empty bars inserted after bar 3; Cmd-Z removes them.
- [ ] `2` behaves like `+2`.
- [ ] `-2` on bar 2 deletes bars 2–3 and later music moves left; Cmd-Z restores.
- [ ] `rest` on a bar with notes leaves a bar rest.
- [ ] `trim` removes only trailing empty bars (a score ending in 3 empty bars shrinks by 3; with a note in the last bar nothing changes).
- [ ] `double`, `||`, `final`, `|]`, `dashed`, `:`, `thick`, `doublethick` give the matching MuseScore barline.
- [ ] `|:` at a bar start → start repeat; `:|` → end repeat; entering `|:` where an end repeat exists makes end/start.
- [ ] Range selection + `|:` adds a closing `:|` at the range end.
- [ ] Selecting an existing barline, Return, popover pre-filled; applying a new one replaces it.
- [ ] `+2q`, `triple`, `tick` are red + "not supported yet".
- [ ] Parser tests pass.

## Tests
`src/dorico/tests/barsparser_tests.cpp`:

| Input | Expected |
|---|---|
| `+2`, `2` | Ok, AddBars 2 |
| `+12` | Ok, AddBars 12 |
| `-6` | Ok, DeleteBars 6 |
| `rest` | Ok, BarRest |
| `trim` | Ok, Trim |
| `+2q` | Unsupported (AddBeats 2 quarter) |
| `-2-6`, `-2 6`, `-2/4` | Unsupported (DeleteBeats 2 quarter) |
| `\|`, `single`, `normal` | Ok, single |
| `\|\|`, `double` | Ok, double |
| `\|\|\|`, `triple` | Unsupported |
| `:`, `dash`, `dashed` | Ok, dashed |
| `\|]`, `fin`, `final` | Ok, final |
| `'`, `tick`, `tickbottom`, `,`, `short`, `shorttop` | Unsupported (or Ok if span presets verified) |
| `thick` | Ok, heavy |
| `doublethick`, `thickdouble` | Ok, double heavy |
| `\|:`, `start` | Ok, start repeat |
| `:\|`, `end` | Ok, end repeat |
| `:\|:`, `:\|\|:`, `end-start`, `endstart` | Ok, end/start |
| `+` | Incomplete |
| `foo`, `+x`, `0` | Invalid |
