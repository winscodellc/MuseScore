<!-- markdownlint-disable -->
# 44 — Selection: extend, Select More, deselect, Tab cycling

Status: done · Depends on: [10-keymap-preset](10-keymap-preset.md), [12-caret-and-navigation](12-caret-and-navigation.md) (arrow-key semantics are owned by 12; this spec owns selection *extension*, *Select More*, *deselect*, *Tab*)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_selecting/write_mode_notes_notation_selecting_deselecting_t.html`,
`B …/write_mode_selections_types_r.html`, `B …/write_mode_selecting_more_t.html`, `B …/write_mode_selections_extending_t.html`,
`B …/write_mode_selecting_marquee_t.html`, `B …/write_mode_system_track_bars_selecting_t.html`,
`B write_mode/write_mode_navigation/write_mode_navigation_items_music_area_t.html`
Reference digests: [reference/dorico-editing.md](../reference/dorico-editing.md) §4 · [reference/musescore-internals.md](../reference/musescore-internals.md) §5

## Goal
Selection gestures and keys follow Dorico: extend by item / to bar end-start, Select All = whole flow, Select More in 3 stages, Deselect All, Tab cycling through items at a position.

## Dorico behaviour

| Key / gesture | Result |
|---|---|
| Click / `Cmd`-click / `Shift`-click | Select / toggle-add / range |
| Click beam → all notes in beam group; click stem → whole chord; click empty staff space in a bar → everything in that bar on that staff | |
| `Shift-Opt`-click | Select an item hidden behind another |
| `Shift`-double-click a note | That note and **all later notes of the same pitch in the voice** |
| Marquee (status-bar tool) | Items entirely inside; notes/tie chains if any part inside |
| `Right`/`Left` | Next/prev item in same voice (owner: spec 12) |
| `Shift-Right`/`Shift-Left` | Extend selection by one item (during note input: select notes before/after last input note without leaving input) |
| `Cmd-Shift-Right`/`Left` | Extend to the **end/start of the bar** |
| `Shift-Up`/`Down` with a whole staff selected | Add staff above/below |
| `Cmd-A` | Select All = everything in the whole flow |
| `Cmd-Shift-A` | **Select More** (below) |
| `Cmd-D`, Edit > Select None, click outside staves | Deselect all |
| `Tab`/`Shift-Tab` | Cycle through items at the same rhythmic position (not system-attached items) |
| Edit > Select To End/Start Of System / Flow | **no default keys** |
| Selected items shown orange | |

**Select More** (expands horizontally over the **same item types**, in the **same voices and staves**):
1. **First press** — one item of a type: expand to its **bar boundaries** (skip to press 2 if nothing else in the bar); two or more items of a type: select everything of that type between the earliest and latest (skip to press 2 if nothing in between).
2. **Second press** — system boundaries (page view) or whole flow (continuous/galley view).
3. **Third press** — whole flow (page view).
Items spanning bars/systems count in the earliest bar/system. Single-item refinements: lyrics → same line number, placement, type; dynamics → same group on same staves first, then others; text → same paragraph style; playing techniques → same category (up/down bow only to each other).

## MuseScore today
- Keys in base `shortcuts.json`: `Shift+Left/Right` = `add-to-selection-prev/next-chord` (`:147-148`); `Ctrl+Shift+Left/Right` = `…-prev/next-measure` (extends by a whole measure, not to bar edge; `:149-150`); `Shift+Home/End` begin/end-system, `Ctrl+Shift+Home/End` begin/end-score (`:151-154`); `Shift+Up/Down` above/below staff (`:155-156`); `Ctrl+A` select-all (`:208`); `Alt+Right/Left` goto-next/prev-element (`:133-134`); `Esc` cancel.
- Handlers: `expandSelection(ExpandSelectionMode)` `notationinteraction.cpp:3773` (registered `notationactioncontroller.cpp:151-160`); `Score::cmdSelectAll` / `cmdSelectSection` (`score.h:758-759`); `Score::selectSimilar` / `selectSimilarInRange` (`score.h:515-516`); `SelectionTarget` handling `src/notation/internal/notationselection.cpp:190-292`; commands `SELECT_SIMILAR*`, `SELECT_ALL_COMMAND` (`notationcommands.h:55-59`).
- Conflicts with Dorico keys to unbind (spec 10 owns the edits): `Ctrl+D` = `add-dynamic` (`shortcuts.json:165`, Dorico: Deselect), `Ctrl+Shift+A` = `insert-note-a` (`:105`, Dorico: Select More), `Alt+Left/Right` goto-prev/next-element (Dorico: Opt-Right/Left = move rhythmically, spec 13; Tab replaces it here).
- MuseScore selects a measure range on empty-staff click and supports Ctrl-click add, Shift-click range, rubber-band (Shift-drag) already.

## Design
`src/dorico/internal/selectmore.{h,cpp}` + small command handlers in `src/dorico/internal/selectioncmds.cpp`.

| Command | Title | Key (mac) | Behaviour |
|---|---|---|---|
| `command://notation/add-to-selection-prev/next-chord` | (existing) | `Shift+Left/Right` | keep |
| `command://dorico/extend-to-bar-start` / `-end` | Extend Selection to Bar Start/End | `Cmd+Shift+Left` / `Right` | range: set range start/end to `measure->tick()` / `measure->endTick()` of the first/last selected item's measure (via `INotationSelection::setRange`/`Score::select(..., SelectType::RANGE)` — verify API); list selection: turns into a range from the anchor. |
| `command://notation/select-all` | (existing) | `Cmd+A` | keep (single-flow score = whole flow) |
| `command://dorico/select-more` | Select More | `Cmd+Shift+A` | staged expansion below |
| `command://dorico/deselect-all` | Select None | `Cmd+D` | `INotationInteraction::clearSelection()` (`notationinteraction.cpp:1012`) |
| `command://notation/goto-next-element` / `goto-prev-element` | (existing) | `Tab` / `Shift+Tab` | rebind from `Alt+Right/Left` |
| `command://notation/add-to-selection-begin/end-system`, `begin/end-score` | Select To Start/End of System/Flow | none (Dorico has none) | keep commands, drop MuseScore keys that conflict, expose via jump bar [46](46-jump-bar.md) |
| `command://dorico/select-same-pitch-onwards` | Select Same Pitch Onwards | – | from the selected note: this and all later notes with equal `epitch()` in the same track (`score->select(n, SelectType::ADD)` loop). Mouse binding (`Shift`+double-click) deferred. |

**Select More algorithm** (pure, testable): 
```cpp
struct SelectMoreState { SelectionSignature last; int stage; };   // reset when selection != last result
Fraction/Range computeRange(const Score*, const std::vector<EngravingItem*>& sel, int stage, ViewMode);
std::vector<EngravingItem*> collect(const Score*, const std::set<std::pair<ElementType,track_idx_t>>& kinds, Range);
```
Stage 1 → bar boundaries or [earliest,latest] per above; stage 2 → system (`measure->system()` first/last measure) in page view, whole score in continuous; stage 3 → whole score. Collect with `score->scanElementsInRange(...)` filtering on `(type(), track())`; apply with `clearSelection` then `select(..., SelectType::ADD)` per item. Skip-stage rule: if `collect(range_n)` equals the current selection, advance to the next stage immediately. Lyrics refinement (same verse/placement) is in scope for stage 1; the other single-item refinements are Deferred.

Persist `SelectMoreState` in the module's per-window context object (spec 00 `IContextSetup`).

Upstream edits: none (keymap via spec 10). `Shift`-double-click and beam/stem clicks: no edit.

## As built
- `internal/selectmore.{h,cpp}`: engine-free `selectMore(all, selected, stage, ctx)` (ticks are ints, bar / system ranges come from callbacks); `tests/selectmore_tests.cpp` covers it. The planned `.mscx` fixtures were not used: `dorico_tests` cannot load scores (spec 40).
- `internal/doricoselectioncontroller.{h,cpp}` (per window, wired like `DoricoPasteController`) has all the handlers. Select More keeps the last result's items; a press carries on from there only while the selection still holds exactly those items, so any other selection change starts again at the first press (no `selectionChanged` subscription).
- Items expand by `(type, staff, voice)`, plus verse and placement for lyrics (the lyrics rule) and grace notes apart from main notes. A spanner counts once, by its first segment, at its start tick. Galley = `Score::linearMode()`.
- `Cmd-Shift-Right/Left`: range from the active chord/rest to the last/first chord/rest of its bar in the same voice; already there goes to the next/previous bar's. `add-to-selection-next/prev-measure` are unbound (menu-only).
- **Tab does not use `goto-next-element`** (it walks on through the whole score). `dorico/next-item-at-position` / `prev-item-at-position` cycle, at the selected item's tick and staff: chord or rest of each voice (a chord is one stop), articulations, lyrics, staff annotations, spanners that start there; system-attached items are skipped; it wraps around. Not in note input.
- Edit menu: Select None, Select More, Select To > End/Start Of System, End/Start Of Flow (existing `add-to-selection-*` commands, no keys).
- `dorico/select-same-pitch-onwards` has a command and no key or menu item (the `Shift`-double-click gesture is deferred).
- `fingeringhandler.cpp` got the includes it was missing (`score.h`, `popovernavigation.h`): adding files shifted the unity batches and exposed it.

## Deferred / unsupported
- Marquee tool as a status-bar mode (MuseScore keeps Shift-drag rubber band).
- Click-beam → beam group, click-stem → chord, `Shift-Opt`-click hidden item, `Shift`-double-click gesture (command provided, gesture later).
- System track selection with signposts (no signposts in MuseScore).
- Select More refinements for dynamics groups, text paragraph styles, playing-technique categories; Select More in "Engrave mode".
- Preference "prioritize noteheads over stems/beams".
- Selection changes in the undo/History list (Dorico records selections; MuseScore does not).

## Acceptance criteria
- [x] `Cmd+A` selects the whole score; `Cmd+D` clears; `Esc` still cancels as upstream.
- [x] `Cmd+Shift+Right` extends a note selection to the end of its bar; `Cmd+Shift+Left` to its start.
- [x] `Shift+Right/Left` extend by one chord/rest.
- [x] Select More: one dynamic in bar 3 with other dynamics in bar 3 → bar's dynamics; press again → system; again → score. If bar 3 has only that dynamic, first press goes straight to stage 2. (UI for the stages; the alone-in-bar skip by unit test only.)
- [x] Select More with two dynamics selected selects all dynamics between them; notes only in selected voice/staff.
- [x] Changing the selection by any other means resets the stage counter.
- [x] `Tab` moves to another item at the same segment; `Shift+Tab` back; `Alt+Right/Left` no longer bound to it.
- [x] `dorico/select-same-pitch-onwards` on a C4 selects every later C4 in that voice only. Checked via the jump bar on the first system of a two-voice score (voice-2 C4s and other pitches stayed unselected); later systems not inspected.

## Tests
- `src/dorico/tests/selectmore_tests.cpp`: pure `computeRange`/`collect` over fixture scores `selectmore_data/*.mscx` (dynamics in 3 bars across 2 systems): assert the set of selected items per stage; same scaffolding as `src/engraving/tests/selectionrange_tests.cpp`.
- `bar-extend` test: `score->select` range, call the helper, compare `selection().tickStart()/tickEnd()`.
- Manual: key walkthrough in a 2-staff, 2-system score; confirm orange selection and stage progression.
