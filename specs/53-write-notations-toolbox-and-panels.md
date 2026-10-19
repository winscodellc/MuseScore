<!-- markdownlint-disable -->

# 53 - Write mode: Notations toolbox and panels

Status: pending review - Depends on: 50, 20 - Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_toolbox_r.html`, `.../write_mode_notations_input/*_panel_r.html` (one per panel), `.../write_mode_panels/write_mode_right_zone_r.html`

Implementer: **Opus** (judgement mapping Dorico's panel sections onto MuseScore's palettes).

## Goal

The right edge of Write mode: a vertical icon strip (Notations toolbox) and, next to it, a collapsible right-zone panel that shows the panel of the chosen button, as in the screenshot.

## Dorico behaviour

See `reference/dorico-ui.md` 2.3. Two modes of the strip, chosen by a **Panels / Popovers** switch at its top:

| Mode     | Buttons                                                                                                                                                                                                                                                      | Click                                                                           |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------- |
| Panels   | Clefs, Key Signatures, Time Signatures, Tempo, Dynamics, Ornaments, Repeat Structures, Bars and Barlines, Holds and Pauses, Playing Techniques, Lines, Cues, Video, Comments, Proofreading                                                                   | show that panel in the right zone (toggle)                                      |
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

## As built

- **Notations toolbox** (`NotationsToolbox.qml`, non-floatable vertical `DockToolBar` `notationsToolbox` at `Location.Right`). The top button switches Panels / Popovers.
  - Panels mode: 15 buttons in manual order. A click shows that panel in the right zone; clicking the active button again closes the zone. The active panel is highlighted.
  - Popovers mode: 19 buttons in manual order. They dispatch `command://dorico/popover/<id>` with spec 20's ids (`clef`, `keysig`, `timesig`, `tempo`, `dynamics`, `ornaments`, `repeats`, `bars`, `holds`, `techniques`, `cues`, `lyrics`, `chordsymbols`, `notetools`, `fingering`, `figuredbass`, `midi-trigger`). They are unavailable until those commands are registered (DoricoToolsModel placeholders). Rehearsal Marks (`notation/add-rehearsalmark-text`) and Text (`notation/add-staff-text`) already work, as in Dorico they input directly.
- **Notations panel** (`NotationsPanel.qml`, dock `notationsPanel`, first in the Write right zone, closed by default). Each section is MuseScore's own `PaletteGridView`. `DoricoPanelModel` (`qml/MuseScore/Dorico/doricopanelmodel.{h,cpp}`) builds a small `PaletteTree` (one `Palette` per section, copying grid size / mag / type of the source palette) from `PaletteCreator`'s master palettes. It wraps the tree in `PaletteTreeModel` + `UserPaletteController` (not user-editable), so a click is `applyPaletteElement` exactly as in the palette (undoable). Sections with no cells are left out; stub panels show `DoricoPlaceholder`.
- **The mapping is a pure table** (`internal/notationspanels.{h,cpp}`): `{panel, section title, source palette, element types, name patterns, rest}`. Cells are matched by element type and untranslated cell name (cell ids are runtime counters), with exact, `prefix*` and `*substring*` patterns. "rest" sections take what the other sections of the panel left. Name patterns that match nothing are logged (`LOGW`) so a renamed MuseScore cell is noticed.
- `zoneDocks(Write, Right)` = notations, palettes, selection filter, history (Cmd-9 opens the Notations panel when nothing was open before).
- `dorico_qml` links `palette` and adds `src/palette` to its include path (palette's internal headers include their module headers without a path). The pure table lives in `dorico` (tested); `dorico` itself does not link palette, because palette pulls in notation widgets that `dorico_tests` cannot link.

### Coverage (panel / section -> MuseScore master-palette cells)

| Panel                               | Section                                                        | Cells                                                                                     |
| ----------------------------------- | -------------------------------------------------------------- | ----------------------------------------------------------------------------------------- |
| Clefs                               | Common                                                         | Clefs: Treble, Bass, Alto, Tenor, Treble 8va bassa, Bass 8va bassa, Percussion, Tablature |
|                                     | Uncommon                                                       | Clefs: all others except archaic                                                          |
|                                     | Archaic Clefs                                                  | Clefs: names containing "century)" (French 18th/20th century, 19th century)               |
|                                     | Octave Lines                                                   | Lines: ottavas                                                                            |
| Key Signatures                      | Key Signatures                                                 | Key signatures palette                                                                    |
|                                     | Accidentals                                                    | Accidentals palette                                                                       |
|                                     | Tonality System, Used in This Flow, Custom, Input              | stub (omitted)                                                                            |
| Time Signatures                     | Common                                                         | Time signatures palette                                                                   |
|                                     | Used in This Flow, Create Time Signature                       | omitted (form: spec 22)                                                                   |
| Tempo                               | Absolute Tempo Change                                          | Tempo: metronome marks ("= 80 BPM")                                                       |
|                                     | Gradual Tempo Change                                           | Tempo: accel., rall., rit., ... (gradual tempo change lines)                              |
|                                     | Reset Tempo                                                    | Tempo: a tempo, tempo primo                                                               |
|                                     | Tempo Equation                                                 | Tempo: metric modulations                                                                 |
|                                     | Used in This Flow, Tap Tempo, Relative Tempo Change, modifiers | omitted (no cells)                                                                        |
| Dynamics                            | Immediate Dynamics                                             | Dynamics: all dynamics not below                                                          |
|                                     | Gradual Dynamics                                               | Dynamics: hairpins, cresc./dim. lines                                                     |
|                                     | Force/Intensity of Attack                                      | Dynamics: sf, sfz, sff, sffz, sfff, sfffz, rfz, rf, fz                                    |
|                                     | Combined Dynamics                                              | Dynamics: fp, pf, sfp, sfpp                                                               |
| Ornaments                           | Baroque and Classical                                          | Ornaments palette                                                                         |
|                                     | Arpeggiation                                                   | Arpeggios & glissandos: arpeggios                                                         |
|                                     | Glissandi                                                      | Arpeggios & glissandos: glissandos, slides (chord lines)                                  |
|                                     | Jazz, Guitar                                                   | omitted (no matching cells)                                                               |
| Repeat Structures                   | Repeat Endings                                                 | Repeats & jumps: voltas                                                                   |
|                                     | Repeat Jumps                                                   | Repeats & jumps: jumps (D.C., D.S., To Coda)                                              |
|                                     | Repeat Sections                                                | Repeats & jumps: markers (Segno, Coda, Fine)                                              |
|                                     | Tremolos                                                       | Tremolos palette                                                                          |
|                                     | Bar Repeats                                                    | Repeats & jumps: measure repeats                                                          |
|                                     | Rhythm Slashes, Numbered Bars                                  | omitted                                                                                   |
| Bars and Barlines                   | Create Barline                                                 | Barlines palette                                                                          |
|                                     | Insert Bars, Insert Bar Rest                                   | omitted (form, no cells)                                                                  |
| Holds and Pauses                    | Fermatas / Breath Marks / Caesuras                             | Breaths & pauses: fermatas / breaths / names containing "caesura"                         |
| Playing Techniques                  | Common                                                         | Text: playing technique annotations (pizz., arco, mute, ...)                              |
|                                     | Brass                                                          | Articulations: muted (closed), open, half-open                                            |
|                                     | Keyboard                                                       | Keyboard palette (pedals, chord brackets)                                                 |
|                                     | Strings                                                        | Articulations: up/down bow, harmonic, snap pizzicato (thumb position is in Guitar)                  |
|                                     | Guitar                                                         | Guitar palette                                                                            |
|                                     | Harp Pedals                                                    | Harp palette (pedal diagrams)                                                             |
|                                     | Wind, Unpitched/Pitched Percussion, Choral                     | omitted (no cells)                                                                        |
| Lines                               | Horizontal                                                     | Lines: text lines, let ring, palm mute, vibrato, note-anchored lines                      |
|                                     | Vertical                                                       | Lines: chord brackets                                                                     |
| Cues, Video, Comments, Proofreading |                                                                | stub                                                                                      |

### After the side-by-side check with Dorico 6 (2026-10-16)

- Two mode buttons at the top, as in Dorico: Panels (Lucide `palette`) and Popovers (`keyboard`); the current mode is highlighted.
- Fix: closing the zone by clicking the active panel button left the button highlighted. `WritePage` passes the panel id only while the dock is visible (a DockPanel's `visible` follows its open state), so Cmd-9 closing it clears the highlight too.
- Still open (medium): Dorico's collapsible sections ("Common Clefs" etc., Uncommon / Archaic closed by default) with 2-3 large cells per row and no "Notations" title bar.

## Verification status

UI-tested in the app on 2026-10-18 (headless Xvfb, `ui-tester`). All acceptance checks pass except the toolbox-arrow item, which waits for the user's screenshot.
Fixed during that test: the Popovers strip (19 buttons, ~850 px) did not fit a short window and set the window's minimum height; the buttons below the Panels/Popovers switch now scroll (`NotationsToolbox.qml`, `availableHeight` from `WritePage.qml`). Cmd-9 before any strip click shows Clefs instead of the stub. Strings no longer asks for "Thumb position" (MuseScore keeps it in the Guitar palette, which the Guitar section shows). `DoricoButton` no longer logs `slice` / `startsWith` errors.

## Acceptance criteria

- [ ] (It kind of works but I want that arrow bigger. I will attach an image of the toolbox) Right edge matches the screenshot: vertical icon strip, collapsible panel.
- [x] All 15 panel buttons and 19 popover buttons exist in manual order; unavailable ones are visibly disabled with a tooltip.
- [x] For each implemented panel, clicking an item applies it to the selection exactly as the MuseScore palette does (undoable).
- [x] Panel sections are named as in the manual (checked against `dorico-ui.md`).
- [x] Cmd-9 toggles the right zone; the strip stays.
- [x] A coverage table (panel/section -> palette cells or "stub") is written into this spec's as-built notes.

## Tests

GoogleTest: the section-mapping function (spec table -> resolved cell ids, unknown id reported). Manual: apply one item from each panel; compare layout with the screenshot.
