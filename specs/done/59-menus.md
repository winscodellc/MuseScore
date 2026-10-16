<!-- markdownlint-disable -->
# 59 - Menus
Status: pending review  -  Depends on: 50  -  Dorico refs: no menu-reference page exists; menu paths are collected from topics ("Choose X > Y"), see `reference/dorico-ui.md` 0.7

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

## As built
- **Upstream hook** (the only upstream edit for this spec), marked `// Dorico fork (spec 59)`:
  - `appshell/.../appmenumodel.h`: includes `dorico/idoricomenuarranger.h` and adds `ContextInject<mu::dorico::IDoricoMenuArranger> doricoMenuArranger`.
  - `appmenumodel.cpp` `load()`: `if (auto arranger = doricoMenuArranger()) items = arranger->arrange(items, this);` just before `setItems`.
  - An unregistered context interface resolves to null quietly (kors IoC), so without the Dorico module the menus are MuseScore's.
- **Re-parenting, not rebuilding.** `DoricoMenuArranger` (`internal/doricomenuarranger.{h,cpp}`, registered by `DoricoContext`) indexes MuseScore's built menu items by id and moves them into Dorico's ten menus from the table in `internal/doricomenu.{h,cpp}`. The table lists command URIs, MuseScore submenu ids, separators and new submenus.
  - Because the items are moved, AppMenuModel's own refreshers keep working: recent files (`menu-file-open`), extensions (`menu-extensions`), workspaces (`menu-workspaces`), undo/redo titles, command state and shortcuts (`setItems` refreshes all shortcuts, so they show the Dorico key map).
  - Dorico commands, and commands placed twice, get new items built the way `makeMenuItem` builds them.
- **Nothing is lost.** Every MuseScore item the table does not place goes to **Window > More MuseScore Commands**, grouped by its old menu. That covers convert to score, publish/share audio, the note-input and toolbar toggles (Toolbars submenu), the Add-menu note commands, and so on. Empty submenus and stray separators are dropped.
- **Menus**:
  - File: new, open, recent, close, save variants, properties, export, print, quit.
  - Edit: undo/redo/history, clipboard + Paste Special (half, double, swap), delete, select all / section, selection filter, search, resets, preferences.
  - Setup: mode, add instruments, parts.
  - Write: mode, note input, MuseScore's Add submenus (notes, intervals, tuplets, measures, text, lines, chords), transpose, Respell, voices, measures, staff explode/implode, chord realization, slashes, regroup rhythms, rehearsal marks, lyrics copy, trailing measures.
  - Engrave: mode, page settings, breaks, stretch, frames, reset beams/layout.
  - Play: mode, play, rewind, loop, metronome, playback setup, mixer, reload cache.
  - Library: style dialog / load / save, master palette, palettes.
  - View: full screen, Show submenu (invisible, frames, page borders, ...), navigator, braille.
  - Window: the five modes, Show Toolbar, the three zones, Hide/Restore Zones, Mixer, Properties, Timeline, Piano keyboard, Percussion, Workspaces, reset layout, Extensions (plugins), More MuseScore Commands.
  - Help: handbook, ask for help, check for updates, about boxes, clear online sounds cache, revert to factory, Diagnostics (dev mode).
- `dorico` links `muse_uicomponents_qml` (where `MenuItem` lives); `dorico_tests` still links.

### Dropped Dorico menu items (no MuseScore action)
- **File**: Project Statistics, Save As Project Template, Import Flows / Tempo Track (MusicXML and MIDI import is File > Open), the separate Export submenu items (one Export dialog instead), Import/Export User Settings.
- **Edit**: Paste Into Voice, Explode, Reduce, Move to Staff Above/Below, Paste Articulations (spec 41/47), Remove Rests, Select None, Go To Page / Rehearsal Mark (the search box is the closest), the Notations property submenu, Propagate Properties, Requantize, Device Setup (in Preferences).
- **Setup**: Propagate Part Formatting.
- **Write**: Create... (popovers, specs 20-36), Calculate Harp Pedals, Insert Scope, Instrument Visibility, Rhythmic Grid, Split Flow, Transform, Input Pitch.
- **Engrave**: Condensing/Note Spacing Change, Grid, Rulers, Reset Bracketing, Split Multi-bar Rest, Staff Spacing.
- **Play**: Fixed Tempo Mode, Scrub Playback, Playback/Space/Stage Template, Reset Playback Overrides.
- **Library**: everything except styles and palettes (no library manager).
- **View**: View Options, System Track, Comments, Attachment Lines, Cutaway Staff Lines, Cycle Locators, Token Placeholders, Highlight, Note and Rest Colors, Signposts.
- **Window**: New Tab, New Window, Video.
- **Help**: Key Commands (Preferences > Shortcuts).

## Verification status
Built and unit-tested (`dorico_tests` 37/37: menu order, the three refreshed submenus are moved, the Window mode items), `check_keymap.py` passes. Manual check pending: walk every menu (native macOS menu bar), check Preferences / About / Quit landed in the app menu (Qt menu roles), run a plugin from Window > Extensions, and look for "Dorico menu: no menu item or command for" warnings in the log.

## Acceptance criteria
- [ ] Menu bar (native on macOS) shows the ten Dorico menus in order.
- [ ] Every item runs a working action; no dead items.
- [ ] Shortcuts shown next to items match the Dorico key map.
- [ ] Every MuseScore action that was reachable only through the old menus is still reachable (Preferences or the Window/Help menus), including plugins.

## Tests
Manual walk-through of every menu; GoogleTest for the table builder if it is pure (e.g. unknown action code reported).
