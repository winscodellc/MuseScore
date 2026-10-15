# 47 — Paste Special: Explode and Reduce

Status: todo (lowest priority) · Depends on: [40-copy-paste](40-copy-paste.md), [41-paste-special](41-paste-special.md), [46-jump-bar](46-jump-bar.md)
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

## Design
Three increasing steps; ship each separately.

1. **Explode wrapper** — `command://dorico/explode`: `dorico/paste` (spec 40) the clipboard into the **first** selected destination staff at the selection start (clipboard is normally one staff of chords), re-select the destination staves as a complete-measure range of the pasted span, then call `ImplodeExplode::explode(score)` (via `INotationInteraction::explodeSelectedStaff()` for the undo/error handling). One undo step (wrap in the paste's transaction). Then compare the allocation to the table above; where MuseScore differs (fewer-notes-than-staves duplication incl. slurs/dynamics, odd extras to upper staves) add a post-pass in `src/dorico/internal/explode.cpp` on the exploded range.
2. **Reduce, rhythm-matched** — `command://dorico/reduce`: paste each clipboard staff to consecutive scratch positions is not possible (destination has fewer staves), so: determine groups with the top-down split `groups[i] = sources [floor(i*n/m) .. floor((i+1)*n/m))`, with the larger groups first (5→3 gives 2,2,1); for each group paste its sources into the destination staff in the **same voice** with Chord-mode merge (spec 40, Chord flag) so notes stack into chords; remove duplicate pitches (`epitch()` + staff) in a chord; delete clef changes, ottavas (octave lines) in the pasted span. Equivalent to `ImplodeExplode::implode` for the matched-rhythm case — prefer calling `implode` after pasting the sources onto consecutive destination staves when enough exist, else use the merge routine.
3. **Reduce, differing rhythms** — merge into additional voices (max 4 in MuseScore): when a group's sources have different rhythmic grids at any position, put each source in its own voice instead of chord-merging, keeping rests; stem directions default.

| Command | Title | Key | Behaviour |
|---|---|---|---|
| `command://dorico/explode` | Paste Special > Explode | none | step 1 |
| `command://dorico/reduce` | Paste Special > Reduce | none | steps 2–3 |

Settings (replace the Note Input Options "Paste Special" page): `dorico/reduce/voicing` = `auto` (merge when rhythms match, else voices; default). No UI page in this spec.

Upstream edits: none.

## Deferred / unsupported
- The Note Input Options **Paste Special** page (voicing approach choices); only `auto` exists.
- Cue removal (MuseScore has no cue model), "reset cross-staff notes" prerequisite helper, slash voices.
- Reduce with more than 4 distinct rhythms (MuseScore 4-voice limit).
- Local time signatures / differing time stretch (upstream also refuses).
- Everything in step 3 until steps 1–2 are done; this whole spec is after specs 40–46.

## Acceptance criteria
- [ ] Copy a 3-note chord line, select 3 staves, `Explode`: top note → staff 1, middle → 2, bottom → 3; one undo removes everything.
- [ ] 2-note chords onto 3 staves: top note staff 1, bottom note staves 2 **and** 3 (fewer notes than staves → duplicate; slurs/dynamics duplicated).
- [ ] 4-note chords onto 3 staves: staff 1 gets 2 notes, staves 2 and 3 one each (extras go to upper staves; check the odd case with 5 notes / 3 staves → 2,2,1).
- [ ] Reduce of 4 staves in identical rhythm onto 2 staves: each destination staff holds chords of 2 sources, unisons removed, clef changes/ottavas of the sources gone.
- [ ] Reduce 5→3 groups are 2,2,1 top-down.
- [ ] Reduce with differing rhythms keeps every rhythm (extra voices), no notes lost.
- [ ] Both commands are single undo steps and unavailable without a clipboard / destination selection.

## Tests
- Extend the existing harness: new cases in `src/dorico/tests/explodereduce_tests.cpp` + fixtures `explodereduce_data/*.mscx`/`-ref.mscx`, modelled on `src/engraving/tests/implodeexplode_tests.cpp` + `implode_explode_data/`. Pure helper `reduceGroups(nSources, nDest)` → `{2,2,1}` etc. gets table tests; explode allocation tested via the 3/2/4/5-note cases above.
- Manual: orchestral-style 5-staff passage reduced to a piano-style 2–3 staff system.
