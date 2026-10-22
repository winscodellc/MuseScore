<!-- markdownlint-disable -->

# 101 - Visual parity sweep (toolbar, zones, panels, music area)

Status: built, pending review - Depends on: 94, 95 (one page: zone defaults are per mode) - Dorico refs: `reference/dorico-ui.md`, the manual's project-window pages per mode

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

## Build notes (2026-10-22)

Done in code (no UI run yet, see Acceptance): T1-T5 (toolbar 37, `#272D33` bands behind each top toolbar's content, dark tab strip with theme colours, "Full score", transport pill with stub video / power / record / volume), S1-S3 (compact pills, "No selection", Follow playhead in Play and Engrave, bottom-centre chevron, `<` `>` edge tabs), P1-P4, P6-P11, Z2, M1, M2.

- **B1** was already fixed by spec 98 (`Text.StyledText` in `PlayersList.qml`). The same escaping bug was in the Play track headers (`trackoverviewmodel.cpp`): fixed with `TextBase::unEscape`.
- **Z1** needs no code: the defaults were already right (`zoneOpenByDefault`). The mixer came from a saved `dorico.zones.write.lower.docks` setting. Run Restore Default Layout before taking parity shots.
- **M1**: the paint hook at `abstractnotationpaintview.cpp` now honours any valid override (a transparent one paints nothing), and `DoricoShellPage.qml` draws the gradient (`#09203F`, `#1D3756` at 60%, `#3A5977`, sampled from Dorico Pro 6) under the paint view. Engrave is flat `#485E6E` (not sampled from a clean canvas: re-measure).
- **M2**: `applyDoricoAppearance`'s sibling block sets the canvas orientation to vertical once (own settings key `dorico/canvasOrientationApplied`).
- **P3**: the Notations panel is 2 cells per row, 70 tall, 230 wide. The active colour of the toolbox buttons (`#40ACFF` in `DoricoButton.qml`) was left as is until it is compared in the parity pass.
- **P8**: the Pages section embeds MuseScore's `NotationNavigator`. Display only: clicking a page does not move the main view.
- **P9**: Tempo and Chords lanes (tempo marks / chord symbols of the score), note dashes (at most 600 per track, `ponytail:` in the model) and a stub toolbar row.
- **T5** is not in Dorico's exact order: record and volume come after MuseScore's playback controls (which end with the settings cog), and the loop in / out buttons stay in the toolbar. Reordering needs an edit in `src/playback`.

## Deferred / unsupported

- T6 title bar: native, its colour is set in the `muse/` submodule (`macosplatformtheme.mm`). Not editable from the fork without forking `muse_framework`.
- P3 collapsible "Uncommon" sections: MuseScore palettes have no such grouping.
- P5 checkbox column and "Empty part" rows in Layouts: needs the selected player from the Players panel across two lazily loaded zones, and undoable add / remove of a part in an excerpt. MuseScore has no empty excerpts.
- T5 exact order (see above).

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
