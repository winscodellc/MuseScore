# 51 - Status bar
Status: todo  -  Depends on: 50  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/user_interface/user_interface_status_bar_r.html`, `.../user_interface_view_types_r.html`

Implementer: Sonnet is fine.

## Goal
A `DoricoStatusBar` shared by all mode pages, with the Dorico items in Dorico's order.

## Dorico behaviour
See `reference/dorico-ui.md` 0.5. Items per mode:

| Item | Modes | MuseScore backing |
|---|---|---|
| Rhythmic grid selector | Write, Engrave | Spec 12 grid service; a disabled stub until spec 12 is done |
| Concert / Transposed pitch | Write, Engrave, Play | `NotationStatusBarModel` concert pitch toggle (two buttons, one active) |
| Order (instrument score order) | Setup | Instruments panel order menu (spec 55); stub otherwise |
| Selection summary | all | New pure formatter, see Design |
| Lower-zone disclosure arrow | Write, Engrave, Play | `dorico/toggle-zone-lower` (spec 50) |
| Follow playhead | Write | `PlaybackToolBar` "auto scroll" action (`playback/...`, verify the command id) |
| Marquee / Hand tool | Write, Engrave | Hand = pan (MuseScore's mouse pan via `notation/...`; verify). Marquee = default selection. Stub the toggle if no equivalent |
| View type: Page / Galley / Fill | Write, Engrave (Engrave: Page only) | Page -> `mode-page`, Galley -> `mode-continuous` (horizontal), Fill -> `mode-float`; approximation |
| Page arrangement (4 options) | Write, Engrave | Only vertical single/spread are partly possible; others stubbed |
| Zoom: Custom, presets, out, in | all | Reuse `ZoomControl.qml` |
| MIDI indicator | all | Stub (grey light) |

## MuseScore today
`NotationStatusBar.qml` + `NotationStatusBarModel` (internals section 7): concert pitch, view mode, zoom, workspace switcher. `ViewModeControl.qml`, `ZoomControl.qml` are reusable. Selection info lives in `NotationAccessibility` (`accessibilityInfo`), a long screen-reader sentence, not a short summary.

## Design
- `DoricoStatusBar.qml` in `src/dorico/qml/MuseScore/Dorico/`, used by `DoricoPage.qml` `statusBar`. A mode page passes which items are visible (`property var items`).
- `internal/selectionsummary.h/.cpp`: pure function `QString summarise(const SelectionInfo&)` where `SelectionInfo` = page, bar range, start/end time, pitch/voice or chord text. Format: `Bars 5-6   00:00:08-00:00:10 (1.5")   Multiple selection` / `Bar 3   Eb4   V1`. A small model (`DoricoStatusModel`) fills `SelectionInfo` from `INotationSelection` (range, element) and the global context's current notation.
- Reuse `NotationStatusBarModel` for zoom, view mode and concert pitch; do not copy it.
- Placement: the bar contains a left group (grid, pitch, order, summary), a right group (follow, tools, view type, arrangement, zoom). Match the screenshot's order.

## Deferred / unsupported
Grid selector until spec 12; Marquee/Hand if no pan tool; Spreads and horizontal page arrangements; MIDI indicator; Fill view exactness (maps to float).

## Acceptance criteria
- [ ] Status bar order matches `dorico-ui.md` 0.5 and the reference screenshot.
- [ ] Concert/Transposed toggles the score pitch display like MuseScore's concert pitch.
- [ ] Page / Galley / Fill switch the view; zoom controls and presets work.
- [ ] Selecting a range shows bar range and duration; selecting a note shows bar, pitch and voice; nothing selected shows nothing.
- [ ] Items not valid for the active mode are hidden.

## Tests
GoogleTest for `summarise()` (single note, chord, range, multiple selection, empty). Manual: compare with the screenshot.
