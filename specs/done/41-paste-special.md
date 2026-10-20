<!-- markdownlint-disable -->
# 41 — Paste Special: Paste Into Voice, Duplicate/Move to Staff, Swap, Paste Articulations

Status: done · Depends on: [40-copy-paste](40-copy-paste.md) (paste engine/planner), [10-keymap-preset](../10-keymap-preset.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_arranging_tools/write_mode_copying_pasting_notes_into_different_voices_t.html`,
`B …/write_mode_copying_pasting_items_t.html` (Duplicate to Staff Above/Below),
`B …/write_mode_notes_moving_other_staves_t.html`, `B …/write_mode_staves_swapping_contents_t.html`,
`B notation_reference/notation_reference_articulations/notation_reference_articulations_copying_t.html`
Reference digests: [reference/dorico-editing.md](../reference/dorico-editing.md) §1.2, 1.5, 2.3 · [reference/musescore-internals.md](../reference/musescore-internals.md) §4

## Goal
The Dorico **Edit > Paste Special** family: Paste Into Voice, Duplicate to Staff Above/Below, Move to Staff Above/Below (`Opt-N` / `Opt-M`), Swap, Paste Articulations. (Explode/Reduce → [47](../47-explode-reduce.md).)

## Dorico behaviour

| Command | Dorico behaviour | Key |
|---|---|---|
| Paste Into Voice > [voice or new voice] | Copy, select the **staff and position** where it should start, then choose the voice. Notes go into that staff+voice (incl. slash voices; up-stem voice pasted into a down-stem voice on another staff is the manual's example). Reset cross-staff notes first to avoid odd beaming. | menu/context only |
| Duplicate to Staff Above / Below | Copies the **selection** to the adjacent staff at the same rhythm; originals untouched. Dynamics/slurs copied to other staves at the same position are auto-linked (deferred). | **no default key** |
| Move to Staff Above / Below | Selection is **cut and pasted** onto the adjacent staff; notes go into the **first voice active** on that staff; notes in tuplets stay in tuplets even if the bracket was not selected. Not applicable to pedal lines / system-attached items. (Keeping the notes attached to the original staff = cross-staff notation, a *different* command: `N`/`M`.) | `Opt-N` above, `Opt-M` below |
| Swap | Select a range on **two staves** → contents of the two staves for the selected duration are exchanged. | no key |
| Paste Articulations | Copy slurs / articulations / jazz articulations, select the **first target note**; they are applied onto notes starting there; **pitches unchanged**. | menu/context only |

Cross-staff reminder from the manual: `N`/`M` (cross-staff notation) ≠ `Opt-N`/`Opt-M` (move = cut/paste).

## MuseScore today
- `MOVE_UP_COMMAND` / `MOVE_DOWN_COMMAND` → `NotationInteraction::moveChordRestToStaff` (`notationinteraction.cpp:3855`, registered `notationactioncontroller.cpp:440-441`, MuseScore keys `Ctrl+Shift+Up/Down`). This is **cross-staff notation** (note stays in its voice/track, drawn on the other staff) = Dorico's `N`/`M`, **not** Dorico's `Opt-N/M`. Spec 10 binds `N`/`M` to these.
- `COPY_PASTE_SWAP_COMMAND` → `swapSelection` (`:5289`, registered `:425`) swaps the selection **with the clipboard**, not two staves — different semantics; leave it, add a new command.
- No voice-targeted paste, no duplicate-to-staff, no articulation-only paste.
- Engine facts that changed the design (details in [reference/musescore-internals.md](../reference/musescore-internals.md) §4, "Paste special internals"):
  - `Paste::pasteStaff` **keeps the source voice**; it does not paste into the voice of the selected chord-rest, and voices 2-4 do not necessarily exist in a bar. Paste Into Voice therefore swaps the two voices over the span (`ExchangeVoices`), pastes, and swaps back.
  - A span can be copied or deleted with a voice filter without touching the system clipboard (`writeStaffSelection` + `Score::deleteRange` with a `SelectionFilter`).

## Design (as built)
`src/dorico/internal/pastespecial.{h,cpp}`; the commands are registered in `DoricoPasteController::init()` (spec 40's controller). Each command is one undo step (`prepareChanges` / `commitChanges`; `rollbackChanges` and a log line on failure). No upstream edits.

| Command | Title | Key (mac) | Implementation |
|---|---|---|---|
| `command://dorico/duplicate-to-staff-above` / `-below` | Duplicate to Staff Above/Below | – | Source = the range, or for a list selection the span of the selected chord-rests (widened to whole tuplets) in their voices. `sliceOf` (in-memory stafflist) → `Paste::pasteStaff` at staff ±1. Disabled when there is no adjacent staff. |
| `command://dorico/move-to-staff-above` / `-below` | Move to Staff Above/Below | `Opt+N` / `Opt+M` | As Duplicate, but the source voices are deleted first (`Score::deleteRange` with a voice filter, rests remain). A single non-first voice lands in voice 1 of the target (voice-swap trick). |
| `command://dorico/paste-into-voice-1`…`-4`, `paste-into-new-voice` | Paste Into Voice > Voice 1-4 / New Voice | – | Clipboard stafflist (one staff, one voice, scanned by `pasteClipVoices`) at the selection or caret, via `pasteAtInVoice`. New Voice = first voice with no chord in the pasted span. Not in parts (voices cannot be exchanged there). |
| `command://dorico/swap-staves` | Swap | – | Range over exactly 2 staves: `sliceOf` each staff, delete both, paste crosswise. |
| `command://dorico/paste-articulations` | Paste Articulations | – | Trial paste of the clipboard at the selected note, read the chords' articulations and the slurs between them (`markingsIn`), roll back, then add them to the chords from the selected note on, in its voice. Pitches are never touched. |

The enable state is in `DoricoCommandsState::commandState` (`pasteSpecialCanMove/CanSwap/HasTarget`). The Edit > Paste Special menu (spec 59) lists all of them; no context-menu entries.

## Deferred / unsupported
- Linking of dynamics/slurs pasted to other staves; MIDI CC.
- Slash voices and >4 voices (MuseScore has 4 voices).
- Move to Staff for pedal lines / system items (Dorico also excludes them).
- Paste Into Voice: a clipboard with more than one voice (refused, logged), more than one staff, and any use in a part (voice exchange is skipped for excerpts). Note-input mode and Insert / Chord mode are not applied to these commands.
- Move / Duplicate with a list selection take the whole span of the selected notes' voices (also unselected chords of those voices in between) and whole chords, not single notes of a chord. A move into a non-first voice overwrites voice 1 of the target in the span.
- Paste Articulations: jazz-articulation categories beyond MuseScore's articulation SymIds; slurs only when both ends are in the copied run; the trial paste needs room at the target.
- Swap for ranges spanning >2 staves; key/clef/transposition differences between swapped staves are not adapted.
- Unit tests cover the clip voice scanner only (`tests/pastespecial_tests.cpp`); the DOM operations were checked through the UI (no `.mscx` fixtures, same reason as spec 40).

## Acceptance criteria
- [x] `Opt+N`/`Opt+M` on a note in staff 2 moves it to staff 1/3 voice 1 at the same tick; source becomes a rest; one undo restores.
- [x] Moving a selection inside a tuplet keeps the tuplet on the target staff.
- [x] Duplicate to Staff Below with a bar selected leaves the original and fills the staff below; disabled on the bottom staff.
- [x] Paste Into Voice 2 puts the clipboard in voice 2 of the selected staff; voice 1 material in that span is untouched.
- [x] Swap on a 2-staff range exchanges the two staves' contents for that range only.
- [x] Paste Articulations applies staccato/accent/slur from a copied run onto the selected note and following notes; pitches identical before/after.
- [x] User's clipboard content is unchanged after Duplicate/Move/Swap.
- [x] `N`/`M` (cross-staff, from spec 10) still behave as before.

## Tests
- `src/dorico/tests/pastespecial_tests.cpp`: `pasteClipVoices` (not a stafflist, first voice only, voice 2 only, several voices with a nested spanner `<location>`, voices restart per staff). Run `dorico_tests --gtest_filter='DoricoPasteSpecial*'`.
- Manual (ui-tester agent): a 4-staff score; each acceptance line, then undo/redo; N / M on a grand staff.
