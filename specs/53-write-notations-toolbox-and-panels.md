# 53 - Write mode: Notations toolbox and panels
Status: todo  -  Depends on: 50, 20  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_toolbox_r.html`, `.../write_mode_notations_input/*_panel_r.html` (one per panel), `.../write_mode_panels/write_mode_right_zone_r.html`

Implementer: **Opus** (judgement mapping Dorico's panel sections onto MuseScore's palettes).

## Goal
The right edge of Write mode: a vertical icon strip (Notations toolbox) and, next to it, a collapsible right-zone panel that shows the panel of the chosen button, as in the screenshot.

## Dorico behaviour
See `reference/dorico-ui.md` 2.3. Two modes of the strip, chosen by a **Panels / Popovers** switch at its top:

| Mode | Buttons | Click |
|---|---|---|
| Panels | Clefs, Key Signatures, Time Signatures, Tempo, Dynamics, Ornaments, Repeat Structures, Bars and Barlines, Holds and Pauses, Playing Techniques, Lines, Cues, Video, Comments, Proofreading | show that panel in the right zone (toggle) |
| Popovers | Clefs, Key Signatures, Time Signatures, Tempo, Dynamics, Ornaments, Repeat Structures, Bars and Barlines, Holds and Pauses, Playing Techniques, Cues, Rehearsal Marks, Text, Lyrics, Chord Symbols, Note Tools, Fingering, Figured Bass, MIDI Trigger Region | open that popover (Rehearsal Marks inputs directly, Text opens the text editor) |

Panel contents (sections): see the table in `dorico-ui.md` 2.3.

## MuseScore today
- Palettes: `PaletteTree -> Palette -> PaletteCell`, `PalettesPanel` in the dock page, applied through `UserPaletteController::applyPaletteElement` (`palette/internal/paletteprovider.cpp`). The master palette has Clefs, Key signatures, Time signatures, Dynamics, Tempo, Lines, Arpeggios & glissandos, Articulations, Ornaments, Repeats & jumps, Breaths & pauses, Barlines, Text, Layout, etc. **Read the palette creation code** (`palette/internal/palettecreator.cpp`, verify the file) to get exact palette ids and cell names before mapping.
- Popover commands come from specs 20-36 (`dorico/popover-*`); which exist depends on what is done.
- Docks: one right-zone `DockPanel` with `location: Right`; strip = narrow vertical `DockToolBar` (internals section 7).

## Design
- `NotationsToolbox.qml`: vertical `DockToolBar` at `Location.Right`, not floatable. Top: a two-state button (Panels/Popovers). Below: a column of icon buttons from a model (`NotationsToolboxModel`: list of {id, icon, tooltip, panelId, popoverCommand}). The active panel button is highlighted.
- `NotationsPanel.qml`: a `DockPanel` named `notationsPanel` in the right zone, closed by default. Content is a `Loader` on `currentPanelId`, a component per panel. Clicking the active button closes the zone (`dock-set-open`).
- Panels are **filtered, regrouped views of existing palette cells**, not new engraving code:
  - A table `panelspec.json` (or a C++ array in `internal/`) maps each Dorico panel and section to a list of palette cell ids (e.g. Dynamics > Immediate Dynamics = `p, mp, mf, f, ...` cells of the Dynamics palette).
  - A `DoricoPanelModel` builds sections from that table by looking cells up in `IPaletteProvider`/`PaletteTree`, and click calls the same `UserPaletteController::applyPaletteElement` the palette uses. Reuse the palette cell grid QML (`PaletteGridView` or similar; verify the name) if it can be embedded.
  - Sections with no MuseScore cells are omitted, not faked.
- Panels with no backing model are **stubs** (centered "Not available yet" label, no buttons): Cues, Video, Proofreading, Comments (unless MuseScore's comments feature maps; check), Tonality Systems and Custom Key Signatures blocks, Harp Pedals diagram, Tap Tempo/Tempo Equation unless MuseScore tempo text covers them.
- Popovers-mode buttons dispatch `command://dorico/popover-<name>`; a button whose command is not registered yet is disabled.
- Do the two sections tied to a form (Time Signatures "Create Time Signature", Key Signatures "Input") by calling the existing popovers (specs 22/23); until then they are hidden.

## Deferred / unsupported
Cues, Video, Proofreading panels; Comments if no mapping; Tonality System management; Custom key signatures; Harp pedal diagrams; Rehearsal Marks and MIDI Trigger Region popovers (MIDI trigger deferred globally); panels' modifier sections that have no palette cell.

## Acceptance criteria
- [ ] Right edge matches the screenshot: vertical icon strip, collapsible panel.
- [ ] All 15 panel buttons and 19 popover buttons exist in manual order; unavailable ones are visibly disabled with a tooltip.
- [ ] For each implemented panel, clicking an item applies it to the selection exactly as the MuseScore palette does (undoable).
- [ ] Panel sections are named as in the manual (checked against `dorico-ui.md`).
- [ ] Cmd-9 toggles the right zone; the strip stays.
- [ ] A coverage table (panel/section -> palette cells or "stub") is written into this spec's as-built notes.

## Tests
GoogleTest: the section-mapping function (spec table -> resolved cell ids, unknown id reported). Manual: apply one item from each panel; compare layout with the screenshot.
