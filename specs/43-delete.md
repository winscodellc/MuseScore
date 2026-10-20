<!-- markdownlint-disable -->
# 43 — Delete, Remove Rests, Delete Bars

Status: pending review · Depends on: [00-module-skeleton](00-module-skeleton.md), [10-keymap-preset](10-keymap-preset.md), [11-note-input-modes](done/11-note-input-modes.md) (Insert mode flag)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_arranging_tools/write_mode_deleting_items_t.html`,
`B notation_reference/notation_reference_bars/notation_reference_bars_deleting_contents_t.html`,
`B notation_reference/notation_reference_bars/notation_reference_bars_deleting_t.html`,
`B notation_reference/notation_reference_rests/notation_reference_rests_deleting_t.html`,
`B write_mode/write_mode_insert_mode/write_mode_insert_mode_c.html`
Reference digests: [reference/dorico-editing.md](reference/dorico-editing.md) §3 · [reference/musescore-internals.md](reference/musescore-internals.md) §3, §4

## Goal
`Delete`/`Backspace` behave like Dorico: Insert mode closes the gap, otherwise implicit rests; the selection moves to the nearest logical item; Remove Rests and Delete Bars exist as commands.

## Dorico behaviour

**Core** (Write mode only): `Backspace` or `Delete` (or Edit > Delete).
- Insert mode **on**: following notes move back to fill the gap, no rests left.
- Insert mode **off**: deleted notes become implicit rests "where appropriate".
- After deleting, the selection moves to the **nearest logical item**; for a note, first the nearest note in the same voice.
- Delete on selected bars removes their contents; barlines and bars remain.

**Per-type behaviour when deleted**

| Item | Dorico behaviour | Action in this spec |
|---|---|---|
| Slur end note | Slur moves to next/previous notehead; deleted if only one note remains under it | check MuseScore default; adopt Dorico via post-fix if different |
| Hold/pause | Not deleted when the note is deleted; moves to nearest note/rest or over the bar rest | MuseScore deletes anchored fermata with the chord → **gap**, see Deferred |
| Repeat ending | Its repeat barlines are kept | check; post-fix |
| Barline | Adjacent bars merge; time signature unchanged | Deferred (MuseScore deletes the barline as a property) |
| Time signature | Re-barred with previous time signature up to the next one / end; none → open meter | rely on `cmdRemoveTimeSig`; open meter Deferred |
| Clef / key signature | Pitches kept, renotated with the previous clef/key; none → atonal/open (not C major) | rely on MuseScore; atonal fallback Deferred |
| Octave line | Notes shown at concert/transposed pitch per layout | MuseScore default |
| Immediate dynamic next to hairpin | Hairpin may be adjusted | Deferred |
| Dynamic group deleted on one staff | Not deleted on linked staves; deleting some of a linked group deletes all linked | n/a (no linking) |
| Tuplet only | Notes stay; notes only → tuplet stays | MuseScore default |
| Rehearsal mark | Later marks renumber | Deferred (MuseScore marks are plain text) |
| Tempo mark | Also removed from tempo track; playback falls back to previous tempo / 120 | MuseScore default (120) |

**Remove Rests** — Edit > Remove Rests (no default key; user alias example `rr`). Select rests (or a region containing them). Dorico sets "Starts voice"/"Ends voice" on the neighbouring notes/rests so the rests are not shown; reversible by turning those properties off. Works on implicit and explicit rests.

**Delete Bars** — `Shift-B` popover `-6` (spec 25) or Edit > Delete Bars on selected bars; music to the right moves left as in Insert mode; signposts in the range are deleted. Deleting only some beats removes only their contents.

## MuseScore today
- `DELETE_COMMAND` ← `Del`, `Backspace` (`shortcuts.json:62`) → `NotationInteraction::deleteSelection` (`notationinteraction.cpp:5308`) → `Score::cmdDeleteSelection` (`src/engraving/editing/edit.cpp:2239`): leaves rests, no gap closing.
- Time-removing delete exists: `Score::cmdTimeDelete` (`edit.cpp:2986`), used by `removeSelectedMeasures` (`notationinteraction.cpp:6423`) and `removeSelectedRange` (`:6477`); `REMOVE_SELECTED_RANGE_COMMAND` ← `Ctrl+Del`/`Ctrl+Backspace` (`shortcuts.json:170`). That is Dorico's "Insert mode delete".
- Voices 2–4 rests can be deleted (`Score::deleteItem`); voice-1 rests cannot. No "Starts voice/Ends voice" concept.
- Note input `Backspace` deletes the previous note (existing note-input handling in `NotationActionController`; check before overriding).

## Design
All in `src/dorico/internal/doricopastecontroller.{h,cpp}` (it already owns Insert-mode Cut and the gap helper `pasteShiftStaffTail`); one undo step each. There is **no `dorico/delete` command and no new key**: the preset already binds `Del`/`Backspace` to `notation/delete`, and a `preDispatch` hook (`tryDelete`, same pattern as `tryCut`) covers keys, Edit menu and context menu. When the hook returns false MuseScore's own handler runs.

| What | Behaviour |
|---|---|
| `notation/delete` hook | Not handled (upstream runs) while text editing, in **note input** (Backspace unchanged), or when the selection holds anything but notes/rests. **Insert on**: a range, or one whole chord/rest, → `closeGap` (the tail of the staff moves back, all voices of the staff, like Cut), then select the nearest chord. **Insert off**: a list of notes/rests in one voice → `deleteSelection()`; if the selection ends up one rest, select the nearest chord of the voice instead. A range with Insert off is left to MuseScore (empty bars, barlines stay). |
| `nearestChord` | first Chord of the track at tick >= end of the deleted span, else the last Chord before its start, else MuseScore's selection stays. |
| `command://dorico/remove-rests` "Remove Rests" (no key) | Selected rests, or those in a range: voice 2–4 → `Score::deleteItem`; voice 1 → `Pid::VISIBLE` false. One undo step. |
| `command://dorico/delete-bars` "Delete Bars" (no key) | Dispatches `notation/remove-selected-range` (`cmdTimeDelete`). Edit menu; replaces that command in the Write menu. The Shift-B popover `-N` (spec 25) has its own path. |

Upstream edits: none.

## Deferred / unsupported
- Remove Rests via "Starts voice/Ends voice" properties; true removal of voice-1 rests (hidden instead — **ponytail ceiling**: hidden rests still occupy spacing).
- Holds/pauses surviving note deletion by re-anchoring; hairpin adjustment when deleting an adjacent immediate dynamic; barline-delete bar merging; atonal/open fallback after deleting the first key signature; rehearsal-mark renumbering; linked-dynamic deletion rules.
- Delete Bars on part of a bar shortens the bar (`cmdTimeDelete`); Dorico keeps the bar length.
- Insert-mode delete of a chord moves every voice of the staff (no per-voice shift), and ties/slurs crossing the cut break (`pasteShiftStaffTail` ceiling).
- Stop-position (Shift-Opt-I) pushing material past a stop point (spec 11 defers Insert scope except Voice).

## Acceptance criteria
- [x] Insert off: delete 2 quarter notes in a bar → 2 quarter rests; selection lands on the next note in the voice.
- [x] Insert on: same delete → following notes shift left, no rests, measure content shortened per `cmdTimeDelete` behaviour.
- [x] Deleting the last note in a voice selects the previous note in that voice.
- [x] Deleting selected bars' contents (bars selected, Insert off) leaves empty bars and barlines.
- [ ] `dorico/delete-bars` removes N selected measures; later music moves left; one undo restores.
- [ ] `dorico/remove-rests` on voice-2 rests removes them; voice-1 rests become invisible; one undo restores both.
- [x] Time signature delete re-bars with the previous time signature; clef delete keeps pitches.
- [x] Note input active: Backspace behaviour unchanged from upstream.

Not ticked: the two Edit-menu boxes (`delete-bars`, `remove-rests`). The UI driver cannot click the native macOS menu bar, so they need a manual check (Edit > Delete Bars; Edit > Remove Rests on voice-2 and voice-1 rests).

## Tests
- `dorico_tests` links only `dorico` and cannot load scores (every test there is pure), so no `ScoreRW` fixtures. The code is engine glue; it is checked in the UI (acceptance list above).
- Manual: toggle `I` and delete in a 4/4 bar; Cmd-Z/Cmd-Shift-Z.
