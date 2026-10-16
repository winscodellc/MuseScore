<!-- markdownlint-disable -->

# 55 - Setup mode

Status: pending review - Depends on: 50 - Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/setup_mode/setup_mode_project_window_r.html`, `.../setup_mode_players_panel_r.html`, `.../setup_mode_layouts_panel_r.html`, `.../setup_mode_flows_panel_r.html`

Implementer: Sonnet is fine.

## Goal

`SetupPage`: Players panel (left), Layouts panel (right), Flows panel (lower), each with Dorico's cards and action bar, backed by MuseScore's instruments and parts.

## Dorico behaviour

See `reference/dorico-ui.md` 1. Three zones, no toolbox. Cards list the items; the action bar at the bottom of each panel has the add/sort/settings/delete buttons. Selecting a card shows checkboxes on cards in the other panels (assignment).

## MuseScore today

- Instruments: `instrumentsscene` (`LayoutPanel` = the instruments panel; models for instruments/staves, add-instruments through the Instruments dialog, move up/down, visibility, delete).
- Parts (excerpts): the Parts dialog (`notationscene` or `notation` parts model; verify) with create, rename, delete, open.
- Flows: no equivalent. A MuseScore project is one score = one flow.

## Design

- Players panel: **reuse** `LayoutPanel` content (instrument tree) in the left zone dock, restyled minimally; map action bar: Add Single Player -> existing "Add instruments" flow (`instruments/add-instruments` command, verify id); Add Section/Ensemble/Group -> disabled; Sort -> disabled; Player Settings -> instrument properties; Delete -> remove instrument.
- Layouts panel: `LayoutsPanel.qml` + `LayoutsPanelModel` over the existing parts list. Cards: Full score + one per part (type, name, page size and orientation, space size; read from the style / page settings). Action bar: Add Instrumental Part Layout -> create part; Add Full Score / Custom Score -> disabled; Layout Options -> open MuseScore's Page settings / Style dialog; Layout Settings -> rename part; Delete -> delete part.
- Flows panel: one fixed card named after the project (read-only). Add/Delete/Notation Options disabled.
- Checkbox assignment UI: deferred (MuseScore assigns instruments to parts in the part's own dialog).
- Status bar "Order" item (spec 51) opens MuseScore's instrument order menu if there is one.

## Deferred / unsupported

- Instruments ordering (MuseScore's Players "…" submenu) left with the instrument tree; choose the order in the Add Single Player (instruments) dialog. Staff rows (per-staff settings) are no longer in the Players panel.

Section players, ensembles, groups, sort orders, custom/full-score layouts beyond the one score, multiple flows, assignment checkboxes, layout numbers.

## As built

- **Players (left, `playersPanel`)** embeds MuseScore's instruments panel (`LayoutPanel`) without its top bar (Add / Move up / Move down / Remove; fix after review). Upstream edit in `instrumentsscene/.../LayoutPanel.qml` (`// Dorico fork (spec 55)`): `controlPanelVisible` and a `layoutTreeModel` alias. Row visibility toggles and each row's settings popup stay in the tree. Below it is one action bar (a `Flow`, wraps in a narrow panel): Dorico's buttons, then Add System Markings (Lucide `milestone` icon) and Remove (trash can), which call the tree model (`addSystemMarkings`, `removeSelectedRows`) and are enabled from its `is...Available` flags. No move up/down buttons: rows are reordered by drag and drop. Dorico's buttons: Add Single Player = `command://instruments/select` (the Add instruments dialog); Add Section Player, Add Ensemble, Add Group, Sort Players and Player Settings are unavailable placeholders (instrument settings stay in each row's own popup).
- **Layouts (right, `layoutsPanel`, `LayoutsPanel.qml`)**: cards from `DoricoTopBarModel` (same list and order as the layout selector) with type, name, and `layoutPageInfo()` (page size in mm, orientation, staff space in pt; blank for parts that were never opened, since they have no score yet). The list now also refreshes on part renames and style changes. Double-click a card shows that layout. Actions call MuseScore's Parts-dialog model `PartListModel` (row i = card i + 1, rebuilt on each change because it builds its list only once): Add Instrumental Part Layout = `createNewPart`, Layout Settings = inline rename (`setPartTitle`), Delete Layout = `DoricoTopBarModel::deleteLayout` (asks first when the part has content; one undo step). It also deletes an instrument's own part, which the Parts dialog cannot: MuseScore regenerates a part for every instrument, so the deleted part ids are kept in the `deletedPartLayouts` score meta tag (`internal/partlayouts.h`) and `MasterNotation::updatePotentialExcerpts` skips them (upstream edit, plus `IExcerptNotation::initialPartId()`). Layout Options opens MuseScore's Style dialog. Add Full Score / Custom Score and Sort are placeholders.
- **Flows (lower, `flowsPanel`)**: one read-only card with the project title and "Flow 1"; all four actions are placeholders.
- Zones: `zoneDocks(Setup)` = players / flows / layouts, so Cmd-7/8/9 work.
- Shared pieces introduced here: `ToolSection.qml` (titled button group over `DoricoToolsModel`), `DoricoButton.qml` (a FlatButton that looks disabled but still shows its "Not available yet" tooltip: a disabled FlatButton shows no tooltip at all), `DoricoPlaceholder.qml` (the "Not available yet" stub, also used by the lower zone). The panel size constants and drop destinations moved from `WritePage` to `DoricoPage`.
- Part names are plain text: `Excerpt::createExcerptsFromParts` strips rich text from the instrument name (3.x files store `<font face=.../>Alto`), and the project title is shown as plain text too. Upstream edit in `engraving/dom/excerpt.cpp`, tested in `parts_tests.cpp`.
- Status bar "Order" stays a label: MuseScore's instrument-order menu lives inside the instruments panel's context menu.

### After the side-by-side screen comparison with Dorico 6 (2026-10-16)

- Layouts: compact rows like Dorico's (icon, name, selected blue), details in the tooltip. Flows: one blue card (`#30678B`) filling the panel ("Flow 1", ticked layout checkbox, number), flow actions in a column on the right. - **Players** (user-approved replacement of MuseScore's tree): `PlayersList.qml`, Dorico-style rows (person icon, name; selected `#45A6F5`; hidden instruments dimmed) over MuseScore's own `LayoutPanelTreeModel`, so add / remove / reorder / hide / settings stay MuseScore's. Click selects, Shift-click selects a range (MuseScore's model allows no Cmd-click), drag reorders (`moveRows`, as the tree did; the player carrying the tempo/markers row stays on top, as in MuseScore), Delete removes. Right-click: Show / Hide, Enabled (shared parts), Settings… (MuseScore's instrument or system-markings popup), Remove. Staff rows are not shown. The panel's "…" menu has **Enable stave sharing** (`command://dorico/toggle-stave-sharing`, checked from `Sid::enableStaveSharing`). The `LayoutPanel.qml` upstream edit is reverted (not used any more).

## Verification status

Built and unit-tested (`dorico_tests` 26/26: Setup zone table, `layoutPageInfo`), QML linted with `qmllint`. Manual check in the app pending.

## Acceptance criteria

- [x] Setup page shows three zones matching `dorico-ui.md` 1.
- [x] Adding or removing an instrument works from the Players panel and shows in the score.
- [x] Layouts panel lists the score and each part; add, rename, delete a part work.
- [x] Flows panel shows a single card; disabled buttons have a "not available yet" tooltip.
- [x] Cmd-7/8/9 toggle Players/Flows/Layouts zones.
- [ ] Top bar with Add button, arrows to move and trash can must be removed and thas can ca go to the bottom with the other controls

## Tests

Manual: add a flute, create its part, delete both.
