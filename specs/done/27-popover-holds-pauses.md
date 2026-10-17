<!-- markdownlint-disable -->
# 27 — Holds and pauses popover (Shift-H)
Status: done · Depends on: 20 · Dorico refs: [fermatas](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_holds_pauses_fermatas_r.html), [breath marks](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_holds_pauses_breath_marks_r.html), [caesuras](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_holds_pauses_caesuras_r.html)

Reference: `../reference/dorico-popovers.md` (Holds and pauses), `../reference/musescore-internals.md` (§3). Manual: "Inputting holds and pauses with the popover", "Correct positioning for caesura input", "Positions of holds and pauses".

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
- Fermatas go on **all staves**; breath marks go on the **selected or caret staves**, to the right of the note; caesuras go on all staves, to the **left** of the note (a caesura before a barline is input on the first note of the next bar).
- No local/global distinction in the popover (Return only).
- Selection: item at the earliest selected item; caret: at the caret.
- Return on a selected hold/pause reopens the popover pre-filled.

## MuseScore today
- Fermata: `Fermata` element (`src/engraving/dom/fermata.h`), an annotation of a ChordRest segment, one per track. Symbols `fermataAbove`, `fermataShortAbove`, `fermataLongAbove`, `fermataVeryShortAbove`, `fermataVeryLongAbove`, `fermataShortHenzeAbove`, `fermataLongHenzeAbove` (palette set); `setSymIdAndTimeStretch` gives the playback stretch. SMuFL `curlewSign` is in Bravura and draws fine as a fermata symbol (no palette cell).
- Breath/caesura: `Breath` element in a `SegmentType::Breath` segment, one per track. `Breath::BREATH_LIST` has the symbols and pauses (`breathMarkComma`, `breathMarkTick`, `breathMarkUpbow`, `breathMarkSalzedo`, `caesura`, `caesuraCurved`, `caesuraShort`, `caesuraThick`, `caesuraSingleStroke`, `chantCaesura`).
- The palette (`ChordRest::drop`) puts a breath in the Breath segment at the chord's end tick. Breath segments sort before ChordRest segments at the same tick, so a Breath segment at a note's **start** tick draws to the left of it: that is how caesuras get Dorico's placement (at a bar's first note the segment is the end of the previous bar, before the barline).

## Design
- Parser `popover/parsers/holdsparser.{h,cpp}`: `HoldEntry { Kind kind; SymId sym; }`, `parseHold`, `holdText(SymId)` (prefill). One alias table, case-insensitive, trimmed; a prefix of any alias is Incomplete.
- Handler `popover/handlers/holdshandler.{h,cpp}`, one undo step per Return:
  - Position: the caret tick, the range start, or the earliest selected item.
  - Fermata: every staff of the master score (parts follow through links) whose chord/rest starts at that tick; any fermata already on the staff there is replaced.
  - Breath mark: the selected (or caret) staves, at the end of the chord/rest that starts at the tick.
  - Caesura: every staff, in the Breath segment at the tick itself (left of the note).
  - A breath or caesura already in that Breath segment on the staff takes the new symbol and pause (replace). Editing a selected breath/caesura uses its own position.
  - Outside note input the first new item is selected, so Return reopens it.
- `prefill`: fermata → its "Above" symbol's entry (`fer`, `longfer`, ...); breath/caesura → `comma`, `//`, `thick//`, ...
- `itemTypes`: `FERMATA`, `BREATH`. Key: `Shift+H` → `command://dorico/popover/holds` (bound since spec 20).

## Deferred / unsupported
- Return on a **note** that has a breath mark after it opens an empty popover (only selected breaths prefill; breaths are not segment annotations). Entering a new one still replaces it.
- Fermatas on staves with no chord/rest starting at that tick (Dorico shows them at the closest position, e.g. over a bar rest that started earlier) are skipped.
- Caret on several staves: MuseScore's caret is one staff, so breath marks go on that staff.
- Fermata playback stretch: MuseScore's default per type; no per-entry control. Local/global: n/a (Return only).

## Acceptance criteria
- [x] Shift-H, `fer`, Return on a note in a 3-staff score creates a fermata on all 3 staves at that position (where the staff has a chord/rest); Cmd-Z removes all in one step.
- [x] `longfer`, `shortfer`, `veryshortfer`, `verylongfer`, `longhenzefer`, `shorthenzefer` use the matching symbol.
- [x] `comma`, `,`, `breathmarktick`, `breathmarkupbow`, `breathmarksalzedo` create breath marks on the selected staves only.
- [x] `//`, `thick//`, `curved//`, `short//`, `single//` create the matching caesura on all staves.
- [x] Applying a second hold at the same position replaces the first.
- [x] Select a fermata, Return → popover pre-filled with `fer`; change to `longfer`, Return replaces.
- [x] `curlew` creates the curlew (Britten) sign (changed from "not supported yet": Bravura has `curlewSign`); `xyz` Invalid.
- [x] Parser tests pass.

## Tests
`src/dorico/tests/holdsparser_tests.cpp`:

| Input | Expected |
|---|---|
| `fer`, `fermata`, `pause`, ` FER ` | Ok, Fermata `fermataAbove` |
| `verylongfer`, `verylongfermata` | Ok, very long |
| `longfer`, `shortfer`, `veryshortfer` | Ok |
| `shorthenzefer`, `longhenzefermata` | Ok, Henze |
| `curlew` | Ok, `curlewSign` |
| `comma`, `,`, `breathmarkcomma` | Ok, Breath comma |
| `breathmarktick`, `breathmarkupbow`, `breathmarksalzedo` | Ok |
| `caes`, `caesura`, `//` | Ok, Caesura default |
| `thick//`, `caesurathick`, `thickcaesura` | Ok, thick |
| `curved//`, `short//`, `single//` | Ok |
| ``, `fe`, `breath`, `/`, `short` | Incomplete |
| `xyz`, `fer2`, `///`, `commas` | Invalid |
| `holdText` | canonical entries round-trip; `chantCaesura` → empty |

Manual test (2026-10-17, our build driven with ClaudeUIControl, temp copy of `demos/Brassed_Up.mscx`, 4 staves): every acceptance box above passed. `fer` on bar 4 put a fermata on all 4 staves; Return on it prefilled `fer`, `longfer` replaced all four; Cmd-Z went back to `fer`, a second Cmd-Z removed all four. `comma` with bar-3 notes Cmd-selected on Trumpet and Trombone 1 put breath marks after those two notes only; Return prefilled `comma`, `breathmarktick` replaced it. `,`, `breathmarkupbow`, `breathmarksalzedo` on the Tuba drew their symbols. `//` on bar 4's first note put caesuras on all staves before the bar-4 barline; `thick//`, `curved//`, `short//`, `single//` each replaced them on all staves. `shortfer`, `veryshortfer`, `verylongfer`, `longhenzefer`, `shorthenzefer`, `curlew` each replaced the fermatas on all staves with the matching symbol. `xyz` stayed red and Return did nothing. In note input (Shift-N) `fer` put fermatas on all staves at the caret.
