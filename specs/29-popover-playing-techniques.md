# 29 — Playing techniques popover (Shift-P)
Status: todo · Depends on: 20 · Dorico refs: pages `dorico_popovers_playing_techniques_*_r.html`, [pedal lines](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_pedal_lines_r.html), [harp pedaling](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_harp_pedaling_r.html), [harmonics](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_harmonics_r.html) (all under `_shared/topics/dorico/dorico_popovers/`)

Reference: `specs/reference/dorico-popovers.md` (Playing techniques), `specs/reference/musescore-internals.md` (§3).

## Goal
Shift-P, type `pizz`, `arco`, `con sord`, `vibrato->`, `ped`, Return → a playing technique text/line/pedal at the selection or caret; suggestions list valid techniques as you type.

## Dorico behaviour

### General syntax
- A suggestion menu of **valid** techniques appears as you type; entries must match a known technique.
- `(arco)` = parenthesised. `[pizz]` = hidden.
- `vibrato->` = with duration (open-ended during note input). `?` ends it with a duration line.
- Entering a new technique ends the open one with a continuation line.
- Outside note input, `A->B` creates two techniques, automatically grouped.
- Only one technique per entry during note input.
- Techniques are voice-specific: placed above for up-stem voices and below for down-stem voices.
- Return only (no documented local variant).
- Return on a selected technique/pedal line reopens the popover pre-filled.

### Common techniques
| Entry | Technique |
|---|---|
| `string 1` … `9` | String indicator outside the staff |
| `nat` | Naturale |
| `vibrato` / `senza vibrato` | Vibrato / senza vibrato |
| `legato` | Legato |
| `o` | Harmonic (o) |
| `con sord` / `senza sord` | Muted / unmuted |
| `stopped` | Stopped (text) |
| `+` | Stopped or muted (+) |
| `downbow` / `upbow` | Bowing |
| `strong air pressure` | Strong air pressure |
| `double-tongue` | Double-tongue |
| `fluttertongue` | Flutter-tongue (f.t.) |
| `flt` | Flutter-tongue (flt.) |
| `flz` | Flatterzunge (flz.) |
| `sul pont` / `sul tasto` / `pst` | Sul ponticello / sul tasto / poco sul tasto |
| `pizz` / `spicc` / `arco` | Pizzicato / spiccato / arco |
| `tongue click` / `finger click` | Stockhausen clicks |
| `motor on` / `motor off` | Vibraphone motor |
| `open` / `damp` / `damp large` | Open / damp |
| `full barre` / `half barre` | Barré |
| `strum up` / `strum down` | Strums |
| `lh` / `rh` | Left / right hand |

The manual says "many further valid" entries exist.

### Pedal lines
| Entry | Meaning |
|---|---|
| `ped` | Sustain pedal line |
| `1/4`, `1/2`, `3/4`, `1` | Sustain pedal level (Pro) |
| `^`, `notch`, `retake` | Retake |
| `nonotch` | Remove retake |
| `*` | Stop sustain pedal during note input |
| `sost` / `s*` | Sostenuto line / stop |
| `unacorda` / `u*` | Una corda line / stop |

### Harp pedaling
After a note name: `^` or `b` = flat; `-` or nothing = natural; `v` or `#` = sharp. Examples: `--^|^---`, `--^^---`, `BbEb` or `DCBbEbFGA` all set D C B♭ E♭ F G A.

### Harmonics
Entry must start with `harm`, `harmonic` or `harmonics`; further tokens separated by spaces:

| Entry | Meaning |
|---|---|
| `nat`, `natural` / `art`, `artificial` | Harmonic type |
| `nh`, `hh` / `ah` / `ph` | Natural / artificial / pinch harmonic |
| `p=4` (`p`, `partial=`) | Partial |
| `n=2` (`n`, `node=`) | Node |
| `s=3`, `s=D4` (`s`, `string=`) | String by number or by pitch |
| `circle`, `circleabove` | Circle above the staff (natural) |
| `diamond`, `diamondnotehead`, `sounding` | Diamond notehead (natural) |
| `white`, `whitediamondnotehead`, `whitediamond`, `touched` | White diamond for touched pitches (natural) |
| `normal` | White diamond for touched pitches (artificial) |
| `pinch` | Pinch artificial harmonic |
| `sounding`, `singlesounding` | White diamond for sounding pitches (artificial) |
| `stopped`, `singlestopped` | White diamond for stopped pitches (artificial) |
| `open` / `noopen` | Show / hide open-string pitch |
| `paren` / `noparen` | Show / hide parenthesised sounding pitch |
| `noharm` | Delete the harmonic |

Example: `harm nat n1 s1 p4 diamond`.

### Stop tokens in note input
`?` stops gradual techniques/dynamics; `*`, `s*`, `u*` stop sustain/sostenuto/una-corda lines.

## MuseScore today
- Playing technique annotations: `PlayTechAnnotation` (`src/engraving/dom/playtechannotation.h`) with `PlayingTechniqueType` (Natural, Pizzicato, Arco, Tremolo, Mute, Open, Distortion, Overdrive, Harmonics, JazzTone) — affects playback. Applied through `applyPaletteElement` (`notationinteraction.cpp:2224`) or `addTextToItem` (:6204) with the matching text style.
- Other text techniques (staff text with the "Expression"/"Technique" style) via `addTextToItem(TextStyleType::..., item)`; **verify** which `TextStyleType` the palette's Playing techniques use.
- Pedal: `Pedal` spanner (`PedalType` / text variants) via `applyPaletteElement`/`EditSpanner::addSpanner`; palette includes ped… * variants. Una corda / sostenuto: **verify** palette lines.
- Vibrato: `Vibrato` spanner (`VibratoType`) — usable for `vibrato->`.
- Bowing/stopped marks: `toggleArticulation` with SymIds `stringsUpBow`, `stringsDownBow`, `stringsHarmonic`, `brassMuteClosed`, `pluckedLeftHandPizzicato`.
- Harp pedal diagrams: `HarpPedalDiagram` (popup model `HARP_DIAGRAM` exists in `abstractelementpopupmodel.h`).
- Note input: `PendingSpanner` helper (spec 21) for open-ended lines.

## Design
- Technique registry: one static table in `popover/parsers/techniquestable.cpp`: each row `{canonical id, aliases[], category, mapping}` where `category ∈ { PlayTech, Text, Articulation, Line, Pedal }`. The `suggest` callback and the parser both read it; "valid technique only" is enforced by the table (matches Dorico's behaviour).
- Parser: `popover/parsers/techniquesparser.{h,cpp}`.
  ```cpp
  struct TechniqueItem { std::string id; bool parenthesised = false; bool hidden = false; bool withDuration = false; };
  struct TechniquesEntry { std::vector<TechniqueItem> items; bool stop = false; std::optional<PedalSpec> pedal; };
  ParseResult<TechniquesEntry> parseTechniques(std::string_view);
  ```
  Steps: strip `( )`/`[ ]`; split on `->` for two-technique entries (`A->B`) vs trailing `->` (with duration); detect pedal tokens (`ped`, level fractions, `^`/`notch`/`retake`/`nonotch`, `*`, `sost`, `s*`, `unacorda`, `u*`); then look up (longest-alias match, case-insensitive) in the table; `harm…` goes to the harmonics sub-grammar (Unsupported in v1, but fully tokenised so errors are right); unknown → Invalid; prefixes of any alias → Incomplete.
- Handler: `handlers/techniqueshandler.cpp`.
  - PlayTech category (pizz, arco, con sord/senza sord, nat, open, damp…): `Factory::createPlayTechAnnotation`, `setTechniqueType`, apply via `applyPaletteElement`.
  - Text category: `StaffText` with the techniques text style, text from the table, parenthesised/hidden handled (`setVisible(false)` for hidden).
  - Articulation category (`downbow`, `upbow`, `o`, `+`, `stopped`): `toggleArticulation(SymId)`.
  - Line category (`vibrato->`): `Vibrato` spanner over the range; note input → `PendingSpanner`; `?` finalises.
  - Pedal: `Pedal` spanner; `ped` over the range, `*`/`^` tokens set end/retake style (**verify** the pedal type that includes `*` and retake `^`); note input → `PendingSpanner` until `*`.
  - Voice-specific placement: set placement by stem direction of the voice (`Pid::PLACEMENT` above for up-stem voices, below for down-stem).
  - `A->B` outside note input: two items at consecutive selected notes, in order; v1 does not group them.
- `suggest(text)`: prefix filter over all aliases, limit 8, sorted by table order.
- `prefill`: `PlayTechAnnotation`/`StaffText` technique → alias; `Pedal` → `ped`.
- `handlesItem`: `PlayTechAnnotation`, techniques `StaffText`, `Pedal`, `Vibrato`.
- Key binding: `Shift+P` → `command://dorico/popover/techniques`.

## Deferred / unsupported
- Harp pedaling entries (`--^|^---`, `BbEb`, `DCBbEbFGA`) → `Unsupported` in v1 (follow-up uses `HarpPedalDiagram`).
- All harmonics entries (`harm …`) → `Unsupported` (tokenised for correct error reporting).
- `string 1`…`9` string indicator outside staff → `Unsupported` unless the palette has an equivalent text.
- Pedal levels (`1/4`, `1/2`, `3/4`, `1`), `sost`, `s*`, `unacorda`, `u*` → `Unsupported` until the palette lines are verified.
- Technique grouping of `A->B` and automatic continuation lines when a new technique starts.
- Techniques absent from the registry: grow the table incrementally; unknown = Invalid.
- Live extension of open-ended lines (spec 20 approximation).

## Acceptance criteria
- [ ] Shift-P, `pizz`, Return on a note → a pizzicato annotation above the staff (up-stem voice) and playback uses pizzicato.
- [ ] `arco` on a later note ends pizzicato playback there.
- [ ] `[pizz]` is invisible but still affects playback; `(arco)` shows parentheses.
- [ ] Typing `piz` shows a suggestion list containing `pizz`; Up/Down + Return applies it.
- [ ] `con sord`, `senza sord`, `sul pont`, `sul tasto`, `nat`, `spicc` produce their text/annotation.
- [ ] `downbow` / `upbow` toggle the bowing articulation.
- [ ] `ped` on a range → a pedal line spanning it; in note input `ped` … `*` produces a line from the start to the stop point.
- [ ] `vibrato->` in note input then `?` ends the vibrato line.
- [ ] `pizz->arco` outside note input creates two annotations on consecutive notes.
- [ ] Down-stem voice puts the technique below the staff.
- [ ] `harm nat` and harp-pedal strings show red + "not supported yet"; `xyzzy` is Invalid.
- [ ] Select an existing technique, Return → pre-filled; change and Return replaces.
- [ ] Parser tests pass.

## Tests
`src/dorico/tests/techniquesparser_tests.cpp`:

| Input | Expected |
|---|---|
| `pizz`, `arco`, `spicc`, `nat` | Ok, one item each |
| `con sord`, `senza sord` | Ok |
| `sul pont`, `sul tasto`, `pst` | Ok |
| `senza vibrato`, `vibrato` | Ok |
| `vibrato->` | Ok, withDuration |
| `?` | Ok, stop |
| `(arco)` | Ok, parenthesised |
| `[pizz]` | Ok, hidden |
| `pizz->arco` | Ok, 2 items |
| `downbow`, `upbow` | Ok |
| `o`, `+`, `stopped` | Ok |
| `string 3` | Unsupported |
| `ped` | Ok, pedal sustain |
| `*` | Ok, pedal stop |
| `^`, `notch`, `retake`, `nonotch` | Ok, pedal retake / remove |
| `1/4`, `1/2`, `3/4` | Unsupported (pedal level) |
| `sost`, `s*`, `unacorda`, `u*` | Unsupported |
| `harm nat n1 s1 p4 diamond` | Unsupported |
| `--^\|^---`, `BbEb`, `DCBbEbFGA` | Unsupported (harp pedaling) |
| `piz` | Incomplete |
| `xyzzy` | Invalid |
| `` | Incomplete |
