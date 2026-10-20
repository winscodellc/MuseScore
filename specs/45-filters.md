<!-- markdownlint-disable -->
# 45 — Filters (Edit > Filter: Select Only / Deselect Only)

Status: todo · Depends on: [44-selection](44-selection.md), [46-jump-bar](46-jump-bar.md) (discoverability: no default keys), [00-module-skeleton](00-module-skeleton.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_selecting/write_mode_filters_r.html`,
`B …/write_mode_filters_select_deselect_changing_t.html`,
`B …/write_mode_filter_notes_by_pitch_dialog_r.html`;
forum (item names, unverified): https://forums.steinberg.net/t/keyboard-shortcuts-for-filter/112654
Reference digests: [reference/dorico-editing.md](reference/dorico-editing.md) §4.7 · [reference/musescore-internals.md](reference/musescore-internals.md) §5

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

## Design
`src/dorico/internal/filters.{h,cpp}` — a **pure** predicate table + one applier.

```cpp
enum class FilterCategory { Notes, GraceNotes, Chords, Rests, Voice1..4, Dynamics, DynamicsGradual, DynamicsImmediate,
    Hairpins, Tempos, Lyrics, Text, Slurs, Ties, Ornaments, Arpeggios, Glissandi, ChordSymbols, KeySignatures,
    TimeSignatures, Clefs, RehearsalMarks, Ottavas, PedalLines, OtherLines, Tuplets, Fermatas, BreathMarks,
    Accidental(arg), Pitch(arg list), NoteInChord(top|bottom|nth), ... };
bool matches(FilterCategory, const FilterArg&, const EngravingItem*);              // pure, ElementType/subtype/track-based
std::vector<EngravingItem*> applyFilter(const std::vector<EngravingItem*>& sel, FilterCategory, const FilterArg&, FilterMode);
```
Apply: take `INotationSelection::elements()` (for a range selection the contained items), `applyFilter`, then `clearSelection()` + `Score::select(el, SelectType::ADD)` per survivor, one selection change (no undo step — selections are not undoable in MuseScore).
Dropped per the manual: barlines, beams, fingerings, articulations, tremolos have no category.

| Command | Title | Key | Behaviour |
|---|---|---|---|
| `command://dorico/filter` (arg: category [+ arg]) | Filter | none | `applyFilter` with the current mode |
| `command://dorico/filter-mode-select-only` / `-deselect-only` | Select Only / Deselect Only | none | sets mode; persisted in setting `dorico/filter/mode` ("select"\|"deselect", default select) |
| `command://dorico/filter-notes-by-pitch` (arg: e.g. `c4,eb,f#5`) | Notes by Pitch | none | pitch tokens: letter + accidental + optional octave (no octave = all octaves); Written/Sounding toggle → `Note::epitch()` vs `ppitch()`+transpose; dialog UI is a second step (QML dialog in the `MuseScore.Dorico` module) |

Each category is registered as its own `CommandInfo` titled `Filter: Dynamics > Gradual` etc. so the Jump bar can find it and the optional Edit > Filter submenu can be generated from the same table. Menu attach point is the optional appshell patch described in [41](done/41-paste-special.md) (the Edit menu is built by the dorico module, spec 59); jump bar is the required path.

Upstream edits: none (menu patch optional).

## Deferred / unsupported
- Dorico categories MuseScore has no model for: note spacing changes, playing-technique categories beyond MuseScore text types, paragraph-style text filter (MuseScore has text *styles*; map `TextStyleType` as an approximation), slash voices, absolute/relative tempo distinction (MuseScore only has tempo text).
- Notes-in-chord positions beyond top/bottom/nth-from-top as MuseScore's `NotesInChordSelectionFilterTypes` offers (7th…2nd) — the Dorico list is not in the manual.
- Staff-relative lyric placement filter; lyric line filter ships first (verse number).
- Persistent filter gating (keep MuseScore's panel instead).

## Acceptance criteria
- [ ] Select a bar range; Filter > Dynamics (Select Only) → only dynamics remain selected; Deselect Only → everything but dynamics.
- [ ] Filter > Dynamics > Gradual selects hairpins/cresc. text only; Immediate selects p/mf/etc. only.
- [ ] Filter > Voices > Voice 2 keeps only voice-2 items (notes and rests in track%4==1).
- [ ] Filter Notes > Chords keeps only multi-note chords; Notes > Grace notes keeps grace notes.
- [ ] Notes by Pitch `c4` selects only C4 (all enharmonic spellings? no — same name+accidental only); `c` (no octave) selects C in every octave; Sounding mode honours transposing instruments.
- [ ] Mode persists across restart; default is Select Only.
- [ ] MuseScore's selection filter panel behaves exactly as before; filters do not alter its flags.
- [ ] Barlines/beams are not offered as categories.

## Tests
- `src/dorico/tests/filters_tests.cpp`: `matches`/`applyFilter` against a fixture `filters_data/mixed.mscx` (notes, chords, grace notes, 2 voices, p, cresc hairpin, slur, lyrics, chord symbols, ottava) — build the element list with `score->selection().elements()` after `score->select(firstMeasure, …RANGE…)`, assert element counts per category and the Deselect Only complement. Model fixtures/infra: `src/engraving/tests/selectionfilter_tests.cpp` + `selectionfilter_data/`.
- Pitch-token parser unit tests (pure): `c`, `c4`, `eb4`, `f#5`, `c4,e` and invalid tokens.
- Manual: run from the jump bar; check the panel is unchanged.
