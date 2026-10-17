<!-- markdownlint-disable -->
# 28 — Ornaments popover (Shift-O)
Status: done · Depends on: 20 · Dorico refs: ornament pages `dorico_popovers_ornaments_*_r.html` under [popover reference](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/), [index](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_ornaments_ornaments_r.html)

Reference: `specs/reference/dorico-popovers.md` (Ornaments), `specs/reference/musescore-internals.md` (§3). Manual pages checked: "Inputting ornaments/trills with the popover", "... arpeggio signs ...", "... glissando lines ...", "... jazz articulations ...", "Jazz articulations", "Types of arpeggio signs".

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
- Ornaments, jazz ornaments and trills (manual): on each selected staff, at the earliest selected note; a trill spans to the end of the latest selected note on that staff (range or list).
- Arpeggio signs: on every selected chord. Jazz articulations: on every selected note ("during note input, this is usually the last note you input").
- Glissandos: between the selected notes; a single note joins the next note in its voice. Not available during note input.
- Note input: ornaments, trills and arpeggios are input at the caret once a note is input or Space advances the caret; a trill spans that advance. Arpeggios need chord input (Q) in Dorico.
- Return on a selected ornament/trill/arpeggio/glissando/jazz articulation reopens the popover pre-filled.

## MuseScore today
- Ornament symbols: `Ornament` (an `Articulation` subclass) on the chord, toggled by `EditChord::toggleArticulation(score, chord, item)` (what `EditNote::toggleOrnament` uses). SymIds `ornamentShortTrill`, `ornamentMordent`, `ornamentTurn`, `ornamentTurnInverted` exist.
- Trill line: `Trill` spanner (`TrillType::TRILL_LINE`) owns an `Ornament` (`trill->ornament()`) whose `intervalAbove` (`OrnamentInterval { IntervalStep, IntervalType }`) covers every degree 1-8 and quality (auto, major, minor, perfect, augmented, diminished). The interval is drawn as a parenthesized cue note.
- Arpeggio: `Arpeggio` on the chord (`ArpeggioType::NORMAL / UP / DOWN / BRACKET / UP_STRAIGHT / DOWN_STRAIGHT`); `Chord::drop` replaces an existing one.
- Glissando: `Glissando` (`GlissandoType::STRAIGHT / WAVY`) added through `Note::acceptDrop` / `Note::drop` (guesses the end note when none is given; returns nothing).
- Jazz articulations are not symbols in MuseScore: they are `ChordLine` (`ChordLineType::FALL / DOIT / PLOP / SCOOP`), curved (Dorico "bend") or straight (Dorico "smooth"), one per note and side.
- Jazz ornaments (flip, smear, jazz turn, bend) are `Articulation`s with SMuFL `brassFlip / brassSmear / brassJazzTurn / brassBend` (as MusicXML import makes them); none is in the default palettes.
- Guitar techniques (bend, hammer-on, pull-off, taps, vibrato bar) exist only in tablature/guitar palette elements.

## Design
- Parser `popover/parsers/ornamentsparser.{h,cpp}`: `parseOrnament(text) -> { status, OrnamentEntry }` with `OrnamentEntry { kind (Ornament, Trill, JazzOrnament, JazzArticulation, Arpeggio, Glissando), sym, interval, chordLine + smooth, arpeggio, glissando }`. Words ignore case; the trill interval keeps it (`M` major, `m` minor). `tr`/`trill` + space + `[quality]degree`; a perfect 2nd/3rd/6th/7th or a major/minor 1st/4th/5th/8th is Invalid. Prefill helpers `ornamentText`, `trillText`, `arpeggioText`, `glissandoText`, `jazzArticulationText`.
- Mapping: `arp`/`arpup`/`arpeggioup` -> `NORMAL` (Dorico's up sign has no arrow by default), `arpdown` -> `DOWN`, `nonarp` -> `BRACKET`; `gliss` -> MuseScore's default type, `glissstraight` / `glisswavy` -> `STRAIGHT` / `WAVY`; `plop`, `scoop`, `doit`, `fall` -> curved `ChordLine`, `plopsmooth`, `doitsmooth`, `fallsmooth` -> straight `ChordLine`.
- Handler `popover/handlers/ornamentshandler.cpp`: one transaction per Return. Ornament symbols and jazz ornaments toggle (the same entry again removes it); a jazz articulation replaces the one on its side of the note (the same one again removes it); an arpeggio replaces the chord's; a trill replaces one starting at the same note in that voice. The new item is selected so Return reopens it.
- Note input: a `PendingOrnaments` object (an `Asyncable`) keeps the entries and the caret position, listens to `notationChanged`, `selectionChanged` and note input `stateChanged`, and on the next event loop turn after the caret moves on adds them to the chord/rest at the old caret position (a trill up to the new caret). It is dropped when the caret moves back or note input ends. (Rest entry via `Score::cmdEnterRest` bypasses the notation undo stack, so `stackChanged` alone missed Space.)
- `prefill` / `itemTypes`: `ORNAMENT`, `TRILL_SEGMENT`, `ARPEGGIO`, `GLISSANDO_SEGMENT`, `CHORDLINE`.
- Key binding: `Shift+O` -> `command://dorico/popover/ornaments` (already in `shortcuts_dorico.json`).
- The dorico module is a unity build: file-local helper names must be unique across `popover/` files (`tempoparser.cpp`'s `Gradual` / `GRADUALS` / `trim` renamed, they clashed with `dynamicsparser.cpp` once the batches shifted).

## Deferred / unsupported
- Guitar techniques (all rows: `bend`, `vibbend`, `vibscoop`, `vibdip`, `wbar`, `w/bar`, `hammer`, `ho`, `pull`, `po`, `hammerpull`, `hopo`, `hp`, `ligado`, `lig`, `tap`, `tappull`, `lhtap`, `lhtappull`) -> `Unsupported` (tablature-only elements; revisit).
- `slurarp` (curved arpeggio) -> `Unsupported` (no MuseScore type).
- Jazz ornaments (flip, smear, jazz turn, bend) are `Articulation`s: Return on one does not reopen this popover (the articulation type is shared with every other articulation).
- `scoop` has no smooth variant in Dorico's popover (Dorico calls the smooth one a lift); a straight scoop has no prefill.
- Arpeggios across voices or staves of one instrument (Dorico spans them): one arpeggio per chord.
- The trill line does not grow while notes are entered: it spans the one note (or Space advance) after the popover. Multi-staff caret: the caret's staff only.
- "Separate trill for each selected note" preference, trill interval changes partway through a trill, item input position preference (rests): not implemented.
- `gliss` during note input does nothing (Dorico does not allow it either); the popover stays open.
- An item in MuseScore's edit mode (e.g. a glissando grip after clicking its end) swallows Shift-O; Return reopens its popover.

## Acceptance criteria
- [x] Shift-O, `tr`, Return on a half note → trill line over the note; Cmd-Z removes it.
- [x] `tr` on a range selection → trill line spanning the range.
- [x] `tr m3` → trill with a minor-third interval shown/ played accordingly (or the nearest supported representation).
- [x] `mor`, `turn`, `invturn`, `shorttr` → matching ornament symbol; pressing again toggles it off (matches toggle semantics).
- [x] `arp`, `arpdown`, `nonarp` on a chord → matching `Arpeggio` type.
- [x] `gliss`, `glisswavy` between two selected notes → straight / wavy glissando.
- [x] `fall`, `doit`, `scoop`, `plop`, `flip`, `smear`, `jazz` → matching jazz articulation.
- [x] Guitar technique entries show red + "not supported yet".
- [x] Select an existing ornament, Return → pre-filled; change and Return replaces it.
- [x] Parser tests pass.

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
| `plopsmooth`, `doitsmooth`, `fallsmooth` | Ok (straight `ChordLine`) |
| `bend`, `hammer`, `ho`, `pull`, `po`, `hp`, `ligado`, `tap`, `lhtap`, `wbar`, `w/bar` | Unsupported |
| `tr` + space (incomplete interval) | Incomplete |
| `xyz` | Invalid |

### Manual test (2026-10-17, generated 6-bar treble score)
| Step | Result |
|---|---|
| Half note, Shift-O `tr` Return | Trill line over the half note, selected; Cmd-Z steps back |
| Return on the trill | Popover pre-filled `tr`; `tr m3` -> trill with a parenthesized E♭ cue note above C |
| Range (4 quarters), `tr` | One trill line over the range |
| `mor`, `turn`, `invturn`, `shorttr` | Matching symbol; `mor` twice toggles it off |
| `arp`, `arpdown`, `nonarp` on chords | Plain wavy, down arrow, bracket (replaces the arpeggio) |
| Two notes `gliss`; one note `glisswavy` | Straight C->E; wavy D->next G; Return pre-fills `glisswavy`, `glissstraight` replaces it |
| `fall`, `doit`, `scoop`, `plop`; Return on fall, `fallsmooth` | Curved jazz lines; straight fall replaces the curved one |
| `flip`, `smear`, `jazz`, `brassbend` | Matching brass symbols |
| `bend` | Red border, "Not supported yet" |
| Note input: `tr` then C / then Space | Trill over the quarter C / over the entered rest |
| Note input: `mor` then D | Mordent on D |
| Chord input (Q): `arpdown`, C E G, Space | Down arpeggio on the chord |
