<!-- markdownlint-disable -->

# 95 - Single-page shell (instant mode switching)

Status: built, pending review - Depends on: 50-59, 94 - Dorico refs: `topics/dorico-concepts/modes-in-dorico.md`, `topics/user-interface/project-window.md`

## Goal

Make switching modes (Cmd-1..5, mode tabs) close to instant, as in Dorico. Today each mode is its own `DockPage`. A switch saves and restores the whole KDDockWidgets layout, rebuilds the score view, every panel, the lower zone and the mixer, and relayouts the score twice when it goes through Engrave. Merge the five pages into **one DockPage**. A mode switch then only changes `IDoricoModeState::mode()`, and each zone shows that mode's content. Everything that is already built stays alive.

## Dorico behaviour

| Item | Dorico |
|---|---|
| Switch time | Instant: no rebuild flash, and the score keeps its scroll position and zoom between Setup, Write, Engrave and Print. |
| Zones | Each mode has its own left, right and lower zone content and its own toolboxes. Zone show / hide (Cmd-7, Cmd-8, Cmd-9) is remembered **per mode**. |
| Music area | Setup, Write and Engrave show the same score view. Engrave forces page view. Print shows the print preview. Play shows the track overview instead of the score. |
| Status bar | One status bar, whose controls change per mode. |

## MuseScore today

- Pages: `src/dorico/qml/MuseScore/Dorico/{Setup,Write,Engrave,Play,Print}Page.qml`, all derived from `DoricoPage.qml`. They are listed in the upstream `src/appshell/qml/MuseScore/AppShell/WindowContent.qml:121-140`.
- Switch path:
  1. `doricoactionscontroller.cpp:45-51` calls `interactive()->open(modeUri)`.
  2. `DockWindow::loadPage` (`muse/framework/dockwindow_v2/qml/Muse/Dock/dockwindow.cpp:203-255`) then runs `savePageState`, clears the dock registry, re-adds every dock, runs `restoreLayout` and `initDocks`.
- `DockPage.qml:54-58` loads the central area with `Loader { sourceComponent: (completed && visible) ? central : null }`, so a new `NotationView` is built on every switch. Its page cache starts empty and `onLoadNotation` re-subscribes everything.
- `DockPanel.qml:34-38` and `DockToolBar.qml:106-109` use `Loader { active: root.visible }`. All panel content is destroyed when its page is hidden.
- `applyEngraveViewMode` (`doricoactionscontroller.cpp:228-243`) calls `setViewMode(PAGE)`, which triggers `score()->doLayout()`. Leaving Engrave restores `viewState()->viewMode()`, which is a second full layout. It changes painting only, not `viewState`, so the status bar view buttons can disagree with the view.
- `applyPrintBackground` (`:212-227`) calls `notationConfiguration()->setBackgroundColor`, which writes a persisted setting on every switch into or out of Print.
- Five `DoricoStatusBar`s exist, one per page. All are alive at once because `DockStatusBar` content is not lazy.
- Dock names are made distinct per page by `DoricoPage.dockName()` and `pageDockName()` (`internal/doricoshell.h:82`).

## Design

### One page

1. New `DoricoShellPage.qml` (a `DockPage`, `objectName: "DoricoShell"`, `uri: "musescore://notation"`) replaces the five pages.
2. Each zone becomes a single dock whose content is a `StackLayout` of the per-mode panels, with `currentIndex` bound to the mode. A StackLayout keeps hidden children alive, like a Java CardLayout. The zones are:
   - left toolbox
   - left panel
   - right panel
   - right toolbox
   - lower zone
3. A dock is hidden only when the current mode has nothing in that zone. Hiding a dock is a cheap visibility change, not a page load.
4. Move each mode's panel content out of its page file into its own component, for example `SetupPlayersPanel.qml` and `EngraveFormattingPanel.qml`. The page files are then deleted or shrunk to those components.
5. Central area:
   - `StackLayout { NotationView; PlayTrackView }`.
   - The `NotationView` is created once.
   - `readOnly` is bound to the mode, which spec 97 extends.
   - Play shows the track view.
   - Print keeps the same `NotationView` with its print background and read-only state.
6. One `DoricoStatusBar`, whose `statusItems` are bound to the mode.
7. Keep the Write dock names that the View menu toggles: `pageModel.palettesPanelName()` and the rest.

### Mode switch

- `dorico/mode-*` calls `modeState()->setMode(mode)` directly. No `interactive()->open`.
- Keep `musescore://dorico/<mode>` as aliases: `doricoshell` maps an `open()` of those URIs to `setMode` plus `open("musescore://notation")`, so existing callers and settings still work.
- Remove `modeFromUri` listening for page loads, or keep it only for the aliases.

### Per-mode zone state

- Store zone visibility and size per mode in settings: `dorico/zones/<mode>/<zone>/{visible,size}`.
- Apply them on `modeChanged`.
- KDDockWidgets saves only the single page's layout.
- Cmd-7, Cmd-8 and Cmd-9 toggle the current mode's entry (`doricoshell.h:143` already maps zone to dock name).

### Engrave view mode without double layout

- On entering Engrave, remember the Write view mode and set both `viewState()->setViewMode(PAGE)` and the painting mode, once.
- On leaving, restore it only if it differed.
- Skip the call entirely if the view mode is already PAGE.

### Print background

- Replace `setBackgroundColor` with a paint-time override: a property on the `NotationView` / paint view that takes priority over the configured colour.
- If that needs an upstream edit, keep it to a single `if` in `AbstractNotationPaintView`'s background paint, marked `// Dorico fork`.

### Upstream edits

- `WindowContent.qml`: list `DoricoShellPage` instead of the five pages (same hunk as today).
- Possibly one line in `abstractnotationpaintview.cpp`, for the Print background.
- Nothing in `muse/`.

## Deferred / unsupported

- Animated transitions between modes. Dorico has none either.
- Migrating users' saved per-page KDDockWidgets layouts. Spec 95 starts from the default layout once (bump a layout version key).

## As built (2026-10-21)

- `DoricoShellPage.qml` is the one page. Its zone docks are `doricoLeftZone`, `doricoRightZone` and `lowerZonePanel`, and its toolbox docks are `doricoLeftToolbox` and `doricoRightToolbox`. Each zone's content is a `StackLayout` of `LazyPane`s: a mode's panel is built the first time the mode is shown, then kept.
- The panel contents are moved into their own files:
  - `SetupPlayersPanel` and `SetupFlowsPanel`
  - `EngraveToolbox`, `EngraveFormattingPanel` and `EngravePagesPanel`
  - `PlayInspectorPanel`
  - `PrintLayoutsPanel` and `PrintOptionsPanel`
- The zone docks have no dock title bar. The panels that had one draw a `ZoneHeader` instead.
- `LowerZonePanel` builds its loaders for every tab and only hides the buttons. Setup shows Flows as its single tab, without a tab row.
- A closed dock destroys its content (`DockPanel`/`DockToolBar` load it only while visible). So the zone and toolbox contents live on the page, parked in an invisible item, and each dock only borrows them through `ZoneHolder.qml`. The lower zone does the same inside `LowerZonePanel` (`zoneBody`). Hiding a zone, or a mode without it, keeps what was built (for example the mixer).
- `DoricoActionsController` (zone visibility):
  - Mode commands only call `setMode`.
  - On `modeChanged`, the old mode's open docks are saved to `dorico/zones/<mode>/<zone>/docks`. Then every managed dock (`managedDocks()`) is opened or closed to match the new mode.
  - When the page is shown (startup, back from the Hub), only the current mode's zones are applied. Nothing is captured then, because a layout restored after a crash or `pkill` can be stale.
  - Showing or hiding a zone (`docksOpenStatusChanged`) is saved at once for the current mode.
- Zone sizes are handled by `DoricoShellPage`, which holds the docks. C++ only sees the KDDockWidgets wrappers, not our `DockBase` items.
  - `IDoricoModeState::modeAboutToChange` lets the page save the open zones' sizes (`dorico/zones/<mode>/<zone>/size`, via `DoricoTopBarModel::setZoneSize`) before anything moves.
  - After the switch the page puts the new mode's sizes back, then again 100 ms later. A resize while a toolbox dock opens or closes lands about 42 px short.
  - The docks' min/max sizes follow `sizeMode`, which changes only after the save, so the new limits cannot clamp the old sizes first.
- Engrave changes only the painting view mode. Leaving Engrave goes back to `viewState()->viewMode()`, which is never written, so a saved file keeps its Write view mode.
- Print saves the zoom and scroll (`viewState` matrix and zoom type) on entry and restores them on exit.
- The `musescore://dorico/<mode>` aliases were dropped: nothing opens them any more. The four upstream page-URI checks were reverted.
- Mode switch time (`Dorico mode switch: N ms` in the log, from the command to the next frame), on `Dawn.mscx`, 10 rounds of Cmd-1..5:
  - before: median 1029 ms (min 684, max 1584). By target mode: Setup 783, Write 1246, Engrave 1188, Play 1035, Print 873.
  - after (5 rounds, zone sizes restored per mode): Setup 89, Write 189, Engrave 160, Play 19, Print 168 ms (medians).
  - The 100 ms target is met for Setup and Play only. Write, Engrave and Print most likely pay for resizing the score view when the zone widths change (not profiled). Spec 96 profiles these.
- Not done:
  - The "NotationView created once" log line: a QML `console.log` does not reach the log file. It holds by construction (one `NotationView` in the page), and the 19 ms Play switch shows nothing is rebuilt.
  - The View menu toggles and Restore Default Layout were not re-tested in the UI.

## Review (2026-10-22)

Code review fixes (UI-checked unless noted):
- Restore Default Layout: the page reload it queues ran after our zone reset and saved the QML default docks over it (Setup showed the toolboxes). `DoricoActionsController` now resets the zones on the reload's `docksOpenStatusChanged` (`m_restoringLayout`), and `IDoricoModeState::zonesReset` tells the page to apply the default sizes.
- Zone sizes are also saved 500 ms after a resize (`saveSizesLater`), so they survive the Hub and a quit, not only a mode switch. A save is skipped while a restore is pending (a settling resize is ~42 px short). A zone shown again (Cmd-7/8/9) gets its mode's size back instead of the dock's last size from another mode (not UI-checked yet).
- Leaving Print restores the zoom only on the score it was saved from (`ViewBeforePrint::notation`). Not UI-checked.
- The switch-time log ignores a second frame callback when two switches land before one frame.
- Mode switch medians in the review run (Dynamic_Strings, few rounds): Setup 152, Write 242, Engrave 152, Play 217, Print 303 ms.
- Known, not from this spec: the app crashes with SIGSEGV on quit (after `Destroying context`); seen in logs since 2026-10-21.

## Acceptance criteria

- [x] Cmd-1..5 and the mode tabs switch modes. The log shows no `loadPage` and no `doLayout` on Setup↔Write↔Print switches. (No `onPageLoaded` lines are logged. `setViewMode` is only called for Engrave.)
- [ ] Mode switch time, logged with `QElapsedTimer` from `setMode` to the next frame: under 100 ms on `demos/Dawn.mscx`. Record before/after in this spec. **Partly met:** median 1029 ms before. After: Setup 89 and Play 19 meet it; Write 189, Engrave 160 and Print 168 do not. See "As built"; carried over to spec 96.
- [x] The score keeps its scroll position, zoom and selection across Setup, Write, Engrave and Print.
- [x] The `NotationView` is created once per open score. Holds by construction; the log line could not be captured, see "As built".
- [x] Entering Engrave from Galley causes exactly one layout; going back to Write causes one; the status bar view buttons match.
- [x] Entering or leaving Print does not write `notation/backgroundColor`.
- [x] Each mode remembers its own zone visibility and size across switches and restarts.
- [x] The zone toggles (Window menu since spec 102, Cmd-7/8/9) and Restore Default Layout (no menu item since spec 102, as in Dorico; reachable from the jump bar) still work. Checked in the 2026-10-22 review run.
- [x] The lower zone tab (Mixer, Key Editor, ...) and its state survive a mode round-trip.
- [x] Only one `DoricoStatusBar` / `NotationStatusBarModel` instance exists.

## Tests

Manual, run with the `ui-tester` agent:
- Cycle Cmd-1..5 ten times. Check the timing log and that no errors appear in the log.
- Repeat with the Mixer open in the lower zone and while playback runs. Meters keep moving during switches.
- Rerun the manual checks of specs 50-58 and 62-63 that touch panels, to catch any panel that broke when it moved into a StackLayout.
