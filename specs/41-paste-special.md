<!-- markdownlint-disable -->
# 41 — Paste Special: Paste Into Voice, Duplicate/Move to Staff, Swap, Paste Articulations

Status: todo · Depends on: [40-copy-paste](40-copy-paste.md) (paste engine/planner), [10-keymap-preset](10-keymap-preset.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_arranging_tools/write_mode_copying_pasting_notes_into_different_voices_t.html`,
`B …/write_mode_copying_pasting_items_t.html` (Duplicate to Staff Above/Below),
`B …/write_mode_notes_moving_other_staves_t.html`, `B …/write_mode_staves_swapping_contents_t.html`,
`B notation_reference/notation_reference_articulations/notation_reference_articulations_copying_t.html`
Reference digests: [reference/dorico-editing.md](reference/dorico-editing.md) §1.2, 1.5, 2.3 · [reference/musescore-internals.md](reference/musescore-internals.md) §4

## Goal
The Dorico **Edit > Paste Special** family: Paste Into Voice, Duplicate to Staff Above/Below, Move to Staff Above/Below (`Opt-N` / `Opt-M`), Swap, Paste Articulations. (Explode/Reduce → [47](47-explode-reduce.md).)

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
- Engine: `Paste::paste` (`paste.cpp:547`) pastes into the **track of the selected chord-rest**, so "paste into voice V" = select the chord-rest/rest at the anchor tick in `track = staffIdx*VOICES + V` first (MuseScore keeps rests in all 4 voices per measure, so a target exists).
- Selection data without touching the system clipboard: `Selection::mimeData()` / `qMimeData()` (`notationselection`/`Selection`), as used by `copypaste_tests.cpp:73-79`.

## Design
New in `src/dorico/internal/pastespecial.{h,cpp}` (shares helpers with spec 40's `dorico_paste`). Each command = one undo step.

| Command | Title | Key (mac) | Implementation |
|---|---|---|---|
| `command://dorico/paste-into-voice` (arg: voice 1-4) | Paste Into Voice | – | `planPaste` anchor → select chord-rest at `(staff, voiceN)` on the anchor tick → `Paste::paste`. "New voice" = first voice index with only rests in the span. |
| `command://dorico/duplicate-to-staff-above` / `-below` | Duplicate to Staff Above/Below | – | `Selection::mimeData()` of the selection → select chord-rest at same tick on `staff±1`, **same voice index** → `Paste::paste`. Disabled if no adjacent staff. |
| `command://dorico/move-to-staff-above` / `-below` | Move to Staff Above/Below | `Opt+N` / `Opt+M` | duplicate step, then delete the source (delete planner of [43](43-delete.md), Insert mode **off** regardless, so rests remain); target voice = first voice of the target staff that is active (else voice 1). |
| `command://dorico/swap-staves` | Swap Staves | – | require a range selection across exactly 2 staves; grab `mimeData` of each staff slice (re-select per staff), delete both slices, paste crosswise, one undo step. |
| `command://dorico/paste-articulations` | Paste Articulations | – | read the clipboard `stafflist` XML (`XmlReader`), collect per-chord articulations/ornaments + slurs in order, apply to consecutive chords from the selected note via `Chord::undoChangeProperty`/`Factory` + `score->undoAddElement`; pitches untouched. May be split off as stretch (see Deferred). |

Clipboard rule: the commands above that use `mimeData()` directly must **not** clobber the user's system clipboard. Where `Paste::paste` requires a `QMimeData` wrapper (`QMimeDataAdapter`, see `copypaste_tests.cpp:80`) pass the in-memory payload; no `QApplication::clipboard()` write is needed.

Menu: entries live in an "Edit > Paste Special" submenu. Registering `UiAction`s from the module (`IUiActionsModule`, like palette) is free; attaching them to the Edit menu needs one patch in `src/appshell/.../appmenumodel.cpp` — **optional upstream edit**. Required discoverability path is the Jump bar ([46](46-jump-bar.md)) + the notation context menu if that is already extensible; otherwise jump bar only.

Upstream edits: none required (menu patch optional).

## Deferred / unsupported
- Linking of dynamics/slurs pasted to other staves; MIDI CC.
- Slash voices and >4 voices (MuseScore has 4 voices).
- Move to Staff for pedal lines / system items (Dorico also excludes them).
- Paste Articulations: slurs between **different** rhythmic patterns, jazz-articulation categories beyond MuseScore's articulation SymIds; may land as a follow-up.
- Swap for ranges spanning >2 staves; swap with cross-staff/tuplet edge cases beyond a straight cut/paste.

## Acceptance criteria
- [ ] `Opt+N`/`Opt+M` on a note in staff 2 moves it to staff 1/3 voice 1 at the same tick; source becomes a rest; one undo restores.
- [ ] Moving a selection inside a tuplet keeps the tuplet on the target staff.
- [ ] Duplicate to Staff Below with a bar selected leaves the original and fills the staff below; disabled on the bottom staff.
- [ ] Paste Into Voice 2 puts the clipboard in voice 2 of the selected staff; voice 1 material in that span is untouched.
- [ ] Swap on a 2-staff range exchanges the two staves' contents for that range only.
- [ ] Paste Articulations applies staccato/accent/slur from a copied run onto the selected note and following notes; pitches identical before/after.
- [ ] User's clipboard content is unchanged after Duplicate/Move/Swap.
- [ ] `N`/`M` (cross-staff, from spec 10) still behave as before.

## Tests
- `src/dorico/tests/pastespecial_tests.cpp` + `paste_special_data/*.mscx` (+ `-ref.mscx`): `moveToStaffBelow`, `duplicateToStaffAbove`, `pasteIntoVoice2`, `swapStaves`, `pasteArticulations`. Same pattern as `src/engraving/tests/copypaste_tests.cpp` (`Engraving_CopyPasteTests::copypastevoice(idx, voice)`, `copypaste_tests.cpp:199`, is the closest model for the voice case).
- Manual: 3-staff piano+bass score; run each command from the jump bar, then undo/redo.
