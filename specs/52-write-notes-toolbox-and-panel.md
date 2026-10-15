# 52 - Write mode: Notes toolbox and Notes panel
Status: todo  -  Depends on: 50, 11  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notes_toolbox_r.html`, `.../write_mode_panels/write_mode_notes_panel_r.html`

Implementer: Sonnet is fine.

## Goal
The left edge of Write mode: the vertical Notes toolbox (15 buttons) and the Notes panel beside it (durations, accidentals, slur, articulations), as in the screenshot.

## Dorico behaviour
See `reference/dorico-ui.md` 2.1 and 2.2. Two columns next to each other: the narrow toolbox at the far left (icon buttons, active ones highlighted in blue) and the Notes panel (the screenshot shows durations at the top with an up/down arrow to show all values, accidentals and slur in the middle, articulations at the bottom).

| # | Toolbox button | Command / state |
|---|---|---|
| 1 | Show Left Zone | `dorico/toggle-zone-left` |
| 2 | Select (mouse input off) | tool state; stub if MuseScore has no equivalent |
| 3 | Start Note Input | `notation/note-input` (toggle; active while `isNoteInputMode`) |
| 4 | Pitch Before Duration (K) | note-input method `BY_DURATION` toggle: reuse spec 11's commands |
| 5 | Chords (Q) | `dorico/toggle-chord-mode` (spec 11), active from `IDoricoModeState` |
| 6 | Insert (I) | `dorico/toggle-insert-mode`; click-hold scope menu: only Voice is real, others disabled |
| 7 | Lock Duration (L) | spec 13 command if done, else disabled |
| 8 | Force Duration (O) | spec 13 command if done, else disabled |
| 9 | Create Voice | click-hold: Next Voice -> `dorico/next-voice`; Create Voice -> `dorico/new-voice`; Slash voice disabled |
| 10 | Dotted Notes | `dorico/cycle-dots` + click-hold one..four dots (`notation/...-dot` commands, verify ids) |
| 11 | Rests | `dorico/rest` |
| 12 | Tuplets | click-hold list: 2:3, 3:2, 4:3, 5:4, 6:4, 7:8 via MuseScore tuplet commands; x:y opens the tuplets popover (spec 31) or is disabled |
| 13 | Grace Notes | MuseScore grace-note actions (acciaccatura = slashed) |
| 14 | Tie | `notation/tie` |
| 15 | Scissors | spec 13 split-at-caret; else disabled |

Notes panel:
| Part | Content | Command |
|---|---|---|
| Durations | whole .. 64th (common set by default: half, quarter, eighth, 16th, 32nd; the arrows reveal the rest) | `dorico/set-duration-N` (spec 11) |
| Accidentals | flat, natural, sharp (toggle on selected / input pitch) | MuseScore accidental toggles (`add-accidental` commands, verify ids) |
| Slur | | `notation/add-slur` |
| Articulations | accent, marcato, stressed, unstressed, staccato, tenuto, staccatissimo, tenuto-staccato | the four articulation toggles in `doricocommands.h`; the others via MuseScore `add-articulation-*` or disabled |

## MuseScore today
`NoteInputBar.qml` + `noteinputbarmodel.cpp` already have a note-input toolbar with durations, accidentals, voices, tie, slur and articulation items and a customisation popup (`NoteInputBarCustomisePopup.qml`). It can be vertical and is used by the old page. `doricocommands.h` has duration, rest, chord/insert toggle, dots, voice and four articulation commands. Active/enabled state comes from `ICommandsState`.

## Design
- `NotesToolbox.qml`: a vertical `DockToolBar` (`location: Location.Left`, not floatable, explicit vertical orientation; see internals section 7 snag). Content is a column of `FlatButton`s from a small QML `ListModel` / C++ model listing {icon, command, tooltip, optional menu}. Reuse the existing `UiAction`/command-state binding that `NoteInputBarModel` uses so highlight and enabled state follow the commands. **Check first** whether `NoteInputBarModel` can simply be given a Dorico default config (`defaultNoteInputConfig()` `:102-160`) instead of writing a new model: that would be the shortest path.
- `NotesPanel.qml`: a `DockPanel` in the left zone (name `notesPanel`) with three sections built from the same kind of button grid.
- Icons: MuseScore's `IconCode` set only; where no icon fits, use text labels.
- Add both to `WritePage.qml`.

## Deferred / unsupported
Select tool (mouse-input off), Insert scope other than Voice, Slash voices, Lock/Force Duration and Scissors until spec 13, x:y tuplets until spec 31, "Show all notes" arrows only if the full set is hidden (otherwise omit).

## Acceptance criteria
- [ ] Left edge matches the screenshot: vertical toolbox with 15 buttons in manual order, Notes panel beside it.
- [ ] Highlight state follows note input, chord mode, insert mode, selected duration, dots.
- [ ] Every enabled button runs the command named in the table; disabled ones show a tooltip "not available yet".
- [ ] Panel's durations enter notes and set the duration of a selection.
- [ ] Cmd-7 hides/shows the toolbox panel (the toolbox itself stays).

## Tests
Manual script: enter notes by clicking panel buttons, toggle chord/insert modes, add a slur and each articulation. GoogleTest only if a new model class has pure logic.
