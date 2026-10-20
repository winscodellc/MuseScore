<!-- markdownlint-disable -->
# 61 — Guitar bends, techniques, harmonics and string indicators

Status: done · Depends on: [28-popover-ornaments](28-popover-ornaments.md), [29-popover-playing-techniques](29-popover-playing-techniques.md) · Dorico refs (local mirror, `specs/reference/manual/topics/notations-input/`):
- [M] `input-methods-for-ornaments-arpeggio-signs-glissando-lines-a/ornaments-popover/guitar-technique-popover-entries.md`
- [M] `input-methods-for-guitar-bends-and-guitar-techniques/` (`inputting-guitar-bends-with-the-popover.md`, `inputting-hammer-ons-pull-offs.md`, `inputting-tapping.md`, `inputting-vibrato-bar-*.md`)
- [M] `input-methods-for-playing-techniques-harmonics-pedal-lines-s/` (`playing-techniques-popover/harmonics-popover-entries.md`, `inputting-harmonics.md`, `inputting-string-indicators-outside-the-staff-with-the-popov.md`)

Reference: [internals](../reference/musescore-internals.md). Roadmap row: [ROADMAP.md](../ROADMAP.md) section 3, tier A.

## Goal
The guitar entries that specs 28 and 29 parse but mark Unsupported now work: Shift-O `bend`, `vibbend`, `vibscoop`, `vibdip`, `wbar`, `hammer`/`pull`/`hopo`..., `tap`/`lhtap`/`tappull`/`lhtappull`; Shift-P `harm ...` (look only) and `string N`. No new engraving model: every item already exists in MuseScore (guitar bends, hammer-on / pull-off, tapping, whammy bar, harmonic mark, string-number fingering). Pre-bends, post-bends and the Properties-panel routes are not popover entries and stay out.

## Dorico behaviour

**Shift-O**

| Entry | Dorico item | Attaches to |
|---|---|---|
| `bend` | guitar bend | two notes (adjacent or not); one selected note bends to the next note of its voice; not in note input |
| `vibbend` | vibrato bar dive (descending) or return (ascending) | as `bend`; not in note input |
| `vibscoop` | vibrato bar scoop | each selected note, or the last input note |
| `vibdip` | vibrato bar dip | the earliest selected note of each staff, or the last input note |
| `wbar`, `w/bar` | vibrato bar line | the selected range per staff (rhythmic position only: a single note) |
| `hammer`, `ho` / `pull`, `po` | hammer-on / pull-off | selected notes, or the last input note; H or P follows the pitch direction |
| `hammerpull`, `hopo`, `hp`, `ligado`, `lig` | ligado (alternating H / P) | same |
| `tap` / `lhtap` | right-hand / left-hand tapping | each selected chord, or the last input note |
| `tappull` / `lhtappull` | tapping with pull-off | same |

**Shift-P** (`harm` must come first, further words follow, separated by spaces)

| Entry | Meaning |
|---|---|
| `harm`, `harmonic`, `harmonics` | natural harmonic by default |
| `nat` / `natural`, `nh`, `hh` | natural |
| `art` / `artificial`, `ah` | artificial |
| `ph`, `pinch` | pinch (artificial) |
| `circle`, `circleabove` | natural: circle above the staff |
| `diamond`, `diamondnotehead` | natural: diamond notehead |
| `noharm` | delete the harmonic |
| `p=N` `n=N` `s=N` `s=D4` `open` `noopen` `paren` `noparen` `white` `touched` `stopped` `sounding` `normal` `singlesounding` `singlestopped` ... | detailed appearance and string / partial / node |
| `string N` (1-9), also `stringN` | string indicator outside the staff; `stringN->` gives it a duration line |

## Mapping (what MuseScore does)

**Shift-O** — new `OrnamentEntry::Kind::Guitar` with a `GuitarTech` enum.
- `bend`, `vibbend`: `Score::addGuitarBend(GuitarBendType::BEND / DIVE, from, to)` (`editing/cmd.cpp`). Note pairs follow the glissando rule (two notes = joined; otherwise each note joins the next in its voice; the last note, or a pair, passes `nullptr` and MuseScore guesses / creates the end note). In note input they are refused. The amount comes from the pitches, so an ascending `vibbend` is a return.
- `vibscoop`, `vibdip`: `addGuitarBend(SCOOP / DIP, note)`, one per selected note (dip: earliest note per staff). They are the only bends allowed during note input (they go on the last input note, like jazz articulations).
- `wbar`, `w/bar`: `Factory::createWhammyBar`, from the first selected tick to the end of the last on each staff, `undoAddElement` (like `addTrill`). During note input it waits at the caret for the next note and spans it (like a trill).
- `hammer`, `pull`, `hopo`, ...: all nine words add a `HammerOnPullOff` slur (`Score::addSlur(first, last, template)`), because MuseScore decides H or P per pair from the pitches (`HammerOnPullOffSegment::updateHopoText`). With one selected note the slur ends on the next chord of the voice (`addSlur` picks it). Dorico's difference (an explicit H / P / ligado) cannot be forced; the text follows the notes.
- `tap`, `lhtap`: `Factory::createTapping` + `setHand(RIGHT / LEFT)` + `EditChord::toggleArticulation` on each selected chord (an existing tapping is replaced; entering the same hand again removes it).
- `tappull`, `lhtappull`: tapping on the first chord plus a hammer-on / pull-off slur over the selection.
- Return on one of these items reopens the popover with its entry (`ornamentPrefill`): guitar bend, hammer-on / pull-off, tapping and whammy bar segments join `itemTypes`.

**Shift-P** — `TechniquesEntry` gets `harmonic` (kind Natural / Artificial / Pinch / Remove, circle) and `stringNumber`.
- Natural: notehead group `HEAD_DIAMOND` (`Pid::HEAD_GROUP`) on each selected note; `circle` adds the `o` articulation (`SymId::stringsHarmonic`) instead.
- Artificial and pinch: a `HarmonicMark` text line over the selection per staff, begin and continue text `AH` / `PH` (the text the Guitar Pro importer uses).
- `noharm`: notehead group back to normal, and the `o` articulation / harmonic mark over the selection removed.
- `string N`: a `Fingering` with `TextStyleType::STRING_NUMBER` (the circled number MuseScore's own palette uses) on the top note of each selected chord, or on the last input note.

## Deferred / unsupported
- `harm` with `p=` / `n=` / `s=`, `open`, `noopen`, `paren`, `noparen`, `white`, `touched`, `stopped`, `sounding`, `normal`, `singlesounding`, `singlestopped`: valid Dorico, stay Unsupported (MuseScore harmonics have no partial / node / string model, and artificial harmonics add no touched notehead).
- `stringN->` (duration line) and `A->B` string chains: Unsupported (MuseScore string numbers are per-note).
- Vibrato bar dive without return as two entries (`fallsmooth` then `wbar`), pre-bends / pre-dives / post-bends (Properties panel in Dorico).
- Forcing H versus P versus ligado (the slur text follows the pitches), the `vibdip` interval option.
- `slurarp` (spec 28) stays Unsupported.
- Return-to-edit on hammer-on / pull-off, tapping, whammy bar and harmonic marks: the first three are wired (`itemTypes`, prefill) but only the guitar bend was UI-checked; harmonic marks have no Return-to-edit.
- Bare `ah`, `ph`, `nh`, `hh`, `noharm` (no `harm` first) are parser-tested only; the UI run covered the `harm ...` forms, which share the handler.
- `tap` on a range that already mixes hands resets the whole selection to the right hand instead of toggling.

## Acceptance criteria
- [x] Shift-O `bend` on two selected notes adds a guitar bend between them; on one note it bends to the next note; one undo step each.
- [x] `vibbend` adds a dive (descending notes) or a return (ascending notes).
- [x] `vibscoop` and `vibdip` add the item on the selected note(s); `vibdip` once per staff.
- [x] `wbar` and `w/bar` add a whammy-bar line over the selected range.
- [x] `hammer`, `ho`, `pull`, `po`, `hammerpull`, `hopo`, `hp`, `ligado`, `lig` add a hammer-on / pull-off over the selected notes.
- [x] `tap`, `lhtap` add tapping; entering the same again removes it; `tappull` / `lhtappull` add tapping plus the slur.
- [x] In note input `vibscoop` (and `vibdip`, `hopo`, `tap`) land on the last input note; `bend` / `vibbend` are refused (nothing added, the popover stays open; UI-checked for `bend` and `vibscoop`).
- [x] Return on a guitar bend reopens Shift-O with its entry (UI-checked on a scoop; hammer-on, tapping and whammy bar are wired the same way, see Deferred).
- [x] Shift-P `harm` (and `nh`, `harm nat`, `diamond`) turns the selected notes into diamond heads; `harm circle` adds the `o`; `ah` / `harm art` / `ph` add the AH / PH line; `noharm` removes them.
- [x] `string 3` and `string3` add a circled 3 to the selected note; `string 3->` and `harm nat n1 s1 p4` are Unsupported.
- [x] Parser tests: every entry above plus the Unsupported ones (`harm p=4`, `harm open`, `string 3->`); the old Unsupported expectations in `ornamentsparser_tests.cpp` and `techniquesparser_tests.cpp` are updated.

## Tests
- `src/dorico/tests/ornamentsparser_tests.cpp`: each guitar word Ok with the right `GuitarTech`; prefixes Incomplete; `slurarp` still Unsupported.
- `src/dorico/tests/techniquesparser_tests.cpp`: harmonics grammar (kinds, circle, noharm, mixed with Unsupported words), `string N` / `stringN`, `string 0` / `string 10` Invalid, `string` Incomplete.
- UI (ui-tester, temp copy of a guitar + TAB score): the acceptance criteria above.
