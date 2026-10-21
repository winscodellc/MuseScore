<!-- markdownlint-disable -->

# 96 - Runtime performance (selection, mixer, buttons, rendering)

Status: built, pending review - Depends on: 95 - Dorico refs: none (behaviour is unchanged; this spec is about speed)

## Goal

Remove the per-event costs the fork added. These include work on every selection change, fader move, scroll or zoom, and per-button rendering. The aim is for selecting, scrolling, playback meters and fader drags to feel as smooth as Dorico. **Measure first**: every fix below states a before/after number.

## How to measure

- Add a `DORICO_PERF` log category (off by default). Wrap the hot paths in `QElapsedTimer` timers: selection change → command states, `DoricoStatusModel` update, mixer `controlParamsChanged`.
- Profile with the QML profiler (`QML_PROFILER` / Qt Creator) for binding storms. Use Instruments (Time Profiler) for C++.
- Use the score `demos/Dawn.mscx` and a large one (`demos/goldberg.mscz`), selecting single notes, then a full range (Cmd-A).

## Measurements (2026-10-21, RelWithDebInfo, macOS, Write mode, 3440x1409 window)

Logged with `QT_LOGGING_RULES="dorico.perf.info=true"` (category `dorico.perf`, `src/dorico/internal/doricoperf.h`; info level, because the app's Qt message handler drops debug lines). Frame times: `QSG_RENDER_TIMING=1` and `--args -d`.

| Event | Before | After |
|---|---|---|
| Single-note click, command-state passes | 2 passes: ~2.0 ms + ~0.2 ms (Dawn and goldberg) | 2 passes: ~1.9 ms + ~0.1 ms |
| Clipboard reads per click | 4 | 0 |
| Status model update per click | 85 us (Dawn), 94 us (goldberg) | 75 us, 62 us |
| Cmd-A on goldberg | 2 passes × 70.8 ms | 10.8 ms + 8.8 ms |
| Cmd-A on Dawn | 2.9 ms + 1.3 ms | 1.8 ms + 0.2 ms |
| Popover `status()` calls while scrolling (about 3 s) | 116 (Dawn), 122 (goldberg): about one per frame | 0 |
| Fader drag: engine echoes that overwrite the dragged value | up to 4 of 23 per drag, and the readout jumps | still sent (3-6 per drag), but the strip shows the dragged value, so there is no snap-back |
| Track overview per pitch edit (goldberg) | 1.16 ms rebuild, then QML rebuilds every delegate | 1.1 ms rebuild, and QML is not notified (`trackOverview changed` 0 times) |
| Status-model subscriptions after 10 part switches | 1 update per click (no leak seen) | 1 |

Notes:
- The cold pass (~1.9 ms) is the one in which command states change. Its timer also covers the synchronous `commandStateChanged` listeners (menus, toolbars). The second pass changes nothing and takes ~0.1 ms, so the computation itself is ~0.1 ms. Two passes per click come from two notifications (selection, then note-input state). At ~0.1 ms the second pass is not worth coalescing.
- Scroll frame times: before, median 2-3 ms, 42% of frames over 16 ms; after, Dawn median 2 ms (45 of 124 frames over 16 ms) and goldberg median 49 ms. The two runs are **not comparable**: the "before" drags were mostly vertical and did not pan the horizontal layout, while the "after" drags did. The slow frames show `blockedForSync` ~50 ms, which is score painting (upstream). The fork's per-frame work, the popover re-validation, is gone.
- First entry into Play on goldberg: a 130-160 ms overview build (all parts × measures, plus QML creation). This is a one-off, so it is left alone.

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
| 10 | `DoricoStatusBar.qml:54-73` | Each status bar builds `NotationStatusBarModel`, `DoricoStatusModel` and `DoricoToolsModel`. Spec 95 leaves only one, so this needs no extra work here. | Check that it is only one after 95. **Done:** one instance (`DoricoShellPage.qml`). |

What was built:
- (1) `DoricoCommandsState` caches the clip (`m_clip`) and refreshes it on `QClipboard::dataChanged`. `pasteSpecialCanExplode` and `CanReduce` are now pure checks on the clip and the number of staves the range covers.
- (2) `SelectionFacts`: `noteList()` and `currentArticulations()` are worked out once per pass.
- (3) `DoricoStatusModel` disconnects the previous notation before subscribing to the new one.
- (4) `DoricoButton` builds the glyph parts and the SVG parts (`Image` + `EffectColorOverlay`) in separate `Loader`s, so glyph buttons have no ShaderEffect. `CurveRendering` stays, because it was chosen for the spec 52 colour fringes.
- (5) The strip shows its dragged value while the fader is pressed. The fork hunk in `mixerpanelmodel.cpp` skips the all-channel solo/mute refresh when the mute state did not change.
- (6) `DoricoShellPage` owns one `MixerPanelModel`, built by an `Instantiator` only once the Mixer or the Play inspector is shown, and one `DoricoMixerModel`. Both are passed to the Mixer, the inspector and the strips.
- (7) Meters rise at once and fall at 30 dB/s, with a peak line held for 1.5 s. Each mixer has one `FrameAnimation`, which stops on the first frame in which nothing is falling.
- (8) The track overview notifies QML only when its content changed. Per-range recompute was not needed: the rebuild is ~1.1 ms on goldberg (there is a `ponytail:` note).
- (9) `PopoverModel`'s `viewMatrix` and `anchorRect` notify through `geometryChanged`, so `status()` and `suggestions()` no longer re-run on scroll. The matrix is bound only while the popover is open.

Anything else the profiler shows above 2 ms per event is added to this table before fixing.

## Deferred / unsupported

- Engine-side meter rate (the 2.5 dB threshold in `muse/`). We don't edit `muse/` casually, and the QML decay hides the steps.
- Layout engine speed (`score()->doLayout()`). That is upstream code.

## Acceptance criteria

- [x] A table of before/after timings is recorded in this spec (see Measurements). The scroll-fps runs are not comparable: re-measure both builds with the same horizontal pan if needed.
- [x] Selecting a note does not touch the system clipboard: the log counter stays at 0.
- [x] The command-state pass on a single-note selection takes under 2 ms: ~0.1 ms of computation, and ~1.9 ms for a pass that includes the listeners of changed states. Cmd-A on goldberg: ~10 ms per pass.
- [x] Dragging a fader: no snap-back; the readout and cap keep the dragged value. Not watched mid-drag, because uictl drags are instantaneous: check by hand in review.
- [x] Meters fall smoothly after playback stops (crops at ~60%, then 15%), with a peak line.
- [x] One `MixerPanelModel` in the shell, shared by the Mixer and the Play inspector (by construction; there is no runtime count).
- [x] Switching parts 10 times: one status update per click before and after the fix (the leak was not reproduced; the disconnect is in place).
- [x] Glyph-only buttons create no ShaderEffect (by construction: the overlay is in the SVG `Loader`). Not checked with the QML profiler.
- [x] Track overview: a pitch edit or a note in a bar that already has notes does not refresh QML (`trackOverview changed` 0 times). The C++ pass still walks every bar (~1.1 ms). A bar changing from empty to filled was not tested.

## Tests

- Add a GoogleTest for the paste-clip cache: the clip is parsed once, and its "can explode" / "can reduce" answers are unchanged from today for the existing `pastespecial` test clips. **Done:** `DoricoPasteSpecial.ExplodeNeedsOneStaffClipAndRangeOfTwoOrMore` and `ReduceNeedsMoreClipStavesThanTheRange` (255/255 pass).
- Manual, run with the `ui-tester` agent: fader drag, meters during playback, scrolling a large score.
