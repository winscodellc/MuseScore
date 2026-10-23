<!-- markdownlint-disable -->

# 102 - Menu bar parity

Status: built, pending review - Depends on: 59, 94, 95 (mode menus follow the single page's mode), 97 - Dorico refs: `reference/dorico-ui.md` (menus), the manual's "also from the X menu" mentions

Ground truth: `reference/dorico-shots/menus/notes.md` Part 1, plus the full dumps `dorico-menubar.txt` and `ours-menubar.txt`. They were taken in Write mode with a note selected, from Dorico Pro 6 on 2026-10-21. Re-dump any time with `osascript reference/dorico-shots/menus/dump-menubar.applescript "Dorico 6" 2` (`mscore 3` for ours).

## Goal

The macOS menu bar matches Dorico Pro 6's menu bar: the same top-level menus in the same order, a mode menu that changes with the mode, and Dorico's labels, order, separators and shortcuts in every menu. Spec 59 moved MuseScore items into Dorico's menu names. This spec finishes the job and replaces the stock MuseScore contents. Items with no MuseScore backing stay visible but disabled, as in spec 59.

## Dorico behaviour

### Top level

`Dorico | File | Edit | View | <mode menu> | Play | Library | Window | Help | Script`

- **Only the current mode's menu is shown** (captured 2026-10-22, `dorico-menubar-{setup,engrave,print,play}.txt`): Setup, Write and Engrave each have a mode menu in that slot. **Play and Print mode have none**: the bar is `Dorico | File | Edit | View | Play | Library | Window | Help | Script`. The Play menu is always there.
- Setup menu (25 rows): New Single Player... ⇧P, New Section Player... ⌥⇧P, Add Instrument to Player... ⇧I, Add Ensemble... ⇧E / Delete Player, Delete Empty Players... / Renumber Instruments / New Player Group, Delete Player Group / New Flow ⇧F, Delete Flow / New Layout, Delete Layout, Propagate Part Formatting..., Duplicate Layout, Renumber Layouts, Clef and Transposition Overrides... / Create Default Part Layouts, Reset Instrument Names.
- Engrave menu: Rulers, Grid, Grid Options..., Show Handles >, Crosshairs > / Format Music Frames > (Frame Break ⇧F, Make Into Frame, Lock Frame, Lock Layout, Reset Layout), Format Systems > (System Break ⇧S, Make Into System, Lock System, Move Bars to Next / Previous System . ,), Note Spacing Change..., Condensing Change..., Split Multi-bar Rest, Reset Bracketing / Staff Spacing >, Note Spacing >, Lyric Offsets >.
- Ours today: `MuseScore Studio | File | Edit | Setup | Write | Engrave | Play | Library | View | Window | Help`. It has all mode menus at once, View is in the wrong place, there is no Script menu, and the app menu has an extra "About Qt...".

### Per menu (Dorico target; `---` = separator)

| Menu | Dorico Pro 6 (Write mode, note selected) |
|---|---|
| File | New ⌘N / New From Project Template > / Open... ⌘O / Open Recent > / --- / Close Project ⇧⌘W / Save ⌘S / Save As... ⇧⌘S / Revert / --- / Close All / Save All / Save As Project Template... / --- / Import > (Flows, MIDI, MusicXML ⇧⌘O, Tempo Track) / Export > (Flows, Audio, MIDI, MusicXML, Tempo Track, Lyrics, Text) / --- / Print... / --- / Project Info... ⌘I / Project Statistics... / --- / Export User Settings... / Import User Settings... |
| Edit | Undo <action> ⌘Z / Redo ⇧⌘Z / History... / --- / Cut / Copy / Paste / Paste Special > (Paste Into Voice >, ---, Reduce, Explode, ---, Move to Staff Above ⌥N, Move to Staff Below ⌥M, ---, Duplicate to Staff Above, Duplicate to Staff Below, ---, Swap, ---, Paste Articulations) / Delete / Delete Bars / --- / Remove Rests / --- / Requantize... / --- / Select All ⌘A / Select More ⇧⌘A / Select To Start Of System / Select To End Of System / Select To Start Of Flow / Select To End Of Flow / Select None ⌘D / Filter > / --- / Go To > (Go To Bar... ⌘G, Page..., Rehearsal Mark..., ---, Previous / Next Flow) / --- / Concert Pitch / Transposed Pitch / --- / Condensing / --- / Flip F / Notations > (Bar Numbers ... Voices; the same submenus as the context menu, spec 98) / --- / Reset Appearance / Reset Position / Propagate Properties / --- / Device Setup... |
| View | Page View ⌥⌘1 / Galley View ⌥⌘2 / Fill View ⌥⌘3 / --- / Pages > / --- / System Track ⌥T / Instrument Filter Overlay / Cycle Locators / --- / Signposts > / Note And Rest Colors > / Bar Numbers > / Page Numbers / Attachment Lines / --- / Comments / Cutaway Staff Lines / Token Placeholders / --- / Highlight ... Regions, Cues / Zoom > (Zoom In ⌘=, Zoom Out ⌘-, 25%..1600%, Whole Page ⌥⌘0, Two Pages, Page Width, Page Height, Custom Zoom... ⌥Z) / Full Screen ⌃⌘F / --- / Marquee Select Tool / Hand Tool / --- / View Options... ⇧⌘V |
| Write | Instrument Visibility > / --- / Note Input ⇧N / Insert I / Insert Scope > / Input Pitch > / --- / Rhythmic Grid > / Respell > / --- / Create Voice ⇧V / Create Slash Voice / Next Voice V / --- / Transpose... / Add Notes Above or Below... / Add Intervals ⇧I / Transform > / --- / Create Bar or Barline ⇧B, Chord Symbol ⇧Q, Clef ⇧C, Cue ⇧U, Dynamic ⇧D, Figured Bass ⇧G, Fingering ⇧F, Hold or Pause ⇧H, Key Signature ⇧K, Lyrics ⇧L, Marker ⌥⇧M, MIDI Trigger Region, Ornament ⇧O, Playing Technique ⇧P, Rehearsal Mark ⇧A, Repeat ⇧R, Rhythmic Feel, System Text >, Tempo ⇧T, Time Signature ⇧M, Tuplet ;, Text > |
| Play | Record ⌘R / Retrospective Record ⌥⌘R / Fixed Tempo Mode / --- / Play From Playhead Position Space / Play From Selection P / Play From Start Of Flow / Play From Start Of Project / Play From Last Start Position ⇧Space / --- / Cycle / Locators > / --- / Scrub Playback / --- / Solo Selected Instruments ⌥S / Deactivate All Solo States / Deactivate All Mute States / --- / Key Editor Grid Resolution +/- / --- / Load Sounds For Unassigned Instruments / Playback Template... / Stage Template... / Space Template... / --- / Automation > / --- / Reset Playback Overrides / Apply Default Output Level / --- / Tracks > |
| Library | Music Fonts / Font Styles / Paragraph Styles / Character Styles / Page Sizes / Chord Diagrams / Chord Symbols / Chord Symbol Kerning Pairs / Music Symbols / Notehead Sets / Playing Techniques / Lines / Line Bodies / Line Annotations / Repeatable Symbols / Expression Maps / Percussion Maps / Playback Techniques / Engraving Options ⇧⌘E / Layout Options ⇧⌘L / Notation Options ⇧⌘N / Note Input Options ⇧⌘I / Playback Options ⇧⌘P / Instruments / Instrument Families / Instrument Score Orders / Export Library / Library Manager (all "...") |
| Window | Minimize ⌘M / Zoom / Bring All To Front / --- / New Window ⇧⌘T / Mixer / Transport / Video / Hub / --- / Setup ⌘1 / Write ⌘2 / Engrave ⌘3 / Play ⌘4 / Print ⌘5 / --- / Previous Mode / --- / Show Tabs / --- / Close Tab ⌘W / New Tab ⌘T / Next Tab ⌃Tab / Move Tab To New Window / Previous Tab / Close Other Tabs / Merge All Windows / --- / No Split / Vertical Split / Horizontal Split / --- / Show Toolbar ⌘6 / Show Left Zone ⌘7 / Show Lower Zone ⌘8 / Show Right Zone ⌘9 / Hide/Restore Zones ⌘0 / --- / Next Layout ⌥⇧] / Previous Layout ⌥⇧[ / Counterpart Layout W / --- / open projects |
| Help | Show Help ⌘/ / --- / Dorico Help / Guided Tour / Key Commands / --- / Support > / Community > / --- / Create Diagnostic Report / Check For Updates |
| Script | Run Script... / Run Last Script / macro recording / Console / Attach Debugger (all disabled stubs in ours) |

Submenus not listed here (Filter, Signposts, Note And Rest Colors, Rhythmic Grid, Respell, Transform, Text, System Text, Automation, Tracks, Locators) are taken verbatim from `dorico-menubar.txt`.

### Label rules

- Title Case, as Dorico uses ("Save As...", "Open Recent").
- `...` written as three dots, matching what Dorico shows.
- No "Dorico:" prefix.
- No shortcut text inside labels (ours embeds "(⇧⌘Z, ⌘Y)" and "(⌦, ⌫)").
- Undo and Redo name the action ("Undo Select").
- No duplicate submenus (ours has "Measures >" twice in Write).
- Open Recent entries carry no ⌘O.

## MuseScore today

- `src/dorico/internal/doricomenu.cpp`: the `MenuSpec` tables and the spec 59 hook in `appmenumodel.cpp`.
- The leftover MuseScore items go to Window > More MuseScore Commands (spec 59). Keep that submenu, since it is a fork-only escape hatch, but put it at the end of Window, after a separator.
- Mode items are disabled while a score is open (ours). Fix this: they must be enabled, with the current mode checked.

## Design

1. **Rebuild every `MenuSpec`** to the tables above. Map each item to an existing command; an item with no backing is disabled, with the tooltip "Not available yet".
   - MuseScore-only items that have no Dorico place (Save a copy, Save selection, Save to cloud, Implode, Realize chord symbols, ...) move to More MuseScore Commands. Nothing is lost.
2. **Mode menu**: one top-level slot whose content is the current mode's menu. Rebuild the menu bar on `IDoricoModeState::modeChanged`, through the spec 59 hook. Capture the Dorico Setup, Engrave and Print menus first.
3. **Order**: move View before the mode menu, and add Script with disabled stubs. Remove "About Qt..." if the platform lets us (it is a Qt default; drop the role).
4. **Shortcut display**: shortcuts come from the keymap (spec 10), so no label text carries keys. Check that each shortcut shown matches the table. A mismatch means either the keymap or this table is wrong; check against `reference/dorico-key-commands.md`.
5. **Context menu reuse**: the Edit > Notations submenus are the spec 98 `MenuSpec`s.

## Deferred / unsupported

All are disabled stubs:
- Script menu;
- tabs and split windows;
- Library editors (spec 78);
- automation (88);
- templates (64, 89);
- cues (82);
- condensing (83);
- markers and video (91);
- Retrospective Record (77).

## Acceptance criteria

- [x] The Setup, Engrave, Print and Play mode menus are captured from Dorico Pro (Print and Play have no mode menu).
- [~] Re-running `dump-menubar.applescript` on ours and diffing it against `dorico-menubar.txt` (labels, order, separators, shortcuts) shows only the documented stubs as differences. Commit the diff summary to this spec.
- [x] Switching mode swaps the mode menu (Setup / Write / Engrave / Print), and Cmd-1..5 items are enabled with the current one checked.
- [x] No label carries a "Dorico:" prefix or embedded shortcut text, and no submenu is duplicated.
- [ ] Every enabled item runs its command (spot-check 20 with the log).
- [x] `check_keymap.py` passes.

## Tests

- GoogleTest over the `MenuSpec` tables: no duplicate ids, every enabled item has a registered command.
- Manual: dump and diff, as above.

## Result (2026-10-22)

- Menu bar order, the mode-menu swap and the Window checkmarks match Dorico in all five modes (dumps in the scratchpad of the test run). Play and Print show no mode menu, as in Dorico. No `Dorico menu: no menu item or command` warnings in the log.
- Built as: `doricomenu.cpp` tables (`modeMenu(Mode)`, `notationsMenu()` from spec 98), `DoricoMenuArranger` (stubs, mode slot, title override), one-line upstream edits in `PlatformMenuBar.qml` (native menu follows the slot's title) and `AppMenuBar.qml` (an untitled top-level menu is hidden).
- Known differences, all deliberate or deferred:
  - Stubs show no shortcut (no command behind them).
  - Shortcuts follow the keymap (spec 10), not this table: Close Project is Cmd+W (the keymap wants it), Project Info Cmd+I and Flip F are deferred keys, Print shows Cmd+P, and Double / Halve Note Duration carry spec 13's keys.
  - Dorico's New From Project Template submenu, the dynamic Paste Into Voice entries and Open Recent's contents are not reproduced.
  - Transposed Pitch is a stub; Concert Pitch toggles.
- Not yet checked: the 20-item "every enabled item runs its command" spot-check (the UI test report did not come back).

## Review (2026-10-22)

- Static check: all 152 `command://` ids in the menu tables (and the reused context-menu builders) are registered and handled. New GoogleTest `DoricoMenu.EveryDoricoItemHasARegisteredCommand` covers the `command://dorico/` ones.
- Fixed: the mode commands are registered as "Dorico: Setup mode" ... again (the bare titles made two "Play" rows in the jump bar and shortcut list); the menus keep their own labels.
- Fixed: Write > Insert, Window > Show Toolbar and Show Left / Lower / Right Zone show a tick while on (`DoricoCommandsState` listens to the dock window).
- Fixed: Linux menu-bar keyboard navigation steps over the untitled mode slot in Play and Print (one line in `navigableappmenumodel.cpp`).
- Fixed: Export > Audio / MIDI / MusicXML open the export dialog on WAV / MIDI / MusicXML (`dorico/export-*` set `IExportProjectScenario::exportInfo().id`, then `project/export`).
- Fixed: Edit > Reset Appearance (`notation/reset-text-style-overrides`) is allowed in Engrave. It still resets text-style overrides only.
- Fixed: Library > Engraving Options shows Cmd+Shift+E first (Cmd+Shift+N is Dorico's Notation Options key).
- Known differences (added): Engrave Frame Break Shift+F, System Break Shift+S, Move Bars `.` / `,` and Setup New Single Player Shift+P have no keys: they would clash with Write's popover keys, and shortcuts are not per mode.
