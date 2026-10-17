<!-- markdownlint-disable -->
# 26 — Tempo popover (Shift-T)
Status: done · Depends on: 20 · Dorico refs: [tempo popover reference](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_tempo_r.html) (page names `dorico_popovers_tempo_*_r.html`), [beat units](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_beat_units_r.html)

Reference: `../reference/dorico-popovers.md` (Tempo, Beat units), `../reference/musescore-internals.md` (§3).

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
As built. Parser `popover/parsers/tempoparser.{h,cpp}` (`parseTempo` -> `TempoEntry { Kind { Absolute, Gradual, Relative, ATempo, TempoPrimo, FreeText }, text, unit (BeatUnit { DurationType, dots }), bpm, wordBpm, parenthesised, hidden, markHidden, gradual, line }`, `tempoSuggestions`, `beatUnitText`), handler `popover/handlers/tempohandler.{h,cpp}` (`tempoPopover()`), registered in `popoverregistry.cpp`. No upstream edits.
- Parser: a whole entry in `[ ]` is `hidden` (the item is not printed but plays). `<unit>=` anywhere starts a metronome mark: units are the beat-unit letters and digits (`2w` before `2`), dots after. `(q=72)` sets `parenthesised`, `Allegro [q=96]` sets `markHidden` (the words print, the mark only plays). The right side is a speed (> 0); `96-112` (range) and a unit (`q=e`, equation) are Unsupported, `q=` and `96-` Incomplete. Leftover words: the gradual table (`rit`, `rit.`, `ritardando`, rall / accel likewise, `...` suffix = continuation line) only without a mark; `a tempo`, `tempo primo`; relative (`Mosso`, `Lento`, `più mosso`, `meno mosso`); the Tempo palette's words with their quarter speeds (`wordBpm`, Allegro 144; copied from `palettecreator.cpp`, the palette module is not linked); else free text, which never errors. Rhythmic feels are Unsupported. Words are kept as typed; matching is ASCII case-insensitive.
- Tempo marks: `TempoText` on track 0 (MuseScore copies system items to the other system staves) at the caret / range start / earliest selected item. Any tempo mark already at that position is replaced (one per position). Text = words + `<sym>metNote…</sym>[<sym>space</sym><sym>metAugmentationDot</sym>…] = N` (the palette's markup, built here so every unit and dot count works). Playback: a mark sets `bpm × quarters(unit) / 60` with `followText` on (editing the printed mark retunes it, as for palette marks); palette words set `wordBpm`; `a tempo` / `tempo primo` use MuseScore's `setATempo` / `setTempoPrimo`; anything else gets `setPlayTempoText(false)` (printed only). Hidden = `setVisible(false)`.
- Gradual: `GradualTempoChange` of the mapped type on track 0, begin text as typed (`rit.`, `ritardando`), continue text in parentheses, `setLineVisible(line)`. Caret: one quarter note from the caret. Range: the range. Single item: that chord/rest's length. A selected gradual change is removed and its span reused.
- After applying outside note input the new item is selected, so Return reopens it.
- `prefill`: words + `q=N` read back through `TempoText::findTempoDuration` and the tempo (`(q=N)` when the text has the parenthesis, `[...]` when invisible, `Allegro [q=96]` when the playing speed is not the words' own); gradual -> begin text + `...` when the line shows.
- `handlesItem`: `TempoText`, `GradualTempoChangeSegment`. Key binding `Shift+T` -> `command://dorico/popover/tempo` (in `shortcuts_dorico.json` since spec 20).
- **Framework part (from spec 20):** `PopoverDef::suggest` (optional). `PopoverModel` has `suggestions`, `currentSuggestion` (-1 = none) and `moveSuggestion(int)`; `Popover.qml` lists the suggestions under the box. Down / Up move the highlight, Return (or a click) applies the highlighted entry, typing clears the highlight. The entry typed exactly is not suggested. Spec 29 reuses it.

## Deferred / unsupported
- Metronome ranges `q=96-112` → `Unsupported` (v1; could use midpoint).
- Tempo equations (`e=e.`, `q=e`, `q.=h`) → `Unsupported`.
- Relative changes (`Mosso`, `Lento`) accepted as free text without a tempo change.
- Rhythmic feel / swing entries → `Unsupported` in v1 (Opt-Return unused). MuseScore's swing is staff text (`StaffTextBase::setSwing`); not wired.
- `più`, `meno`, `Rubato` print without a tempo change. Words outside the palette table never change playback.
- Lento: Dorico calls it a relative change, so it prints without a tempo change (MuseScore's palette gives it 52.5).
- Entering a gradual change with a tempo mark selected adds it beside the mark instead of replacing it.
- Syllabification of gradual text.
- Tuplet beat units.

## Acceptance criteria
- [x] Shift-T, `Allegro`, Return on a note → "Allegro" tempo text at the note with the palette's Allegro tempo; playback changes accordingly.
- [x] `q=120` → "♩ = 120" tempo text, playback 120 bpm.
- [x] `q.=80` → dotted-quarter = 80; playback 120 quarter bpm.
- [x] `Presto q=176` → text "Presto" plus metronome mark.
- [x] `(q=72)` shows parentheses; `[q=96]` is invisible but still sets the playback tempo.
- [x] `rit.` on a selected range creates a gradual tempo change over the range; `rit....` adds the line.
- [x] In note input `rit.` gets a one-quarter default length.
- [x] `Mosso` creates the plain text; `q=96-112`, `q=e` show "not supported yet".
- [x] Typing `all` shows suggestions including "Allegro"; Down + Return picks one.
- [x] Select a tempo mark, Return → pre-filled; change to `q=100`, Return replaces it.
- [x] Parser tests pass.

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

Built and unit-tested (`dorico_tests` 93/93, 8 new `DoricoTempoParser` tests).

Manual test (2026-10-17, our build driven with ClaudeUIControl, temp copy of `demos/Amazing_grace.mscz`, transport tempo display read after each step): every acceptance box above passed. `Allegro` picked from the menu (`all`, Down, Down, Return) gave "Allegro" at 144; Return on it prefilled `Allegro`; `q=100`, `q.=80` (transport 120), `Presto q=176`, `(q=72)` and `[q=96]` (shown faded, plays 96, prefills `[q=96]`) each replaced the selected mark; `Mosso` left the default 120; `q=96-112` and `q=e` showed "Not supported yet". `rit.` over bars 3-4 spans the range with its line hidden (grey only because Show Invisible is on); `rall....` over bar 5 draws the line (tested `rall....` rather than `rit....`; same path). In note input `accel.` at a dotted-quarter G spans exactly one quarter and prefills `accel.`. Each Return was one undo step. Not checked: hidden-item playback by ear (read from the transport only).
