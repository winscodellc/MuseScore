<!-- markdownlint-disable -->

# 52 - Write mode: Notes toolbox and Notes panel

Status: pending review - Depends on: 50, 11 - Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notes_toolbox_r.html`, `.../write_mode_panels/write_mode_notes_panel_r.html`

Implementer: Sonnet is fine.

## Goal

The left edge of Write mode: the vertical Notes toolbox (15 buttons) and the Notes panel beside it (durations, accidentals, slur, articulations), as in the screenshot.

## Dorico behaviour

See `reference/dorico-ui.md` 2.1 and 2.2. Two columns next to each other: the narrow toolbox at the far left (icon buttons, active ones highlighted in blue) and the Notes panel (the screenshot shows durations at the top with an up/down arrow to show all values, accidentals and slur in the middle, articulations at the bottom).

| #   | Toolbox button            | Command / state                                                                                                                     |
| --- | ------------------------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| 1   | Show Left Zone            | `dorico/toggle-zone-left`                                                                                                           |
| 2   | Select (mouse input off)  | tool state; stub if MuseScore has no equivalent                                                                                     |
| 3   | Start Note Input          | `notation/note-input` (toggle; active while `isNoteInputMode`)                                                                      |
| 4   | Pitch Before Duration (K) | note-input method `BY_DURATION` toggle: reuse spec 11's commands                                                                    |
| 5   | Chords (Q)                | `dorico/toggle-chord-mode` (spec 11), active from `IDoricoModeState`                                                                |
| 6   | Insert (I)                | `dorico/toggle-insert-mode`; click-hold scope menu: only Voice is real, others disabled                                             |
| 7   | Lock Duration (L)         | spec 13 command if done, else disabled                                                                                              |
| 8   | Force Duration (O)        | spec 13 command if done, else disabled                                                                                              |
| 9   | Create Voice              | click-hold: Next Voice -> `dorico/next-voice`; Create Voice -> `dorico/new-voice`; Slash voice disabled                             |
| 10  | Dotted Notes              | `dorico/cycle-dots` + click-hold one..four dots (`notation/...-dot` commands, verify ids)                                           |
| 11  | Rests                     | `dorico/rest`                                                                                                                       |
| 12  | Tuplets                   | click-hold list: 2:3, 3:2, 4:3, 5:4, 6:4, 7:8 via MuseScore tuplet commands; x:y opens the tuplets popover (spec 31) or is disabled |
| 13  | Grace Notes               | MuseScore grace-note actions (acciaccatura = slashed)                                                                               |
| 14  | Tie                       | `notation/tie`                                                                                                                      |
| 15  | Scissors                  | spec 13 split-at-caret; else disabled                                                                                               |

Notes panel:

| Part          | Content                                                                                              | Command                                                                                                        |
| ------------- | ---------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------- |
| Durations     | whole .. 64th (common set by default: half, quarter, eighth, 16th, 32nd; the arrows reveal the rest) | `dorico/set-duration-N` (spec 11)                                                                              |
| Accidentals   | flat, natural, sharp (toggle on selected / input pitch)                                              | MuseScore accidental toggles (`add-accidental` commands, verify ids)                                           |
| Slur          |                                                                                                      | `notation/add-slur`                                                                                            |
| Articulations | accent, marcato, stressed, unstressed, staccato, tenuto, staccatissimo, tenuto-staccato              | the four articulation toggles in `doricocommands.h`; the others via MuseScore `add-articulation-*` or disabled |

## MuseScore today

`NoteInputBar.qml` + `noteinputbarmodel.cpp` already have a note-input toolbar with durations, accidentals, voices, tie, slur and articulation items and a customisation popup (`NoteInputBarCustomisePopup.qml`). It can be vertical and is used by the old page. `doricocommands.h` has duration, rest, chord/insert toggle, dots, voice and four articulation commands. Active/enabled state comes from `ICommandsState`.

## Design

- `NotesToolbox.qml`: a vertical `DockToolBar` (`location: Location.Left`, not floatable, explicit vertical orientation; see internals section 7 snag). Content is a column of `FlatButton`s from a small QML `ListModel` / C++ model listing {icon, command, tooltip, optional menu}. Reuse the existing `UiAction`/command-state binding that `NoteInputBarModel` uses so highlight and enabled state follow the commands. **Check first** whether `NoteInputBarModel` can simply be given a Dorico default config (`defaultNoteInputConfig()` `:102-160`) instead of writing a new model: that would be the shortest path.
- `NotesPanel.qml`: a `DockPanel` in the left zone (name `notesPanel`) with three sections built from the same kind of button grid.
- Icons: MuseScore's `IconCode` set only; where no icon fits, use text labels.
- Add both to `WritePage.qml`.

## Deferred / unsupported

Select tool (mouse-input off), Insert scope other than Voice, Slash voices, Lock/Force Duration and Scissors until spec 13, x:y tuplets until spec 31, "Show all notes" arrows only if the full set is hidden (otherwise omit).

## As built

- **No new model.** `NoteInputBarModel` hard-codes its tool-config key and item list, so the toolbox and panel use the fork's `DoricoToolsModel` (a list of command URIs whose MenuItems follow the command state). It gained an optional `titles` list for menu entries.
- **Notes toolbox** (`NotesToolbox.qml`, a non-floatable vertical `DockToolBar` `notesToolbox` at `Location.Left`; a non-floatable toolbar keeps the orientation it is given), 15 buttons in manual order:
  Show Left Zone `dorico/toggle-zone-left`; Select (placeholder); Start Note Input `dorico/return` (spec 11's start/stop); Pitch Before Duration `notation/toggle-note-input-by-duration`; Chords `dorico/toggle-chord-mode`; Insert `dorico/toggle-insert-mode` (hold: Voice real, Player / Global / Global Adjustment placeholders); Lock Duration `notation/toggle-note-input-repitch` (spec 11 maps L to re-pitch); Force Duration (placeholder); Create Voice = `dorico/next-voice` (hold: Create Voice `dorico/new-voice`, Create Slash Voice placeholder, Next Voice); Dotted Notes `dorico/cycle-dots` (hold: `notation/toggle-dot`..`toggle-dot4`); Rests `dorico/rest`; Tuplets = triplet (hold: 2:3 duplet, 3:2 triplet, 4:3 quadruplet, 5:4 quintuplet, 6:4 sextuplet, 7:8 septuplet, x:y = MuseScore's custom tuplet dialog until spec 31); Grace Notes = appoggiatura (hold: Unslashed / Slashed = acciaccatura); Tie `notation/toggle-tie`; Scissors (placeholder).
- Click-and-hold opens the menu (the MouseArea's `pressAndHold`, which also suppresses the click).
- **Highlight state**: `DoricoCommandsState` now reports `checked` for `dorico/return` (note input on), `toggle-chord-mode` and `toggle-insert-mode`, refreshed on `modesChanged` and `noteInputStateChanged`. MuseScore's own commands already report checked for the input method, duration, dots, accidentals and articulations.
- **Articulation highlights** (fix after review): the four Dorico articulation commands report `checked` from `INotationCommandsController::currentArticulations()` (the input articulations in note input, else those on the selected notes; tenuto-staccato needs both parts because MuseScore splits combined articulations). `DoricoCommandsState` also refreshes on `selectionChanged`. MuseScore refreshed its own four only on note-input state changes, so selecting another note outside note input left them stale; one line in `notationscene/internal/notationcommandsstate.cpp` (`// Dorico fork (spec 52)`) refreshes them on selection change too.
- **Articulation toggling off** (second fix after review): `Chord::updateArticulations` in Remove mode only removed accent / marcato / tenuto / staccato, so a second click on staccatissimo, stressed or unstressed did nothing. `engraving/dom/chord.cpp` now removes any other articulation too (matched after flipping Below symbols to Above), and `ARTICULATION_PLACEMENTS` in `articulation.cpp` gained the stress / unstress above-below pairs. Test: `engraving_tests` `Engraving_NoteTests.ToggleOffAnyArticulation` (fails without the fix). Tenuto-staccato toggles its two parts together in `DoricoNoteInputController::onToggleArticulation` (MuseScore splits combined articulations).
- **Durations** (after review, from the Dorico screenshot): Show/Hide All Notes is one toggle with a chevron above and below the durations (Lucide `chevron-up/down`, 28 px). Common set whole..32nd (our choice; the manual only says "most common"); all = maxima..1/1024, maxima an unavailable placeholder (MuseScore has no maxima). Accidentals in Dorico's order: natural, flat, sharp.
- **Notes panel** (`NotesPanel.qml`, dock `notesPanel`, the Write left zone): Show/Hide All Notes arrow; durations `notation/set-duration-*` (common: half..32nd; all: breve..128th); Flat / Natural / Sharp / Slur; the eight articulations (accent = `toggle-sforzato`, marcato, staccato, tenuto from MuseScore; stressed, unstressed, staccatissimo, tenuto-staccato from spec 11, shown as text buttons since MuseScore has no icons for them).
- **Write page**: MuseScore's note-input bar and the Layout (instruments) panel are gone from Write (instruments live in Setup, spec 55). Palettes moved to the right zone, closed by default. `zoneDocks(Write)`: left = `notesPanel`; right = palettes, selection filter, history (spec 53 puts the Notations panel first).
- The lower zone Keyboard tab's Set Duration stays a placeholder: MuseScore has no "apply the input duration to the selection" command.

### After the side-by-side check with Dorico 6 (2026-10-16, second pass)

- Bravura icons use `Text.CurveRendering` (grayscale): `QtRendering` still drew them with LCD antialiasing on macOS, which tinted stems and noteheads blue and orange. Longa is Bravura's `mensuralWhiteLonga` (U+E95D, like the maxima) instead of MuseScore's icon-font glyph.
- 1/128..1/1024 are stacked fractions, as in Dorico (`DoricoButton` `frac:<n>` icons).
- Toolbox divider lines after Show Left Zone, Insert and Force Duration (`groupEnd` in `NotesToolbox.qml`).
- Articulations: two columns with a divider between them, in Dorico's order (left accent, marcato, stressed, unstressed; right staccato, tenuto, staccatissimo, tenuto-staccato). The list is row by row because the Flow lays out rows.
- Not done: highlighting the input duration with nothing selected. MuseScore resets the input duration on every selection change outside note input (`InputState::update`), so there is nothing to show. It needs a remembered "last input duration" in engraving, which also changes the duration note input starts with.

## Verification status

Built and unit-tested (`dorico_tests` 29/29), QML linted. Manual check in the app pending (the vertical toolbar orientation after View > Reset layout is worth a look: `DockToolBarView::resetToDefault` forces horizontal).

## Acceptance criteria

- [x] Left edge matches the screenshot: vertical toolbox with 15 buttons in manual order, Notes panel beside it.
- [x] Highlight state follows note input, chord mode, insert mode, selected duration, dots.
- [x] Every enabled button runs the command named in the table; disabled ones show a tooltip "not available yet".
- [x] Panel's durations enter notes and set the duration of a selection.
- [x] Cmd-7 hides/shows the toolbox panel (the toolbox itself stays).
- [x] Articulations from the toolbox panel are toggle buttons (all 8 checked in the app, 2026-10-16: second click removes, highlight follows).
- [x] Arrow at the top of the toolbox panel toggles more duration options (maxima..1/1024).

## Tests

Manual script: enter notes by clicking panel buttons, toggle chord/insert modes, add a slur and each articulation. GoogleTest only if a new model class has pure logic.
