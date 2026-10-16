<!-- markdownlint-disable -->
# 54 - Write mode: lower zone
Status: done  -  Depends on: 50  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_panels/write_mode_lower_zone_r.html`, `.../write_mode_keyboard_panel_r.html`, `.../write_mode_fretboard_panel_r.html`, `.../write_mode_drum_pads_panel_r.html`, `.../properties/properties_panel_r.html`

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
Fretboard and Key Editor (stubs); Properties panel layout is MuseScore's (no horizontal group scroll, no Active/Local scope filters); Keyboard panel "respell using name above/below" buttons until spec 12; MuseScore's automatic opening of the percussion panel on drum staves (it targets a dock that no longer exists on Write); View-menu check marks for these panels; dragging a tab out of the zone.

## Acceptance criteria
- [x] Cmd-8 / status-bar arrow open and close the lower zone.
- [x] Six tabs in manual order; each real tab shows the matching MuseScore panel and works.
- [x] Stub tabs show a "Not available yet" message.
- [x] Open tab and size persist across restart.

## As built
- **One custom panel, not native dock tabs.** The dock library cannot report or set the current tab (KDDockWidgets makes the last-added tab current), so a reopened zone would always land on the last tab. `LowerZonePanel.qml` is a single `DockPanel` (`lowerZonePanel`) with its own selector row and lazily loaded `Loader`s (a visited tab stays alive). Its `tabs` property lets specs 56/57 reuse it with fewer tabs.
- **Tabs:** Properties, Keyboard, Fretboard (stub), Drum Pads, Mixer, Key Editor (stub), plus **Timeline** as a 7th MuseScore-only tab (Dorico has none).
- **State:** the selected tab is `IDoricoModeState::lowerZoneTab()`; `LowerZoneModel` mirrors it to QML and saves it in settings (`dorico/lowerZoneTab`). Open/closed state and size persist through the dock layout save.
- **MuseScore's panel toggles** (F3/F10 mixer, F8 properties, piano keyboard, percussion, timeline; menu items and the top-bar Mixer button) are caught in `DoricoActionsController` via `preDispatch` and select the matching tab (a second press on the current tab closes the zone). Mapping: `lowerZoneTabForCommand()` in `doricoshell.h`. Cmd-8 and the status-bar arrow use the generic `toggleZone` on the single dock.
- **Write page changes:** Properties moved from the right zone to this tab, so the right zone (Selection filter, History) is empty until spec 53. The old Mixer/Piano/Timeline/Percussion/Properties docks were removed from `WritePage.qml`. A previously saved Write layout that mentions them may need View > Reset layout.
- **Keyboard tab** has a button row (Advance Caret = `dorico/space`, Delete Left, Set Duration, Slur). Delete Left and Set Duration are disabled placeholders until specs 43 and 52.

- **QML snags (read before reusing the panel in specs 56/57):** the build uses `muse/framework/dockwindow_v2` (not `dockwindow/`). `DockPanel`'s default property is ONE lazily loaded `Component` (`contentComponent`) with its own id scope. So: (1) the panel body must hold exactly one object, the content `Item` (a `Component {}` declared beside it silently replaces it; the last one wins, which made the first build show only Timeline); (2) ids inside the content are invisible to root-level bindings, so `LowerZoneModel` is a root property and the loaders `Repeater` is handed to the root on load. The dock title is fixed ("Lower zone") because `DockBase` passes the title to the title bar only once, at creation.

### After the side-by-side screen comparison with Dorico 6 (2026-10-16)

- Icon tabs in every mode (Properties, Keyboard, Fretboard, Drum Pads, Mixer, Key Editor, Timeline) with the current tab's toolbar beside them; Dorico's compact Mixer (spec 57's `DoricoMixer`) in every mode. `LowerZonePanel.iconTabs` / `doricoMixer` turn either off.

## Verification status
Built, unit-tested (`dorico_tests` 24/24, `check_keymap.py` 0 failures) and checked by hand in the app: all acceptance criteria pass.

## Tests
Manual for the UI. GoogleTest (`tests/doricoshell_tests.cpp`): the lower zone is one dock, `lowerZoneTab` default and change notification, and the MuseScore-toggle-to-tab mapping.
