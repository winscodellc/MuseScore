<!-- markdownlint-disable -->
# 32 — Popover: chord symbols (Shift-Q)

Status: done · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_chord_symbols_*_r.html`
- [P] `.../dorico_popovers_navigation_r.html`
- [M] `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_input/write_mode_chord_symbols_inputting_t.html`

Reference: [popovers](../reference/dorico-popovers.md) (B1, B2 "Chord symbols") · [internals](../reference/musescore-internals.md) (section 3, `addTextToItem`).

Names such as `parse`, `Result` and `apply` follow the contract in spec 20; rename to match its final API.

## Goal
Shift-Q opens a popover. Typing a chord such as `Cmaj7`, `Dm7b5/G` or `F#dim` and pressing Return puts a chord symbol on the selected note. Space moves on to the next beat so a whole progression can be typed.

## Dorico behaviour

**Roots** (English: `C`, `Db`, `F#`, `B`; German `H`, `Des`, `Fis`; fixed-do `do`, `reb`, `fa#`; Nashville `1`, `2b`, `4#`).
Accidentals: `#`; `b`; `x` or `##`; `x#`, `#x`, `###`; `bb`; `bbb`.

**Qualities**

| Quality | Entries |
|---|---|
| Major | `M` (case-sensitive), `maj`, `ma`, or nothing |
| Minor | `m` (case-sensitive), `min`, `mi` |
| Diminished | `dim`, `di`, `o` |
| Augmented | `aug`, `au`, `ag`, `+` |
| Half-diminished | `half-dim`, `halfdim`, `hd` |
| 6/9 | `6/9`, `69`, `%` |

**Intervals and alterations**:
- Degrees `1`, `2`, `3`, and so on. Major 7th is `^7` or `^`; major 9th is `^9`, `maj9` or `9maj7`.
- Altered degrees: `+9` / `#9` (sharpened), `-5` / `b5` (flattened).
- `add+11`, `addb9`, `addF#`, `addBb` (added note), `sus4`, `sus9`, `omit3` / `no3`.

**Modal chords**: `ionian`, `dorian`, `phrygian`, `lydian`, `mixolydian`, `aeolian`, `locrian`, `melodic minor`, `harmonic minor`, `whole tone`, plus the diminished / octatonic half-whole and whole-half names listed in reference B2.

**Other**

| Entry | Meaning |
|---|---|
| `Cmajb5/Eb`, `Cmajb5,Eb` | Altered bass |
| `Cmaj7\|D`, `Cmaj7;D` | Polychord |
| `N.C.`, `NC`, `no chord`, `none` | No chord |
| `reg` | Chord symbol region |
| `(Cmaj7)` | Parenthesised |

**Popover behaviour**:
- Opening it stops note input.
- It accepts MIDI keyboard or on-screen Keyboard panel input in place of typing.
- **Space**: input the entry and move to the next beat. **Shift-Space**: previous beat.
- **Tab**: start of the next bar. **Shift-Tab**: start of the current bar if mid-bar, otherwise the previous bar.
- **Right / Left**: next / previous note, rest, grid position or character, whichever is closest.
- **Cmd-Right / Cmd-Left**: next / previous existing chord symbol.
- **Up / Down**: chord symbol line number.
- **Return**: input and close, using the local/global type shown in the popover.
- **Opt-Return**: input using the opposite type for this one entry. **Opt-L / Opt-G**: set the popover to local / global.
- **Esc**: close.
- **Editing existing**: Return on a selected chord symbol reopens the popover with its text.

## MuseScore today
- Text-style route: `NotationInteraction::addTextToItem(TextStyleType, item)` (`src/notation/internal/notationinteraction.cpp:6204`) → `Score::addText` (`src/engraving/editing/edit.cpp:640`) → `startEditText`. The chord-symbol styles are `TextStyleType::HARMONY_A`, `HARMONY_ROMAN` and `HARMONY_NASHVILLE`. The controller wrapper is `NotationActionController::addText` (`notationactioncontroller.cpp:2407`).
- Existing parser: the engraving `Harmony` class parses chord text into root, bass and quality (look for `parseHarmony` in `src/engraving/dom/harmony.cpp`; confirm the exact name and signature).
- Existing shortcut: Ctrl+K for `add-chord-text` (research; collides with a Cmd-K prefix only if multi-key shortcuts are used).
- MuseScore has no local/global chord-symbol distinction. A chord symbol belongs to a segment on one staff.

## Design
- **New files**: `src/dorico/popover/parsers/chordsymbolsparser.h/.cpp` (`parseChordSymbol(text) -> ChordSymbolParse`), `popover/handlers/chordsymbolshandler.h/.cpp` (`chordSymbolsPopover()`), and `popover/popovernavigation.h/.cpp` (beat / bar / item moves, shared with later navigating popovers such as figured bass).
- **Approach:** do not write a second chord parser. The Dorico parser is a translation layer:
  1. Normalise Dorico spellings to MuseScore's input spelling: `^7` → `maj7`, `half-dim` / `halfdim` / `hd` → `m7b5`, `o` → `dim`, `ag` / `au` → `aug`, `%` / `69` → `6/9`, `x` → `##`, `no3` → `omit3`, German and fixed-do roots → English.
  2. Validate: the root and bass must be notes, and MuseScore's `ParsedChord::parse` must find every token of the rest understandable (`Harmony::parseHarmony` alone accepts any text after a valid root, so `Cxyz` would pass).
  3. The result holds both the original text and the normalised text.
- **Apply:** create the chord symbol on the selected chord/rest's segment, using the `HARMONY_A` style (or the roman/Nashville style from a setting). Set its text to the normalised string inside one `startEdit()` / `apply()`.
- **Space / Shift-Space / Tab / Shift-Tab:** a *navigating* popover (`PopoverDef::applyAt` set) sits at a `PopoverPosition` (tick + staff) rather than the selection, because a beat inside a long note has nothing to select. These keys apply the entry there (no undo step if the text is unchanged) and move the position; the popover reopens there, prefilled with any chord symbol it finds. Beats follow MuseScore's own chord-symbol navigation (dotted quarter in compound time). `INotationInteraction::navigateToNearHarmony` and friends were not reusable: they only work while a harmony is being text-edited. A beat with no note gets an empty ChordRest segment (`Measure::undoGetSegment`), as MuseScore's own Space does.
- **Cmd-Right / Cmd-Left:** jump to the next/previous existing harmony on the staff.
- **Opt-Return, Opt-L, Opt-G:** accepted and ignored. They produce the same result as Return (see Deferred).
- **Up / Down line number:** deferred.
- **Opening the popover** dispatches the note-input stop command first.
- **Upstream edits:** none.

## Deferred / unsupported
- Local vs global chord symbols, and so Opt-Return, Opt-L and Opt-G.
- Chord symbol regions (`reg`).
- Chord symbol line numbers (Up / Down).
- MIDI keyboard and Keyboard-panel entry into the popover.
- Modal chord names (`dorian` and the rest), triple accidentals (`x#`, `bbb`) and added notes by name (`addF#`): "not supported yet". Parenthesised chords and polychords (`|`, `;`) work, as MuseScore's Harmony parses them itself.
- Nashville and roman-numeral entry (needs the style switch): "not supported yet".
- Right / Left moving the popover to the nearest note, rest or grid position: the arrows move the text cursor only.
- Entries with a space (`no chord`, `melodic minor`) cannot be typed, because Space moves the popover. The parser accepts them; `NC` / `N.C.` are the way to type a no-chord symbol.
- Editing an existing chord symbol replaces it with a new one, so manual offsets on the old one are lost.
- Lower-case English roots (`c`, `f#m`): only upper-case English and German roots, and lower-case fixed-do syllables (`do`, `fa#`) are roots.
- Anything the MuseScore harmony parser rejects after normalisation goes through the spec 20 invalid-input state.

## Acceptance criteria
- [x] Shift-Q on a selected note opens the popover; typing `Cmaj7` and Return adds a chord symbol reading Cmaj7.
- [x] `Dm7b5/G` produces a half-diminished chord with bass G.
- [x] Space inputs the entry and moves the popover to the next beat; five entries in a row fill five beats.
- [x] Tab moves to the next bar; Shift-Tab to the bar start.
- [x] Return on an existing chord symbol reopens it pre-filled; changing the text and pressing Return replaces it.
- [x] `N.C.` and `NC` insert a no-chord symbol.
- [x] Opening the popover during note input stops note input.
- [x] `xyz!` shows the error state.
- [x] One undo per entry.

## Tests
GoogleTest, `src/dorico/tests/chordsymbolsparser_tests.cpp` (plus the beat math in `popoverregistry_tests.cpp`). "Normalised" is the text handed to the MuseScore parser:

| Input | Normalised | Valid |
|---|---|---|
| `C` | `C` | yes |
| `Cmaj7` | `Cmaj7` | yes |
| `C^7` | `Cmaj7` | yes |
| `C^` | `Cmaj7` | yes |
| `Cm` | `Cm` | yes |
| `CM` | `C` | yes (case-sensitive major) |
| `Cmin7` | `Cm7` | yes |
| `Cdim` | `Cdim` | yes |
| `Co` | `Cdim` | yes |
| `Chd` | `Cm7b5` | yes |
| `Chalf-dim` | `Cm7b5` | yes |
| `Caug` / `C+` | `Caug` | yes |
| `C6/9` / `C69` / `C%` | `C6/9` | yes |
| `Dm7b5/G` | `Dm7b5/G` | yes |
| `Cmajb5,Eb` | `Cmajb5/Eb` | yes |
| `Fx` | `F##` | yes |
| `Csus4` | `Csus4` | yes |
| `Cno3` | `Comit3` | yes |
| `N.C.` / `NC` / `none` / `no chord` | `N.C.` | yes |
| `H` | `B` | yes (German root) |
| `Des` | `Db` | yes |
| `xyz!` | — | no |
| `""` | — | no |
