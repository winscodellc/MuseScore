<!-- markdownlint-disable -->
# 45 — Filters (Edit > Filter: Select Only / Deselect Only)

Status: done · Depends on: [44-selection](44-selection.md), [46-jump-bar](46-jump-bar.md) (discoverability: no default keys), [00-module-skeleton](../00-module-skeleton.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_selecting/write_mode_filters_r.html`,
`B …/write_mode_filters_select_deselect_changing_t.html`,
`B …/write_mode_filter_notes_by_pitch_dialog_r.html`;
forum (item names, unverified): https://forums.steinberg.net/t/keyboard-shortcuts-for-filter/112654
Reference digests: [reference/dorico-editing.md](../reference/dorico-editing.md) §4.7 · [reference/musescore-internals.md](../reference/musescore-internals.md) §5

## Goal
Dorico's one-shot **filters**: with a (large) selection made, choose Edit > Filter > *category* to reduce the selection to only that category (**Select Only**, default) or to everything *except* it (**Deselect Only**).

## Dorico behaviour
- Path: **Edit > Filter > [Item] > [Item type]** (also context menu). A filter exists for (almost) every notation item type: e.g. arpeggio signs, chord symbols, key signatures, playing techniques, note spacing changes.
- **Edit > Filter > Select Only** (default mode): the result = only the filtered type. **Deselect Only**: result = the selection minus the filtered type (this is Dorico's "inverse selection").
- Works on the current selection (typically a large selection from [44](44-selection.md)); mainly used before copy/delete.
- Categories with sub-types:

| Filter | Sub-types |
|---|---|
| Notes | Notes, grace notes, chords; by accidental; by pitch; by **position within chords** (forum: *Notes in Chords* → e.g. "Top Notes Or Single Notes"; the manual does not enumerate top/bottom/nth) |
| Voices | By stem direction; slash voices |
| Dynamics | All / gradual / immediate |
| Tempos | All / absolute / relative / gradual |
| Lyrics | All / by line number / by type / by staff-relative placement |
| Text | All / by paragraph style |

- **Not filterable:** barlines, and things that belong to notes (fingerings, beams, articulations, tremolos).
- **Notes by Pitch > Specific Pitches** → *Filter Notes by Pitch* dialog: Written/Sounding toggle; list of pitch filters (Remove Selected, Clear); note-name menu; accidental menu; "in octave" checkbox + value (off = all octaves); Add button.
- No default key commands for filters (assignable in Key Commands).

## MuseScore today
- Selection **filter panel** (persistent "may be selected" flags): `src/engraving/dom/selectionfilter.h` — `VoicesSelectionFilterTypes` (4), `NotesInChordSelectionFilterTypes` (TOP, 7th…2nd, BOTTOM), `ElementsSelectionFilterTypes` (20: DYNAMIC, HAIRPIN, FINGERING, LYRICS, CHORD_SYMBOL, OTHER_TEXT, ARTICULATION, ORNAMENT, SLUR, TIE, FIGURED_BASS, OTTAVA, PEDAL_LINE, OTHER_LINE, ARPEGGIO, GLISSANDO, FRET_DIAGRAM, BREATH, TREMOLO, GRACE_NOTE), `includeSingleNotes`; `SelectionFilter::canSelect(const EngravingItem*)`, `canSelectNoteIdx`, `canSelectVoice(track)`. UI: `SelectionFilterPanel.qml` + `…/NotationScene/selectionfilter/*model.cpp`; interface `src/notation/inotationselectionfilter.h`.
- `Score::selectSimilar(e, sameStaff)` / `selectSimilarInRange(e)` (`score.h:515-516`) and commands `SELECT_SIMILAR*` (`notationcommands.h:55-57`) select by *type of one item* — not a filter of the current selection.
- Gap: MuseScore's filter is a **persistent gate on future selections**; Dorico's is a **one-shot reduce/invert of the current selection**, with a Select/Deselect mode. They coexist: leave MuseScore's panel untouched.

## Design (as built)
Pure core `src/dorico/internal/filters.{h,cpp}` (no engine types, unit tested) plus glue in `DoricoSelectionController` (`internal/doricoselectioncontroller.cpp`), which owns selection.

- `FilterItem` = what the filter needs to know about one selected item (`FilterKind`, voice, lyric verse, chord group, grace, stem direction, position in chord, written and sounding `FilterPitch`). The controller describes engine items (`filterItemOf`), `applyFilter(items, predicate, deselectOnly)` returns the survivors, and the controller does `clearSelection()` + `select(survivors, ADD)`. A selection change is not an undo step.
- **Parts never stay** (stems, hooks, beams, dots, accidentals, ledger lines) and **attached items follow their note** (articulations, fingerings, tremolos, chord lines): a stem left in a list selection would be deleted from the music by Delete. This also gives the manual's "barlines and things that belong to notes are not filterable".
- `filterDefs()` is the table: one `FilterDef { id, group, title, matches }` per category. Notes: notes, grace notes, chords, top notes, bottom notes, rests. Voices: 1-4, stem up, stem down. Dynamics: all, gradual (hairpins), immediate. Tempo: all, gradual. Lyrics: all, lines 1-4. Single: text, chord symbols, figured bass, slurs, ties, ornaments, arpeggio signs, glissandi, octave lines, pedal lines, other lines, tuplets, holds and pauses, rehearsal marks, playing techniques, clefs.
- Commands (no default keys, found through the jump bar): one `command://dorico/filter/<id>` per row, titled `Filter: <Group> > <Title>`; `command://dorico/filter-select-only` / `filter-deselect-only` (checkable, setting `dorico/filter/mode` = `select` | `deselect`, default `select`); `command://dorico/filter-notes-by-pitch`.
- **Notes by Pitch** opens the QML dialog `FilterNotesByPitchDialog` (`musescore://dorico/filternotesbypitch`): Written / Sounding, the pitch list with Remove selected and Clear, note-name and accidental dropdowns, "In octave" + number, Add, OK / Cancel. The list is kept as tokens (`c`, `c4`, `eb4`, `f#5`, `bb` = B flat, `cx` = C double sharp; no octave = every octave; `parsePitchTokens`) in settings `dorico/filter/pitches` and `dorico/filter/pitchesSounding`, so the dialog reopens with the last list. A pitch matches by spelling: `c4` does not match B#3.
- Menu: Edit > Filter in `doricomenu.cpp` (`filterMenu()`, generated from `filterDefs()`; `MenuSpec.title` on an item gives the menu a short name for a long command title).

Upstream edits: none.

## Deferred / unsupported
- Dorico categories MuseScore has no model for: note spacing changes, playing-technique categories beyond MuseScore text types, paragraph-style text filter (MuseScore has text *styles*; map `TextStyleType` as an approximation), slash voices, absolute/relative tempo distinction (MuseScore only has tempo text).
- Notes-in-chord positions beyond top/bottom/nth-from-top as MuseScore's `NotesInChordSelectionFilterTypes` offers (7th…2nd) — the Dorico list is not in the manual.
- Staff-relative lyric placement filter; lyric line filter ships first (verse number).
- Persistent filter gating (keep MuseScore's panel instead).

## Acceptance criteria
- [x] Select a bar range; Filter > Dynamics (Select Only) → only dynamics remain selected; Deselect Only → everything but dynamics.
- [x] Filter > Dynamics > Gradual selects hairpins/cresc. text only; Immediate selects p/mf/etc. only.
- [x] Filter > Voices > Voice 2 keeps only voice-2 items (notes and rests in track%4==1).
- [x] Filter Notes > Chords keeps only multi-note chords; Notes > Grace notes keeps grace notes.
- [x] Notes by Pitch `c4` selects only C4 (same name+accidental only, unit tests); `c` (no octave) selects C in every octave; Sounding mode honours transposing instruments (UI: Dawn, sounding E4 on clarinet and horn).
- [x] Mode persists across restart; default is Select Only.
- [x] MuseScore's selection filter panel behaves exactly as before; filters do not alter its flags. Checked via the jump bar with Filter: Dynamics only: the panel was identical before and after, and unticking Dynamics in the panel still excluded dynamics from a fresh range selection.
- [x] Barlines/beams are not offered as categories (unit test on the table).

UI run (Dynamic_Strings, Dawn demos, filters run from the jump bar): also checked that a filter is not an undo step, that Top Notes + Delete turns only those notes into rests, and the dialog layout.

## Tests
- `src/dorico/tests/filters_tests.cpp` (pure): the table's ids, Select Only / Deselect Only complement, parts dropped, attached items following their note, voices, chords / top / bottom / grace, lyric lines, `parsePitchTokens` (valid and invalid), `matchesPitch` (octave, spelling). `dorico_tests` cannot load scores, so there is no `.mscx` fixture; engine glue is checked in the UI.
- Manual: run each filter from the jump bar (the macOS menu bar is out of reach of the UI driver); open the Notes by Pitch dialog; check MuseScore's selection filter panel is unchanged.
