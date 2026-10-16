<!-- markdownlint-disable -->
# 24 — Clefs and octave lines popover (Shift-C)
Status: todo · Depends on: 20 · Dorico refs: [clefs](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_clefs_octave_lines_clefs_r.html), [clef octave shifts](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_clefs_octave_lines_clef_octave_shifts_r.html), [octave lines](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_clefs_octave_lines_octave_lines_r.html)

Reference: `specs/reference/dorico-popovers.md` (Clefs and octave lines), `specs/reference/musescore-internals.md` (§3).

## Goal
Shift-C, type `bass`, `g8ba`, `8va`, `15mb` etc., Return → a clef change, or an octave line over the selection/caret.

## Dorico behaviour

### Clefs
| Clef | Entries |
|---|---|
| Treble G | `g`, `g2`, `sol`, `treble` |
| Bass F | `f`, `f4`, `fa`, `bass` |
| Tenor C | `ct`, `c4`, `ut4`, `tenor` |
| Alto C | `ca`, `c3`, `ut3`, `alto` |
| Treble 8vb | `g8ba`, `g8d`, `treble8ba`, `treble8d` |
| Treble 15mb (Pro) | `g15ba`, `g15d`, `treble15ba`, `treble15d` |
| Treble 8va (Pro) | `g8va`, `g8u`, `treble8va`, `treble8u` |
| Treble 15ma (Pro) | `g15ma`, `g15u`, `treble15ma`, `treble15u` |
| Alto 8vb (Pro) | `ca8ba`, `ca8d`, `alto8ba`, `alto8d` |
| Tenor 8vb (Pro) | `ct8ba`, `ct8d`, `tenor8ba`, `tenor8d` |
| Bass 8vb (Pro) | `f8ba`, `f8d`, `bass8ba`, `bass8d` |
| Bass 15mb (Pro) | `f15ba`, `f15d`, `bass15ba`, `bass15d` |
| Bass 8va (Pro) | `f8va`, `f8u`, `bass8va`, `bass8u` |
| Bass 15ma (Pro) | `f15ma`, `f15u`, `bass15ma`, `bass15u` |
| Percussion | `perc` |
| 4-string tab | `tab4` |
| 6-string tab | `tab6` |
| French violin (Pro) | `g1` |
| Soprano (Pro) | `soprano`, `c1`, `ut1` |
| Mezzo (Pro) | `mezzo`, `c2`, `ut2` |
| Baritone C (Pro) | `baritone`, `c5`, `ut5` |
| Baritone F (Pro) | `baritonebass` |
| Sub-bass (Pro) | `subbass` |
| Invisible (Pro) | `invisible` |

Clef octave shift: `<clef>+N` / `<clef>-N` shifts by N octaves (e.g. `g+2`); append `c` for concert pitch or `t` for transposed pitch (e.g. `treble+1t`).

### Octave lines
| Shift | Entries |
|---|---|
| Up 1 octave | `8va`, `8`, `8u`, `1u` |
| Up 2 octaves | `15ma`, `15`, `15u`, `2u` |
| Up 3 octaves | `22ma`, `22`, `22u`, `3u` |
| Down 1 octave | `8ba`, `8vb`, `8d`, `1d` |
| Down 2 octaves | `15ba`, `15vb`, `15d`, `2d` |
| Down 3 octaves | `22ba`, `22vb`, `22d`, `3d` |
| Loco | `loco` |
| Stop during note input | `\|`, `stop` |

### Placement and variants
| | Return | Opt-Return |
|---|---|---|
| Octave lines | All voices | Current voice only |
| Clefs | Staff-wide (no local variant documented) | same |

- **Range selection:** octave line spans the range; a clef is inserted at the start **and a restorative clef at the end of the range** (the clef in force before).
- **Single item / caret:** clef at the earliest selected item or caret (mid-bar allowed in Dorico; MuseScore supports mid-bar clef changes).
- `loco` ends an octave line at the selection/caret.
- In note input an octave line is open-ended and stops with `|`/`stop` (or end of input).
- Return on a selected clef or octave line reopens the popover pre-filled.

## MuseScore today
- Clef: `INotationInteraction::insertClef(ClefType)` — `src/notation/internal/notationinteraction.cpp:8392`; `EditClef::insertClef` — `src/engraving/editing/editclef.h:58`.
- Octave lines: `addOttavaToSelection(OttavaType)` — `notationinteraction.cpp:5419`; `EditSpanner::addSpanner`.
- `ClefType` (`src/engraving/types/types.h`) contains (verify exact names): `G`, `G15_MB`, `G8_VB`, `G8_VA`, `G15_MA`, `G1` (French), `C1`–`C5`, `C4_8VB`, `F`, `F15_MB`, `F8_VB`, `F_8VA`, `F_15MA`, `F_B` (baritone F), `F_C` (sub-bass), `PERC`, `PERC2`, `TAB`, `TAB4`, serif variants.
- `OttavaType` is 8va/15ma/8vb/15mb (22nd is **not** present — verify; if absent, 22xx is Unsupported).
- Octave line voice scope: ottava is a staff-level spanner in MuseScore, no voice assignment — Opt-Return voice-specific variant cannot be honoured (see Deferred).

## Design
- Parser: `popover/parsers/clefparser.{h,cpp}`.
  ```cpp
  struct ClefEntry   { std::string clefId; int octaveShift = 0; enum class Pitch { Default, Concert, Transposed } pitch; };
  struct OctaveEntry { int octaves; /* ±1..3 */ bool loco; bool stop; };
  using ClefPopoverEntry = std::variant<ClefEntry, OctaveEntry>;
  ParseResult<ClefPopoverEntry> parseClefOrOctave(std::string_view);
  ```
  Order: try octave-line forms first only for strings made purely of `^\d+(va|ma|ba|vb|mb|u|d)?$`, `loco`, `stop`, `|`; otherwise try the clef-name table (case-insensitive); then the `<clef>[+-]N[ct]` suffix grammar.
- Clef name table → `ClefType`: encode in one `static const std::unordered_map<std::string, ClefType>` with all aliases from the table above; unsupported rows map to `std::nullopt`.
- Handler: `handlers/clefhandler.cpp`.
  - Clef: single/caret → `insertClef(type)`. Range → `insertClef(type)` at the start, then compute the clef in force at the selection start (`Staff::clef(tick)`) and insert it again at the end tick (restorative).
  - Octave line: `addOttavaToSelection(type)`; note input → `PendingSpanner` (spec 21) until `|`/`stop`; `loco` ends the line (on a selected ottava: shorten to the selection end).
- `prefill`: clef → its canonical name (`treble`, `bass8ba`…); ottava → `8va` etc.
- `handlesItem`: `Clef`, `Ottava`.
- Key binding: `Shift+C` → `command://dorico/popover/clef`.

## Deferred / unsupported
- `invisible` clef, `ca8ba` / `alto8ba` / `alto8d` (alto 8vb), `ct8d` variants not present in `ClefType`, `baritone`/`subbass` if the type is missing: `Unsupported` (list the final set after checking `types.h`).
- Clef octave shift suffix `c` / `t` (concert/transposed) and shifts other than ±1/±2 mapping to an existing `ClefType`: `Unsupported`.
- 22ma / 22mb octave lines if `OttavaType` lacks them.
- Opt-Return "current voice only" for octave lines (MuseScore ottava has no voice scope): treated as Return.
- Mid-bar clef placement quirks (courtesy clefs follow MuseScore defaults).

## Acceptance criteria
- [ ] Shift-C, `bass`, Return on a note → bass clef from that note onward; Cmd-Z restores.
- [ ] Range selection of bars 2–3, `alto` → alto clef at bar 2, treble (previous clef) restored at the end of bar 3.
- [ ] `g8ba` → treble 8vb; `f15ma` → bass 15ma; `perc` → percussion; `tab6` → 6-string tab.
- [ ] `g+1` → treble 8va clef; `f-2` → bass 15mb.
- [ ] `8va` over a selected range creates an 8va line spanning it; `8ba`/`15ma` give the right types.
- [ ] Note input: `8va` then entering notes, then `|` stops the line at the right place.
- [ ] `loco` on a selected 8va shortens/ends it at the selection end.
- [ ] Selecting an existing clef or ottava and pressing Return reopens pre-filled; entering another replaces it.
- [ ] `22ma` behaves per the verified `OttavaType` (works or shows Unsupported).
- [ ] Parser tests pass.

## Tests
`src/dorico/tests/clefparser_tests.cpp`:

| Input | Expected |
|---|---|
| `treble`, `g`, `g2`, `sol` | Ok, Clef G |
| `bass`, `f`, `f4`, `fa` | Ok, Clef F |
| `tenor`, `ct`, `c4`, `ut4` | Ok, Clef C4 |
| `alto`, `ca`, `c3`, `ut3` | Ok, Clef C3 |
| `g8ba`, `g8d`, `treble8ba` | Ok, G 8vb |
| `g15ma`, `treble15u` | Ok, G 15ma |
| `f8va`, `bass15ba` | Ok |
| `perc`, `tab4`, `tab6` | Ok |
| `g1`, `soprano`, `c1`, `mezzo`, `c2`, `baritone`, `c5` | Ok |
| `invisible` | Unsupported |
| `g+2` | Ok, G, octaveShift +2 |
| `treble+1t` | Ok (or Unsupported for suffix t; decide + document) |
| `8va`, `8`, `8u`, `1u` | Ok, octaves +1 |
| `15ma`, `15`, `2u` | Ok, +2 |
| `22ma`, `22`, `3u` | Ok, +3 (Unsupported at apply if no 22 type) |
| `8ba`, `8vb`, `8d`, `1d` | Ok, −1 |
| `15ba`, `15vb`, `2d` | Ok, −2 |
| `22ba`, `3d` | Ok, −3 |
| `loco` | Ok, loco |
| `\|`, `stop` | Ok, stop |
| `tre` | Incomplete |
| `8x`, `xyz`, `9va` | Invalid |
