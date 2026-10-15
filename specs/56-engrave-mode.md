# 56 - Engrave mode
Status: pending review  -  Depends on: 50  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/engrave_mode/engrave_mode_window_r.html`, `.../engrave_mode_engrave_toolbox_r.html`, `.../engrave_mode_formatting_panel_r.html`, `.../engrave_mode_frames_panel_r.html`, `.../engrave_mode_graphic_slices_panel_r.html`, `.../engrave_mode_pages_panel_r.html`

Implementer: Sonnet; use Opus if the command mapping in the table turns out unclear.

## Goal
`EngravePage`: Engrave toolbox, left zone (Formatting / Frames / Graphic Slices), right zone (Pages), lower zone (Properties, Mixer). Page view only.

## Dorico behaviour
See `reference/dorico-ui.md` 3. Always page view (the view-type control is hidden). The toolbox chooses the tool and the left-zone panel. The left zone hides while Staff/Note Spacing is active.

| Dorico control | MuseScore equivalent (verify each command id in `notationuiactions.cpp` / `shortcuts.json`) |
|---|---|
| Move bars to previous/next system | `move-measure-to-prev-system` / `...next-system` |
| Insert System Break | `system-break` (layout break) |
| Lock System / Make into System | `lock-system`, `make-into-system` |
| Insert Frame Break | `page-break` (closest) |
| Lock Layout / Reset Layout | `reset-layout`, lock = disabled |
| Insert bracket/brace | MuseScore bracket items (via staff properties or palette); else disabled |
| Insert Frames (Music/Text/Graphics) | `insert-vbox` / `insert-tbox` / `insert-hbox` (verify) |
| Graphic Editing | normal selection + edit mode |
| Staff Spacing / Note Spacing | disabled (MuseScore uses style / stretch) |
| Frames panel, Constraints | disabled |
| Graphic Slices | disabled (MuseScore PNG/SVG export covers part) |
| Pages panel | stub (MuseScore has no page templates or flow headings) |

## MuseScore today
Layout-related commands exist as palette items (Layout palette: breaks, spacers, system locks) and menu actions (Format menu: page settings, style, reset). Frames are inserted from the Add menu. Properties panel and mixer are docks.

## Design
- `EngraveToolbox.qml`: vertical `DockToolBar` on the left (6 buttons from the manual, tool state in `IDoricoModeState` or a small page-local property).
- `FormattingPanel.qml`: section headers and buttons exactly as the manual; each button dispatches the MuseScore command above or is disabled. Reuse the same button-grid component as spec 52/53.
- `FramesPanel.qml`, `GraphicSlicesPanel.qml`, `PagesPanel.qml`: stubs, except the Frames panel's "Insert Frames" three buttons (real).
- Force page view when the page opens (`mode-page` command from the status-bar model); restore the previous view mode when leaving.
- Lower zone: Properties and Mixer tabs only (spec 54 component with two tabs).

## Deferred / unsupported
Staff Spacing, Note Spacing, Graphic Slices, Pages panel (templates, flow headings, page swaps), Lock Layout/Frame, Coordination Lines, frame constraints, bracketing barline joins, rulers and grid.

## As built
- **Engrave toolbox** (`EngravePage.qml`, non-floatable vertical `DockToolBar` `engraveToolbox` at the left edge): Show Left Zone, Graphic Editing (default), Frames, Staff Spacing and Note Spacing (unavailable), Graphic Slices. The tool is a page property (`tool`) that chooses what the left zone shows.
- **Left zone** (`engraveLeftPanel`, one dock whose content follows the tool):
  - **Formatting**, sections in manual order:
    - Moving Bars Between Systems: `notation/move-measure-to-prev-system`, `-next-system`.
    - Format Systems: `toggle-system-break`, `toggle-system-lock`, `make-into-system`.
    - Format Music Frames: Insert Frame Break = `toggle-page-break` (MuseScore has no music frames); Lock Frame and Make into Frame are placeholders.
    - Lock Layout: Lock Layout is a placeholder; Reset Layout = `reset-to-default-layout`.
    - Coordination Lines: placeholder.
    - Bracketing: MuseScore's bracket/brace palette cells through spec 53's `DoricoPanelModel` (panel id `engraveBracketing`, applied like the palette). Change Barline Joins is a placeholder.
  - **Frames**: Insert Frames = `insert-vbox`, `insert-textframe`, `insert-hbox`, labelled with MuseScore's names (vertical / text / horizontal frame), since MuseScore has no music or graphics frames. Constraints is a placeholder.
  - **Graphic Slices**: stub that points to File > Export (PNG/SVG).
- **Pages** (right, `pagesPanel`): stub, closed by default.
- **Lower zone**: `LowerZonePanel` with `tabs: ["properties", "mixer"]`. MuseScore's panel toggles (F3 / F8) also select those tabs here, because `zoneDocks(Engrave, Lower)` is the lower-zone dock.
- **Page view**: `DoricoActionsController::applyEngraveViewMode` remembers the notation's view mode when Engrave opens and calls `INotation::setViewMode(PAGE)` (direct and synchronous, unlike a dispatch). It restores the old mode when you leave Engrave. The status bar on Engrave has no view-type control (spec 51).
- `PaletteSections.qml` (titled palette grids) is now shared by the Notations panel and Engrave's Bracketing.
- Zones: `zoneDocks(Engrave)` = engraveLeftPanel / lowerZonePanel / pagesPanel.

## Verification status
Built and unit-tested (`dorico_tests` 33/33), QML linted. Manual check in the app pending. Engrave depends on the page-gating fix (notation commands used to be disabled outside Write).

## Acceptance criteria
- [ ] Engrave page matches `dorico-ui.md` 3 in zones and button order.
- [ ] Each Formatting button either performs its MuseScore action (undoable) or is disabled with a tooltip.
- [ ] Insert Frames adds a vertical, text or horizontal frame at the selection.
- [ ] Opening the page sets page view; the view-type control is hidden.

## Tests
Manual: break a system, move a bar to the next system, insert a text frame, undo each.
