<!-- markdownlint-disable -->
# 27 — Holds and pauses popover (Shift-H)
Status: todo · Depends on: 20 · Dorico refs: [fermatas](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_holds_pauses_fermatas_r.html), [breath marks](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_holds_pauses_breath_marks_r.html), [caesuras](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_holds_pauses_caesuras_r.html)

Reference: `specs/reference/dorico-popovers.md` (Holds and pauses), `specs/reference/musescore-internals.md` (§3).

## Goal
Shift-H, type `fer`, `comma`, `//`, Return → fermata, breath mark or caesura at the selection or caret.

## Dorico behaviour

### Fermatas
| Entry | Fermata |
|---|---|
| `fer`, `fermata`, `pause` | Fermata |
| `verylongfermata`, `verylongfer` | Very long |
| `longfermata`, `longfer` | Long |
| `shortfermata`, `shortfer` | Short |
| `veryshortfermata`, `veryshortfer` | Very short |
| `shorthenzefermata`, `shorthenzefer` | Short (Henze) |
| `longhenzefermata`, `longhenzefer` | Long (Henze) |
| `curlew` | Curlew (Britten) |

### Breath marks
| Entry | Breath mark |
|---|---|
| `breathmarkcomma`, `comma`, `,` | Comma |
| `breathmarktick` | Tick-like |
| `breathmarkupbow` | Upbow-like |
| `breathmarksalzedo` | Salzedo |

### Caesuras
| Entry | Caesura |
|---|---|
| `caes`, `caesura`, `//` | Caesura |
| `caesurathick`, `thickcaesura`, `thick//` | Thick |
| `caesuracurved`, `curvedcaesura`, `curved//` | Curved |
| `caesurashort`, `shortcaesura`, `short//` | Short |
| `caesurasingle`, `singlecaesura`, `single//` | Single-stroke |

### Placement
- Fermatas go on **all staves**; breath marks go on the **selected or caret staves**, to the right of the note; caesuras go on all staves, to the **left** of the note.
- No local/global distinction in the popover (Return only).
- Selection: item at the earliest selected item; caret: at the caret.
- Return on a selected hold/pause reopens the popover pre-filled.

## MuseScore today
- Fermata: `Fermata` element (`src/engraving/dom/fermata.h`), attached to a segment per track, SymId variants `fermataAbove`, `fermataShortAbove`, `fermataLongAbove`, `fermataVeryShortAbove`, `fermataVeryLongAbove`, `fermataShortHenzeAbove`, `fermataLongHenzeAbove` (**verify** names; the palette has them). Applied with `applyPaletteElement` (`notationinteraction.cpp:2224`), which drops it on the selected chord/rest's segment.
- Breath/caesura: `Breath` element with `SymId` (`breathMarkComma`, `breathMarkTick`, `breathMarkUpbow`, `breathMarkSalzedo`, `caesura`, `caesuraCurved`, `caesuraShort`, `caesuraThick`, `caesuraSingleStroke`) — **verify** against the palette's Breaths & Pauses set (`src/palette/internal/palettecreator.cpp`).
- MuseScore places a breath mark after the note; there is no "to the left of the note" caesura placement (see Deferred).
- No API that adds a fermata to every staff: loop over staves at the segment.

## Design
- Parser: `popover/parsers/holdsparser.{h,cpp}`.
  ```cpp
  struct HoldEntry { enum class Kind { Fermata, Breath, Caesura }; Kind kind; std::string variant; };
  ParseResult<HoldEntry> parseHold(std::string_view);   // variant = canonical id, e.g. "fermata_long"
  ```
  One alias table `alias → (Kind, variant)` (static map), case-insensitive. Prefix matching for Incomplete.
- Handler: `handlers/holdshandler.cpp`.
  - Fermata: for each staff of the score (all staves; Dorico), locate the `ChordRest` at the selection's start tick on that staff (create nothing if there is none — skip silently), `Factory::createFermata`, `setSymIdAndTimeStretch(symId)`, `applyPaletteElement`. For simplicity use the selection start segment and loop over staves with `Segment::element(track)`.
  - Breath: `Factory::createBreath`, `setSymId`, apply on selected staves only (selected or caret staves).
  - Caesura: same as breath but all staves (placement fidelity in Deferred).
  - Replace behaviour: if a fermata/breath already exists at that segment+track, replace it (remove then add inside the same transaction).
- `prefill`: `Fermata` → alias of its symbol; `Breath` → alias.
- `handlesItem`: `Fermata`, `Breath`.
- Key binding: `Shift+H` → `command://dorico/popover/holds`.

## Deferred / unsupported
- `curlew` (Britten) → `Unsupported` (no matching symbol; verify SMuFL availability first).
- Caesura placement to the left of the note (MuseScore breath placement is to the right) — placed with MuseScore's default.
- Breath mark symbols not present in MuseScore's set: report `Unsupported` per symbol after checking.
- Local/global variant (n/a in Dorico for this popover).
- Fermata playback duration stretch: set whatever default `Fermata` uses; no per-entry control.

## Acceptance criteria
- [ ] Shift-H, `fer`, Return on a note in a 3-staff score creates a fermata on all 3 staves at that position (where the staff has a chord/rest); Cmd-Z removes all in one step.
- [ ] `longfer`, `shortfer`, `veryshortfer`, `verylongfer`, `longhenzefer`, `shorthenzefer` use the matching symbol.
- [ ] `comma`, `,`, `breathmarktick`, `breathmarkupbow`, `breathmarksalzedo` create breath marks on the selected staves only.
- [ ] `//`, `thick//`, `curved//`, `short//`, `single//` create the matching caesura on all staves.
- [ ] Applying a second hold at the same position replaces the first.
- [ ] Select a fermata, Return → popover pre-filled with `fer`; change to `longfer`, Return replaces.
- [ ] `curlew` is red + "not supported yet"; `xyz` Invalid.
- [ ] Parser tests pass.

## Tests
`src/dorico/tests/holdsparser_tests.cpp`:

| Input | Expected |
|---|---|
| `fer`, `fermata`, `pause` | Ok, Fermata default |
| `verylongfer`, `verylongfermata` | Ok, very long |
| `longfer`, `shortfer`, `veryshortfer` | Ok |
| `shorthenzefer`, `longhenzefermata` | Ok, Henze |
| `curlew` | Unsupported |
| `comma`, `,`, `breathmarkcomma` | Ok, Breath comma |
| `breathmarktick`, `breathmarkupbow`, `breathmarksalzedo` | Ok |
| `caes`, `caesura`, `//` | Ok, Caesura default |
| `thick//`, `caesurathick`, `thickcaesura` | Ok, thick |
| `curved//`, `short//`, `single//` | Ok |
| `fe` | Incomplete |
| `breath` | Incomplete |
| `xyz` | Invalid |
| ` FER ` (case/space) | Ok, Fermata |
