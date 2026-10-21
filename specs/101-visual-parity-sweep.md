<!-- markdownlint-disable -->

# 101 - Visual parity sweep (toolbar, zones, panels, music area)

Status: todo - Depends on: 94, 95 (one page: zone defaults are per mode) - Dorico refs: `reference/dorico-ui.md`, the manual's project-window pages per mode

Ground truth: `reference/dorico-shots/modes/notes.md`, with full-window shots `dorico-<n>-<mode>.png` / `ours-<n>-<mode>.png` and crops. These are from Dorico Pro 6, 2026-10-20 (the SE run was discarded). Window sizes differed (Dorico 2924x1225, ours 3440x1409, both 1 px = 1 pt), so compare sizes in points, not ratios of the window.

## Goal

Close every chrome gap from the modes audit that specs 98-100 and 102 do not own. Every value below is a measured Dorico Pro value. Before fixing a row, re-measure it at equal window sizes. The parity check (see Tests) runs on the finished spec.

## Gaps (Dorico → ours → fix)

### Bugs

| # | Where | Gap | Fix |
|---|---|---|---|
| B1 | Setup > Players list | Names show a literal `&amp;` ("Flutes 1 &amp; 2"); the Layouts list shows `&` correctly. | Use the plain-text name in `PlayersList.qml` / its model, the same as the Layouts list. |

### Default zones per mode

| # | Mode | Dorico default | Ours | Fix |
|---|---|---|---|---|
| Z1 | Write, Engrave | No lower zone. | Mixer lower zone open, with full strips. | Lower zone closed by default (the spec 95 per-mode zone defaults). |
| Z2 | Play | Lower zone = Mixer with **compact** strips (pan, level, name). Inserts, EQ and Sends are collapsed. | All sections expanded. | Default the Inserts, EQ and Sends sections to collapsed (spec 63 already has section toggles). |

### Top toolbar

| # | Item | Dorico | Ours | Fix |
|---|---|---|---|---|
| T1 | Toolbar height (below the title bar) | ~37 | ~28 | Match. |
| T2 | Toolbar background | `#272D33`, one step lighter than the panels | `#1F2429`, the same as the panels | New theme token or a `DoricoTopBar` colour. |
| T3 | Mode tabs | 56 wide (Write), ~29 tall; background `#36414D`; active `#295B82`; grouped in a dark box | ~52 x 17; `#3D4A58`; active `#2C678F`; not grouped | Size, colours, group box. Find out why the theme `button_color #36414D` renders as `#3D4A58` (hover or opacity?). |
| T4 | Layout combo | "Full score" | "Score" | Label from the excerpt or layout name ("Full score"). |
| T5 | Right group | video, mixer, **power** (audio engine, blue), transport readout `1.1.1.000`, rewind, **record** (red), play (green), metronome, tempo / click, **volume**, **3-dot**, undo, redo, help, all in a dark pill | no power, record, volume or 3-dot; different grouping | Add the buttons. Power, record and volume may be stubs (spec 77 for record). Match the order and grouping. |
| T6 | Title bar | 28 tall, `#222222` | 32, `#1D2024` | Check whether this can be controlled (native title bar); if it can't, list it as deferred. |

### Status bar

| # | Dorico | Ours | Fix |
|---|---|---|---|
| S1 | Left: note-input dropdown, "Concert Pitch \| Transposed Pitch" pills (compact), "No selection" text | "Grid: 1/4" + Concert / Transposed buttons about 130 px each, in a split bar | Compact pills, and selection summary text (spec 51 has a summary item; check why it is hidden). |
| S2 | Right: Follow playhead toggle, view mode icons, Page / Galley / Fill, zoom | No Follow playhead | Add the toggle (MuseScore has "follow playback"). |
| S3 | A collapse chevron at bottom centre (lower zone), and `<` `>` tabs at the left and right window edges | none | Zone collapse tabs (spec 53 deferred). Each toggles Cmd-7, Cmd-8 or Cmd-9. |

### Toolboxes and panels

| # | Where | Dorico | Ours | Fix |
|---|---|---|---|---|
| P1 | Toolbox strip | `#272D33`, icons `#DBE0E6` | `#1F2429`, icons `#6F7479` (dim) | Strip colour, icon colour. |
| P2 | Write left flyout (durations) | ~92 wide | ~140 wide | Cell size and spacing. |
| P3 | Notations panel | ~230 wide, **2 large cells** per row (~110x70), engraved glyphs, dull-blue active, collapsible "Uncommon" sections | ~300 wide, 7 small cells, bright cyan active | Re-grid to Dorico's layout (spec 53 deferred). |
| P4 | Setup Players | chevron to expand, person icon, "+" on the selected row, selected `#40ACFF`, unselected `#36414D` | no chevron, no "+", no bright selection; extra "Tempos" row | Match. The "Tempos" row moves out of Players (Dorico shows tempo in the system track). |
| P5 | Setup Layouts | checkbox column on the right, "Empty part" rows, chevrons | icons on the left, no checkboxes | Match (the checkbox = player in this layout). |
| P6 | Setup bottom | "Order: Orchestral" dropdown, Concert / Transposed pills | "Order" label only | Order dropdown (MuseScore score order). |
| P7 | Engrave left panel | buttons ~23 tall; Bracketing grid of 2 rows | ~14 tall; 1 row | Size and grid. |
| P8 | Engrave right panel | Pages (thumbnails), Page Templates (set dropdown, thumbnails) | "Not available yet" stubs | Pages panel with real page thumbnails (MuseScore can render pages). Page Templates stays a stub (87). |
| P9 | Play track view | Tempo and Chords tracks, note-preview dashes in blocks, track toolbar row | none | Add Tempo / Chords lanes (read-only), mini piano-roll dashes in blocks, a toolbar row (stub buttons). |
| P10 | Play Flow field | enabled | disabled | Enable it (one flow). |
| P11 | Print right panel | orientation as icons, "Print odd final page on", Scale factor, Collate ticked by default | text buttons, missing two rows, Collate unticked | Match (spec 62 follow-up). |

### Music area

| # | Dorico | Ours | Fix |
|---|---|---|---|
| M1 | Dark navy gradient (`#09203F` region) around the page in Write, and its own Engrave colour | flat `#2C4D6F` | Gradient background (spec 60 deferred). Find the paint hook for the canvas background; keep any upstream edit to one line. |
| M2 | Write opens in a single-page vertical view with a scrollbar | horizontal multi-page | Match the default view per mode (Dorico default: Page view, pages vertical? confirm against the shots). |

## Deferred / unsupported

- Dorico icon artwork. We use MuseScore, Bravura and Lucide icons chosen to look close (hard constraint).
- The native title bar, if it can't be styled (T6).
- Engrave frame-edit overlay and Page Templates (spec 87).

## Acceptance criteria

- [ ] B1 is fixed: "Flutes 1 & 2" in the Players list.
- [ ] Every row above is fixed or moved to Deferred with a reason.
- [ ] Parity shots: for each mode, the same window size as Dorico, crops of the toolbar, left zone, right zone and status bar side by side. Sizes within 2 pt, colours within ΔE ≈ 3 (or ±4 per channel).
- [ ] Write and Engrave open with no lower zone; Play opens the compact Mixer.

## Tests

- Manual parity pass with the `ui-tester` agent:
  - Launch Dorico Pro using the steps in `.claude/agents/ui-tester.md`.
  - Size both windows to 2924x1225.
  - Recapture the modes shots and compare region by region.
