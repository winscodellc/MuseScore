# 23 — Key signatures popover (Shift-K)
Status: todo · Depends on: 20 · Dorico refs: [key signatures](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_key_signatures_r.html)

Reference: `specs/reference/dorico-popovers.md` (Key signatures), `specs/reference/musescore-internals.md` (§3).

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
- Mid-bar key signatures are not natively allowed in MuseScore (key signatures belong to measure starts). **Verify** whether the caret mid-bar case can be satisfied by inserting at the next measure start (see Deferred).

## Design
- Parser: `popover/parsers/keysigparser.{h,cpp}`.
  ```cpp
  struct KeySigEntry { enum class Kind { Open, Tonal }; int fifths; /* -7..7 */ bool minor; };
  ParseResult<KeySigEntry> parseKeySig(std::string_view);
  ```
  Algorithm:
  1. `open`/`atonal` (case-insensitive) → Open.
  2. Leading digit: `^([1-7])([#sbf])$` → major, `fifths = ±n`. `0` → C major (accept `0#`? Invalid).
  3. Else `^([A-Ga-g])([#sbf]?)$`: upper-case letter → major, lower → minor. Compute fifths from the table of letter + accidental; out-of-range (e.g. `D#` major = 9 sharps) → Invalid. Letter `b` alone is B minor; `bb` is B♭ minor; `Bb` is B♭ major.
  4. Prefixes of valid input (`o`, `ope`, `3`) → Incomplete.
- Handler: `handlers/keysighandler.cpp`.
  - Create `KeySig` with `KeySigEvent` (`setConcertKey`, `setMode`); for Open set the atonal/none mode.
  - Apply via `applyPaletteElement` with local variant modifier for Opt-Return (**verify**); otherwise loop `EditKeySig::undoChangeKeySig` over selected staves.
  - Note input active: use the input segment's tick; if it is not a measure start, apply at the **next measure start** and set a one-line message (v1).
  - The typed key is the **concert key**, consistent with MuseScore's palette; transposing staves get the written key automatically (**verify**; if not, apply `Staff::transposeKey`).
- `prefill`: key name from the event (`Eb`, `f#`, `open`).
- `handlesItem`: `KeySig`.
- Key binding: `Shift+K` → `command://dorico/popover/keysig`.

## Deferred / unsupported
- Custom key signatures (panel-only in Dorico too; MuseScore has its own palette/dialog).
- Mid-bar key signature at the caret (MuseScore limitation) — placed at next bar start, v1.
- Keys beyond ±7 sharps/flats (`D#`, `8#`) → Invalid.
- Other tonality systems (non-12-EDO).

## Acceptance criteria
- [ ] Shift-K, `Eb`, Return on bar 1 gives 3 flats on all staves; Cmd-Z restores.
- [ ] `c` gives C minor (3 flats, minor mode); `f#` gives F♯ minor (3 sharps, minor).
- [ ] `3#` gives A major; `4b` gives A♭ major.
- [ ] `open` / `atonal` removes the signature accidentals and marks the key open.
- [ ] On a score with a B♭ clarinet staff the written key is correct (concert Eb → written F).
- [ ] Opt-Return with only staff 1 selected changes only staff 1.
- [ ] Selecting an existing key signature and applying a new one replaces it.
- [ ] Return on a selected key signature reopens pre-filled with its name.
- [ ] `D#`, `8#`, `xyz` are red/Invalid.
- [ ] Parser tests pass.

## Tests
`src/dorico/tests/keysigparser_tests.cpp`:

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
