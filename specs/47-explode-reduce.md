<!-- markdownlint-disable -->
# 47 — Paste Special: Explode and Reduce

Status: done · Depends on: [40-copy-paste](done/40-copy-paste.md), [41-paste-special](done/41-paste-special.md), [46-jump-bar](done/46-jump-bar.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B write_mode/write_mode_arranging_tools/write_mode_arranging_exploding_t.html`,
`B write_mode/write_mode_arranging_tools/write_mode_arranging_reducing_t.html`
Reference digests: [reference/dorico-editing.md](reference/dorico-editing.md) §1.6 · [reference/musescore-internals.md](reference/musescore-internals.md) §4

## Goal
Edit > Paste Special > **Explode** (spread chords of copied music across several destination staves) and **Reduce** (merge several copied staves onto fewer destination staves), following Dorico's allocation rules.

## Dorico behaviour

**Explode**
- Prerequisites: choose settings on the *Paste Special* page of Note Input Options (`Cmd-Shift-I`); reset cross-staff notes first.
- Procedure: copy music → select the **destination staves** → Edit > Paste Special > Explode (also context menu).
- Each destination staff receives at least one note from each chord, allocated from the **top staff downward**:
  - notes == staves → one note per staff;
  - fewer notes than staves → the same note goes to several staves (slurs/dynamics duplicated on each);
  - more notes than staves → spread as evenly as possible; with an odd count the extras go to the **upper** staves.

**Reduce**
- Prerequisite: voicing approach set on the *Paste Special* page of Note Input Options.
- Procedure: select music (several staves) → copy → select destination staves → Edit > Paste Special > Reduce. Tip: reduce rhythmic-unison sections separately from sections with different rhythms.
- If rhythms match throughout the selection: merged into **a single voice** by default, **unison notes removed** (one note per pitch per position), **clef changes, octave lines and cues deleted**.
- If rhythms differ anywhere: as many voices as needed to keep the rhythms.
- Distribution: each destination staff gets at least one source instrument, computed top-down; e.g. 5 sources onto 3 staves → staff 1 gets sources 1+2, staff 2 gets 3+4, staff 3 gets 5.

## MuseScore today
- Commands `STAFF_EXPLODE_COMMAND` / `STAFF_IMPLODE_COMMAND` (`command://notation/staff-explode` / `staff-implode`, `notationcommands.h`; registered `notationactioncontroller.cpp:545-546`, **no default keys**) → `NotationInteraction::explodeSelectedStaff` / `implodeSelectedStaff` (`notationinteraction.cpp:6367, 6383`, both require a **range** selection) → `ImplodeExplode::explode(Score*)` / `implode(Score*)` (`src/engraving/editing/implodeexplode.cpp:43, 242`).
- `explode`: range on the top staff plus following staves; if only one staff is selected the destination count is the maximum number of notes in any chord of the range (`lastStaff = min(nstaves, srcStaff + n)`), distributes chord notes top-down, works on complete measures, errors on differing local time stretch (`MsError::CANNOT_EXPLODE_IMPLODE_LOCAL_TIMESIG`). In-place: it operates on the score, not on the clipboard.
- `implode`: merges selected staves onto the first one (voices when rhythms differ) — the closest existing analogue of Reduce.
- Existing tests: `src/engraving/tests/implodeexplode_tests.cpp` with `implode_explode_data/`.
- Gap: Dorico is **clipboard-driven** (copy anywhere, paste into chosen staves) and has the exact "more notes than staves" / "fewer notes" / odd-extras and unison/cue/clef-change rules; MuseScore's functions are in-place and have their own allocation (verify against the rules above before relying on it).

## Design (as built)
Both live in `src/dorico/internal/pastespecial.{h,cpp}` next to the other Paste Special commands (no new `explode.cpp`), and run in one `transact` (one undo step). Internals found on the way: [reference/musescore-internals.md](reference/musescore-internals.md) "Explode / Reduce internals".

1. **Explode** (`pasteSpecialExplode`): `ImplodeExplode::explode` is **not** used (it keeps one note per staff, so 4 notes onto 3 staves lose a note, and it forces whole bars). The one-staff clip is pasted onto every selected staff at the selection start, then each staff prunes every chord (all voices, grace chords) to its share, `explodeNotes(nNotes, nStaves)`: more notes than staves = contiguous runs with the extras on the upper staves (4/3 = 2,1,1; 5/3 = 2,2,1); fewer notes = a note is shared (2/3 = top, bottom, bottom). Slurs and dynamics are duplicated by the paste itself.
2. **Reduce, rhythm-matched** (`pasteSpecialReduce`): the clip is split per staff (`staffClipOf`), groups come from `reduceGroups(nSources, nDest)` (5/3 = 2,2,1). Source `j` of a group is pasted into **voice `j`** of its destination staff (`pasteAtInVoice`). If every voice of a group has the same chord rhythm, the group is merged into voice 1 with `ImplodeExplode::implode` on that one staff (same-duration chords stack, unison pitches are skipped), then the clef changes and octave lines in the span are removed.
3. **Reduce, differing rhythms**: the same paste, and the merge is skipped for that staff, so the sources stay in their own voices (max 4 per staff).

| Command | Title | Key | Behaviour |
|---|---|---|---|
| `command://dorico/explode` | Paste Special > Explode | none | needs a one-staff clip and a range of 2+ staves |
| `command://dorico/reduce` | Paste Special > Reduce | none | needs a clip with more staves than the range has |

Settings (replace the Note Input Options "Paste Special" page): `dorico/reduce/voicing` = `auto` (merge when rhythms match, else voices; default). No UI page in this spec.

Upstream edits: none. `DoricoCommandsState` also refreshes on `QClipboard::dataChanged`, so the two items enable as soon as something is copied.

## Deferred / unsupported
- The Note Input Options **Paste Special** page (voicing approach choices); only `auto` exists.
- Cue removal (MuseScore has no cue model), "reset cross-staff notes" prerequisite helper, slash voices.
- Reduce with more than 4 distinct rhythms (MuseScore 4-voice limit).
- Local time signatures / differing time stretch (upstream also refuses).
- A copied staff that holds more than one voice, and a destination staff that would need more than four voices: Reduce refuses (`LOGW`), nothing changes.
- Sources that already sit in the destination voices (destination music in voices the sources do not use is merged too); slurs on chords that Reduce merges away are dropped by `ImplodeExplode::implode`.
- Rhythm-matched means the chords of every voice start and last the same; a group with a rest where another voice has a chord counts as different and is left in voices.

## Acceptance criteria
- [x] Copy a 3-note chord line, select 3 staves, `Explode`: top note → staff 1, middle → 2, bottom → 3; one undo removes everything.
- [x] 2-note chords onto 3 staves: top note staff 1, bottom note staves 2 **and** 3 (fewer notes than staves → duplicate; slurs/dynamics duplicated).
- [x] 4-note chords onto 3 staves: staff 1 gets 2 notes, staves 2 and 3 one each (extras go to upper staves; check the odd case with 5 notes / 3 staves → 2,2,1).
- [x] Reduce of 4 staves in identical rhythm onto 2 staves: each destination staff holds chords of 2 sources, unisons removed, clef changes/ottavas of the sources gone.
- [x] Reduce 5→3 groups are 2,2,1 top-down.
- [x] Reduce with differing rhythms keeps every rhythm (extra voices), no notes lost.
- [x] Both commands are single undo steps (UI-checked).
- [ ] Both commands are unavailable (greyed) without a clipboard / destination selection. **Not verified**: the Help-search access the UI tester uses shows no greyed state (a 2-staff clip on a 2-staff range dispatched nothing). Enabling logic: `pasteSpecialCanExplode` / `pasteSpecialCanReduce`, refreshed on selection and clipboard changes.

## Tests
- `src/dorico/tests/pastespecial_tests.cpp`: table tests for `reduceGroups` (5/3, 4/2, 3/3, 7/3) and `explodeNotes` (3/3, 2/3, 1/3, 4/3, 5/3), and `staffClipOf` (one staff kept, spanners filtered by track, every staff starts at the clip tick, round trip through `pasteClipFrom` / `pasteClipVoices`). The `.mscx` fixtures of the first draft were dropped: `dorico_tests` cannot load scores.
- UI (ui-tester, generated 5-staff score): Explode 3 / 2 / 4 / 5 notes onto 3 staves, Explode at a non-zero bar, Reduce 4 onto 2, 5 onto 3, differing rhythms, one undo step each. Found and fixed: staves after the first were pasted at tick 0 (their leading `<location>` rewinds from the previous staff; `staffClipOf` now rewrites it).
