<!-- markdownlint-disable -->
# 50 - UI shell and modes
Status: done  -  Depends on: 00, 10  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/user_interface/user_interface_windows_c.html`, `.../user_interface_toolbar_r.html`, `.../user_interface_transport_options_r.html`, `.../user_interface_panels_r.html`, `.../user_interface_tab_bar_r.html`

Implementer: **Opus** (dock framework quirks, upstream edit). Every other 5x spec builds on this one, so land it first.

## Goal
Replace MuseScore's score page with a Dorico-style window: a top bar with the five mode tabs, one dock page per mode (empty at first), mode state, and the mode and zone keys. Later specs fill the pages.

## Dorico behaviour
See `reference/dorico-ui.md` 0.2, 0.3, 0.4 and 0.6.

| Item | Behaviour |
|---|---|
| Mode tabs | Setup / Write / Engrave / Play / Print, one active. Write is the default when a project opens |
| Layout selector | Choose the layout (score or a part) shown in this tab; disabled in Print |
| View menu | Show/hide Tabs, System Track, Comments, ... (only the items MuseScore can back; see Deferred) |
| Hide/Restore Zones | One button toggles all open zones |
| Mixer | Opens the mixer (MuseScore: toggle the mixer panel) |
| Mini transport | Activate Project (deferred), time, rewind, cycle, play, record, click, tempo, scrub, transport window |
| Undo / Redo, Help | |
| Keys | Cmd-1..5 modes; Cmd-6 toolbar; Cmd-7 left zone; Cmd-8 lower zone; Cmd-9 right zone |

## MuseScore today
See `reference/musescore-internals.md` section 7. Key points:
- `WindowContent.qml:100-118` hard-codes the pages; `NotationPage.qml` has `uri: "musescore://notation"`.
- Top-row toolbars: `NotationToolBar`, `PlaybackToolBar`, `UndoRedoToolBar` (reusable as they are).
- Window-level `MainToolBar` (Home/Score/Publish) is drawn from `WindowContent.qml:64-98`.
- `IDoricoModeState` only has Chord/Insert flags (spec 11).
- Cmd-1..5 are reserved/unbound by spec 10 (`README.md` deferred list). Check `shortcuts_dorico.json` for Cmd-6..9 and any MuseScore binding on them before adding rows.

## Design
New files in `src/dorico/qml/MuseScore/Dorico/` (add to that folder's `CMakeLists.txt`; add `IMPORTS` for the dock, appshell, notationscene, playback and instrumentsscene QML targets):
- `DoricoPage.qml`: a base component wrapping `DockPage` (one `Item` containing the shared parts: top bar, `central: NotationView`, `statusBar`). Each mode page sets only its toolboxes and panels.
- `WritePage.qml` (**keeps** `uri: "musescore://notation"`), `SetupPage.qml`, `EngravePage.qml`, `PlayPage.qml`, `PrintPage.qml` (`musescore://dorico/<mode>`). At first each contains only the shared parts.
- `DoricoTopBar.qml`: mode tabs (use `RadioButtonGroup` + `PageTabButton` like `MainToolBar.qml:70-103`), layout selector (bind to the existing excerpts/parts list: find how `NotationParts` / `excerpts` are listed for the Parts dialog), View menu, Hide/Restore Zones, Mixer, then the existing `PlaybackToolBar` and `UndoRedoToolBar`, then Help.
- `DoricoModeModel` (C++ or QML-exposed, `internal/`): holds `currentMode`, dispatches `loadPage` via `IInteractive`/`InteractiveProvider` (the same call `MainToolBarModel` uses to open a page URI).

C++ changes:
- `idoricomodestate.h`: add `enum class Mode { Setup, Write, Engrave, Play, Print }`, `mode()`, `setMode()`, and a `modeChanged()` notification, following how Chord/Insert flags are done. Mode is per window (it lives in the context service).
- `doricocommands.h`: `command://dorico/mode-setup|write|engrave|play|print`, `dorico/toggle-zone-left|lower|right`, `dorico/toggle-all-zones`. Register in the commands register and state (always enabled).
- `DoricoModule`: register the four `musescore://dorico/<mode>` URIs with `IInteractiveUriRegister::registerPageUri(uri, ..., PageType::PrimaryPage)`.
- Zone toggles dispatch `dock-toggle` with the panel names of the active page (see `NotationPageModel::toggleDock` `notationpagemodel.cpp:195-209` for the call). Each page declares which panel names are its left / lower / right zone.
- Keys: add Cmd-1..9 rows to `shortcuts_dorico.json` and update `tools/check_keymap.py` data if it lists reserved keys.

Upstream edits (the only ones for this spec):
1. `WindowContent.qml`: `import MuseScore.Dorico`; replace the `NotationPage {}` entry by `WritePage {}`, `SetupPage {}`, `EngravePage {}`, `PlayPage {}`, `PrintPage {}`; hide the window-level `MainToolBar` bar while a Dorico page is open (or empty its `toolBars`). Keep `HomePage`, `PublishPage`, `NotationReviewPage`.
2. Nothing else. The Score tab keeps pointing at `musescore://notation`, which is now `WritePage`.

Idiom note (Java/TS): QML files are like React components that are compiled into a module; `IDoricoModeState` is an interface injected like a Spring bean, and `Q_PROPERTY`-style notification (`modeChanged`) is the observer pattern.

Risk: page layouts are saved per page name. If the page `objectName` of the old `NotationPage` is reused, a stale saved layout from MuseScore's page may load. Give `WritePage` a new `objectName` and check that a saved layout is not restored wrongly.

## As built (decisions taken while implementing)
- **Write keeps MuseScore's panels for now**, arranged as Dorico zones: left = Palettes + Layout (instruments), right = Properties + Selection filter + History, lower = Mixer, Piano keyboard, Timeline, Percussion; the Note input bar stays. Specs 52-54 replace them. Setup/Engrave/Play/Print contain only the shared parts (top bar, score, status bar).
- **Home and Publish buttons** sit at the far left of the Dorico top bar, because the Home/Score/Publish bar is hidden on Dorico pages (`WindowContent.qml` closes `mainToolBar` in `onPageLoaded` on `musescore://notation` and `musescore://dorico/*`, opens it elsewhere). The MuseScore bar's Score tab still opens `musescore://notation` = `WritePage`.
- Zones are a C++ table, not declared per page: `internal/doricoshell.h` `zoneDocks(Mode, Zone)` lists dock names. Specs 55-58 fill the table for their mode. Zone commands call `IDockWindow::setDockOpen`; hidden docks are remembered so the zone comes back with the same panels.
- Mode state follows the page: `DoricoActionsController` listens to `IInteractive::currentUri()` and sets `IDoricoModeState::mode`. Mode commands only open the page URI.
- New command `dorico/toggle-toolbar` (Cmd-6) shows/hides the three top-level docks (`doricoTopBar`, `playbackToolBar`, `undoRedoToolBar`).
- Only `WritePage` calls `NotationPageModel::init()`; it subscribes to `dock-toggle` requests, so initialising it on every page would handle each request once per page.
- Page `objectName`s are new (`DoricoWrite`, `DoricoSetup`, ...), so MuseScore's saved "Notation" layout is not restored.
- View menu holds the six score display toggles (invisible, unprintable, frames, page borders, sound flags, irregular measures). Help opens the online handbook.
- `dorico/mode-*` keys are enabled only while a project is open and no text is being edited (so Ctrl+digit keeps its `edittext-advance-*` meaning).
- **Fixed after spec 50:** notation, playback and app-shell command states only counted `musescore://notation` as the project page (`interactive()->isOpened(...)`), and `UiContextResolver` returned `UiCtxUnknown` on the other pages, so every notation/playback command and legacy shortcut was disabled on Setup/Engrave/Play/Print. Four small upstream edits (marked `// Dorico fork:`) also accept a `musescore://dorico/*` page; see README, cross-cutting design 1.

## Deferred / unsupported
| Item | Reason |
|---|---|
| MuseScore Notation toolbar (Save/Print/...) and Extensions toolbar on Dorico pages | Not in Dorico's toolbar; the extensions toolbar returns if a spec needs it |
| Home page, Publish page | Stay as MuseScore's |
| Tab bar (multiple tabs/windows per layout), split partitions | No multi-view model; the layout selector replaces tabs |
| Activate Project, Scrub Playback, Transport Window, Video, MIDI Record | No MuseScore equivalent (record: only with MIDI input, later) |
| View menu items for System Track, Comments, Highlight... | No backing feature; only items MuseScore has (e.g. page number, invisible, frames) are listed |
| Help overlay | Maps to MuseScore's Help menu |

## Acceptance criteria
- [x] Opening a score shows `WritePage` with the Dorico top bar; no Home/Score/Publish bar inside the score.
- [x] The five mode tabs switch pages (empty pages are fine) and the active tab is highlighted; Cmd-1..5 do the same.
- [x] Layout selector lists the score and every part, and switching it changes the displayed notation.
- [x] Transport (play, rewind, cycle, click, tempo, time) and undo/redo work as in MuseScore.
- [x] Cmd-7/8/9 toggle the left/lower/right zone of the active page; Hide/Restore Zones toggles all.
- [x] Layout per page persists across restart; Home and Publish still work.
- [x] `check_keymap.py` passes.

### After the side-by-side screen comparison with Dorico 6 (2026-10-16)

- Top bar: no Home / Publish buttons (now **Window > Hub** = `command://dorico/open-hub`, MuseScore's Home page, enabled without a project, and **File > Publish** = `command://dorico/open-publish`). Mode tabs are one rounded strip (`#3D4A58`, current tab `#2C678F`); the View menu is an eye icon and Hide/Restore Zones a window icon, together in a second strip.
- The score/part tab row above the music is hidden on the Dorico pages (`NotationView.showNotationTabs`, upstream edit `// Dorico fork`); layouts are chosen in the top bar.
- Title bars only where Dorico has them: none on Write's Notes panel, Engrave's panels, Play's left zone, Print Options and the lower zone (`floatable: false; closable: false`, which makes DockFrameModel drop the title bar).

## Verification status
Built, unit-tested (`dorico_tests`, `check_keymap.py`) and checked by hand in the app: all acceptance criteria pass.

## Tests
- GoogleTest (`tests/doricoshell_tests.cpp`, passing): `Mode` state (default, set/get, `modeChanged` fires once per change), mode <-> URI round trip, zone table.
- Manual: open a score, click through the five tabs, press Cmd-1..9, restart and check the layout is remembered.
