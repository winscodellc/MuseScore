<!-- markdownlint-disable -->
# 28 — Ornaments popover (Shift-O)
Status: todo · Depends on: 20 · Dorico refs: ornament pages `dorico_popovers_ornaments_*_r.html` under [popover reference](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/), [index](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_ornaments_ornaments_r.html)

Reference: `specs/reference/dorico-popovers.md` (Ornaments), `specs/reference/musescore-internals.md` (§3).

## Goal
Shift-O, type `tr`, `mor`, `turn`, `arpdown`, `gliss`, `fall`, Return → ornament, trill line, arpeggio, glissando or jazz articulation on the selection or caret.

## Dorico behaviour

### Ornaments
| Entry | Ornament |
|---|---|
| `trill`, `tr` | Trill |
| `tr m3` (space required) | Trill with interval |
| `shorttr` | Short trill |
| `mordent`, `mor` | Mordent |
| `turn` | Turn |
| `invertedturn`, `invturn` | Inverted turn |

Trill intervals: `1`–`8` degree; `M`/`maj`/`major` major; `m`/`min`/`minor` minor; `perf`/`per`/`p` perfect; `diminished`/`dim`/`d`; `augmented`/`aug`/`a`.

### Jazz ornaments and articulations
- Ornaments: `brassbend`; `flip`; `jazz` / `shake` (jazz turn); `smear`.
- Articulations: `plop`; `plopsmooth`; `scoop`; `doit`; `doitsmooth`; `fall`; `fallsmooth`.

### Arpeggio signs
| Entry | Sign |
|---|---|
| `arpeggioup`, `arpup`, `arp` | Up |
| `arpeggiodown`, `arpdown` | Down |
| `nonarpeggio`, `nonarp` | Non-arpeggio |
| `slurarp` | Curved |

### Glissando
`gliss` (default style), `glissstraight`, `glisswavy`.

### Guitar techniques
| Entry | Technique |
|---|---|
| `bend` | Guitar bend |
| `vibbend` | Vibrato bar dive and return |
| `vibscoop` | Vibrato bar scoop |
| `vibdip` | Vibrato bar dip |
| `wbar`, `w/bar` | Vibrato bar line |
| `hammer`, `ho` | Hammer-on |
| `pull`, `po` | Pull-off |
| `hammerpull`, `hopo`, `hp`, `ligado`, `lig` | Hammer-on/pull-off (ligado) |
| `tap` | Right-hand tapping |
| `tappull` | Right-hand tapping with pull-off |
| `lhtap` | Left-hand tapping |
| `lhtappull` | Left-hand tapping with pull-off |

### Placement
- Return only (no local variant documented).
- Selection: on each selected note; ranges span the range for lines (trill, glissando).
- Note input: a trill's length comes from the next note input or a Space advance (use `PendingSpanner`, spec 21).
- Return on a selected ornament/arpeggio/glissando reopens the popover pre-filled.

## MuseScore today
- Ornament symbols and articulations: `INotationInteraction::toggleOrnament(SymId)` (`notationinteraction.cpp:8374`), `toggleArticulation(SymId)` (:8367), `toggleArticulationForSelection` (:5581); engraving `EditChord::toggleArticulation`.
- Ornament SymIds (**verify**): `ornamentTrill`, `ornamentShortTrill`, `ornamentMordent`, `ornamentTurn`, `ornamentTurnInverted`. Ornaments in MuseScore 4 can carry intervals above/below (`Pid::INTERVAL_ABOVE/BELOW`, `OrnamentInterval`) — **verify** in `src/engraving/dom/ornament.h`.
- Trill line: `Trill` spanner (`TrillType::TRILL_LINE`, `UPPRALL_LINE`, `DOWNPRALL_LINE`, …) via `applyPaletteElement`/`EditSpanner::addSpanner`.
- Arpeggio: `Arpeggio` element (`ArpeggioType::NORMAL / UP / DOWN / BRACKET / UP_STRAIGHT / DOWN_STRAIGHT`) — dropped on a chord via `applyPaletteElement`.
- Glissando: `Glissando` (`GlissandoType::STRAIGHT / WAVY`) — dropped on a note pair via `applyPaletteElement`.
- Jazz articulations are SMuFL `brass*` symbols in the palette's Articulations (brassBend, brassFlip, brassJazzTurn, brassSmear, brassScoop, brassPlop, brassDoit*, brassFall*, brassLift*). Exact set per palette — **verify** against `palettecreator.cpp`.
- Guitar techniques (bend, hammer-on, pull-off, taps, vibrato bar) exist only in tablature/guitar palette elements (Bend, etc.).

## Design
- Parser: `popover/parsers/ornamentsparser.{h,cpp}`.
  ```cpp
  struct OrnamentEntry {
      enum class Kind { Ornament, TrillLine, JazzOrnament, JazzArticulation, Arpeggio, Glissando, Guitar };
      std::string id;                     // canonical, e.g. "trill", "mordent", "arp_down"
      std::optional<TrillInterval> interval;   // for "tr m3"
  };
  ParseResult<OrnamentEntry> parseOrnament(std::string_view);
  ```
  Alias table (static map) like the other popovers; `tr` followed by a space and `<quality?><degree>` is parsed with a small interval grammar (`^(M|m|perf|dim|aug|per|p|d|a|maj|min|major|minor|diminished|augmented)?([1-8])$`, `M`/`m` case-sensitive). Degree outside 1–8 → Invalid.
- Mapping table (canonical id → handler action):

| Canonical id | MuseScore action |
|---|---|
| `trill` | `Trill` spanner `TRILL_LINE` over the selection's duration (single note: its duration) |
| `shorttr` | `toggleOrnament(SymId::ornamentShortTrill)` |
| `mordent` | `toggleOrnament(SymId::ornamentMordent)` |
| `turn` | `toggleOrnament(SymId::ornamentTurn)` |
| `invturn` | `toggleOrnament(SymId::ornamentTurnInverted)` |
| `arp_up` / `arp_down` / `nonarp` | `Arpeggio` `UP` / `DOWN` / `BRACKET` |
| `gliss`, `glissstraight`, `glisswavy` | `Glissando` `STRAIGHT` (default style) / `STRAIGHT` / `WAVY` |
| `flip`, `smear`, `jazz`/`shake`, `brassbend` | `toggleArticulation(brassFlip / brassSmear / brassJazzTurn / brassBend)` |
| `plop`, `scoop`, `doit`, `fall` (+`smooth`) | matching `brass*` symbol from the palette; the `smooth` variants map to the `Smooth` symbols if present, else Unsupported |

  `tr m3` → ornament `ornamentTrill` toggled, then set `INTERVAL_ABOVE` to the parsed `OrnamentInterval` (**verify** API) — or `Trill` with `ornament()` interval.
- Handler: `handlers/ornamentshandler.cpp`; every apply is one transaction. Selection of a note range for arpeggio applies to each chord separately.
- `prefill`: ornament symbol → alias; arpeggio type → alias; glissando → `gliss`.
- `handlesItem`: `Ornament`/articulation with ornament SymId, `Trill`, `Arpeggio`, `Glissando`, articulation with a jazz SymId.
- Key binding: `Shift+O` → `command://dorico/popover/ornaments`.

## Deferred / unsupported
- Guitar techniques (all rows: `bend`, `vibbend`, `vibscoop`, `vibdip`, `wbar`, `w/bar`, `hammer`, `ho`, `pull`, `po`, `hammerpull`, `hopo`, `hp`, `ligado`, `lig`, `tap`, `tappull`, `lhtap`, `lhtappull`) → `Unsupported` (tablature-only elements; revisit).
- `slurarp` (curved arpeggio) → `Unsupported`.
- `plopsmooth`, `doitsmooth`, `fallsmooth` if no matching symbol → `Unsupported` (after verification).
- Trill interval beyond what `OrnamentInterval` supports (degrees ≠ 2nd/3rd...) — report `Unsupported` for unmapped combos, `Invalid` for degree > 8.
- Live extension of the trill line while the caret moves (uses the `PendingSpanner` approximation).

## Acceptance criteria
- [ ] Shift-O, `tr`, Return on a half note → trill line over the note; Cmd-Z removes it.
- [ ] `tr` on a range selection → trill line spanning the range.
- [ ] `tr m3` → trill with a minor-third interval shown/ played accordingly (or the nearest supported representation).
- [ ] `mor`, `turn`, `invturn`, `shorttr` → matching ornament symbol; pressing again toggles it off (matches toggle semantics).
- [ ] `arp`, `arpdown`, `nonarp` on a chord → matching `Arpeggio` type.
- [ ] `gliss`, `glisswavy` between two selected notes → straight / wavy glissando.
- [ ] `fall`, `doit`, `scoop`, `plop`, `flip`, `smear`, `jazz` → matching jazz articulation.
- [ ] Guitar technique entries show red + "not supported yet".
- [ ] Select an existing ornament, Return → pre-filled; change and Return replaces it.
- [ ] Parser tests pass.

## Tests
`src/dorico/tests/ornamentsparser_tests.cpp`:

| Input | Expected |
|---|---|
| `trill`, `tr` | Ok, Trill |
| `tr m3` | Ok, Trill, interval minor 3rd |
| `tr M2`, `tr 2`, `tr perf4`, `tr aug2` | Ok with the interval |
| `tr 9`, `tr m` | Invalid / Incomplete (decide: `tr m` Incomplete, `tr 9` Invalid) |
| `shorttr` | Ok |
| `mordent`, `mor` | Ok |
| `turn` | Ok |
| `invertedturn`, `invturn` | Ok |
| `arp`, `arpup`, `arpeggioup` | Ok, arp up |
| `arpdown`, `arpeggiodown` | Ok |
| `nonarp`, `nonarpeggio` | Ok |
| `slurarp` | Unsupported |
| `gliss`, `glissstraight`, `glisswavy` | Ok |
| `plop`, `scoop`, `doit`, `fall`, `flip`, `smear`, `jazz`, `shake`, `brassbend` | Ok |
| `plopsmooth`, `doitsmooth`, `fallsmooth` | Ok or Unsupported (document after verifying) |
| `bend`, `hammer`, `ho`, `pull`, `po`, `hp`, `ligado`, `tap`, `lhtap`, `wbar`, `w/bar` | Unsupported |
| `tr` + space (incomplete interval) | Incomplete |
| `xyz` | Invalid |
