<!-- markdownlint-disable -->
# 23 — Key signatures popover (Shift-K)
Status: done · Depends on: 20 · Dorico refs: [key signatures](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_key_signatures_r.html)

Reference: `specs/reference/dorico-popovers.md` (Key signatures), `specs/reference/musescore-internals.md` (§3). Syntax checked against the Popover Reference 6.2 topic `7lf5PyOwarkepiBNCKF12g` (2026-10-16): matches the table below.

## Goal
Shift-K, type `Eb`, `f#`, `3#`, `atonal`, Return → key signature at the selected bar (or caret) on all staves.

## Dorico behaviour

### Entry syntax
| Entry | Meaning |
|---|---|
| `open`, `atonal` | Open / atonal key |
| `C`, `D`, `E` … (upper case) | Major keys |
| `c`, `d`, `e` … (lower case) | Minor keys |
| `#` or `s` / `b` or `f` | Sharp / flat in the name (`Eb`, `f#`) |
| `3#`, `3s` | Major key with 3 sharps |
| `4b`, `4f` | Major key with 4 flats |

- Custom key signatures are panel-only (not in the popover).

### Placement and variants
| | Return | Opt-Return |
|---|---|---|
| Staves | All staves | Only selected staves or the staves the caret spans |

- During note input the key signature goes **at the caret**, even mid-bar.
- Entering a key signature with an existing one selected replaces it.
- Range selection: key signature at the earliest selected item.
- Return on a selected key signature reopens the popover pre-filled.
- Local/Global indicator is shown in the popover.

## MuseScore today
- Engraving: `EditKeySig::undoChangeKeySig(tx, score, staff, tick, KeySigEvent)` — `src/engraving/editing/editkeysig.h:57`.
- Palette route: build a `KeySig` and call `applyPaletteElement` (`notationinteraction.cpp:2224`); this already handles all-staves vs local and transposing instruments (palette Ctrl-drop = local; **verify** the modifier and the transposition handling in `Measure::drop`/`Score::...KeySig`).
- `KeySigEvent` holds `Key`, `KeyMode` (major/minor/none), `isAtonal`/custom state (**verify** names in `src/engraving/dom/keysig.h`, `types.h`). Key range is ±7.
- Mid-bar key signatures **are** supported: `undoChangeKeySig` at a mid-bar tick creates a `KeySig` segment there (`ChordRest::drop` with Ctrl does exactly this). So no "next bar start" fallback is needed.
- `Measure::drop` / `KeySig::drop` just loop `undoChangeKeySig` over every staff (Ctrl = this staff only); `undoChangeKeySig` itself transposes the key for transposing staves when concert pitch is off, skips drum staves and walks linked staves. The handler calls it directly instead of going through `applyPaletteElement`.
- `KeySigEvent` (`dom/key.h`): `setConcertKey(Key)` sets both concert and written key; `setMode(KeyMode::MAJOR/MINOR/NONE)`; `isAtonal()` = mode `NONE`; the palette's "Open/Atonal" cell is `C` + `setCustom(true)` + `KeyMode::NONE`.

## Design
- Parser: `popover/parsers/keysigparser.{h,cpp}` (as built, same shape as `timesigparser`).
  ```cpp
  struct KeySigEntry { enum class Kind { Open, Tonal }; Kind kind; int fifths; /* -7..7 */ bool minor; };
  struct KeySigParse { ParseStatus status; KeySigEntry entry; };
  KeySigParse parseKeySig(const std::string&);
  std::string keySigText(const KeySigEntry&); // prefill: "Eb", "f#", "open"
  ```
  Algorithm:
  1. `open`/`atonal` (case-insensitive) → Open.
  2. Leading digit: `^([1-7])([#sbf])$` → major, `fifths = ±n`. `0` → C major (accept `0#`? Invalid).
  3. Else `^([A-Ga-g])([#sbf]?)$`: upper-case letter → major, lower → minor. Compute fifths from the table of letter + accidental; out-of-range (e.g. `D#` major = 9 sharps) → Invalid. Letter `b` alone is B minor; `bb` is B♭ minor; `Bb` is B♭ major.
  4. Prefixes of valid input (`o`, `ope`, `at`, `3`) → Incomplete (case-insensitive for `open`/`atonal`).
- Handler: `handlers/keysighandler.cpp`.
  - Build a `KeySigEvent` (`setConcertKey`, `setMode` MAJOR/MINOR; Open = C + custom + `KeyMode::NONE`, as the palette).
  - Tick: the caret's chord/rest (`InputState::tick()`, mid-bar allowed), a range's start segment, or the earliest selected item.
  - Return: `EditKeySig::undoChangeKeySig` on every staff of the master score (also when a part is open). Opt-Return: only the selected / caret staves. Drum staves skipped; nothing left → refused.
  - The typed key is the **concert key**, consistent with MuseScore's palette; `undoChangeKeySig` writes the transposing staves' keys (verified with a B♭ trumpet).
  - Outside note input the new key signature (top selected staff) is selected afterwards, so Return reopens it.
- `prefill`: key name from the event's concert key and mode (`Eb`, `f#`, `open`); a custom key prefills nothing.
- `handlesItem`: `KeySig`.
- Key binding: `Shift+K` → `command://dorico/popover/keysig`.

## Deferred / unsupported
- Custom key signatures (panel-only in Dorico too; MuseScore has its own palette/dialog).
- Caret **inside** a note or rest (spec 12's grid caret): the key goes at the start of that note/rest, not at the exact grid position (our choice, same as the dynamics popover; `IDoricoCaret::materialize` could cut the note first if needed). A caret on a note/rest start mid-bar works.
- Keys beyond ±7 sharps/flats (`D#`, `8#`) → Invalid.
- Other tonality systems (non-12-EDO).

## Acceptance criteria
- [x] Shift-K, `Eb`, Return on bar 1 gives 3 flats on all staves; Cmd-Z restores.
- [x] `c` gives C minor (3 flats, minor mode); `f#` gives F♯ minor (3 sharps, minor).
- [x] `3#` gives A major; `4b` gives A♭ major.
- [x] `open` / `atonal` removes the signature accidentals and marks the key open.
- [x] On a score with a B♭ clarinet staff the written key is correct (concert Eb → written F).
- [x] Opt-Return with only staff 1 selected changes only staff 1.
- [x] Selecting an existing key signature and applying a new one replaces it.
- [x] Return on a selected key signature reopens pre-filled with its name.
- [x] `D#`, `8#`, `xyz` are red/Invalid.
- [x] Parser tests pass.

## Tests
`src/dorico/tests/keysigparser_tests.cpp` (5 tests, all pass; full `dorico_tests` 74/74). Also: `Ef`, `a#` (+7 minor), `ab` (−7 minor), `1b`, `Open` Ok; `at`, `atona`, `O`, `7` Incomplete; `ff`, `0`, `8`, `3x`, `33#`, `Ebb`, `H`, `s`, `#`, `opens` Invalid; `keySigText` round-trips every key, major and minor.

| Input | Expected |
|---|---|
| `C` | Ok, 0, major |
| `c` | Ok, −3, minor |
| `a` | Ok, 0, minor |
| `A` | Ok, +3, major |
| `Eb` | Ok, −3, major |
| `Es` | Invalid (E♯ major = 11 sharps, out of range) |
| `f#` | Ok, +3, minor |
| `Fs` | Ok, +6, major |
| `bb` | Ok, −5, minor |
| `Bb` | Ok, −2, major |
| `b` | Ok, +2, minor |
| `Cb`, `C#` | Ok, −7 / +7, major |
| `D#` | Invalid |
| `3#`, `3s` | Ok, +3, major |
| `4b`, `4f` | Ok, −4, major |
| `7#` | Ok, +7 |
| `8#`, `0b` | Invalid |
| `open`, `atonal`, `ATONAL` | Ok, Open |
| `ato` | Incomplete |
| `` | Incomplete |
| `xyz` | Invalid |

### Manual test (2026-10-16, ClaudeUIControl on our build, temp copy of `demos/Brassed_Up.mscx`: B♭ Trumpet, 2 Trombones, Tuba, concert A♭ major, transposed view)
| Step | Result |
|---|---|
| Bar 1 selected, Shift-K | popover shows "Global"; `Eb` Return → 3 flats on trombones/tuba, trumpet written F (1 flat); new key selected |
| Return on it | reopens with `Eb`; `c` → same 3 flats, reopens as `c` (minor stored) |
| `f#` / `3#` / `4b` | concert 3 sharps (trumpet 5 sharps) / 3 sharps / 4 flats (trumpet 2 flats), reopens as `Ab` |
| `open` | accidentals gone on all staves, reopens as `open` |
| `D#`, `8#`, `xyz` / `ato` | red border / neutral |
| Cmd-Z ×6 | original key back, one step per Return |
| Trumpet bar 1 only, `D` Opt-Return | only the trumpet changes (written E major) |
| Note input on trombone 1, caret moved to beat 3 (an eighth), `G` Return | key signature mid-bar at beat 3 on all staves (trumpet written A), note input continues |
| Clicked original key signature (selects every staff), Return | reopens with `Ab`; `Bb` replaces it (trumpet written C) |

Side note, not ours: MuseScore's undo of a step made in note input re-enters note input, so a click right after Cmd-Z enters a note.
