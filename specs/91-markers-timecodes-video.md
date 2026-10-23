<!-- markdownlint-disable -->

# 91 - Markers, timecodes and video

Status: in-progress (phase A done, B next) - Depends on: 26 (tempo popover), 53 (Video panel stub), 55 (Flows panel), 57 (Play mode track overview), 63 (Dorico mixer), 102 (Window menu) - Dorico refs: the local manual mirror, pages listed under "Dorico behaviour"

## Goal

Score to picture the way Dorico Pro 6.2 does it. The user writes music to film clips and finds the hit points themselves by watching the clip, so the video player has to be in the first usable version. The video stays **frame-accurate** when the playhead stops or moves. Everything the manual documents for markers, timecodes and video is in scope. Anything we cannot do goes under Deferred, with the reason.

The user's words, from the brainstorm (2026-10-23):
- "Writing music to film clips, start with markers and timecodes."
- "I find them myself."
- "I will need to be able to open the video player exactly as Dorico does it."
- "Frame-accurate, exactly like Dorico."
- "I want it to be implemented exactly as how Dorico does it."

Non-goals: Dorico's Tempo track and Tempo editor (roadmap 88), standalone tempo-track import/export (roadmap 76), multiple flows (roadmap 86), DeckLink / external video output, and a Video track with thumbnails in Play mode (it is in Dorico 6's binary, `showVideoTrack`, but the 6.2 manual does not document it).

Anti-goals, which make it a failure even if it technically works:
- The picture shows the wrong frame when stopped on a hit.
- The picture drifts against the music during playback.
- Markers stay glued to a beat when the tempo changes. They are glued to **time**.
- The score file breaks or loses data when opened in a build without this feature.

## Evidence (how Dorico does it)

From the 6.2 manual (local mirror `specs/reference/manual/topics/`), plus a REA / `strings` pass over `/Applications/Dorico 6.app` on 2026-10-23. Binary evidence is architectural only. No code was looked at for copying.

- **Video is a separate plug-in**: `Contents/Components/videoengine.bundle`, which links AVFoundation, VideoToolbox, Metal, FFmpeg and Bento4. Video audio is extracted by `dorico-extractaudio.bundle` into a cached soundtrack and played through an engine mixer channel (`VideoSoundtrackLRUCache`, `createVideoSoundtrackChannelIfRequired`).
- **Video is per flow**: `ScoreVideoManager`, `FlowVideoData`, `AttachVideoAction`, `DetachVideoAction`, and the keys `kVideoEventAttachmentPosition`, `kVideoClipTimecodeStart`, `kVideoChangeOffset`, `timeDisplayPlaybackOptions.projectFrameRate`.
- **Frame rates**: an 11-value enum, `EnumStringMapping<FrameRate, 11>`: 23.98, 24, 24.98, 25, 29.97, 29.97 dfps, 30, 50, 59.94, 59.94 dfps, 60.
- **Markers**: `MarkerEvent`, with `kMarkerText`, `kMarkerIsImportant`, `kTimecode`. Also `AddMarkerDialog`, `FindTempoDialog`, `FoundTemposModel`, `TimecodeSpinBox`, `MIDIFileExportMarkerHelper`, `kTimecodeStave`, `markerLayoutOptions.timecodeFrequency` (`kEverySystem` / `kEveryNBars` / `kEveryBar`), and `markerEngravingOptions.showHoursInTimecode` (and Minutes / Seconds / Frames).
- **Find Tempo widgets**: `beatUnit`, `lowerValue` / `upperValue`, `integralTempoOnlyButton`, `allowQuarterStepTempoButton`, `foundTemposTable`, `markerTable`, `considerTempoChanges` (undocumented, so not built).

## Dorico behaviour

Manual pages (all under `topics/`): `setup-mode/videos*.md`, `notations-input/inputting-markers-timecodes*.md`, `notation-reference/markers*.md`, `notation-reference/timecodes*.md`, `play-mode/tracks/markers-track*.md`, `playback/transport-window*.md`, `project-window/toolbar/mini-transport.md`, `file-import-and-export/exporting-midi.md`, `page-formatting/staff-spacing/vertical-spacing-options.md`. Re-read the page before building each part.

### Time model

| Concept | Dorico |
|---|---|
| Project frame rate | One per project, chosen from the 11 rates. Defaults to the video's rate ("Use video frame rate"). It can be set with no video attached. |
| Timecode format | `hh:mm:ss:ff`. Drop frame uses `;` before the frames and the suffix "dfps", non-drop uses `:` and "fps". Drop frame skips frame numbers 00 and 01 every minute except every tenth minute. |
| Timecode start | The timecode at the start of the video. |
| Flow attachment position | The rhythmic position where the video starts: Beat count x Beat unit, counted from 0. For example, 8 quarters in 4/4 is bar 3 beat 1. |
| Video start offset | The position inside the video that lands on the attachment position. Earlier video is still shown if it falls inside the flow. |
| Flow start timecode | `Timecode start + Video start offset - musicTime(attachment position)`. Manual example: 02:00:00:00, attached at 8 beats, 60 bpm, gives a flow start of 01:59:52:00. It is shown on the flow card. |
| Marker | Text (default "Marker"), a timecode and an important flag. It is locked to **time**, so changing the tempo moves it against the music. It cannot be input in negative time, that is before the flow start. |

### Video

| Action | Dorico |
|---|---|
| Attach | Setup mode > Flows panel > right-click the flow > Video > Attach > file dialog > Video Properties dialog opens > OK. The flow card shows a film-reel icon and a timecode. Re-attaching a missing file keeps all the old settings. The Video submenu is **Attach... / Detach / Properties...**. The flow card shows the flow start timecode at bottom left, in small text, but not while it is 00:00:00:00. |
| Missing file | The flow card shows a warning icon instead of the reel. |
| Properties | Video > Properties, or the **Properties...** button in the Write-mode Video panel. Fields: Video file (read-only), Video frame rate (read-only), Project frame rate (menu), Use video frame rate, Flow attachment position (Beat count + Beat unit), Video start offset, Timecode start. **With no video** (checked in Dorico 6.2), only Project frame rate (default 25 fps), Flow attachment position (Beat count 0; Beat unit is a row of toggle buttons: 16th, 8th, quarter (default), half, whole, plus single dot and double dot), Video start offset and Timecode start (both 00:00:00:00) are shown. Timecode fields are masked overtype spin boxes: a digit replaces the one at the cursor, an impossible digit is ignored, and Up / Down step the field under the cursor. |
| Detach | Video > Detach. |
| Video window | A separate window, shown and hidden with **F4**, the toolbar's **Show Video** button, or **Window > Video**, in any mode. Drag an edge or corner to resize it, or Shift-drag to keep its shape. The size is remembered for all projects. It plays in sync with the music. |
| Video audio | Played in sync. A **Video** channel in the Mixer (shown with the toolbar's **Video** filter) has a fader and mute. |
| Formats | MOV (H.263, H.264, ProRes, DV, DNxHR), MP4 (H.263, H.264), AVI (DV, MJPEG). Constant frame rate only, up to 8K. |

### Markers and timecodes: input and editing

| Action | Dorico |
|---|---|
| Add Marker | **Shift-Alt-M** in Write mode, the **+** button in the Video panel's Markers section, or **Add Marker** in the Markers track header in Play mode. Opens the **Add Marker** dialog: Text ("Marker", pre-selected), Timecode (defaults to the playhead), "Valid range: 00:00:00:00 - 00:01:04:00" (the flow's timecode range, read-only), Cancel / OK. A timecode outside the range is **clamped** into it, with no message. |
| Video panel (Write, right zone) | Video section: **Properties**. Markers section: a table with Timecode / Text / Imp. columns. Double-click Timecode or Text to edit. Imp. is a checkbox, and important rows are shown bold. Action bar: **+** (Add Marker) and a trash button (Delete Marker, one at a time, enabled once a row is selected). Below it is a full-width **Find Tempo...** button (enabled when at least one marker is important). Double-clicking a timecode opens a timecode spin box in the cell. |
| Change timecode | Editing it in the table moves the marker against the music. |
| Marker text | Also editable in the Properties panel (Markers group, **Marker text**) in Write and Engrave mode. |
| Markers track (Play) | A lane between Tempo and Chords showing each marker as a magenta pill with a white diamond and its text, plus a **+** (Add Marker) button in its header that adds at the playhead. One per project. Hidden by default; it is shown with a toggle in the track list header (metronome = Tempo, pin = Markers, "C7" = Chords). |
| MIDI export | Markers are included in exported MIDI by default. |
| Transport display | Clicking the mini transport time display cycles through bars/beats/ticks "2.2.1.009" (bar.beat.sixteenth.tick), elapsed time "00:00:02.541" and timecode "01:00:02;16" (including Timecode start). A new project starts on bars and beats. The Transport window (F2) toggles elapsed time and timecode. Preferences > Play sets the default. |

### Markers and timecodes on the page

| Setting | Where in Dorico | Values |
|---|---|---|
| Show markers | Layout Options > Markers and Timecode (per layout) | On in full score layouts, off in parts |
| Marker vertical position | Layout Options (per layout) | Above system / Below system / Timecode staff (+ "Position timecode staff above bracket") |
| Timecode frequency | Layout Options (per layout) | Start of system / Every bar / Never, + "Show timecode at start of first system" |
| Show timecode | Layout Options (per layout) | Above or below start of system / Below timecode staff, + "Timecode position relative to system" (Above / Below), + "Offset at start of system" |
| Timecode staff gap | Layout Options > Vertical Spacing | "Timecode staff to staff" |
| Timecode in markers | Engraving Options > Markers (project-wide) | Show / Do not show, + order: Text above timecode / Timecode above text |
| Show in timecode | Engraving Options > Markers | Hours / Minutes / Seconds / Frames checkboxes |
| Border style | Engraving Options > Markers > Design | Rectangle / Rounded rectangle / Capsule / Angled ends rectangle, + enclosure line thickness, + default vertical position |
| Fonts | Library > Font Styles | Marker Text Font, Marker Timecode Font, Timecode Font (font, size, style, underline) |

The timecode staff is a single-line staff above a chosen bracketed instrument family. Markers sit on it and timecodes go below it. There is only ever one per system.

### Find Tempo dialog

It considers the markers of one flow and needs at least one important marker.

| Field | Behaviour |
|---|---|
| Flow | Read-only name. |
| Beat unit | For example, dotted quarter for 6/8. |
| Tempo increments | Integral only, or allow increments of 0.25. |
| Tempo range | Minimum and maximum BPM. |
| Tempos found | Columns BPM, IFO, NFO, AFO: the average frames by which important, non-important and all markers miss a significant beat, early or late. Sorted by IFO, best first. Recomputed live when the options change. |
| Markers | Per marker for the selected tempo: Timecode, Text, Imp., Fr. Off (frames from the nearest beat), Time Diff. (seconds), Not. Diff. (fractions of a whole note). |
| Selected tempo | Shows the selected tempo. |
| Apply | Puts that tempo as a tempo mark at the start of the flow and **deletes every other tempo mark in the flow**. |

## MuseScore today

- **No timecode, SMPTE or frame-rate code** in `src/`. MuseScore's `engraving/dom/marker.cpp` is segno / coda, which is unrelated.
- **Tick to time**: `Score::utick2utime` / `utime2utick` (`engraving/dom/score.h:665-666`), `tempoTimeline(bool expandRepeats)` (`score.h:668`, `engraving/dom/tempotimeline.h`), `INotationPlayback::secToTick` (`notation/inotationplayback.h:75`). Playback position in seconds comes from `IPlaybackState::playbackPosition()` / `playbackPositionChanged()` (`context/iplaybackstate.h:43-44`).
- **Per-score data precedent**: JSON in `Score::metaTag`, written undoably with `ChangeMetaTags` (`dorico/.../doricomixermodel.cpp:42,137-144`). Per-layout JSON lives in the excerpt's metaTag with a fallback to the master (`printmodel.cpp:49,221,236`).
- **Stubs to fill**:
  - Video toolbox button: `NotationsToolbox.qml:67`. Its panel shows `DoricoPlaceholder` (`NotationsPanel.qml:52-55`, `internal/notationspanels.h:73`).
  - Flow card context menu: `ctxStub("flow-video","Video")` at `internal/contextmenu.cpp:343`.
  - Toolbar Video button: `DoricoShellPage.qml:547` (`command:""`).
  - Window > Video: `stub("Video")` at `internal/doricomenu.cpp:565`.
  - F4: deferred in `tools/dorico_keys.json:53`. Shift+Alt+M is unbound.
  - Mixer Video filter: `DoricoMixer.qml:220` (`kind:""`).
- **Track overview lanes**: Tempo and Chords are a `Repeater` at `TrackOverview.qml:168-180`, fed by `TrackOverviewModel` `tempos` / `chords` (`trackoverviewmodel.cpp:217-220`). The x position is quarter notes times `pxPerQuarter`.
- **Transport**: the shell embeds MuseScore's `PlaybackToolBar` (`DoricoShellPage.qml:502`; time field `PlaybackToolBarActions.qml:125`, bars/beats L150). There is no cycling, and the pref "Elapsed Time / Timecode / Bar.Beat.Ticks" exists but is unused (`prefstable.cpp:686`).
- **Options dialogs**: Cmd-Shift-L / Cmd-Shift-E open MuseScore's Page Settings / Style dialog. Font Styles is a stub (`doricomenu.cpp:535`). Properties is MuseScore's `PropertiesPanel`.
- **Measure numbers are the layout precedent**: they are created and laid out by the layout engine, styled per score / excerpt, and never saved by the user (`rendering/score/measurenumberlayout.h:33`).
- **MIDI**: a `META_MARKER` writer already exists for rehearsal marks (`importexport/midi/internal/midiexport/exportmidi.cpp:398-418`).
- **Tempo insertion**: `tempopopover::createTempoText` / `addTempoText` (`popover/handlers/tempohandler.cpp:104,133`), file-local for now.
- **Video**: no Qt Multimedia in the build (`muse/buildscripts/cmake/SetupQt6.cmake`) or in our installed Qt 6.11.2. `muse/framework/media` is an FFmpeg **encoder** for video export. It loads FFmpeg at runtime from a user-set folder (`MediaConfiguration::ffmpegLibsDir`), so FFmpeg is not bundled.

## Design

Everything lives in `src/dorico/`, in a new `video/` folder. The exceptions are called out under "Upstream edits".

### Decisions made with the user
- The full manual scope, "exactly as Dorico".
- Frame-accurate video.
- Video in the first usable version: markers and timecodes first, video right after.

### Calls made by the designer (the user can overrule them)
1. **One flow means one video per score.** The Flows panel's single card carries the Video submenu.
2. **Storage**: one JSON blob in the master score's metaTag `doricoVideo`, written undoably with `ChangeMetaTags` (the mixer precedent). It holds the frame rate, timecode start, attachment position (beat count + beat unit), start offset, the video path (absolute, plus relative to the score file, tried in that order), video volume and mute, and the marker list. A build without the fork just carries the tag along untouched.
   - Per-layout settings (show markers, position, frequency, ...) go in each score's / excerpt's metaTag `doricoMarkersLayout`, falling back to defaults (the `printmodel.cpp` pattern).
   - Project-wide engraving settings and fonts go in the master's `doricoMarkersEngraving`.
3. **Marker time is stored as seconds of real time since timecode 00:00:00:00**, as a double, with a stable id, text and the important flag. A marker is a timecode; Dorico's marker carries `kTimecode`.
   - Changing the project frame rate keeps every marker at the same instant and only changes how its timecode reads.
   - Changing the attachment position or the start offset keeps markers at their timecode, so they move against the music. That is the point of markers.
   - Changing **Timecode start** moves every marker by the same amount, so the markers stay on the music. Checked in Dorico 6.2 (2026-10-23): with Timecode start 00:00:00:00 -> 01:00:00:00, a marker at 00:00:03:00 became 01:00:03:00. Built that way, in the same transaction.
   - Not measured yet: what attachment position and start offset do to markers. Phase A keeps the markers' timecodes. Check it in phase B, with a video attached.
4. **Video playback uses Qt Multimedia**: `QMediaPlayer` + `QAudioOutput` + QML `VideoOutput`, found with `find_package(Qt6 OPTIONAL_COMPONENTS Multimedia)` in `src/dorico/CMakeLists.txt` (no `muse/` edit).
   - Without the module, the build still works and the video commands are disabled with a tooltip.
   - Install: macOS `aqt install-qt mac desktop 6.11.2 -m qtmultimedia`; Arch `pacman -S qt6-multimedia qt6-multimedia-ffmpeg`. CI gets the module if CI builds the fork.
   - Qt 6's FFmpeg backend plays MOV / MP4 / AVI with the listed codecs.
5. **Video audio is played by `QAudioOutput`, not by the muse audio engine.** The Mixer's Video strip drives its volume and mute. This differs from Dorico (engine channel, extracted soundtrack). The cost is a fixed latency offset between the engine audio and the video audio. Phase B measures it and adds a constant correction if it is audible (more than ~20 ms).
6. **Markers and timecodes on the page are generated by the layout engine, like measure numbers.** They are created during layout from the `doricoVideo` marker list and the per-layout settings, never written to the file, and printed and exported like any other item. An on-screen overlay was rejected because Dorico prints markers.
   - **The user chose the exact Dorico look over a small footprint** (2026-10-23). The CLAUDE.md rule is now "keep edits to upstream obvious", no longer "tiny".
   - So this phase adds real engraving item types, with their own drawing for all four border styles. That means edits to `types.h`, the factory and the layout / draw code, as needed.
7. **Repeats**: video sync follows the playback timeline (expanded repeats, which is what you hear). Marker positions on the page use the first pass (`tempoTimeline(false)`). Film cues rarely repeat. The builder notes any oddity found.
8. **Phase order**: A, B, C, D (below). Each phase is its own session with its own acceptance run.
9. **Both platforms must pass**: macOS, where Qt Multimedia uses AVFoundation, and Linux / Arch, where it uses system FFmpeg. The user works on both. Run the spike on whichever machine the phase B session is on and repeat it on the other before phase B is marked done. The fallback decode path is needed only on a platform that fails.
10. **Playback tempo multiplier**: the video follows the bars. Video position is computed from the playhead's score time at nominal tempo, and `playbackRate` = the multiplier. At 0.5x the picture plays at half speed and stays on the hits. Drift is measured against that.
11. **Specs 63 and 102 are still "pending review"**. Phase B's Mixer strip and Window menu edits are small and self-contained, so build on the current code and adjust if those reviews change it.

### Pure core (unit-tested, no UI): `video/timecode.{h,cpp}`
- `enum class FrameRate` with the 11 Dorico values. Each has a rational rate (24000/1001, 24, 25000/1001 for "24.98", 25, 30000/1001, 30000/1001 drop, 30, 50, 60000/1001, 60000/1001 drop, 60), a label ("29.97 dfps") and a drop flag.
  - "24.98" is 25000/1001 = 24.975, the value the manual's "24.975" uses.
- Conversions:
  - `framesToTimecode` / `timecodeToFrames`, with correct drop-frame numbering, including the 59.94 drop rule (4 frames per minute).
  - `parseTimecode` accepts `:` or `;` and also short input like `1:00:10:12`.
  - `formatTimecode` takes the H/M/S/F visibility flags and the separator.
  - `secondsToFrames` / `framesToSeconds`.
- `TimeMap`: from the settings and a `TempoTimeline`, it gives `flowStartSeconds`, `musicSecondsToTimecodeSeconds`, `markerTick(marker)` and `videoPositionAt(musicSeconds)`, using the formulas in "Time model".
- `video/findtempo.{h,cpp}`, a pure function: `findTempos(markers, flowStartSeconds, frameRate, beatUnitQuarters, minBpm, maxBpm, step) -> vector<{bpm, ifo, nfo, afo}>`, plus `markerFit(...)` for the lower table.
  - Our choice: a "significant beat" is every beat of the chosen beat unit, counted from the flow start under a constant tempo.
  - Frames off = |marker - nearest beat| in frames.
  - Sorted by IFO ascending, ties by AFO.
  - Before choosing the defaults (tempo range, beat unit), check in Dorico what the dialog actually opens with.

### Model and commands
- `video/videosettingsmodel.{h,cpp}`: reads and writes `doricoVideo`, emits changes, and lets you add / edit / delete markers undoably.
- Commands (`doricocommands.h`, `doricocommandsregister.cpp`, `doricoactionscontroller.cpp`):
  - `dorico/add-marker` (Shift+Alt+M)
  - `dorico/toggle-video-window` (F4)
  - `dorico/video-attach`, `dorico/video-properties`, `dorico/video-detach`
  - `dorico/find-tempo`
- Add the rows to `shortcuts_dorico.json` and `tools/dorico_keys.json`, then run `check_keymap.py`.

### Phase A: time, markers and panels (no video yet)
- **Video Properties dialog** (QML, Dorico field order). Without a video, the file and video rate rows show "–" and "Use video frame rate" is disabled. It opens from the Flows card menu (Video > Properties), from the Video panel's **Properties** button, and on attach.
- **Video panel**: a dedicated QML panel for the `video` id, replacing the placeholder. It has a Video section and a Markers table with inline edit, the Imp. checkbox, bold important rows, and Add / Delete / Find Tempo.
- **Add Marker dialog**: Text (default "Marker"), Timecode (default = playhead), and Valid range. A timecode outside the range is clamped into it with no error, as in Dorico 6.2.
- **Flow card**: shows the flow start timecode (`timecodes.md`), plus the reel icon or a warning icon once phase B exists.
- **Markers lane in Play mode**: a third lane in the `TrackOverview` repeater, showing marker text at `markerTick`, with an **Add Marker** button in its header. Built: it sits between Tempo and Chords, hidden by default, with a pin toggle in the track tools (the Tempo and Chords lanes got toggles too). The toggles last for the session only.
- **Time display**: our own mini-transport time field, which cycles bars/beats/ticks, then elapsed, then timecode on click. Its default comes from the existing (unused) Play preference. Built: `DoricoTimeDisplay.qml` sits before `PlaybackToolBar { showTimeFields: false }` in the shell, which hides MuseScore's own time and bar fields. Transport window parity is roadmap 101's business.
- **MIDI export**: markers are written as `META_MARKER` at their tick (upstream edit, below).

### Phase B: video
- **Spike first** (throwaway, `src/dorico/tools/` or scratch, not merged):
  - Open an H.264 long-GOP MP4, a ProRes MOV and an MJPEG AVI.
  - Seek with `setPosition` to 50 random frame numbers, grab the shown frame with `QVideoSink`, and compare it with the frame number burnt into the test clip. Make the test clips with ffmpeg's `testsrc` / `drawtext`.
  - Pass means every frame is exact. If it fails, the fallback is our own decode path on the FFmpeg that Qt Multimedia ships: seek back to the keyframe, then decode forward to the exact PTS. Report to the user before switching.
- **Attach, detach, properties**:
  - Attach opens a file dialog, then the Properties dialog.
  - The video frame rate is read from the file. A variable-rate file is refused with a message.
  - Detach clears the video fields and keeps the markers.
- **Video window**: a top-level QML `Window` with `VideoOutput`. Shift-drag keeps the aspect ratio. Its geometry goes in app settings (global, not per score). It is toggled by F4, the toolbar button (`DoricoShellPage.qml:547`) and Window > Video (`doricomenu.cpp:565`), in every mode.
- **`video/videosync.{h,cpp}`** links `IPlaybackState` to `QMediaPlayer`:
  - While **stopped or seeking**: pause, then set the exact frame for the playhead (frame time + half a frame, so rounding cannot fall to the previous frame).
  - While **playing**: start at the matching position, re-seek if drift goes over one frame, and keep `playbackRate` at 1.
  - Outside the video (before the attachment minus the offset, or after the end): show black.
  - Playback tempo multipliers (`setTempoMultiplier`): see call 10.
- **Mixer Video strip**: when the toolbar's **Video** filter is on and a video is attached, `DoricoMixer.qml` shows one extra strip named "Video" with a fader and mute. It is bound to `QAudioOutput` and saved in `doricoVideo`.

### Phase C: markers and timecodes on the page
- **New engraving item types** (call 6): for example `TIMECODE_MARKER` and `TIMECODE_LABEL` in `ElementType`, created with `Factory`, laid out and drawn by their own `rendering/score/` code. They are measure-number style: created during layout, never saved (generated), selectable, and drawn exactly with Dorico's four border styles, enclosure thickness, the text / timecode order and the three fonts.
  - Markers are bordered text with an optional timecode line, placed at the x of `markerTick`, interpolated between segments the way `NotationAutomationController::pointsDataInStaff` does.
  - Timecodes go at the start of each system or every bar, or never.
- **Settings UI**: there is no Dorico Layout Options or Engraving Options dialog in the fork yet. So this phase adds one fork dialog, **Markers and Timecode**, with two tabs that reproduce the manual's two pages exactly: Layout (per layout, with the layout list on the left as in Dorico) and Engraving (project-wide, including the three fonts).
  - It is reachable from the Video panel and from Library / Layout menu entries in Dorico's place.
  - When the real Layout Options / Engraving Options / Font Styles dialogs get built, these pages move into them.
- **Marker text in the Properties panel**: a property on the new item plus a Markers group in `propertiespanel`. Editing it changes the text in `doricoVideo`.
- **Timecode staff**: a single-line staff above the chosen bracket group, plus the "Timecode staff to staff" gap. Build it last in phase C.

### Phase D: Find Tempo
- A QML dialog over `findtempo`, live-updated, with Dorico's columns and sort order.
- **Apply**: one transaction that removes every `TempoText` in the score and adds one at tick 0, with the BPM and beat unit. It reuses `createTempoText` / `addTempoText`, moved from file-local into `tempohandler.h`.

### Upstream edits (keep them obvious, per CLAUDE.md)
- `importexport/midi/internal/midiexport/exportmidi.{h,cpp}`: a static `ExportMidi::extraMarkers` hook (score -> first-pass tick + text), which the dorico module sets in `onInit`. The markers go on the first staff's track, mapped through `repeatList().tick2utick`, so each is written once.
- `playback/qml/MuseScore/Playback/PlaybackToolBar.qml` and `internal/PlaybackToolBarActions.qml`: a `showTimeFields` property (default true) that collapses the time and bar/beat fields.
- `engraving`: the new item types and their layout / draw code (phase C). Keep the time model and settings parsing in `src/dorico/video/`. The engraving side receives the resolved marker list (tick, text, timecode string, important) and the settings through a narrow interface the fork registers, so the engraving code does not parse fork JSON.
- `propertiespanel`: a Markers group with **Marker text** for the new item (phase C).
- The timecode staff (phase C, last): the engraving changes it needs. Stop and ask the user only if it turns out to need a rework of system / staff layout rather than an addition.
- List every one under README "Cross-cutting design 1" when done.

## Deferred / unsupported
- Video track with thumbnails in Play mode (undocumented in 6.2).
- DeckLink / external video output.
- Engine-routed video audio (call 5).
- `considerTempoChanges` in Find Tempo (undocumented).
- "Playback must be activated" prerequisites: the fork has no Activate Project. Treat playback as always active.
- Per-flow videos, until roadmap 86 adds flows.
- MIDI from the command-line converter (`mscore -o x.mid`) has no markers: the console app does not load the dorico module, so the hook is never set. The GUI export has them.
- Lane show/hide in Play mode is not saved (session only).
- What attachment position and video start offset do to existing markers was not measured in Dorico; phase A keeps marker timecodes unchanged. Check it in phase B with a real video.
- Find Tempo... is shown disabled until phase D.
- Marker times, the valid range and clamping round to the nearest frame (a marker 1 s in at 29.97 fps is 29.97 frames in, so it shows as ;00 of the next second). The playhead timecode rounds down, to the frame showing.
- Cmd-Z right after an inline edit in the Video panel does nothing until the score canvas has focus again (panel focus keeps the undo shortcut from firing). Inline edits also drop an unsaved edit when another cell's editor opens, where Dorico may commit it.
- The drop-frame timecode field shows U+037E (which looks like ';') before the frames, because Qt cuts an input mask at its first ';'. Every value the field emits carries a real ';'.
- Transport window (F2) time toggle: only if roadmap 101's Transport window exists by then; otherwise the mini transport only.

## Acceptance criteria
- [x] **A** Timecode unit tests pass: drop and non-drop round trips at all 11 rates, the 10-minute drop rule, parsing with `:` / `;` / short input, and the manual's 01:59:52:00 example.
- [x] **A** Shift-Alt-M opens Add Marker at the playhead's timecode, and OK adds "Marker" there. A timecode outside the valid range is clamped into it (Dorico 6.2).
- [x] **A** The Video panel table lists the markers, edits timecode and text inline, toggles Imp. (bold), deletes one, and undo / redo works for each.
- [x] **A** Changing a tempo mark moves the markers against the bars (Play lane), and their timecodes stay the same.
- [x] **A** Video Properties with no video sets the frame rate and Timecode start, and the flow card shows the flow start timecode.
- [x] **A** Clicking the mini transport time cycles bars/beats, elapsed time and timecode.
- [x] **A** Exported MIDI contains the markers as marker meta events at the right ticks.
- [x] **A** Save, close and reopen keeps every setting and marker. A build without the fork opens the file without error.
- [ ] **B** The spike report shows 50 of 50 exact frames for each test clip, or the user approved the fallback.
- [ ] **B** Video > Attach on the flow card opens the file dialog and then Properties, and the reel icon appears. Renaming the file shows the warning icon, and Attach again restores it with the old settings.
- [ ] **B** F4, the toolbar button and Window > Video show and hide the Video window in all five modes. A resized window keeps its size in a new project, and Shift-drag keeps its shape.
- [ ] **B** Stopping or moving the playhead shows the exact frame for the playhead's timecode (checked against burnt-in frame numbers). During a 3-minute playback the drift stays at or under one frame.
- [ ] **B** Start offset and attachment position move the picture against the music as in the manual's example (5 s of video on beat 8).
- [ ] **B** The Mixer's Video filter shows a Video strip, and its fader and mute change the video's audio.
- [ ] **C** Markers show above the system in the full score and not in parts by default. Each Layout setting and each Engraving setting changes the page as the manual describes, including the four border styles and the H/M/S/F visibility.
- [ ] **C** Markers and timecodes appear in PDF export and print.
- [ ] **C** The four border styles look like the manual's example images (`specs/reference/manual/images/markers_border_style_*.png`).
- [ ] **C** Marker text edited in the Properties panel updates the Video panel table, and the other way round.
- [ ] **C** A timecode staff above the chosen bracket shows the markers on it and the timecodes below it, with the "Timecode staff to staff" gap.
- [ ] **D** Find Tempo is disabled without an important marker. Its list changes live with range, step and beat unit. Its columns and sort match the manual, and the defaults match Dorico.
- [ ] **D** Apply leaves exactly one tempo mark, at the start, with the chosen tempo, in one undo step.

## Tests
- GoogleTest: `src/dorico/tests/timecode_tests.cpp` (formatting, parsing, drop frame, TimeMap formulas) and `findtempo_tests.cpp` (a synthetic marker set where 120 bpm hits every marker exactly and is ranked first; the 0.25 step; IFO / NFO / AFO averages).
- Manual: each phase's acceptance list, run by the `ui-tester` agent (ask the user before each launch) on a temp copy of a `demos/` score, plus the test clips with burnt-in frame numbers from the spike.
