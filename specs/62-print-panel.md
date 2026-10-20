<!-- markdownlint-disable -->

# 62 - Print panel parity

Status: todo - Depends on: 58 - Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/print_mode/print_mode_print_options_panel_r.html`, `.../print_mode_printers_c.html`, `.../print_mode_page_range_printing_exporting_t.html`, `.../print_mode_pages_arrangements_for_printing_exporting_r.html`, `.../print_mode_booklet_printing_c.html`, `.../print_mode_paper_size_orientation_setup_r.html`, `.../print_mode_duplex_printing_r.html`, `.../print_mode_annotations_r.html`, `.../print_mode_graphics_files_formats_r.html`

Local mirror: `reference/manual/topics/print-mode/` (`project-window-in-print-mode/print-options-panel.md`, `printers.md`, `specifying-ranges-of-pages-flows.md`, `page-arrangements-for-printing-exporting*.md`, `page-vs-paper-size/`, `duplex-printing.md`, `annotations.md`, `exporting-layouts-as-graphics-files*`, `graphics-file-formats.md`).

## Goal

Make the Print Options panel (spec 58) do what Dorico's does without the system dialog: choose a printer, copies and collate, a page range, a job type (Normal, Spreads, 2-up, Booklet), paper size, orientation and scale, duplex, and annotations. Every setting is **per layout** and **saved in the project**.

## Dorico behaviour

All settings are stored per layout. Editing a control with several layouts checked applies to all of them; a control whose layouts disagree shows blank (Copies) or no name (Printer menu).

| Section | Control | Behaviour |
|---|---|---|
| Destination | Printer / Graphics | Which one is the default is not stated; we default to Printer |
| | Printer menu | System default printer; per layout. Changing it narrows the Paper size list to the sizes the printer supports and enables "Both sides automatically" only if it can do automatic duplex |
| | Copies, Collate copies | Number field (blank when mixed), checkbox |
| | Graphics | Format PDF / PNG / SVG / TIFF, Mono / Color, Resolution (PNG, TIFF), Destination folder, File Name Options |
| Job Type | Job type menu | **Normal**: one page per sheet. **Spreads**: two pages per sheet, odd pages on the right, even on the left. **2-up**: two pages per sheet, first page of the range on the left of sheet 1. **Booklet**: two pages per sheet imposed so the folded sheets read in order; for 4 pages side 1 is p4 left / p1 right, side 2 is p2 left / p3 right; an odd count is padded with blank pages at the end (6 pages print as 8); full range only |
| | Print range | **All** (default) / **Pages** / **Flows** |
| | Pages | Field: `1-4` range, `3-` from page 3 to the end, `1,3,5-8` list. Separate ranges are exported as separate files (graphics) |
| | Flows | **Choose** opens the Print Flows dialog (checkbox list, Select All / None / Invert); includes every page the flow appears on. PDF only: **Separate file for each flow** |
| Page Setup | Paper size | Printer's supported sizes; picked from the layout's page size when the printer has it. Never changes the layout's page size |
| | Paper orientation | Portrait / Landscape. Choosing a job type switches it (landscape for Spreads, 2-up, Booklet), the user can override |
| | Print odd final page on | Spreads and 2-up only: a separate paper size and orientation for the last, unpaired page |
| | Fit to Paper / Custom Scale | Fit scales the page to the paper, enlarging if needed. Custom shows a Scale factor in percent; at 100 an A3 page on A4 paper overflows |
| | Graphics | Only orientation is editable; exports use the layout's own page size |
| Duplex (Printer only) | Print on | One side only / Both sides manually (after the outward pages a message asks to turn the stack over, then the inward pages print) / Both sides automatically (only if the printer supports it) |
| | Flip image (portrait), (landscape) | Flip automatically / Flip long side / Flip short side |
| | Reverse order of outward pages | Booklets printed with manual duplex: outward pages in reverse order |
| Annotations | Crop marks, Border | Short corner lines / outline around the page edge; only printed when the page is smaller than the paper |
| | Date and time | Printing date and time at the bottom of each page |
| | Watermark + text | Large translucent text across the middle of each page; for graphics only in Color exports |
| | View options | Includes active view options (signposts, comments, colours) |
| Button | Label | **Print** if every selected layout is set to Printer, **Export** if all Graphics, **Print and Export** if mixed |

Not stated by the manual: defaults for copies, collate, scale and duplex; whether the OS gets one job or one per layout; limits on the scale factor. We use 1, off, Fit to Paper, One side only, and one job per layout.

## MuseScore today

- `PrintModel` (`src/dorico/qml/MuseScore/Dorico/printmodel.{h,cpp}`): `print()` calls `IPrintProvider::printNotation` once per checked layout (always shows `QPrintDialog`); `exportTo(suffix, filePerLayout)` goes through `IExportProjectScenario::askExportPath` and `exportScores`.
- `PrintPage.qml` has the Print Options sections inline. Pages, Flows, Two-sided, Crop marks and Border are `available: false` buttons; copies and the printer are chosen in the system dialog.
- `PrintProvider::printNotation` (`src/print/internal/printprovider.cpp:44-94`) builds a `QPrinter`, shows the dialog, then calls `INotationPainting::paintPrint`. No way to skip the dialog.
- `ScorePaintOptions` (`src/engraving/rendering/iscorerenderer.h:110`) has `isSetViewport`, `fromPage`, `toPage`, `copyCount`, `deviceDpi`, `onNewPage`. `Paint::paintScore` (`rendering/score/paint.cpp:38`) paints a contiguous page range; with `isSetViewport = false` the caller owns the painter transform, so a page can be placed and scaled anywhere on a sheet.
- `PdfWriter` (`importexport/imagesexport/internal/pdfwriter.cpp`) has no range, scale or paper override.
- No flow model (`SetupPage.qml` hard-codes "Flow 1"); the nearest thing is section breaks.
- Excerpt scores write their own `metaTag`s (`engraving/rw/write/writer.cpp:171`); `ChangeMetaTags` (`engraving/editing/editscoreproperties.h:30`) changes them with undo.
- `Qt::PrintSupport` is linked only by `src/print` (`QPrinterInfo`, `QPrinter` duplex/collate/copies exist).

## Design

No upstream edits. All new code is in `src/dorico/`.

- **`internal/printjob.{h,cpp}`** (pure, no Qt PrintSupport, unit tested):
  - `PrintSettings` and `toJson()` / `fromJson()` (missing keys take defaults, so old projects load).
  - `parsePageRanges(text, pageCount)` returns ranges or an error (own parser: `QPageRanges` has no `3-`).
  - `imposeSheets(pageCount, JobType)` returns sheets of `{left, right}` page indexes, -1 = blank.
  - `manualDuplexPasses(sheets, reverseOutward)` returns the outward and inward lists.
  - `placePage(pageSizeIn, slotRectIn, fit, scalePercent)` returns the centred device rect.
  - `defaultOrientation(jobType, layoutOrientation)`.
- **`qml/MuseScore/Dorico/doricoprinter.{h,cpp}`** (needs `Qt::PrintSupport`): builds the `QPrinter` from the settings (`setPrinterName`, `setCopyCount`, `setCollateCopies`, `setDuplex`, `QPageSize`, full page, zero margins, same as `printprovider.cpp`), and `renderSheets` paints each slot with `paintPrint` (`isSetViewport = false`, own `setViewport` / `setWindow` as in `paint.cpp:63`), then the annotations on the full sheet. Manual duplex shows an `IInteractive` message between the passes. Graphics PDF uses the same `renderSheets` on a `QPdfWriter`, one file per range; PNG / SVG keep the spec 58 `exportScores` path.
- **`PrintModel`**: one `PrintSettings` per layout, stored as JSON in a `doricoPrintSettings` metaTag on that layout's `Score`; read when the layout list loads. New: `settings` (values shared by the checked layouts, blank when mixed), `setSetting(key, value)`, `printers`, `paperSizes`, `autoDuplexAvailable`, `buttonLabel`, `run()`. `print()` stays as the System Dialogs path (ignores the panel, as in Dorico).
- **`PrintPage.qml`**: replace the stub buttons with controls in the manual's order. Flows stays unavailable (tooltip).
- Add the new files to `src/dorico/CMakeLists.txt` and `qml/MuseScore/Dorico/CMakeLists.txt` (`Qt::PrintSupport`), the test to `tests/CMakeLists.txt`.

## Deferred / unsupported

- **Flows range** and **Separate file for each flow**: no flow model (roadmap 86). The Flows button stays unavailable.
- TIFF, Resolution menu, Transparent background, Export File Names dialog and recipes: MuseScore's image-export configuration is used as is.
- Page ranges and job types for PNG / SVG (their writers only do one file per page).
- **View options** annotation, the Print Watermark Font style (a fixed font is used), the concert-pitch warning.
- The preview does not reflect job type, paper size or scale.

## Acceptance criteria

- [ ] `parsePageRanges` accepts `1-4`, `3-`, `1,3,5-8` and rejects `0`, `5-2`, `a`, out-of-range pages.
- [ ] Booklet of 4 pages: side 1 = p4 | p1, side 2 = p2 | p3; 6 pages pad to 8 with blanks at the end. Spreads puts odd pages on the right; 2-up puts page 1 on the left.
- [ ] The Printer menu lists the system printers and selects the default one; Copies and Collate are set on the `QPrinter`.
- [ ] Print prints the checked layouts with no system dialog, each with its own settings; the button reads Print / Export / Print and Export.
- [ ] Pages `1,3-` on a graphics PDF writes two files with the right page counts.
- [ ] Job type Booklet/2-up/Spreads writes landscape sheets in the right order; Fit to Paper and Custom Scale change the page size on the sheet.
- [ ] Both sides manually prints the outward pages, shows the "turn the stack over" message, then the inward pages; Reverse order of outward pages reverses the first set.
- [ ] Crop marks and Border appear only when the page is smaller than the paper; Date and time and Watermark appear on every page; the watermark is absent from Mono graphics.
- [ ] Settings are per layout, survive save and reopen, and mixed values show blank.
- [ ] Unsupported controls (Flows, TIFF, View options) are unavailable with a tooltip.

## Tests

GoogleTest `src/dorico/tests/printjob_tests.cpp`: page-range parsing (valid and invalid), imposition for 1-8 pages in every job type, manual-duplex passes, `placePage` fit and scale maths, JSON round trip (including an empty object). Manual: export a demo score to PDF with Pages `1,3-`, Booklet and a watermark, check page count and size with a `python3 -I` PDF dump and a cropped screenshot; print to a PDF printer and compare.
