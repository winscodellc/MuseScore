# 46 — Jump bar (J) and Go To Bar (Cmd-G)

Status: todo · Depends on: [00-module-skeleton](00-module-skeleton.md), [20-popover-framework](20-popover-framework.md) (overlay host in `NotationView.qml`), [10-keymap-preset](10-keymap-preset.md)
Dorico refs: `B = https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/` →
`B user_interface/user_interface_jump_bar_r.html`, `B …/user_interface_jump_bar_commands_performing_t.html`,
`B …/user_interface_jump_bar_going_to_locations_t.html`, `B …/user_interface_jump_bar_aliases_assigning_t.html`, `B …/user_interface_jump_bar_aliases_removing_t.html`,
`B write_mode/write_mode_navigation/write_mode_navigation_go_to_bar_t.html`, `B user_interface/user_interface_key_commands_searching_t.html`
Reference digests: [reference/dorico-editing.md](reference/dorico-editing.md) §5 · [reference/musescore-internals.md](reference/musescore-internals.md) §1, §3

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

## Design
`src/dorico/internal/jumpbar/` + QML `src/dorico/qml/MuseScore/Dorico/JumpBar.qml` (hosted by the overlay `Loader` that spec 20 adds next to `SearchPopup` at `NotationView.qml:~344`; no second upstream edit).

- `commandcatalog.{h,cpp}` — builds the searchable list from `commandInfoList()`, keeps those whose `Availability` includes Dispatch and that are enabled (`commandState(cmd).enabled`), attaches key text via `shortcut(cmd)`. Ranking: exact alias > prefix of title > all query tokens prefix-match some word in title > substring; ties by use count.
- `usagestore.{h,cpp}` — settings `dorico/jumpbar/usage` (map command-uri → count) and `dorico/jumpbar/aliases` (map alias → command-uri), via `muse::GlobalInject<IConfiguration>`-style settings used elsewhere in the module (spec 00).
- `gotoparser.{h,cpp}` — **pure** parser for the Go To syntax producing `GoToTarget{flow?, bar?, rehearsal?, page?, relative}`; resolution **translates to the legacy syntax** and reuses upstream: `b32` → `"32"`, `p6` → `"p6"`, `rK` → `"K"`/`"rK"` as `SearchCommandsParser` expects (verify exact rehearsal string form), then `notation->elements()->search(text)` + `interaction->select(...)` + `interaction->showItem(item)` (`notationinteraction.cpp:8450`) + playhead move (`IPlaybackController`). Relative forms (`b+32`, `p+6`) = offset from the current bar/page (**our interpretation**, document in code). Flow: `f1`/`f1b20` accepted, `f≥2`, `fn`, `fp` → invalid (single flow).
- `jumpbarmodel.{h,cpp}` — QML-exposed (`QML_ELEMENT`, `Q_PROPERTY(mode, text, results, selectedIndex, error)`, `Q_INVOKABLE accept(bool optionKey)`), handles `Ctrl+1/2` mode switch, `Down` on empty → top-5 usage, `\=` alias syntax.
- Execution: `ICommandDispatcher::dispatch(cmd)`; commands with required parameters are not run directly — `Opt-Return` is documented to open their dialog; if none exists the entry is shown disabled.
- Error state: invalid Go To text → red field, Return does nothing (undocumented in Dorico; our choice).

| Command | Title | Key (mac) | Behaviour |
|---|---|---|---|
| `command://dorico/jump-bar` | Jump Bar | `J` | open in last-used mode |
| `command://dorico/jump-bar-commands` | Jump Bar: Commands | `Ctrl+1` (physical Control = Qt `Meta`) | open/switch to Commands |
| `command://dorico/jump-bar-goto` | Jump Bar: Go To | `Ctrl+2` | open/switch to Go To |
| `command://dorico/go-to-bar` | Go To Bar | `Cmd+G` | open Go To mode prefilled with `b` (replaces the dialog) |

Upstream edits: only the overlay Loader owned by spec 20.

## Deferred / unsupported
- In-bar option editing (checkboxes, menus) for parametrised commands; `Opt-Return` opens a dialog only where one exists.
- Real Go To Bar **dialog** with Flow menu (replaced by Go To mode prefilled `b`).
- Multiple flows (`fn`, `fp`, `f≥2`); Play-mode behaviour (playhead to start of bar only).
- Key Commands page alias editing (aliases are jump-bar only); direct-open key per mode beyond the three above.
- Options-dialog search (`Cmd-L`, `Cmd-F`, `Cmd-G` inside Dorico option dialogs).
- Dorico has no Edit > Find; MuseScore's `Ctrl+F` search popup is unbound (MuseScore-only keys are removed, see README); the Jump bar replaces it.

## Acceptance criteria
- [ ] `J` opens the bar (text field focused), `Esc` closes it without running anything; typing letters does not trigger note shortcuts.
- [ ] Typing `remove rests` lists *Remove Rests* with its key/alias; `Return` runs it, selection state preserved; only enabled commands appear.
- [ ] `Down` on an empty field lists the 5 most-used commands (after running some); reopening pre-selects the last command.
- [ ] `Remove Rests\=rr` + `Return` runs and assigns; typing `rr` then ranks it first; `Opt+Return` assigns without running.
- [ ] Go To: `b32` selects bar 32 and scrolls; `rK`/`rk` jumps to rehearsal mark K; `p6` goes to page 6; `b+4` moves 4 bars on; `f1b3` works; `f2` shows the error state.
- [ ] `Cmd+G` opens Go To with `b`; `Ctrl+1`/`Ctrl+2` switch modes inside the bar.
- [ ] `J` no longer triggers enharmonic spelling; `Ctrl+G`/`Cmd+G` no longer adds figured bass.
- [ ] Bar jump moves the playhead to that bar.

## Tests
- `src/dorico/tests/gotoparser_tests.cpp`: valid (`b32`, `B32`, `r K`?, `rk`, `p6`, `b+32`, `p+6`, `f1b20`) and invalid (`b`, `bx`, `f2`, `fn`, empty) inputs → `GoToTarget`; plus translation to legacy strings fed to `SearchCommandsParser::parse` and asserting `SearchData::type`.
- `commandcatalog_tests.cpp` with fake `ICommandsRegister`/`ICommandsState`: ranking, enabled-only filter, alias precedence, usage top-5 (follow the mocking style in `muse/framework/rcommand/tests`).
- Manual: open on a 40-bar score with rehearsal marks A–K across 3 pages.
