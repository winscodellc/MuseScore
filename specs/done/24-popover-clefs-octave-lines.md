<!-- markdownlint-disable -->
# 24 — Clefs and octave lines popover (Shift-C)
Status: done · Depends on: 20 · Dorico refs: [clefs](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_clefs_octave_lines_clefs_r.html), [clef octave shifts](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_clefs_octave_lines_clef_octave_shifts_r.html), [octave lines](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_clefs_octave_lines_octave_lines_r.html)

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
| Loco line (a *loco* indication line, [Clefs panel](https://www.steinberg.help/api/khub/maps/BYbh2hvBDwUCjSijIhHcqA/topics/N_dBVuMUrGRudh~juycrkQ/content)) | `loco` |
| Stop during note input | `\|`, `stop` |

### Placement and variants
| | Return | Opt-Return |
|---|---|---|
| Octave lines | All voices | Current voice only |
| Clefs | Staff-wide (no local variant documented) | same |

- **Range selection:** octave line spans the range; a clef is inserted at the start **and a restorative clef at the end of the range** (the clef in force before).
- **Single item / caret:** clef at the earliest selected item or caret (mid-bar allowed in Dorico; MuseScore supports mid-bar clef changes).
- `loco` inputs a *loco* line (the manual lists it as "Loco indication"; the Clefs panel calls it "a loco line"). It does **not** end an existing octave line (the first draft of this spec said so; corrected after checking the manual).
- In note input an octave line is open-ended and stops with `|`/`stop` (or end of input).
- Return on a selected clef or octave line reopens the popover pre-filled.

## MuseScore today
- Clef: `EditClef::undoChangeClef(tx, score, staff, item, ClefType)` — `src/engraving/editing/editclef.cpp:141`: adds a clef before `item` (moved to the end of the previous bar at a bar start), or changes `item` when it is a clef. It silently skips a clef whose staff group (standard / percussion / tab) does not match the staff. `INotationInteraction::insertClef` only works at the note-input caret.
- Octave lines: `Score::cmdAddOttava` (`src/engraving/editing/edit.cpp:954`), staff-level spanner, no voice assignment.
- `ClefType` (`src/engraving/types/types.h:717`) has every clef in the table **except alto 8vb and invisible**. `G_1` = French violin, `F_B` = baritone F (F on line 3), `F_C` = sub-bass (F on line 5), `C4_8VB` = tenor 8vb, `TAB` = 6-string, `TAB4` = 4-string.
- `OttavaType` (`src/engraving/dom/ottava.h:46`) has 8va/8vb/15ma/15mb/**22ma/22mb**: all six octave lines are supported.

## Design (as built)
- Parser: `popover/parsers/clefparser.{h,cpp}`, case-insensitive.
  ```cpp
  struct ClefEntry { enum class Kind { Clef, OctaveLine, Stop }; Kind kind; ClefType clef; int octaves; /* -3..3 */ };
  struct ClefParse { ParseStatus status; ClefEntry entry; };
  ClefParse parseClef(const std::string&);
  std::string clefText(ClefType);   // prefill: first entry of the clef's row ("treble", "bass8ba"); "" for clefs with no entry
  std::string octaveLineText(int);  // prefill: "8va", "15ba", ...
  ```
  One table of clef rows (ClefType + entries, canonical entry first; INVALID rows = Unsupported) and one of octave lines. Order: `|`/`stop`, `loco`, octave-line entries, clef entries, then `<clef>[+-]N[c|t]`, then prefixes of any entry → Incomplete. Octave shifts move along the families G (15mb … 15ma), F (15mb … 15ma) and tenor (8vb, loco); anything else, or a `c`/`t` suffix, is Unsupported.
- Handler: `popover/handlers/clefhandler.cpp`.
  - Clef: a target at a bar's first item is passed as the bar, so bar 1 changes its initial clef (as MuseScore's palette drop). Caret → before the caret's chord/rest on the caret staff; list selection → before the earliest selected item **of each selected staff** (a selected clef is changed in place); range → at the range start on every staff of the range, plus a restorative clef at the range end (the clef in force just before the end; skipped when the range reaches the score end or a clef change is already at the end). Staves whose staff group does not fit the clef are skipped; none left → refused (popover stays open).
  - Octave line: range → each staff spans the range; list → from the earliest selected item to the end of the latest one, on the earliest one's staff; a selected octave line is replaced by the new type over the same span. Placement (above / below) comes from the type's style.
  - Note input: `8va` starts a pending line at the caret on the caret staff; `|`, `stop`, the next octave-line entry or the end of note input creates it up to the caret (same pending pattern as spec 21: it appears when stopped).
  - Outside note input the new clef / line is selected, so Return reopens it.
- `prefill`: clef → `clefText`, octave line → `octaveLineText`. `itemTypes`: `CLEF`, `OTTAVA_SEGMENT`.
- `supportsLocal = false`: Opt-Return behaves as Return.
- Key binding: `Shift+C` → `command://dorico/popover/clef` (already in `shortcuts_dorico.json` from spec 20). No upstream edits.

## Deferred / unsupported
- `invisible`, alto 8vb (`alto8ba`, `ca8ba`, `ca8d`, `alto8d`): no such `ClefType` → Unsupported.
- Clef octave shifts outside the families above (`g+3`, `tenor+1`, `alto-1`, `perc+1`) and the `c` / `t` (concert / transposed pitch only) suffix → Unsupported.
- `loco` (loco line): MuseScore has no loco octave-line type → Unsupported.
- Opt-Return "current voice only" for octave lines: MuseScore octave lines have no voice scope; treated as Return.
- The pending octave line in note input does not grow while notes are entered; it appears when stopped.
- A percussion or tab clef on a pitched staff (and the reverse) is refused: MuseScore changes the staff type for that, not the clef.
- Mid-bar clef placement quirks (courtesy clefs follow MuseScore defaults).

## Acceptance criteria
Manual run 2026-10-16 in our build with ClaudeUIControl, on temp copies of a 4-bar treble score, `midimapping_data/test16-ref.mscx` (drum staff) and `tab_transpose_data/low_frets.mscx` (6-string tab).
- [x] Shift-C, `bass`, Return on a note → bass clef from that note onward (at the end of the previous bar when the note starts a bar); Cmd-Z restores. On the score's first note the initial clef is replaced.
- [x] Range selection of bars 2–3, `alto` → alto clef at bar 2, previous clef restored at the start of bar 4; one Cmd-Z removes both. From bar 1 the initial clef is replaced.
- [x] `g8ba` → treble 8vb; `f15ma` → bass 15ma; `perc` on a drum staff → percussion clef; `tab4` / `tab6` on a tab staff → 4- / 6-string tab clef. `perc` on a pitched staff and `treble` on a drum staff are refused (popover stays open).
- [x] `g+1` → treble 8va clef; `f-2` → bass 15mb.
- [x] `8va` over a selected range creates an 8va line spanning it; `8ba` / `15ma` / `22ba` give the right types (8vb and 22mb below the staff).
- [x] Note input: `8va`, enter 4 notes, then `|` → the line spans exactly those notes. `8ba`, 2 notes, then leaving note input (Shift-N) also ends it there.
- [x] `loco` shows "not supported yet" in red.
- [x] Return on a selected clef or octave line reopens pre-filled (`bass`, `8va`, `tab4`); entering another replaces it in place.
- [x] `22ma` / `22ba` create 22ma / 22mb lines.
- [x] Parser tests pass (79/79 `dorico_tests`).

Found and fixed during the run: a clef on a bar's first item was added as a second clef after the initial one in bar 1 (EditClef only uses the header clef when given the bar). The handler now passes the bar, as MuseScore's palette drop does (`notationinteraction.cpp:3103`).

Observed, not spec 24: opening a second score with `open -a mscore.app file` while one is open retitles the window but keeps showing the first score, and the save dialog then offers to save it under the second file's name. Not investigated (spec 50 shell or upstream).

## Tests
`src/dorico/tests/clefparser_tests.cpp` (5 tests, all pass; full `dorico_tests` 79/79). Decisions: `treble+1t` → Unsupported (suffix); `loco` → Unsupported; `22ma` Ok.

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
| `invisible`, `alto8ba`, `loco` | Unsupported |
| `g+2` | Ok, G, octaveShift +2 |
| `treble+1t`, `g+3` | Unsupported |
| `8va`, `8`, `8u`, `1u` | Ok, octaves +1 |
| `15ma`, `15`, `2u` | Ok, +2 |
| `22ma`, `22`, `3u` | Ok, +3 |
| `8ba`, `8vb`, `8d`, `1d` | Ok, −1 |
| `15ba`, `15vb`, `2d` | Ok, −2 |
| `22ba`, `3d` | Ok, −3 |
| `\|`, `stop` | Ok, stop |
| `tre` | Incomplete |
| `8x`, `xyz`, `9va` | Invalid |
