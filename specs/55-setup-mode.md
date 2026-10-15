# 55 - Setup mode
Status: todo  -  Depends on: 50  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/setup_mode/setup_mode_project_window_r.html`, `.../setup_mode_players_panel_r.html`, `.../setup_mode_layouts_panel_r.html`, `.../setup_mode_flows_panel_r.html`

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

## Acceptance criteria
- [ ] Setup page shows three zones matching `dorico-ui.md` 1.
- [ ] Adding or removing an instrument works from the Players panel and shows in the score.
- [ ] Layouts panel lists the score and each part; add, rename, delete a part work.
- [ ] Flows panel shows a single card; disabled buttons have a "not available yet" tooltip.
- [ ] Cmd-7/8/9 toggle Players/Flows/Layouts zones.

## Tests
Manual: add a flute, create its part, delete both.
