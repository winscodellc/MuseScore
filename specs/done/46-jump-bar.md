<!-- markdownlint-disable -->
# 46 — Jump bar (J) and Go To Bar (Cmd-G)

Status: done · Depends on: [00-module-skeleton](00-module-skeleton.md), [20-popover-framework](done/20-popover-framework.md) (overlay host: the paint view, as for the popovers), [10-keymap-preset](10-keymap-preset.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B user_interface/user_interface_jump_bar_r.html`, `B …/user_interface_jump_bar_commands_performing_t.html`,
`B …/user_interface_jump_bar_going_to_locations_t.html`, `B …/user_interface_jump_bar_aliases_assigning_t.html`, `B …/user_interface_jump_bar_aliases_removing_t.html`,
`B write_mode/write_mode_navigation/write_mode_navigation_go_to_bar_t.html`, `B user_interface/user_interface_key_commands_searching_t.html`
Reference digests: [reference/dorico-editing.md](../reference/dorico-editing.md) §5 · [reference/musescore-internals.md](../reference/musescore-internals.md) §1, §3

## Goal
`J` opens a small bar: **Commands** mode (type any command name, run it) and **Go To** mode (jump to bar / rehearsal mark / page / flow). `Cmd-G` is Go To Bar. The jump bar is also the discoverability path for every command in specs 41, 44, 45 that has no key.

## Dorico behaviour

**Opening / modes**
- `J` opens it (Setup, Write, Engrave, Play). Modes: **Commands** (`Ctrl-1` on macOS; Windows `Alt-C`) and **Go To** (`Ctrl-2`; Windows `Alt-G`), or click the mode names. Separate direct-open keys can be assigned.

**Commands mode**
- Typing shows a menu of matching commands/options; Up/Down choose. Selecting an option makes its setting editable in the bar (Space toggles checkboxes/opens menus, Left/Right moves between buttons, Up/Down change values, **Opt-Return opens the corresponding dialog**, Return applies).
- `Down` on an empty field shows the **5 most frequently used** commands; reopening after a command shows the previous entry pre-selected.
- Only commands valid in the current mode/state run. The menu shows each command's key command or alias.
- **Aliases**: type `Command name\=alias` (e.g. `Remove Rests\=rr`); `Return` runs and assigns, `Opt-Return` assigns without running. Aliases can also be set/removed on the Key Commands page.

**Go To mode syntax**

| Destination | Entry | Examples |
|---|---|---|
| Bar n | `b[n]` | `b32`, `b+32` (the page does not explain `+`) |
| Rehearsal mark | `r[n]` | `rK`, `rk` |
| Page | `p[n]` | `p6`, `p+6` |
| Flow | `f[n]`, `fn` (next), `fp` (previous) | `f3` |
| Combined | `f3b20` | bar 20 of flow 3 |

Result: view scrolls, playhead moves to the start of the corresponding bar/flow.

**Go To Bar** — `Cmd-G` opens a dialog: Flow menu + Bar field + OK; scrolls so the top staff is near the top-left and moves the playhead.

**Key command search** — Preferences > Key Commands has a search field (MuseScore already has a shortcuts preferences page with search; keep it).

## MuseScore today
- `Ctrl+F` → `command://notation/view/show-search` → `ViewController::showSearch`; `SearchPopup.qml` (bottom bar, `TextInputField`) → `SearchPopupModel::search` → `INotationElements::search(const QString&)` (`src/notation/inotationelements.h:37`, impl `notationelements.cpp:51`).
- `SearchCommandsParser` (`src/notation/internal/searchcommandsparser.{h,cpp}`) parses `"12"`, `"12-16"`, `"p5"`, `"rA"` or any text as rehearsal mark into `SearchData{Type::Measure|MeasureRange|Page|RehearsalMark}`.
- Commands catalogue: `muse::rcommand::ICommandsRegister::commandInfoList()` (`muse/framework/rcommand/icommandsregister.h:40`) gives `CommandInfo` (title, description, input schema, availability flags); availability per command: `ICommandsState::commandState(cmd).enabled` (`icommandsstate.h:40`); key display: `ICommandShortcutsRegister::shortcut(cmd)` (`muse/framework/shortcuts_v2/icommandshortcutsregister.h:50`).
- Dispatch: `ICommandDispatcher` (`muse/framework/rcommand/icommanddispatcher.h`).
- Key conflicts (spec 10 unbinds): `J` = `enharmonic-spell-both` (`shortcuts.json:225`; also `enter-fret-8` on TAB staves, `:250`); `Ctrl+G` = `add-figured-bass` (`:227`). Text-field focus swallowing letter shortcuts is already solved by `SearchPopup` (same mechanism is reused).
- MuseScore has **one score = one flow** (no flows).

## Design (as built)
`src/dorico/internal/jumpbar/` + `src/dorico/ijumpbarcontroller.h` + QML `JumpBar.qml` / `jumpbarmodel.{h,cpp}` in `src/dorico/qml/MuseScore/Dorico/`. Hosted by a second `Loader` in `DoricoPage.qml` next to the popover's (same parent, the paint view); no upstream edit.

- `commandcatalog.{h,cpp}`: pure. `rankCommands` (exact alias > title prefix > every query word starts a title word > substring; ties by use count, then title), `topUsed` (Down on an empty field), `splitAlias` (`Name\=alias`).
- `gotoparser.{h,cpp}`: pure `parseGoTo` -> `GoToTarget{bar, barRelative, page, pageRelative, rehearsal}`. `f1`, `f1b20` accepted; `f2`, `fn`, `fp`, `b`, `bx`, `b0`, `b-4` invalid (one flow).
- `JumpBarController` (`IJumpBarController`, one per window): the four commands; the catalogue is built per keystroke from `ICommandsRegister::commandInfoList()`, keeping commands with `Availability::Dispatch`, no input parameters and `commandState().enabled`; key text from `sequencesToNativeText(shortcut(cmd).sequences)`. Usage counts, aliases and the last command are settings `dorico/jumpbar/{usage,aliases,last}`. One alias per command, one command per alias. A command runs after the bar closed (it may open a popover or need the focus).
- Go To resolution: bar `n` = the bar the status bar calls `n` (`Measure::measureNumber() + 1`; a pickup bar has none), so it matches `barbeat().displayedBar`; **not** MuseScore Find's measure index. `rK` = `elements()->search("rK")` then `findMeasure()`. `pN` = `pages()[N-1]->firstMeasure()`. Relative forms (`b+4`, `p+2`) count measures / pages on from the bar the selection starts in, or from the first (our reading; the manual does not explain `+`). The result is `select({measure}, SINGLE, 0)` (a range selection of that bar on staff 1) and `showItem`; the range selection moves the playhead (`PlaybackController::onSelectionChanged`).
- Bar UI: Return runs / goes, Opt-Return assigns the alias only, Esc closes, Up/Down move the highlight (at most 8 rows), Down on an empty field lists the most used. Ctrl-1 / Ctrl-2 (physical Control, Qt `Meta` on macOS) switch mode inside the bar only. An invalid Go To entry turns the field red after Return and nothing happens. J opens in the last-used mode; reopening Commands pre-selects the last command's title.

| Command | Title | Key (mac) | Behaviour |
|---|---|---|---|
| `command://dorico/jump-bar` | Jump Bar | `J` | open in last-used mode |
| `command://dorico/jump-bar-commands` | Jump Bar: Commands | none (`Ctrl+1` works inside the bar only) | open in Commands |
| `command://dorico/jump-bar-goto` | Jump Bar: Go To | none (`Ctrl+2` inside the bar only) | open in Go To |
| `command://dorico/go-to-bar` | Go To Bar | `Cmd+G` | open Go To mode prefilled with `b` (replaces the dialog) |

Upstream edits: none.

## Deferred / unsupported
- In-bar option editing (checkboxes, menus) for parametrised commands; `Opt-Return` opens a dialog only where one exists.
- Real Go To Bar **dialog** with Flow menu (replaced by Go To mode prefilled `b`).
- Multiple flows (`fn`, `fp`, `f≥2`); Play-mode behaviour (playhead to start of bar only).
- The bar is hosted next to the popovers, so it exists in Write mode only (not Setup, Engrave, Play).
- Key Commands page alias editing (aliases are jump-bar only); direct-open key per mode beyond the three above.
- Options-dialog search (`Cmd-L`, `Cmd-F`, `Cmd-G` inside Dorico option dialogs).
- Dorico has no Edit > Find; MuseScore's `Ctrl+F` search popup is unbound (MuseScore-only keys are removed, see README); the Jump bar replaces it.

## Acceptance criteria
- [x] `J` opens the bar (text field focused), `Esc` closes it without running anything; typing letters does not trigger note shortcuts.
- [x] Typing `remove rests` lists *Remove Rests* with its key/alias; `Return` runs it, selection state preserved; only enabled commands appear.
- [x] `Down` on an empty field lists the 5 most-used commands (after running some); reopening pre-selects the last command.
- [x] `Remove Rests\=rr` + `Return` runs and assigns; typing `rr` then ranks it first; `Opt+Return` assigns without running.
- [x] Go To: `b32` selects bar 32 and scrolls; `rK`/`rk` jumps to rehearsal mark K; `p6` goes to page 6; `b+4` moves 4 bars on; `f1b3` works; `f2` shows the error state.
- [x] `Cmd+G` opens Go To with `b`; `Ctrl+1`/`Ctrl+2` switch modes inside the bar.
- [x] `J` no longer triggers enharmonic spelling; `Ctrl+G`/`Cmd+G` no longer adds figured bass.
- [x] Bar jump moves the playhead to that bar.

## Tests
- `src/dorico/tests/gotoparser_tests.cpp`: valid (`b32`, ` B 32 `, `b+4`, `p6`, `p+6`, `rK`/`rk`, `f1`, `f1b20`) and invalid (`b`, `bx`, `b0`, `p0`, `r`, `f`, `f2`, `fn`, `fp`, `f1b`, `f1b+3`, `b-4`, overflow, empty).
- `src/dorico/tests/commandcatalog_tests.cpp`: ranking tiers, all-words rule, exact alias first, usage tie-break, empty query, top-5, `splitAlias`. No fake `ICommandsRegister` is needed: the catalogue is a plain list handed to the pure functions.
- Manual (UI, on a 40-bar score with rehearsal marks A-K over 2 pages): J / Esc, `remove rests`, Down top list, `\=` alias with Return and Opt-Return, `b5`, `b32`, `rK`/`rk`, `p2`, `b+4`, `f1b3`, `f2` (error), Cmd+G, Ctrl+1/2, J and Cmd+G with a note selected (no enharmonic / figured bass), playhead. All passed.
- The UI driver (`uictl.sh type`) keeps the modifier of the previous combo: after `key mscore cmd+g`, `type 5` arrives as Cmd+5 and is lost. Use `key mscore 5` after a modifier combo.
