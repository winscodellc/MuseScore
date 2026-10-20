<!-- markdownlint-disable -->
# 42 — Repeat (R) and Opt-click / Opt-drag copy

Status: done · Depends on: [40-copy-paste](40-copy-paste.md), [10-keymap-preset](10-keymap-preset.md), [11-note-input-modes](11-note-input-modes.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_arranging_tools/write_mode_copying_pasting_items_t.html`;
forum (pre-release dev statements, treat as unverified): https://forums.steinberg.net/t/note-input-copy-paste-repeat-note/662458 , https://forums.steinberg.net/t/use-of-the-r-shortcut-key/673739
Reference digests: [reference/dorico-editing.md](../reference/dorico-editing.md) §1.1, 2.1 · [reference/musescore-internals.md](../reference/musescore-internals.md) §4

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

## Design (as built)
`R` stays `command://notation/repeat-selection`; the key map did not change.
1. **R on a range**: `DoricoPasteController::tryRepeat` (`src/dorico/internal/doricopastecontroller.cpp`), hooked through `preDispatch` like Paste / Cut. It copies the range after itself with the Insert and Chord rules (shared `pasteClipAt`, extracted from `tryPaste`) and selects the copy. Note input, a single chord and a list of notes fall through to MuseScore's `repeatSelection`, which already matches Dorico. Gap checks: (a) after R on one note only the copy is selected, so `Opt-Up` raises only it, nothing to add; (b) the range path appends bars when the copy does not fit; (c) Insert pushes the following music right, Chord merges.
2. **Opt-click / Opt-drag copy**: `NotationInteraction::dropRange` is the one funnel for both. A copy (`!deleteSourceMaterial`) now goes through the optional `IDoricoPaste` (`src/dorico/idoricopaste.h`, implemented by `DoricoPasteController`), so Chord / Insert apply. Where the copy lands is still MuseScore's own computation.
3. **Opt-click with a note / chord selected**: `mousePress_considerStartPasteRangeOnRelease` selects the chord-rest as a range first.
Upstream edits (marked `// Dorico fork (spec 42)`): `notationinteraction.{h,cpp}`, `notationviewinputcontroller.cpp`.

| Command | Title | Key (mac) | Handler |
|---|---|---|---|
| `command://notation/repeat-selection` | Repeat | `R` | upstream; range case intercepted by `DoricoPasteController::tryRepeat` |

## Deferred / unsupported
- Rhythmic-grid ruler + shadow noteheads while Opt-dragging.
- Opt-drag up/down to add notes (pitch duplication).
- Dorico's "mouse editing disabled" gating (MuseScore has no Select/mouse-input split).
- Repeating a list of notes that spans several ticks: MuseScore reports "cannot repeat selection".
- Opt-click on one note of a chord copies the whole chord-rest, not only that note.
- A plain drag (move) in Insert mode does not close the gap left at the source.
- Opt-drag through the Dorico rules is built but **not UI-tested**: `uictl.sh drag` cannot hold a modifier key.

## Acceptance criteria
- [x] Select a bar, `R` -> identical bar follows; one undo removes it; repeated `R` keeps adding.
- [x] Select one note, `R` -> the note is repeated right after it and only the new note is selected, so `Opt+Up` raises only the copy.
- [x] Note input active: `R` repeats the previous chord at the caret (MuseScore's own path).
- [x] `Cmd-C`, `Opt-click` on a bar pastes the selection there; a second `Opt-click` pastes again (source still the selection).
- [ ] `Opt-drag` copies; plain drag moves. Plain drag checked; Opt-drag needs a manual check.
- [x] With Chord mode on, `R` onto occupied notes merges into chords. With Insert mode on, `R` pushes the following music right.
- [x] One selected note, `Opt-click` on a bar copies it there.

## Tests
- `src/dorico/tests/pastespecial_tests.cpp`: `pasteClipFrom` (the clip parser R uses).
- UI run (ui-tester, 2026-10-19): 7 of 8 checks passed, Opt-drag blocked. Selection and mode behaviour needs the GUI, so there is no score-level unit test.
