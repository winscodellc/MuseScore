# 59 - Menus
Status: todo  -  Depends on: 50  -  Dorico refs: no menu-reference page exists; menu paths are collected from topics ("Choose X > Y"), see `reference/dorico-ui.md` 0.7

Implementer: **Opus** (upstream hook in the menu model).

## Goal
Replace MuseScore's menu bar with Dorico's top-level menus (File, Edit, Setup, Write, Engrave, Play, Library, View, Window, Help), filled with the items MuseScore can back.

## Dorico behaviour
See `reference/dorico-ui.md` 0.7. The list is incomplete in the manual; use it as the target and keep items not in MuseScore out (or disabled). The Window menu includes the five mode items with Cmd-1..5, zone toggles and Show Toolbar.

## MuseScore today
`src/appshell/internal/appmenumodel.cpp` builds the menu tree (File, Edit, View, Add, Format, Tools, Plugins, Help) from `UiAction` ids; items are `MenuItem`s created from actions via `makeMenuItem(actionCode)`. The native macOS menu bar is driven by the same model.

## Design
- Smallest upstream edit: one hook in `AppMenuModel::load()` so that if a `IDoricoMenuProvider` (new interface in `src/dorico/`) is registered it supplies the top-level `MenuItemList`; otherwise the existing code runs. Read `appmenumodel.cpp` first and decide between the hook and a flag; do not copy the file.
- `DoricoMenuModel` builds the Dorico tree from a table {menu, submenu, label, actionCode or commandUri}. Reuse `makeMenuItem` helpers; for `command://` commands use whatever the model already does for command-based items (verify).
- Mapping, first pass (all to verify against ids):
  - File: New, Open, Open Recent, Save, Save As, Import/Export (MusicXML, MIDI, PDF, audio), Project properties, Close.
  - Edit: Undo/Redo, Cut/Copy/Paste (Paste Special items from spec 41), Delete, Select (all, similar, range), Filter (selection filter dock), Preferences.
  - Setup: Instruments (add/edit), Parts.
  - Write: Create... (links to popovers, specs 21-36), Transpose, Respell, Rhythmic Grid, Insert Scope (stub).
  - Engrave: Page settings, Style, Reset layout, system/page breaks.
  - Play: Playback settings, Metronome, Loop, Mixer, Reset playback.
  - Library: Style dialogs, Palettes, Instruments (MuseScore has no library manager; mostly omitted).
  - View: View Options (zoom, view mode), Invisible, Formatting marks, Frames, Page numbers.
  - Window: Setup/Write/Engrave/Play/Print, Show Toolbar, Show Left/Right/Lower Zone, Hide/Restore Zones, Mixer, Piano keyboard.
  - Help: MuseScore help, Key Commands (shortcuts preferences), About.
- Keep MuseScore's Plugins/Extensions menu under Window or Help so nothing becomes unreachable.

## Deferred / unsupported
Library manager and style editors, Video, Comments, Proofreading, cues, flow management, Play templates, anything in 0.7 with no MuseScore action. List what was dropped in the as-built notes.

## Acceptance criteria
- [ ] Menu bar (native on macOS) shows the ten Dorico menus in order.
- [ ] Every item runs a working action; no dead items.
- [ ] Shortcuts shown next to items match the Dorico key map.
- [ ] Every MuseScore action that was reachable only through the old menus is still reachable (Preferences or the Window/Help menus), including plugins.

## Tests
Manual walk-through of every menu; GoogleTest for the table builder if it is pure (e.g. unknown action code reported).
