# 55 - Setup mode
Status: pending review  -  Depends on: 50  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/setup_mode/setup_mode_project_window_r.html`, `.../setup_mode_players_panel_r.html`, `.../setup_mode_layouts_panel_r.html`, `.../setup_mode_flows_panel_r.html`

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
Section players, ensembles, groups, sort orders, custom/full-score layouts beyond the one score, multiple flows, assignment checkboxes, layout numbers.

## As built
- **Players (left, `playersPanel`)** embeds MuseScore's instruments panel (`LayoutPanel`) unchanged: it keeps its own add / move / delete / visibility controls, so adding and removing instruments happen there. Below it is Dorico's action bar: Add Single Player = `command://instruments/select` (the Add instruments dialog); Add Section Player, Add Ensemble, Add Group, Sort Players and Player Settings are unavailable placeholders (instrument settings stay in each row's own popup).
- **Layouts (right, `layoutsPanel`, `LayoutsPanel.qml`)**: cards from `DoricoTopBarModel` (same list and order as the layout selector) with type, name, and `layoutPageInfo()` (page size in mm, orientation, staff space in pt; blank for parts that were never opened, since they have no score yet). The list now also refreshes on part renames and style changes. Double-click a card shows that layout. Actions call MuseScore's Parts-dialog model `PartListModel` (row i = card i + 1, rebuilt on each change because it builds its list only once): Add Instrumental Part Layout = `createNewPart`, Layout Settings = inline rename (`setPartTitle`), Delete Layout = `removePart` (asks first when the part has content). Layout Options opens MuseScore's Style dialog. Add Full Score / Custom Score and Sort are placeholders.
- **Flows (lower, `flowsPanel`)**: one read-only card with the project title and "Flow 1"; all four actions are placeholders.
- Zones: `zoneDocks(Setup)` = players / flows / layouts, so Cmd-7/8/9 work.
- Shared pieces introduced here: `ToolSection.qml` (titled button group over `DoricoToolsModel`), `DoricoButton.qml` (a FlatButton that looks disabled but still shows its "Not available yet" tooltip: a disabled FlatButton shows no tooltip at all), `DoricoPlaceholder.qml` (the "Not available yet" stub, also used by the lower zone). The panel size constants and drop destinations moved from `WritePage` to `DoricoPage`.
- Status bar "Order" stays a label: MuseScore's instrument-order menu lives inside the instruments panel's context menu.

## Verification status
Built and unit-tested (`dorico_tests` 26/26: Setup zone table, `layoutPageInfo`), QML linted with `qmllint`. Manual check in the app pending.

## Acceptance criteria
- [ ] Setup page shows three zones matching `dorico-ui.md` 1.
- [ ] Adding or removing an instrument works from the Players panel and shows in the score.
- [ ] Layouts panel lists the score and each part; add, rename, delete a part work.
- [ ] Flows panel shows a single card; disabled buttons have a "not available yet" tooltip.
- [ ] Cmd-7/8/9 toggle Players/Flows/Layouts zones.

## Tests
Manual: add a flute, create its part, delete both.
