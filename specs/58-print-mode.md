<!-- markdownlint-disable -->
# 58 - Print mode
Status: pending review  -  Depends on: 50, 55  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/print_mode/print_mode_window_r.html`, `.../print_mode_layouts_panel_r.html`, `.../print_mode_print_options_panel_r.html`

Implementer: Sonnet is fine.

## Goal
`PrintPage`: Layouts panel (left), Print Options panel (right), preview in the centre. No lower zone, layout selector disabled.

## Dorico behaviour
See `reference/dorico-ui.md` 5. Selected layouts (score and parts) are printed or exported. Preview shows the first selected layout and is read-only; view options are Page Width and Whole Page.

## MuseScore today
- Print: `print` module (`PrintProvider`, Qt print dialog via `file-print`).
- Export: `importexport` module (PDF, PNG, SVG, MP3, MIDI...), exported through the export dialog (`ExportDialog` with a parts list and per-format settings).
- `Publish` page (`PublishPage`) exists upstream; not used here.

## Design
- Left: `PrintLayoutsPanel.qml` with the same part list as spec 55 plus checkboxes, showing the page count per layout when known (from the notation's page count).
- Right: `PrintOptionsPanel.qml`, sections in the manual order, each backed by the existing export/print settings:
  - Destination: Printer / Graphics file (PDF, PNG, SVG); file name and folder via the export dialog's file chooser.
  - Job Type: all pages / page range (PDF/print range), one file per part.
  - Page Setup: paper size, orientation, scale: MuseScore page settings (read, link to the Page settings dialog).
  - Duplex, Annotations (crop marks, border): disabled stubs.
  - Button label: Print / Export / Print and Export by destination; calls `file-print` or the export scenario with the chosen layouts.
- Centre: the notation view in page view, read-only (`NotationView` with editing disabled), or the existing print preview if MuseScore has one (verify; Qt print preview is a dialog, so likely not embeddable).

## Deferred / unsupported
Also (as built): the preview does not switch to the first checked layout (it shows the current one); page and flow ranges; copies and printer choice live in the system dialog.
Booklet/2-up arrangements, duplex, crop marks, graphics-file colour/resolution beyond what MuseScore's export offers, view options Page Width/Whole Page (map to zoom presets if simple).

## As built
- **`PrintModel`** (`qml/MuseScore/Dorico/printmodel.{h,cpp}`) lists the full score and the parts that exist (`excerpts()`, sorted like the Parts dialog; parts that were never created are left out, as in MuseScore's Export dialog). Each row has a check box and an info line (page count when the layout has been laid out, then `layoutPageInfo`). The current layout starts checked. The list reloads each time the page is shown and when parts change.
- **Print** calls `IPrintProvider::printNotation` once per checked layout, so the system print dialog opens once per layout (MuseScore has no multi-notation print).
- **Export** (Destination PDF / PNG / SVG) builds an `ExportType`, asks for the path with `IExportProjectScenario::askExportPath` and writes with `exportScores`, then opens the folder. Unit type comes from the pure `exportUnitType()` (`doricoshell.h`): PDF = one file per layout (check box, default) or one combined file; PNG/SVG = one file per page (the only mode their writers support).
- **Print Options panel** sections in manual order: Destination (dropdown), Job Type (all pages; per-layout check box for PDF), Page Setup (size/orientation/space of the current layout + "Page Setup..." = `command://notation/open-page-settings`), Duplex Printing and Annotations (unavailable placeholders), then the Print / Export button (disabled while nothing is checked).
- **Centre**: `DoricoPage.readOnlyView` (new) drives `NotationView.readOnly`, so Print shows the current layout as a non-editable preview. The layout selector was already disabled in Print (spec 50).
- Zones: `zoneDocks(Print)` = left / right, no lower.

## Verification status
Built and unit-tested (`dorico_tests` 28/28: Print zones, `exportUnitType`), QML linted. Manual check in the app pending.

## Acceptance criteria
- [ ] Print page matches `dorico-ui.md` 5 in zones; layout selector is disabled.
- [ ] Checking layouts and pressing Export writes one PDF (or one per part) with the chosen layouts.
- [ ] Print opens the system print dialog with the chosen layouts.
- [ ] Unsupported controls are disabled with a tooltip.

## Tests
Manual: export score + one part to PDF; print to PDF from the system dialog.
