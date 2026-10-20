<!-- markdownlint-disable -->
# 65 — Musical transformations (invert, reverse, rotate)

Status: done · Depends on: [35-popover-note-tools](35-popover-note-tools.md), [59-menus](59-menus.md) · Dorico refs (local mirror, `specs/reference/manual/topics/selecting-and-editing-music/musical-transformations/`):
- [M] `inverting-pitches.md`, `reversing-pitches-rhythms.md`, `rotating-pitches-rhythms.md`
- [M] `reversing-pitches-rhythms/reverse-and-invert-pitches-and-reverse-rhythms-dialogs.md`, `rotating-pitches-rhythms/rotate-pitches-rhythms-dialogs.md`
- [M] `note-tools-popover/inversion-popover-entries.md`, `reversal-popover-entries.md`, `rotation-popover-entries.md`
- Images: `images/reverse_invert_pitches_reverse_rhythms_dialogs.png`, `images/rotate_pitches_rhythms_dialog.png`

Reference: [internals](../reference/musescore-internals.md) (sections 2 and 3). Roadmap row: [ROADMAP.md](../ROADMAP.md) section 3, tier A.

## Goal
Shift-I accepts `inv`, `ret` / `rev` and `rot` and the Write > Transform menu does the same through two small dialogs. Both routes end in one apply path that inverts, reverses or rotates the pitches and/or rhythms of the selected notes in a single undo step. Repeat pitches, pitch mapping and scale mapping stay Unsupported (spec 35 behaviour).

## Dorico behaviour

**Popover entries** (note tools popover, Shift-I; all case-insensitive except the note names' accidentals)

| Tool | Entry | Meaning |
|---|---|---|
| Invert | `inv`, `invert`, `inversion`, `mirror` | invert around the outermost pitches (highest note <-> lowest note) |
| | `C4`, `eb4`, `F5`, `g#6` | central pitch (note name, accidental, octave; `inveb4` = `inv eb4`) |
| | `root` / `first` / `top` / `bottom` | central pitch is the root of the prevailing key (C in open keys) / the first note / the top / bottom note of the first chord |
| | `diatonic`, `diat` / `chromatic`, `chrom` | inversion type |
| Reverse | `retrograde`, `ret`, `reverse`, `rev` | reverse pitches **and** rhythms |
| | `pitch`, `pt` / `rhythm`, `rm` | restrict to pitches / rhythms |
| | `ret invert`, `rev inv` | reverse and invert |
| | `player` / `voice` | also items of the whole player / only the selected voice |
| Rotate | `rotate`, `rot` | rotate pitches **and** rhythms |
| | `pitch`, `pit` / `rhythm`, `rhy`, `rm` | restrict |
| | `N` / `-N`, `bw N`, `backwards N`, `prev N` | steps forwards / backwards |

Examples from the manual: `inveb4` (invert around E♭4), `rev pt voice`, `rot rm bw 3`.

**Write menu > Transform** (all enabled when notes are selected, Write mode)

| Submenu | Item | Dialog |
|---|---|---|
| Pitches | Invert Pitches | Invert |
| | Reverse Pitches | none, runs `rev pt` |
| | Reverse and Invert Pitches | Invert |
| | Rotate Pitches | Rotate |
| Rhythm | Reverse Rhythms | none, runs `rev rm` |
| | Rotate Rhythms | Rotate |
| All | Reverse Pitches and Rhythms | none, runs `rev` |
| | Reverse and Invert Pitches and Reverse Rhythms | Invert |
| | Rotate Pitches and Rhythms | Rotate |

**Invert dialog**: *Invert using* (segmented): Outermost pitches / Specific pitch / Root / First note. *Inversion type*: Diatonic / Chromatic (disabled for Outermost). *Specific pitch*: note name, accidental, octave (only for Specific pitch). *When first note is a chord, use*: Top note / Bottom note (only for First note). OK / Cancel.
**Rotate dialog**: "Enter the number of steps by which to rotate." *Number of steps* (spin box), *Direction*: Forwards / Backwards. OK / Cancel.

## Semantics (our reading of the manual; verify against the pictures)

- **Events.** Per selected track, an event is one chord or rest in the selection span (grace notes excluded). A tie chain is one event with the summed length. *Pitch* operations see only the note events (rests stay where they are); *rhythm* operations see all events, rests included; both together move events as whole units.
- **Reverse.** Pitches: the i-th note event gets the pitch set of the (n-1-i)-th (chords move as a unit). Rhythms: the i-th event gets the length of the (n-1-i)-th event. The selection's total length does not change, so every event lands on a new tick.
- **Rotate.** `N` steps forwards: value i moves to position (i+N) mod n; backwards shifts to the left. `N` may exceed n. Applies to pitches, rhythms or both, like Reverse.
- **Invert.** Central pitch `c`.
  - Chromatic: `p' = 2c - p` (semitones; spelled with `pitch2tpc(p', key, Prefer::NEAREST)`).
  - Diatonic: `step' = 2·step(c) - step(p)` on the prevailing key's scale; the pitch of `step'` comes from the key scale (`absStep2pitchByKey`); an accidental the note carries against the key is mirrored (a note a semitone above its scale pitch becomes a semitone below the new scale pitch).
  - Outermost: the highest and lowest notes of the whole selection swap. Always diatonic (the dialog disables the type for it): `step' = stepHi + stepLo - step`.
  - Root: tonic of the key at the first note's tick, in the octave nearest that note (our choice; the manual does not say).
  - First / top / bottom: the pitch of the first chord's note (`first` = `top`).
  - Reverse+invert: reverse first, then invert (the centre is taken from the original selection).
- **Key.** One key for the whole selection: `Staff::concertKey` at its first note. Transposing instruments are inverted in concert pitch (ponytail).
- **Range limit.** If any result leaves MIDI range 0-127 nothing is changed and the popover shows "out of range" (same pattern as spec 35).
- **Scope.** All selected tracks, treated independently, except the centre (`first`, `top`, outermost), which uses the whole selection. Drum staves are skipped.
- **Selection after apply**: pitch operations keep it; rhythm operations leave the selected range (`setNoteRest` replaces the notes).

## MuseScore today
- Shift-I already recognises the keywords: `UNSUPPORTED_TOOLS` at `src/dorico/popover/parsers/notetoolsparser.cpp:77` returns Unsupported for `inv`, `invert`, `inversion`, `mirror`, `ret`, `retrograde`, `rev`, `reverse`, `rot`, `rotate` (and `rep`, `repeat`, which stay). Prefixes are Incomplete (`:276`). `NoteToolsParse` (`notetoolsparser.h:69`) holds the transpose / add data.
- Handler: `src/dorico/popover/handlers/notetoolshandler.cpp` (`noteToolsPopover()` `:132`; `targetChains` `:48` collects `selection().noteList()`, one note per tie chain, drum staves skipped; prepare / rollback / commit at `:160-173`).
- Menu: `src/dorico/internal/doricomenu.cpp` Write menu `:156`, Respell submenu `:166` is the model to copy. No Transform submenu today. Commands are `muse::rcommand::Command` constants in `doricocommands.h`, titled in `doricocommandsregister.cpp`, enabled in `doricocommandsstate.cpp:150-165`, handled with `onNotation(...)` (`doricoselectioncontroller.cpp:302`).
- Dialog pattern: `FilterNotesByPitchDialog.qml` (spec 45), a `StyledDialogView` with no C++ model, registered with `registerQmlUri` at `doricomodule.cpp:102`, opened by `DoricoSelectionController::filterNotesByPitch()` (`:495`) and returning a string.
- Engine helpers (all existing): `Staff::key(tick)`; `pitch2tpc`, `absStep2pitchByKey`, `pitch2absStepByKey` (`dom/pitchspelling.h`); `EditNote::undoChangePitch(Score*, Note*, pitch, tpc1, tpc2)` (`editing/editnote.h:50`); `Score::makeGap` (`score.h:413`), `Score::setNoteRest` (`score.h:406`), `NoteInput::addNote` (`editing/noteinput.h:66`).
- Dorico-local helpers `transact`, `chordsIn`, `rhythmOf` in `src/dorico/internal/pastespecial.cpp` (`:413`, `:549`, `:572`) are file-local and must move to a shared header.
- The only existing retrograde is the QML plugin `share/plugins/new_retrograde/NewRetrograde.qml` (checklist of what a reversal must preserve: chords, rests, tuplets, ties, grace notes, articulations).
- No score-level test infrastructure exists in `src/dorico/tests/` (`dorico_tests` cannot load scores); keep the maths pure.

## Design

1. **Parser.** `notetoolsparser.{h,cpp}`: add `struct TransformSpec { enum Op {Invert, Reverse, Rotate}; Op op; bool pitches, rhythms; bool invert; Center center; int cStep, cAlter, cOctave; bool diatonic; int steps; }` and `std::optional<TransformSpec> transform` to `NoteToolsParse`. Remove `inv`/`invert`/`inversion`/`mirror`/`ret`/`retrograde`/`rev`/`reverse`/`rot`/`rotate` from `UNSUPPORTED_TOOLS` (keep `rep`/`repeat`, `=`, `snap`, `spell`, `to`). Other words after the tool word are the modifiers in the tables above; unknown words are Invalid, a trailing partial word is Incomplete. `player` returns Unsupported with the reason "player scope not supported yet".
2. **Pure maths.** New `src/dorico/internal/transform.{h,cpp}`, namespace `mu::dorico::transform`: `struct Event { Fraction len; std::vector<int> pitches; }` (empty = rest) and `std::vector<Event> apply(const std::vector<Event>&, const TransformSpec&, int key, int centre)`, plus the small helpers `reverseOrder`, `rotateOrder`, `invertPitch` that the unit tests call directly. No Score dependency; spelling is done by the adapter.
3. **Score adapter.** New `src/dorico/internal/transformapply.{h,cpp}`: `bool applyTransform(INotationPtr, const TransformSpec&, std::string* error)`. Collect events per track, run `apply`, then write back inside one `transact` (moved from `pastespecial.cpp` into a shared `internal/transact.h`, with `chordsIn`):
   - Pitch-only: `EditNote::undoChangePitch` on every note of each tie chain, spelled with `pitch2tpc`; if the chord size changes, add (`NoteInput::addNote`) or remove (`undoRemoveElement`) notes.
   - Rhythm / All: `Score::makeGap` for the span, then `Score::setNoteRest` per event (it splits into tied durations); chord notes beyond the first via `NoteInput::addNote`, tied as `Score::addInterval` does; articulations of the original event cloned onto the new first chord.
   - Refuse (status "not supported on tuplets / grace notes") a rhythm operation whose span contains a tuplet or grace notes; pitch operations still work.
4. **Handler.** `notetoolshandler.cpp`: when `transform` is set, call `applyTransform` and use the existing rollback / commit pattern.
5. **Menu and commands.** Nine commands `command://dorico/transform/{invert-pitches,reverse-pitches,reverse-invert-pitches,rotate-pitches,reverse-rhythms,rotate-rhythms,reverse-all,reverse-invert-all,rotate-all}` in `doricocommands.h` / `doricocommandsregister.cpp`, enabled when the selection contains notes (`doricocommandsstate.cpp`). Add `menu("dorico-menu-transform", "Transform", {...})` with the three submenus next to Respell in `doricomenu.cpp`. A controller method per command: no-dialog items build the popover text (`rev pt`, ...), dialog items open the dialog, receive an entry string and feed it to the same parser. Update `doricomenu_tests.cpp`.
6. **Dialogs.** `InvertPitchesDialog.qml` (title passed as a parameter, so one file serves the three invert items) and `RotateDialog.qml`, plain `StyledDialogView` as in `FilterNotesByPitchDialog.qml`. Each returns a popover entry (`inv root diat`, `ret inv chrom eb4`, `rot pit bw 3`). Register the URIs in `doricomodule.cpp`, add the files to `qml/MuseScore/Dorico/CMakeLists.txt`, remember the last values in `muse::settings()`.
7. **Build files.** `src/dorico/CMakeLists.txt` (new .cpp), unity-build rule: helper names in `popover/*.cpp` and `internal/*.cpp` batches must be unique, so everything lives in a named namespace.
8. **Docs.** Spec 35 "Deferred" gets a pointer to this spec; internals found go into `reference/musescore-internals.md`.

Upstream edits: none expected.

## Deferred / unsupported
- Repeat pitches, pitch mapping, scale mapping (`rep`, `=`, `snap`, `spell`, `to`): stay Unsupported.
- EDO / microtonal centres (`d1/24 4`).
- Reversing and rotating staff-attached items (dynamics, hairpins, slurs spanning the range) and the `player` scope. `voice` is accepted and has no extra effect (we only touch the selected voices).
- Lyrics, slurs, dynamics and other annotations moving with their notes in rhythm operations: they stay at their tick or are dropped with the segment they hang on (articulations and stem direction do travel with the chord).
- The "allow transformations that shorten the selection" option (Note Input Options > Musical Transformations).
- Rhythm operations over tuplets and grace notes (refused, see Design 3).

## Acceptance criteria
- [x] Shift-I `inv` on C D E F in C major (outermost) gives F E D C: the highest and lowest notes swap, rhythm unchanged (UI-checked).
- [x] `inv c4 chrom` on C4 D4 E4 F4 gives C4 B♭3 G♯3 G3 (pitches 60, 58, 56, 55; the new notes are spelled from the key, so 56 is G♯, not A♭); `inv c4 diat` gives C4 B3 A3 G3.
- [x] `inveb4` is accepted without a space and inverts around E♭4.
- [x] `rev` on quarter, eighth, eighth, half reverses both pitches and rhythm (half, eighth, eighth, quarter); the total length is unchanged.
- [x] `rev pt` reverses only the pitches (rhythm and rests stay); `rev rm` only the rhythm.
- [x] `rot 1` shifts pitches and rhythms right by one; `rot rm bw 3` shifts only the rhythm left by three; rotating by more than the event count wraps (unit-tested).
- [x] Chords move as one unit in reverse and rotate; `inv` mirrors every note of a chord.
- [x] A rest is part of a rhythm reversal (`rev` over quarter, eighth rest, eighth, half).
- [x] A result outside MIDI range changes nothing and logs the reason (`inv c9 chrom` on C4..F4: bar unchanged, no undo entry; the popover stays open).
- [x] Every operation is one undo step.
- [x] `rep`, `=` and `player` still show Unsupported; `inv`, `ret`, `rot` prefixes are Incomplete (unit-tested).
- [x] Write > Transform has the three submenus with 4, 2 and 3 items (unit-tested). With nothing selected the commands are not offered in the Jump bar. The menu bar itself cannot be clicked by `uictl`.
- [x] The Invert and Rotate dialogs open with the controls listed above; OK applies the same result as the equivalent popover entry; Cancel changes nothing.
- [x] Rhythm operations over a tuplet or grace notes are refused with a log message (`rev rm` over a triplet: unchanged); pitch operations over them work (`rev pt` kept the tuplet).

## Tests
- `src/dorico/tests/notetoolsparser_tests.cpp`: the manual examples (`inveb4`, `rev pt voice`, `rot rm bw 3`, `ret inv`), each keyword alias, `-3` / `bw 3` / `prev 3`, Incomplete prefixes, `player` Unsupported, `rep` still Unsupported.
- `src/dorico/tests/transform_tests.cpp` (new): `reverseOrder`, `rotateOrder` (N = 0, 1, n, n+1, negative), `invertPitch` chromatic and diatonic in C major and E♭ major, outermost, mirrored out-of-key accidentals, reverse with chords and rests.
- `src/dorico/tests/doricomenu_tests.cpp`: the Transform submenu and its nine items.
- UI (ui-tester, temp copy of a `demos/` score): the acceptance criteria above, one undo step each, dialogs, tuplet refusal.
