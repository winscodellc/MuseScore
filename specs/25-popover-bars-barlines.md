<!-- markdownlint-disable -->
# 25 — Bars and barlines popover (Shift-B)
Status: pending review · Depends on: 20 · Dorico refs: [bars](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_bars_barlines_bars_r.html), [beats](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_bars_barlines_beats_r.html), [barlines](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_bars_barlines_barlines_r.html), [barline procedure](https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_input/write_mode_barlines_inputting_popover_t.html), [deleting bars](https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/notation_reference/notation_reference_bars/notation_reference_bars_deleting_t.html)

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
From the manual ([bars/beats procedure](https://www.steinberg.help/api/khub/maps/BYbh2hvBDwUCjSijIhHcqA/topics/BtGYQAkc2C4cWPQo5EpAlw/content), [barline procedure](https://www.steinberg.help/api/khub/maps/BYbh2hvBDwUCjSijIhHcqA/topics/lW8qk1i9lE_lj7kF7yG2dg/content), [bar rests](https://www.steinberg.help/api/khub/maps/BYbh2hvBDwUCjSijIhHcqA/topics/m~jhm9oiErcz6AZaXN1BoQ/content)):
- Return: all staves. Opt-Return: only the selected staves (barlines on single staves only work if those staves have an independent time signature).
- Bars "are added after selected barlines and before other selected items, including time signatures". During note input they go in from the caret; a caret mid-bar gets extra beats so the last new bar is whole.
- A barline goes at the rhythmic position of the earliest selected item, or at the caret. Mid-bar, the music around it adjusts (Dorico re-bars).
- A repeat barline entered where one already exists **combines** into end/start.
- A final barline at the end of the flow can trim the flow.
- `rest` puts a bar rest in the caret's voice (documented for note input); notes there are replaced.
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
As built. Parser `popover/parsers/barsparser.{h,cpp}` (`parseBars` -> `BarsEntry { Kind { AddBars, DeleteBars, BarRest, Trim, Barline }, count, BarLineType }`, `barlineText` for prefill), handler `popover/handlers/barshandler.{h,cpp}` (`barsPopover()`), registered in `popoverregistry.cpp`. No upstream edits.
- Parser: entries are trimmed and lower-cased. The barline table (exact match) is checked **before** numbers, so `:` and `|` are barlines. `[+-]?N` is bars (`-` deletes, N 1..999); `N` followed by a beat unit (`q`, `e.`, ` 6`, `-6`, `/4`) is beats, recognised as Unsupported. Prefixes of any entry are Incomplete.
- `+N` / `N`: with a selection, `INotationInteraction::addBoxes(Measure, N, BeforeSelection)`: before the earliest selected item's bar, after a selected barline (MuseScore's own Insert Measures rule matches the manual). At the caret: `Score::insertMeasure` before the caret's bar if the caret is at its start, else after that bar.
- `-N`: from the caret's / earliest selected item's bar, N bars (clamped at the end) are range-selected and `Score::cmdTimeDelete` removes them.
- `rest`: in each target bar and voice (caret's voice; each selected item's voice; voice 1 of each staff of a range) `Score::deleteRange` clears the music, then the rests are swapped for one bar rest (as `cmdFullMeasureRest` does). The caret stays in the bar; outside note input the bar rest is selected.
- `trim`: `Score::cmdRemoveEmptyTrailingMeasures` (MuseScore's Remove Empty Trailing Measures).
- Barlines: the position is a selected barline's tick, else the caret / earliest selected item. Mid-bar, the bar is split there first (`SplitJoinMeasure::splitMeasure`), so the barline is a real bar boundary. The barline is then dropped on the bar before it (start repeat: the bar after it) through `Measure::drop`, the Barlines palette path, so linked parts, mm-rests and repeat flags behave as in MuseScore. `|:` where an end repeat is (or `:|` where a start repeat is) becomes end/start; a repeat first resets a double/final barline there to single, so it replaces it. A range with `|:` also gets `:|` at its end. A barline MuseScore placed mid-bar is changed in place. Outside note input the new barline is selected, so Return reopens it.
- Opt-Return: barlines go on each selected staff only (`ControlModifier` drop). Repeats always span every staff (MuseScore model). Bars cannot be added / deleted locally: Opt-Return does nothing for them.
- `prefill`: `single`, `double`, `dashed`, `final`, `thick`, `doublethick`, `start`, `end`, `endstart`; empty for MuseScore-only types (dotted, reverse final).
- `handlesItem`: `BarLine`. Key binding `Shift+B` -> `command://dorico/popover/bars` (already in `shortcuts_dorico.json` since spec 20).

## Deferred / unsupported
- Beats: `+2q`, `-2q`, `+2-6`, `+2 6`, `+2/4` -> `Unsupported` (no beat-granular insert/delete in MuseScore). `trim` removes empty bars only, never part of a bar.
- Triple barline `|||`, tick/short barlines (`'`, `tick`, `tickbottom`, `,`, `short`, `shorttop`) -> `Unsupported`. MuseScore has span presets (`BARLINE_SPAN_FROM/TO`) that could draw tick / short barlines; not wired.
- Bars at a caret in the middle of a bar: whole bars go after that bar (Dorico adds beats to finish the caret's bar).
- Bars added or deleted on some staves only (Opt-Return).
- `rest` over a range rewrites voice 1 only.
- Final barline trimming the flow.
- Independent per-staff barlines when staves lack an independent time signature (MuseScore allows a per-staff barline type anyway).
- Signpost handling (no signposts in MuseScore).

## Acceptance criteria
- [ ] Shift-B, `+4`, Return with bar 3 selected → 4 empty bars inserted before bar 3 (the manual: before selected items, after a selected barline); Cmd-Z removes them.
- [ ] `2` behaves like `+2`.
- [ ] `-2` on bar 2 deletes bars 2–3 and later music moves left; Cmd-Z restores.
- [ ] `rest` on a bar with notes leaves a bar rest.
- [ ] `trim` removes only trailing empty bars (a score ending in 3 empty bars shrinks by 3; with a note in the last bar nothing changes).
- [ ] `double`, `||`, `final`, `|]`, `dashed`, `:`, `thick`, `doublethick` give the matching MuseScore barline.
- [ ] `|:` at a bar start → start repeat; `:|` → end repeat; entering `|:` where an end repeat exists makes end/start.
- [ ] Range selection + `|:` adds a closing `:|` at the range end.
- [ ] Selecting an existing barline, Return, popover pre-filled; applying a new one replaces it.
- [ ] `+2q`, `triple`, `tick` are red + "not supported yet".
- [x] Parser tests pass.

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

Built and unit-tested (`dorico_tests` 85/85, 6 new `DoricoBarsParser` tests). Manual check in the app pending.
