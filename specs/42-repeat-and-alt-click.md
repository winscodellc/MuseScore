<!-- markdownlint-disable -->
# 42 — Repeat (R) and Opt-click / Opt-drag copy

Status: todo · Depends on: [40-copy-paste](40-copy-paste.md), [10-keymap-preset](10-keymap-preset.md), [11-note-input-modes](done/11-note-input-modes.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_arranging_tools/write_mode_copying_pasting_items_t.html`;
forum (pre-release dev statements, treat as unverified): https://forums.steinberg.net/t/note-input-copy-paste-repeat-note/662458 , https://forums.steinberg.net/t/use-of-the-r-shortcut-key/673739
Reference digests: [reference/dorico-editing.md](reference/dorico-editing.md) §1.1, 2.1 · [reference/musescore-internals.md](reference/musescore-internals.md) §4

## Goal
`R` repeats the selection after itself, and `Opt-click` / `Opt-drag` copy material to a click/drag position, matching Dorico.

## Dorico behaviour

| Gesture | Result |
|---|---|
| `R` (notes/items selected) | Repeats the selected material **directly after itself**. |
| `R` during note input | Repeats the previous note/chord (forum; does not truncate at the barline). |
| `R` then `Opt-Up/Down` | New copy stays selected so its pitch can be changed immediately (forum). |
| `Opt-click` a position | With notes/items selected: pastes them at the clicked position (and staff). Repeat `Opt-click` on further positions; source stays the selection. Requires mouse editing enabled (not "Select" tool). |
| `Opt-drag` | Copies instead of moves; shadow noteheads show the destination, affected region is highlighted with the rhythmic grid shown above it. |
| Opt-drag selected notes up/down | Adds notes above/below (duplicates in pitch). |
| Paste rules for these copies | Same as spec 40 (overwrite / Chord merge / Insert push). |

## MuseScore today
- `R` = `REPEAT_SELECTION_COMMAND` (`notationcommands.h:480`) → `NotationActionController::repeatSelection` (`:2266`) → `NotationInteraction::repeatSelection` (`notationinteraction.cpp:5115`): note-input + single selection → re-add the chord; list selection → `Paste::repeatListSelection` (`paste.cpp:288`); range → `Paste::pasteStaff` (`:78`) after the selection. **Already ≈ Dorico.** Gaps to check: (a) does the new copy end up selected? (b) does repeating a single chord not at a barline cross the barline correctly? (c) behaviour in Insert/Chord mode.
- `Alt+click` with a **range** selection pastes the range at the click point: `mousePress_considerStartPasteRangeOnRelease`, `src/notationscene/internal/notationviewinputcontroller.cpp:795`. Qt maps macOS Option to `Qt::AltModifier`, so `Opt-click` already reaches it.
- Drag/drop with `Alt` forces copy instead of move (`notationviewinputcontroller.cpp:1587, :1634`) = Dorico's Opt-drag copy, minus the grid highlight.
- `Ctrl+Shift+drag` drags a single element out (`:777`) — MuseScore-only, leave.

## Design
Lazy path: **reuse the existing `R` command**; only wrap if the gap checks fail.
1. `shortcuts_dorico.json` keeps `R` → `command://notation/repeat-selection` (spec 10 owns the file). No new code if criteria below pass.
2. If gap (a)/(c) fails: add `command://dorico/repeat` (`src/dorico/internal/repeat.{h,cpp}`) that calls `INotationInteraction::repeatSelection()` and then re-selects the pasted items (record `[tick,end)` before, select the range `[end, end+len)` after via the selection API), honouring Chord/Insert flags (spec 11) exactly like spec 40; rebind `R` to it.
3. Opt-click: extend the existing branch at `notationviewinputcontroller.cpp:795` — **the one permitted upstream C++ edit of this spec** — so that it also fires for list/single-note selections (not only ranges) and keeps the original selection as the source across successive clicks. Keep the edit to a few lines; the paste itself still goes through spec 40's `dorico_paste` (call `DoricoPaste::pasteAt(anchor)`), so Chord/Insert semantics apply. If the maintainer prefers zero upstream edits, ship step 1–2 only and list Opt-click for lists/singles as deferred.
4. Opt-drag copy: nothing to build (already present). Optional later: grid ruler overlay (deferred).

| Command | Title | Key (mac) | Handler |
|---|---|---|---|
| `command://notation/repeat-selection` (or `dorico/repeat` if wrapped) | Repeat | `R` | upstream / `DoricoRepeat` |

## Deferred / unsupported
- Rhythmic-grid ruler + shadow noteheads while Opt-dragging.
- Opt-drag up/down to add notes (pitch duplication).
- Dorico's "mouse editing disabled" gating (MuseScore has no Select/mouse-input split).

## Acceptance criteria
- [ ] Select a bar, `R` → identical bar follows; one undo removes it; repeated `R` keeps adding.
- [ ] Select one note, `R` → the note is repeated right after it and the new note is selected so `Opt+Up` raises only the copy.
- [ ] Note input active: `R` repeats the previous chord at the caret.
- [ ] `Cmd-C`, `Opt-click` on an empty bar pastes the selection there; second `Opt-click` elsewhere pastes it again (source still selection).
- [ ] `Opt-drag` copies; plain drag moves.
- [ ] With Chord mode on, `Opt-click` onto occupied notes merges into chords (follows spec 40).

## Tests
- `src/dorico/tests/repeat_tests.cpp` (+ fixtures): range repeat, list repeat, single-note repeat, selection-after-repeat assertion (`score->selection()` contains the copy). Model: `src/engraving/tests/copypaste_tests.cpp`.
- Manual: Opt-click / Opt-drag need the GUI — script in the PR description.
