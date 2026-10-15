# 54 - Write mode: lower zone
Status: todo  -  Depends on: 50  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_panels/write_mode_lower_zone_r.html`, `.../write_mode_keyboard_panel_r.html`, `.../write_mode_fretboard_panel_r.html`, `.../write_mode_drum_pads_panel_r.html`, `.../properties/properties_panel_r.html`

Implementer: Sonnet is fine.

## Goal
A single lower-zone dock panel with a tab selector (Properties, Keyboard, Fretboard, Drum Pads, Mixer, Key Editor) toggled by Cmd-8 and the status-bar disclosure arrow.

## Dorico behaviour
See `reference/dorico-ui.md` 2.4. One zone, one visible panel at a time, chosen by selector buttons at the top left.

| Tab | MuseScore component |
|---|---|
| Properties | `PropertiesPanel` (dock panel in `NotationPage.qml`), laid out horizontally if possible; else as is |
| Keyboard | `PianoKeyboardPanel` plus the toolbar buttons Advance Caret / Delete Left / Set Duration / Slur (reuse commands from spec 52) |
| Fretboard | Stub: "Not available yet" (MuseScore has no fretboard input panel) |
| Drum Pads | MuseScore `PercussionPanel` (`notationscene` percussion panel, verify path) |
| Mixer | `MixerPanel` |
| Key Editor | Stub (deferred) |

## MuseScore today
`NotationPage.qml:402-568` has Mixer, Piano keyboard, Timeline and Percussion as separate bottom docks (group `HORIZONTAL_PANELS`), and Properties as a side panel. Toggling uses `dock-toggle` by panel name.

## Design
- Smallest approach: keep the existing panels as docks but make them tabs of one frame. The dock framework already tabs panels in the same group in one frame (`DockFrame`/`DockTabBar`, horizontal). Put the five real panels in the Dorico `WritePage` with the same `groupName` and the bottom location, order matching the manual, and let the framework draw the tab bar. **Try this first**; it needs no new component.
- If tab styling is too different, add `LowerZoneSelector.qml`: a custom row of buttons inside one `DockPanel` that swaps a `StackLayout` between the embedded panel contents (embedding the existing panel QML types).
- Stubs: two small QML components with a centered label.
- `dorico/toggle-zone-lower` (spec 50) opens/closes the lower dock group; the status-bar arrow uses the same command.

## Deferred / unsupported
Fretboard and Key Editor (stubs); Properties panel layout is MuseScore's (no horizontal group scroll, no Active/Local scope filters); Keyboard panel "respell using name above/below" buttons until spec 12.

## Acceptance criteria
- [ ] Cmd-8 / status-bar arrow open and close the lower zone.
- [ ] Six tabs in manual order; each real tab shows the matching MuseScore panel and works.
- [ ] Stub tabs show a "Not available yet" message.
- [ ] Open tab and size persist across restart.

## Tests
Manual only.
