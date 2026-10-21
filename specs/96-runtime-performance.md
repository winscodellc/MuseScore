<!-- markdownlint-disable -->

# 96 - Runtime performance (selection, mixer, buttons, rendering)

Status: todo - Depends on: 95 - Dorico refs: none (behaviour is unchanged; this spec is about speed)

## Goal

Remove the per-event costs the fork added. These include work on every selection change, fader move, scroll or zoom, and per-button rendering. The aim is for selecting, scrolling, playback meters and fader drags to feel as smooth as Dorico. **Measure first**: every fix below states a before/after number.

## How to measure

- Add a `DORICO_PERF` log category (off by default). Wrap the hot paths in `QElapsedTimer` timers: selection change → command states, `DoricoStatusModel` update, mixer `controlParamsChanged`.
- Profile with the QML profiler (`QML_PROFILER` / Qt Creator) for binding storms. Use Instruments (Time Profiler) for C++.
- Use the score `demos/Dawn.mscx` and a large one (`demos/goldberg.mscz`), selecting single notes, then a full range (Cmd-A).

## Suspects (from code reading; confirm with the profiler before changing anything)

| # | Where | Problem | Fix |
|---|---|---|---|
| 1 | `src/dorico/internal/doricocommandsstate.cpp:91-99`, `pastespecial.cpp:84-91,809-821` | Every selection change recomputes about 220 command states. `pasteSpecialCanExplode` / `CanReduce` each read the system clipboard and parse its XML, twice per change. On macOS the pasteboard is synchronous. | Cache the parsed clip: parse it once on `QClipboard::dataChanged` and reuse it. The command-state pass reads only the cache. |
| 2 | `doricocommandsstate.cpp:160-161` | `score->selection().noteList()` is built once per transform command (9 times per pass). | Compute it once per pass and pass it in. |
| 3 | `doricostatusmodel.cpp:57-66` | `subscribeToSelection` never disconnects the previous notation. Subscriptions accumulate when you switch parts or tabs. | Disconnect the old notation first (keep the `async::Asyncable` receiver and call `unsubscribe`). |
| 4 | `DoricoButton.qml:146-202` | Every button makes a `TextMetrics`, a `Text` with `Text.CurveRendering`, an SVG `Image` and an `EffectColorOverlay` (a ShaderEffect). The overlay exists even for glyph-only buttons, and it breaks scene-graph batching. | Create the overlay only for SVG icons, through a `Loader`. Use `NativeRendering` for glyphs at toolbar sizes, or pre-tint SVGs through the icon provider so no shader is needed. Drop `TextMetrics` when the size is fixed. |
| 5 | `src/playback/qml/MuseScore/Playback/mixerpanelmodel.cpp:298-332` (fork hunk) | Every `controlParamsChanged` reloads the output params (FX chain, insert counts for all channels) and loops over all channels. During a drag, the engine's echoed values overwrite the fader, so it jitters. | Ignore echoes while the fader is pressed (the strip reports drag start and end). Reload inserts only when inserts change, not on volume or pan. Keep the upstream hunk small. |
| 6 | `PlayPage.qml:153-166`, `DoricoMixer.qml:150-158`, `DoricoChannelStrip.qml:83-86` | Two `MixerPanelModel`s, each subscribed to every track's signal changes. There is also one `DoricoMixerModel` per strip, and each re-parses the score meta-tag JSON. | Share one `MixerPanelModel` (spec 95's single page makes this simple). Use one `DoricoMixerModel` per mixer, passed to the strips. |
| 7 | `DoricoChannelStrip.qml:339-371` | Meters jump: upstream only sends a level update when the value moves by at least 2.5 dB (`signalnode.cpp:29,88`). With no decay, the bars step instead of falling smoothly. | Add a peak-hold and decay in QML: `Behavior on height { SmoothedAnimation }` or a per-mixer `FrameAnimation` that decays every level. Use one timer per mixer, not one per strip. Do not change the engine. |
| 8 | `trackoverviewmodel.cpp:146-149`, `TrackOverview.qml:55-57,90-124,277-289` | The whole overview is rebuilt on every undo-stack change, walking parts × measures. `trackColor()` reads `overview.tracks.length` per block, which converts the whole QVariantList every time. | Rebuild only the changed measure range (`ScoreChanges` tick range). Cache `tracks.length` and the colour list in a JS property. |
| 9 | `Popover.qml:51`, `popovermodel.cpp:107-111` | `viewMatrix: root.view.matrix` emits `changed()` on every scroll or zoom frame, even while the popover is closed. That re-runs `status()` and `suggestions()`. | Bind the matrix only while the popover is open (`when: root.visible`). Emit a geometry-only signal for matrix changes. |
| 10 | `DoricoStatusBar.qml:54-73` | Each status bar builds `NotationStatusBarModel`, `DoricoStatusModel` and `DoricoToolsModel`. Spec 95 leaves only one, so this needs no extra work here. | Check that it is only one after 95. |

Anything else the profiler shows above 2 ms per event is added to this table before fixing.

## Deferred / unsupported

- Engine-side meter rate (the 2.5 dB threshold in `muse/`). We don't edit `muse/` casually, and the QML decay hides the steps.
- Layout engine speed (`score()->doLayout()`). That is upstream code.

## Acceptance criteria

- [ ] A table of before/after timings is recorded in this spec for: single-note selection, Cmd-A on goldberg, fader drag (events/s and jitter), and scroll fps in Write.
- [ ] Selecting a note does not touch the system clipboard (an `NSPasteboard`-level breakpoint, or a log counter, stays at 0).
- [ ] The command-state pass on a single-note selection takes under 2 ms. Record the Cmd-A number.
- [ ] Dragging a fader is smooth: no snap-back, and the value tracks the mouse.
- [ ] Meters fall smoothly between level updates (no visible 2.5 dB steps at normal speed).
- [ ] One `MixerPanelModel` while the Mixer is open in Play.
- [ ] Switching parts 10 times leaves one selection subscription per status model (log count).
- [ ] The QML profiler shows no ShaderEffect for glyph-only buttons.
- [ ] Track overview: typing a note in Write does not rebuild every row (log the rebuilt range).

## Tests

- Add a GoogleTest for the paste-clip cache: the clip is parsed once, and its "can explode" / "can reduce" answers are unchanged from today for the existing `pastespecial` test clips.
- Manual, run with the `ui-tester` agent: fader drag, meters during playback, scrolling a large score.
