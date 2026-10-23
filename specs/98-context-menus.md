<!-- markdownlint-disable -->

# 98 - Context menus (right-click)

Status: built, pending review - Depends on: 94, 97 (the menu follows the mode policy) - Dorico refs: `topics/glossary/c/context-menu.md`; about 148 manual pages say "You can also choose this option from the context menu" (for example `topics/selecting-and-editing-music/arranging-tools/*.md`, `filters.md`)

Ground truth: `reference/dorico-shots/menus/notes.md`, Part 2. It was transcribed from Dorico Pro 6 on 2026-10-21. Crops: `dorico-ctx-note.png`, `ours-ctx-note.png`.

## Goal

Right-clicking in the music area, and in the Setup panels, shows Dorico's menus. The menus have Dorico's items, order, submenus and look. Today ours shows MuseScore's untouched `NotationContextMenuModel`: the same 8 rows for a note, rest, dynamic or text, a measure menu for empty bars and ranges, and a page menu outside the staves.

## Dorico behaviour

### Rules

| Rule | Dorico |
|---|---|
| What the menu is built from | The **current selection**, not the item under the pointer. Right-click **never** changes the selection, even on an unselected item (verified 2026-10-22: with a slur selected, right-clicking an unselected note kept the slur selected and showed the slur menu). |
| Paste | A "Paste" row appears only when the clipboard has music. Otherwise the menu starts Cut, Copy, Paste Special. |
| No Delete | There is no Delete row, no "Edit element" and no Style row. |
| Outside the page | No menu at all, in Write and Engrave. |
| Empty bar, or a blank page area | `Filter >`, `---`, `Bar Numbers >`, `Staff Size >`. |
| Item-specific rows | Added to the list for the item types in the selection: `Edit Font...` for text, dynamics or tempo, `Dynamics >`, `Slurs >`, and so on. |
| Look | Rounded corners of about 6 px, with a 1 px lighter border and a shadow. Background about `#2e2e2e`, slightly translucent. System font about 13 px, rows about 18 px. No icons and no underlined accelerators. Shortcuts right-aligned as glyphs (⌘X, ⌥N). Check marks sit in a left gutter. The highlight is the macOS accent blue. Submenus are marked with a filled triangle. |

### Write mode, note selected (15 rows)

```
Cut                Cmd+X
Copy               Cmd+C
Paste Special  >
---
Filter         >
---
Bar Numbers    >
Beaming        >
Chord Symbols and Diagrams >
Fingering      >
Notehead       >
Octave Line    >
Pedal Lines    >
Staff Size     >
Staff          >
Stem           >
Voices         >
```

| Submenu | Items |
|---|---|
| Paste Special | Paste Into Voice > / --- / Move to Staff Above ⌥N / Move to Staff Below ⌥M / --- / Duplicate to Staff Above / Duplicate to Staff Below / --- / Swap |
| Filter | Select Only (check) / Deselect Only / --- / Notes and Chords / Grace Notes / Notes by Pitch > / Voices > / Notes in Chords > / --- / Suppressed Playback (the rest as in the Edit > Filter menu, spec 45) |
| Notehead | Circled Noteheads / Default Noteheads / Large Circled Noteheads / Larger Noteheads / Slashed Noteheads (Bottom Left to Top Right) / Slashed Noteheads (Top Left to Bottom Right) / --- / Crosses > / Triangular > / Diamonds > / Slashes > / Round and Square > / --- / Scale Degree > / Pitched > / --- / Reset to Default Notehead / --- / Toggle Round Brackets / Toggle Square Brackets |
| Voices | Default Stems Up / Default Stems Down / --- / Rhythmic Slashes > / --- / Swap Voice Contents / Change Voice > / --- / Swap Voice Order |
| Stem | Force Stem Up / Force Stem Down / Remove Forced Stem / --- / Force Centered Beam / Custom Centered Beam... / Remove Centered Beam |
| Beaming | Beam Together / Split Beam / Make Unbeamed / Split Secondary Beam / Set Partial Beam Direction > / Create Fanned Beam > / Stemlets > / Reset Beaming |
| Staff | Add Staff Above / Add Staff Below / --- / Create Ossia Above / Create Ossia Below / --- / Change Divisi... / Restore Unison / --- / Create Manual Cutaway / Create Start Cutaway / Create End Cutaway / --- / Remove Staff / --- / Manual Staff Visibility... |
| Bar Numbers | Add Bar Number Change... |
| Chord Symbols and Diagrams | Line > (Line 1 to Line 5) / --- / Generate Chord Symbols from Selection... / Generate Notes from Chord Symbols in Selection... |
| Fingering | Reset Fingering |
| Octave Line | Add Corner / Make Angled / Remove Corner/Angle |
| Pedal Lines | Add Retake / Add Change to One Quarter Depressed / Add Change to Half Depressed / Add Change to Three Quarters Depressed / Add Change to Fully Depressed / Remove Retake / --- / Split Pedal Line / Merge Pedal Lines |
| Staff Size | 60% / 75% / 100% / --- / Custom Staff Size... |
| Cross Staff | Only for a multi-staff instrument (piano), between Chord Symbols and Diagrams and Fingering. Not expanded; ship as a disabled stub. |
| Paste (row) | With music on the clipboard, `Paste ⌘V` sits between Copy and Paste Special. |

### Other selections (Write)

| Selection | Rows (after Cut / Copy / Paste Special / --- / Filter / ---) |
|---|---|
| Rest | Same as a note. |
| Dynamic | Edit Font..., --- , Bar Numbers, Dynamics, Staff Size, Staff. `Dynamics >` (Write) = Group Dynamics / Ungroup Dynamics / Remove from Group / --- / Link / Unlink / --- / Increase Dynamic Intensity / Decrease Dynamic Intensity |
| Slur | Short menu (verified 2026-10-22): Bar Numbers, Slurs, Staff Size, Staff. No Beaming, Chord Symbols, Fingering, Notehead, Octave Line, Pedal Lines, Stem or Voices. `Slurs >` = Link / Unlink |
| Tempo or text | Edit Font..., --- , Bar Numbers, Staff Size, Staff |
| Range over several bars | Edit Font..., then the note list plus Dynamics (the union of the types in the selection) |
| Empty bar, nothing selected | Filter > / --- / Bar Numbers > / Staff Size > (no Cut / Copy) |

### Engrave mode

- With nothing selected, or on the page background: the same 3-row menu as an empty bar.
- Note selected: Filter > / --- / Bar Numbers > / Staff Size > / Staff >. No Cut, Copy or Paste Special. `Staff >` = Create Manual Cutaway / Create Start Cutaway / Create End Cutaway / --- / Create Coordination Line / --- / Manual Staff Visibility...
- Dynamic selected: Filter > / --- / Edit Font... / --- / Bar Numbers > / Dynamics > / Staff Size > / Staff >. `Dynamics >` = Group Dynamics, Ungroup Dynamics, Remove from Group (all grey) / --- / Link, Unlink (grey) / --- / Align Dynamics.
- `Filter >` in Engrave has only: ✓ Select Only, Deselect Only.

### Setup mode (panels)

| Target | Dorico items |
|---|---|
| Player | Soloist / --- / Rename Player... / Add Instrument to Player... / Add Player to Group > / --- / Flows > / Layouts > / --- / Duplicate Player / Delete Player / --- / Chord Symbols > / Chord Diagrams > / --- / Combine Instruments Into Kit / Create Empty Kit / Import Kit... |
| Flow | Rename / Notation Options... ⇧⌘N / Project Info... ⌘I / --- / Layouts > / Players > / --- / Video > / --- / Duplicate Flow / Delete Flow |
| Layout | Rename / Renumber Layouts / Layout Options... ⇧⌘L / Propagate Part Formatting... / Clef and Transposition Overrides... / Flow Heading Overrides... / --- / Flows > / Players > / --- / Delete Layout / Duplicate Layout |
| Music area | No menu (spec 97 makes the score inert in Setup). |

## MuseScore today

- Score menu:
  - Built in `src/notationscene/qml/MuseScore/NotationScene/notationcontextmenumodel.cpp` (`loadItems` `:52`, `makeItemsByElementType` `:67`).
  - Created in `NotationView.qml:69` and shown at `:166-199`.
  - Triggered from `notationviewinputcontroller.cpp:987-995` (right-click) and `:487` (Menu key), and displayed by `abstractnotationpaintview.cpp:654`.
  - Right-click **selects** the item under the pointer before showing the menu.
- Setup panels:
  - `PlayersList.qml:267-294` has a 3-item menu.
  - Layouts and Flows have none.
- Existing Dorico menu specs to reuse: `src/dorico/internal/doricomenu.cpp`, with `filterMenu()` at `:40` and the Paste Special and Edit items at `:98-130`.
- Commands that already exist: Paste Special (spec 41), Filter (45), Explode / Reduce (47), voices, stem and beam modes (MuseScore actions such as `flip`, `beam-start`, `no-beam`, `beam-mid`), noteheads (MuseScore notehead groups through `Pid::HEAD_GROUP`), add or remove staff (instruments model), bar number change (measure `noOffset`).

## Design

1. **`DoricoContextMenuModel`** (`src/dorico/internal/` + QML-exposed, an `AbstractMenuModel` subclass):
   - Input: the mode, the selection's element types, the clipboard state (the spec 96 cache), and whether the click was on the page.
   - Output: `MenuItemList`, built from `MenuSpec` tables that are shared with `doricomenu.cpp`, so the Edit menu and the context menu stay in sync.
   - Each item maps to an existing command code. An item with no backing is shown **disabled**, with the tooltip "Not available yet" (Dorico parity for the label, honesty for the action). Pass the item list through the spec 97 policy.
2. **Hook** (tiny upstream edit): `NotationView.qml` gets `property var contextMenuModelOverride: null`. `DoricoShellPage` sets it to a `DoricoContextMenuModel`. `showContextMenu` uses the override when it is set. If a QML property cannot carry it, use one virtual hook in `AbstractNotationPaintView::showContextMenu`, marked `// Dorico fork`.
3. **Selection rule**:
   - With a Dorico menu, right-click on an item that is already selected keeps the selection.
   - Right-click on an unselected item: match whatever Dorico does (verify).
   - Right-click outside the page shows nothing.
   - Implement this in the input controller path behind the override flag (`notationviewinputcontroller.cpp:987`).
4. **Look**: a menu style that matches the table above.
   - Preferred: `QMenu` popup built from the `MenuItemList` (a native-looking menu, like Dorico's).
   - Alternative: a Dorico `StyledMenu` variant with no icons, no accelerator underline, rows of about 20 px, and background `#2e2e2e`.
   - Decide by comparing crops with `dorico-ctx-note.png`. Whichever is chosen is reused by the Setup panel menus.
5. **Setup panel menus**: replace the `PlayersList.qml` menu, and add menus to the Layouts and Flows lists, using the tables above.
   - Wire the existing actions: rename, delete, duplicate (if available), Layout Options (= Style / Page settings), Project Info (score properties).
   - The rest are disabled stubs.

## Deferred / unsupported

These appear disabled, with their labels kept:
- ossia and cutaway items;
- Change Divisi, Restore Unison;
- Custom Centered Beam, Force Centered Beam;
- Create Fanned Beam, Stemlets;
- Scale Degree and Pitched noteheads (if MuseScore lacks the group);
- kits;
- Video;
- Propagate Part Formatting;
- Clef and Transposition Overrides, Flow Heading Overrides.

Also disabled as built: `Edit Font...` (no Dorico-style font dialog), `Cross Staff >` (not shown at all), the sub-submenus of Notehead (Crosses, Triangular, Diamonds and Slashes are single rows that apply the main group; Larger, Round and Square, Scale Degree, Pitched are stubs), Chord Symbols > Line and Generate rows, Octave Line, Pedal Lines, Dynamics, Slurs, Staff Size, Bar Numbers, Fingering, Rename Player, Soloist, Duplicate / Delete Flow, Rename Flow.

The Roadmap specs (82-93) enable them later.

## Acceptance criteria

- [x] The missing submenus (Chord Symbols and Diagrams, Fingering, Octave Line, Pedal Lines, Staff Size, Dynamics, Slurs, Engrave note and dynamic) are captured from Dorico Pro and added to this spec before coding (2026-10-22, `notes.md`).
- [x] In Write, a note selection shows the 15-row menu in Dorico's order, and every submenu matches the tables.
- [x] A dynamic, a slur, tempo text and a range each show their Dorico variant. An empty bar shows the 3-row menu. Outside the page there is no menu.
- [x] There is no Delete, Edit element or Style row anywhere in the score menu.
- [x] Paste appears only when the clipboard holds music.
- [x] Every enabled item works (checked: stems, Crosses / Default noteheads, Make Unbeamed, Filter; the rest dispatch existing commands). Unbacked items are disabled. They carry the tooltip "Not available yet" (`QAction::toolTip`, `setToolTipsVisible`), but it **did not show** under uictl's synthetic mouse moves, so it is unverified with a real mouse.
- [x] Engrave and Setup menus follow the mode (no editing items in Engrave).
- [x] The Setup Players, Flows and Layouts lists have Dorico's menus.
- [ ] (not measured with a pixel tool; by eye in the 2026-10-22 run: dark, rounded, no icons, no underlines) A side-by-side crop with `dorico-ctx-note.png` matches: row height within 2 px, no icons, same colours within a small tolerance.

## Review (2026-10-22)

- Fixed (UI-checked): the new player menu dropped the only Setup-shell switches for MuseScore's hidden instruments and shared parts. Added after Delete Player (not in Dorico): Hide / Show Instrument and Disable / Enable Part; the one that does not apply is disabled (both Part rows on a row that is not a shared part).
- Fixed (UI-checked): in note input, a right-click removed the note under the pointer. With `INTERACTION_DORICO_RIGHT_CLICK` it now opens the menu.
- Fixed: "Not available yet" is only on stub rows (`dorico-ctx-*`), not on commands disabled by their state, and uses `muse::qtrc("dorico", ...)`.
- Fixed: each popup freed none of the previous MenuItem tree (`replaceItems`).
- Fixed (UI-checked, approximated): a right double-click on a layout card switched the layout.
- Fixed (UI-checked): stem and notehead commands are disabled with only rests selected.
- Open: Escape does not close the score menu while note input is on (it reaches the score as `notation/cancel`).

## Tests

- GoogleTest `DoricoContextMenu*`: (mode, selection types, clipboard) → item ids, for each table row above.
- Manual, run with the `ui-tester` agent: right-click each target in Write, Engrave and Setup, and compare against the tables.
