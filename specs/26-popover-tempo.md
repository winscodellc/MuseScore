# 26 — Tempo popover (Shift-T)
Status: todo · Depends on: 20 · Dorico refs: [tempo popover reference](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_tempo_r.html) (page names `dorico_popovers_tempo_*_r.html`), [beat units](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_beat_units_r.html)

Reference: `specs/reference/dorico-popovers.md` (Tempo, Beat units), `specs/reference/musescore-internals.md` (§3).

## Goal
Shift-T, type `Allegro`, `q=120`, `Presto q=176`, `rit.`, `a tempo`-style free text; Return creates a tempo marking (or gradual tempo line) at the selection or caret.

## Dorico behaviour

### General
- A suggestion menu appears as you type; **free text is allowed**.
- `(q=72)` = parenthesised. `[q=96]` = hidden (plays, not printed).
- During note input a gradual change gets a fixed quarter-note length and does not extend. With a range selected it spans the range.
- Picking a suggestion syllabifies gradual changes (rit-ar-dan-do).
- Return: all staves; **Opt-Return**: applies a rhythmic feel to one instrument only.

### Entries
| Kind | Entries |
|---|---|
| Absolute tempo | `Adagio`, `Allegro`, `più`, `meno`, `Rubato`, `Presto q=176` (text plus metronome mark) |
| Metronome marks | `q=176`; `(q=52)`; range `q=96-112` |
| Gradual changes | `rit.`, `ritardando`, `rit....` (continuation line); `rall.`, `rallentando`, `rall....`; `accel.`, `accelerando`, `accel....` |
| Relative changes | `Mosso`, `Lento` |
| Tempo equations | `e=e.`, `q=e`, `q.=h` (tuplet durations not supported yet) |
| Rhythmic feel | `Light swing 16ths` / `Light swing 8ths`; `Medium swing 16ths` / `8ths`; `Heavy swing 16ths` / `8ths`; `Straight (no swing)`; `2:1 swing 16ths (fixed)` / `8ths (fixed)`; `3:1 swing 16ths (fixed)` / `8ths (fixed)` |

Beat units for the left side of `=`:

| Unit | Entries |
|---|---|
| 64th | `z`, `2` |
| 32nd | `y`, `3` |
| 16th | `x`, `4` |
| Eighth | `e`, `5` |
| Quarter | `q`, `6` |
| Half | `h`, `7` |
| Whole | `w`, `8` |
| Double whole | `2w`, `9` |
| Dot | `.` |

### Placement
- Selection: tempo at the earliest selected item; gradual changes span the range.
- Caret: tempo at the caret.
- Return on a selected tempo mark reopens the popover pre-filled.

## MuseScore today
- Tempo text: `TempoText` (`src/engraving/dom/tempotext.h`) — `setTempo(BeatsPerSecond)`, `setFollowText(bool)` (tempo follows the text), `setXmlText`. Created through palette (`applyPaletteElement`, :2224) or `addTextToItem(TextStyleType::TEMPO, item)` (`notationinteraction.cpp:6204`) which starts a text edit.
- Palette tempo cells carry tempo values for Largo/Adagio/… — **find** them in the palette module's creator (`src/palette/internal/palettecreator.cpp`, tempo palette) and reuse the table for absolute tempo words.
- Gradual tempo changes exist as `GradualTempoChange` spanners (`src/engraving/dom/gradualtempochange.h`; types for rit./rall./accel. — **verify** the enum), applied with `EditSpanner::addSpanner` / `applyPaletteElement`.
- Swing: staff text with swing settings (`StaffText::setSwing`) — **verify**; not used in v1.
- Hidden/non-printing: `EngravingItem::setVisible(false)`; invisible tempo marks still play.
- Gap: metronome range, equations, relative changes (Mosso/Lento) are plain text without defined tempo.

## Design
- Parser: `popover/parsers/tempoparser.{h,cpp}`.
  ```cpp
  struct BeatUnit { int denominatorPower; int dots; };   // quarter = {2, 0}
  struct TempoEntry {
      enum class Kind { Absolute, Gradual, Relative, Equation, Feel, FreeText };
      std::string text;                 // the words, e.g. "Allegro"
      std::optional<BeatUnit> unit;     // metronome unit
      std::optional<double> bpm;        // or bpm range lo
      std::optional<double> bpmHigh;    // range
      bool parenthesised = false;
      bool hidden = false;
      std::string gradualId;            // "rit", "rall", "accel"
      bool continuationLine = false;
  };
  ParseResult<TempoEntry> parseTempo(std::string_view);
  ```
  Steps: strip `( )`→parenthesised, `[ ]`→hidden; find `<unit>=<number>` (regex `([zyxeqhw2]|[2-9])\.*=(\d+(?:\.\d+)?)(?:-(\d+))?` with the digit forms from the beat-unit table); leftover words = text; leftover matches gradual table (`rit.`, `rall.`, `accel.` + `...`) → Gradual; matches swing table → Feel; matches `a=b` units without bpm → Equation; else free text. Free text never errors; empty is Incomplete.
- Handler: `handlers/tempohandler.cpp`.
  - Absolute / free text with metronome: `TempoText`, `setXmlText(text + " <sym>metNoteQuarterUp</sym> = 120")` using the palette-style markup (reuse how palette tempo cells build their text — **verify**), `setTempo(bpm/60 × unitQuarterFactor)`, `setFollowText(true)`.
  - Absolute with no bpm (`Allegro`): bpm from the palette table; unknown words → text-only tempo mark with `followText=false` (no playback change).
  - Hidden: `setVisible(false)`. Parenthesised: wrap text in parentheses.
  - Gradual: `GradualTempoChange` of the mapped type; range → span the range; caret/single → one quarter note; `...` → continuation-line style (**verify** option).
  - Suggestions (`suggest`): prefix filter over a static list of tempo words + gradual terms (+ palette names).
- Placement via `applyPaletteElement`.
- `prefill`: `TempoText` → its plain text + bpm string; gradual → `rit.` etc.
- `handlesItem`: `TempoText`, `GradualTempoChange`.
- Key binding: `Shift+T` → `command://dorico/popover/tempo`.

## Deferred / unsupported
- Metronome ranges `q=96-112` → `Unsupported` (v1; could use midpoint).
- Tempo equations (`e=e.`, `q=e`, `q.=h`) → `Unsupported`.
- Relative changes (`Mosso`, `Lento`) accepted as free text without a tempo change.
- Rhythmic feel / swing entries → `Unsupported` in v1 (Opt-Return unused).
- Syllabification of gradual text.
- Tuplet beat units.

## Acceptance criteria
- [ ] Shift-T, `Allegro`, Return on a note → "Allegro" tempo text at the note with the palette's Allegro tempo; playback changes accordingly.
- [ ] `q=120` → "♩ = 120" tempo text, playback 120 bpm.
- [ ] `q.=80` → dotted-quarter = 80; playback 120 quarter bpm.
- [ ] `Presto q=176` → text "Presto" plus metronome mark.
- [ ] `(q=72)` shows parentheses; `[q=96]` is invisible but still sets the playback tempo.
- [ ] `rit.` on a selected range creates a gradual tempo change over the range; `rit....` adds the line.
- [ ] In note input `rit.` gets a one-quarter default length.
- [ ] `Mosso` creates the plain text; `q=96-112`, `q=e` show "not supported yet".
- [ ] Typing `all` shows suggestions including "Allegro"; Down + Return picks one.
- [ ] Select a tempo mark, Return → pre-filled; change to `q=100`, Return replaces it.
- [ ] Parser tests pass.

## Tests
`src/dorico/tests/tempoparser_tests.cpp`:

| Input | Expected |
|---|---|
| `Allegro` | Ok, Absolute, text "Allegro" |
| `q=120` | Ok, unit quarter, bpm 120 |
| `e=90` | Ok, unit eighth |
| `h.=60` | Ok, unit dotted half |
| `5=100` | Ok, unit eighth (digit form) |
| `Presto q=176` | Ok, text "Presto", unit q, bpm 176 |
| `(q=72)` | Ok, parenthesised |
| `[q=96]` | Ok, hidden |
| `q=96-112` | Unsupported (range) |
| `rit.`, `ritardando` | Ok, Gradual rit |
| `rit....` | Ok, Gradual rit, continuationLine |
| `rall.`, `rallentando`, `rall....` | Ok, Gradual rall |
| `accel.`, `accelerando`, `accel....` | Ok, Gradual accel |
| `Mosso`, `Lento` | Ok, Relative (text) |
| `e=e.`, `q=e`, `q.=h` | Unsupported (Equation) |
| `Light swing 8ths`, `Straight (no swing)`, `2:1 swing 16ths (fixed)` | Unsupported (Feel) |
| `anything goes` | Ok, FreeText |
| `` | Incomplete |
| `q=` | Incomplete |
| `q=0`, `q=-5` | Invalid |
