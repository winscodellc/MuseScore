<!-- markdownlint-disable -->
# 40 — Copy / Cut / Paste (Dorico semantics)

Status: done · Depends on: [00-module-skeleton](00-module-skeleton.md), [10-keymap-preset](10-keymap-preset.md), [11-note-input-modes](11-note-input-modes.md) (Chord/Insert flags)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_arranging_tools/write_mode_copying_pasting_items_t.html`,
`B …/write_mode_copying_pasting_multiple_staves_t.html`, `B …/write_mode_copying_pasting_across_selection_t.html`,
`B write_mode/write_mode_note_input/write_mode_chord_mode_c.html`, `B write_mode/write_mode_insert_mode/write_mode_insert_mode_c.html`
Reference digests: [reference/dorico-editing.md](../reference/dorico-editing.md) §1.1, 1.3, 1.4, 1.7, 1.8, 9 · [reference/musescore-internals.md](../reference/musescore-internals.md) §4

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

## Design (as built)
Paste and cut are **intercepted**, not replaced: `DoricoPasteController` (`src/dorico/internal/doricopastecontroller.{h,cpp}`) listens to `ICommandDispatcher::preDispatch()` for `command://notation/paste` and `command://notation/cut` (same trick as `PopoverController` for `screen-edit-text`). Cmd-V, Cmd-X, the Edit menu and the context menu all get the Dorico rules; no new command ids, no keymap change, no upstream edit. Copy stays upstream.
- `internal/pasteplanner.{h,cpp}` — **pure** (`PasteTarget { staff, tick }`, `planListPaste`, `planRangePaste`), unit-tested in `tests/pasteplanner_tests.cpp`.
  - List selection: one target per selected staff, all at the earliest tick on the top selected staff. A clipboard of several staves is pasted once from the top staff down.
  - Range selection: the clipboard repeats while a whole copy fits (always at least one copy), on every selected staff. Insert mode pastes once.
- Paste (`tryPaste`): reads the `application/musescore/stafflist` payload (`len`, `staves` attributes), plans the targets, and runs `Paste::pasteStaff` per target inside **one** `INotationUndoStack::prepareChanges/commitChanges`. MuseScore's own handler still runs (`allowDispatch` stays true) when the clipboard is not a staff list (symbols such as an immediate dynamic paste once) or when the plan is a single target outside note input with neither mode on.
  - Default mode: `pasteStaff` overwrites only the clipboard's voices, other voices are untouched.
  - Chord mode: the notes already in the pasted span are snapshotted and re-added (`NoteInput::addNote`) to the chord that now starts at the same tick.
  - Insert mode: per staff, the tail (from the target to the end of the last chord's bar) is copied, the span deleted, bars appended if needed, and the tail pasted `len` later (`pasteShiftStaffTail`). Barlines stay; the music is re-split across them.
  - Note input on: one target at the caret, and the caret moves to the end of the pasted music.
  - After a paste outside note input the pasted region (all staves, first to last tick) is selected.
- Cut (`tryCut`): only in Insert mode with a range selection: copy, then `pasteShiftStaffTail(staff, rangeEnd, rangeStart)` per staff so the music after the cut closes the gap. Otherwise MuseScore's cut runs (rests are left).

## Deferred / unsupported
- Linked dynamics/slurs across staves (setting + behaviour) — MuseScore has no linked-item model for these.
- MIDI CC copying/pasting; Key Editor.
- Chord symbol region copy → showing chord symbols on target players.
- Pasting a different number of staves than selected beyond the simple rule above.
- Dorico's behaviour for pasting into a **different voice's** existing material beyond "leave it" (covered by [41](41-paste-special.md) Paste Into Voice).
- A copied hairpin or other symbol is pasted once (symbol payload), never repeated across a range. Only staff-list copies fill a range.
- Insert mode moves the whole staff tail (all voices), not only the selected voices; ties and slurs crossing the insert point break (the tail goes through a clipboard-style copy). Other staves do not move. The Insert scopes beyond Voice stay deferred (README).
- Insert mode plus a range selection pastes once (no fill).
- Chord mode merges only into a chord that starts at the same tick as a pasted chord; ties of the old notes are not kept, and old notes under the middle of a longer pasted chord are lost.
- Unit tests cover the planner only. The DOM operations (overwrite, merge, insert, cut) were verified through the UI; no `.mscx` fixture tests (they would need the `src/braille/tests` ScoreRW environment).
- The error popup for an impossible paste (e.g. into a tuplet) is only logged (`Paste: no room …`) when the Dorico path runs; the single-target default path still uses MuseScore's popup.

## Acceptance criteria
- [x] `Cmd+V` with one note selected pastes at that note (single target, default mode: MuseScore's own paste runs unchanged).
- [x] 2-staff score, copy bar 1 of staff 1, select a note in bar 3 on staff 1 **and** a note in bar 3 on staff 2, `Cmd+V` → both staves receive the bar at bar 3.
- [x] Selected targets at different ticks on two staves: both pasted at the **top staff's** earliest tick.
- [x] Copy 1 bar (4/4), select bars 3–6 on one staff, paste → 4 copies; select 3 bars after copying 2 bars → 1 copy (no partial third), bar 5 untouched.
- [x] Immediate dynamic copied, range selected → pasted once, not repeated.
- [x] Default mode overwrites target-voice notes; notes in another voice at that tick are unchanged.
- [x] Chord mode on: paste of 4 single notes over 4 existing notes → 4 chords.
- [x] Insert mode on: later music is pushed right by the pasted length (bars are appended at the end when needed; barlines stay).
- [x] Whole paste (incl. multi-target) is one undo step.
- [x] Cut in Insert mode closes the gap; otherwise leaves rests.

## Tests
- `src/dorico/tests/pasteplanner_tests.cpp` (GoogleTest, pure): single anchor, list at different ticks, multi-staff clipboard, range exact fit / remainder / shorter than clipboard / no fill, two-staff range.
- UI (ui-tester, 2-staff 8-bar generated score): all ten checks passed; Insert-mode paste to bar 1 and bar 7 and Insert-mode cut verified.
