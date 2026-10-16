<!-- markdownlint-disable -->
# 40 — Copy / Cut / Paste (Dorico semantics)

Status: todo · Depends on: [00-module-skeleton](00-module-skeleton.md), [10-keymap-preset](10-keymap-preset.md), [11-note-input-modes](done/11-note-input-modes.md) (Chord/Insert flags)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_arranging_tools/write_mode_copying_pasting_items_t.html`,
`B …/write_mode_copying_pasting_multiple_staves_t.html`, `B …/write_mode_copying_pasting_across_selection_t.html`,
`B write_mode/write_mode_note_input/write_mode_chord_mode_c.html`, `B write_mode/write_mode_insert_mode/write_mode_insert_mode_c.html`
Reference digests: [reference/dorico-editing.md](reference/dorico-editing.md) §1.1, 1.3, 1.4, 1.7, 1.8, 9 · [reference/musescore-internals.md](reference/musescore-internals.md) §4

## Goal
`Cmd-C` / `Cmd-X` / `Cmd-V` follow Dorico's destination rules: where the paste lands, how many copies are made, and what happens to music already there (overwrite / Chord mode merge / Insert mode push).

## Dorico behaviour

| Situation | Dorico result |
|---|---|
| One item selected | That item is the **anchor**; clipboard starts there (voice/staff of the item). |
| Several staves, one target item selected **on each staff** | One copy is pasted to **every** selected staff. *"The earliest selected item on the top selected staff sets the rhythmic position for the pasted items on all staves."* |
| A **range** selected (one or more staves) | Clipboard is pasted repeatedly to fill the range, **only whole copies, never extending past the range end**. |
| Fill-range constraint | Only clipboard content that has a duration that can fill a selection (notes, gradual dynamics) repeats; an immediate dynamic does not. |
| Default | Pasted notes **overwrite** existing notes in the target voice (material in *other* voices is left alone). |
| Chord mode (Q) on | Pasted notes are **merged into chords** with existing notes (4 copied notes onto 4 existing notes → 4 chords). |
| Insert mode (I) on | Pasted material **pushes later music** along within the Insert scope. In the Voice scope outside note input the scope = all selected voices. |
| Chord + Insert | Mutually exclusive (state owned by spec 11). |
| During note input (caret active) | Paste at the caret (caret voice/staves). |
| Cut (`Cmd-X`) | Copy + delete (delete rules in [43](43-delete.md): Insert mode closes the gap, otherwise rests). |
| Preference "Link dynamics and slurs to existing items when pasting" (default on) | Dynamics/slurs pasted onto other staves at the same rhythmic position are linked. |
| MIDI CC pasted with notes | Not applicable (no Key Editor). |

Not stated by the manual (do **not** invent more than this): behaviour when N staves are copied and a different number of staves is selected with plain Paste. Rule adopted here: staves map top-to-bottom from the top selected staff; surplus clipboard staves are dropped only if no staff exists for them, surplus selected staves get nothing. Explode/Reduce ([47](47-explode-reduce.md)) are the documented tools for changing staff count.

## MuseScore today
- Commands: `COPY_COMMAND`, `PASTE_COMMAND`, `PASTE_SPECIAL_COMMAND` (`src/notationscene/notationcommands.h:339-346`); registered `notationactioncontroller.cpp:424-443`.
- `copySelection` `src/notation/internal/notationinteraction.cpp:5090` → `selection()->qMimeData()`; `cutSelection` controller `:2255`.
- `pasteSelection(PastingType)` controller `:2276`, `resolvePastingScale` `:2290`; `NotationInteraction::pasteSelection(scale)` `:5228` → `Paste::paste` `src/engraving/editing/paste.cpp:547`; helpers `pasteStaff` `:78`, `pasteSymbol` `:617`, `pasteStaffList` `:686`, `pasteSymbolList` `:720`.
- Clipboard MIME: `application/musescore/{symbol,symbollist,stafflist}` (`src/engraving/dom/mscore.h:77-79`) — **keep**; Dorico semantics sit on top of the same clipboard so copy/cut stay upstream commands.
- MuseScore pastes once at the selected element / range start, on the staff of the selection; there is no "one copy per selected staff" and no "fill the range" (`paste-half/double/special` change scale only).

## Design
New files (all under `src/dorico/`, SPDX header like neighbours):
- `internal/pasteplanner.{h,cpp}` — **pure** function on the engraving DOM, no UI:
  ```cpp
  struct PasteTarget { staff_idx_t staff; track_idx_t track; Fraction tick; };   // one Paste::paste call each
  std::vector<PasteTarget> planPaste(const Score*, const Selection&, Fraction clipboardLen, staff_idx_t clipboardStaves);
  ```
  Rules: (a) list selection → one target per distinct staff, tick = earliest selected item on the **top** selected staff (tick shared by all staves); (b) range selection → targets at `start, start+len, start+2·len …` while `t+len <= rangeEnd` per staff; (c) single anchor → one target. Clipboard length/staff count come from the stafflist payload (`<StaffList … len=… staves=…>` — **verify the attribute names in `Selection::mimeData` / `paste.cpp`** before coding).
- `internal/dorico_paste.{h,cpp}` — handler for `command://dorico/paste`: reads the system clipboard, calls `planPaste`, then for each target selects the anchor chord-rest in the right track (`Score::select`) and calls `Paste::paste` inside **one** undo step (`INotationUndoStack::prepareChanges/commitChanges` — verify names; or `INotationInteraction::startEdit()/apply()` per the CLAUDE.md note). Chord/Insert modes (from spec 11 state) map to: Chord → do **not** clear the target range first (merge); Insert → use the timewise path (`Score::cmdPaste` with insert, see TIMEWISE note-entry method); default → current overwrite.
- Cut: `command://dorico/cut` = existing `notation/cut`, but routed so the delete step honours Insert mode (calls the delete planner of spec 43). If Insert mode is off it is literally `notation/cut`.

| Command | Title | Default key (mac) | Handler |
|---|---|---|---|
| `command://dorico/paste` | Paste | `Cmd+V` | `DoricoPaste::paste` |
| `command://dorico/cut` | Cut | `Cmd+X` | `DoricoPaste::cut` |
| `notation/copy` (unchanged) | Copy | `Cmd+C` | upstream |

Upstream edits: **none** (key map rebinding in `shortcuts_dorico.json`: unbind `command://notation/paste` / `cut`, bind the dorico ones; `paste-half/double/special` keep their MuseScore keys if free). Menus still show `notation/paste`; menu → dorico routing (if wanted) uses the `preDispatch` veto on `command://notation/paste` as a fallback only.

## Deferred / unsupported
- Linked dynamics/slurs across staves (setting + behaviour) — MuseScore has no linked-item model for these.
- MIDI CC copying/pasting; Key Editor.
- Chord symbol region copy → showing chord symbols on target players.
- Pasting a different number of staves than selected beyond the simple rule above.
- Dorico's behaviour for pasting into a **different voice's** existing material beyond "leave it" (covered by [41](41-paste-special.md) Paste Into Voice).

## Acceptance criteria
- [ ] `Cmd+V` with one note selected pastes at that note (same result as upstream for the single-staff case; existing copypaste01-08 fixtures still pass through the planner).
- [ ] 2-staff score, copy bar 1 of staff 1, select a note in bar 3 on staff 1 **and** a note in bar 3 on staff 2, `Cmd+V` → both staves receive the bar at bar 3.
- [ ] Selected targets at different ticks on two staves: both pasted at the **top staff's** earliest tick.
- [ ] Copy 1 bar (4/4), select bars 3–6 on one staff, paste → 4 copies; select 3 bars after copying 2 bars → 1 copy (no partial third), bar 5 untouched.
- [ ] Immediate dynamic copied, range selected → pasted once, not repeated.
- [ ] Default mode overwrites target-voice notes; notes in another voice at that tick are unchanged.
- [ ] Chord mode on: paste of 4 single notes over 4 existing notes → 4 chords.
- [ ] Insert mode on: later music is pushed right by the pasted length (measure count grows per MuseScore TIMEWISE rules).
- [ ] Whole paste (incl. multi-target) is one undo step.
- [ ] Cut in Insert mode closes the gap; otherwise leaves rests.

## Tests
- `src/dorico/tests/pasteplanner_tests.cpp` (GoogleTest, model after `src/engraving/tests/copypaste_tests.cpp`: `ScoreRW::readScore`, build the target list, compare). Cases: single anchor, multi-staff list, range fill (exact fit / remainder), immediate-dynamic no-repeat. Pure planner tests need no clipboard.
- `src/dorico/tests/dorico_paste_tests.cpp` + fixtures `src/dorico/tests/paste_data/*.mscx` and `*-ref.mscx` (generate by performing the action once, review in MuseScore, commit) using `Paste::paste` + `ScoreComp::saveCompareScore` exactly as `copypaste_tests.cpp:60-98`. If linking the engraving test utils (`utils/scorerw.h`, `scorecomp.h`) from `src/dorico/tests` is impractical, put the cases in a new `src/engraving/tests/dorico_paste_tests.cpp` (one extra line in that CMakeLists — a permitted test-only upstream edit).
- Manual: score with 2 staves, run the three bullets with mouse+keyboard; verify undo is one step.
