<!-- markdownlint-disable -->
# 57 - Play mode
Status: pending review  -  Depends on: 50  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/play_mode/play_mode_project_window_r.html`, `.../play_mode_track_inspector_r.html`, `.../play_mode_vst_midi_panel_r.html`, `.../play_mode_track_overview_r.html`, `.../mixer/mixer_panel_r.html`

Implementer: Sonnet is fine.

## Goal
`PlayPage`: left zone (Track Inspector, VST and MIDI), centre (track overview), lower zone (Mixer, Key Editor). No toolbox and no right zone.

## Dorico behaviour
See `reference/dorico-ui.md` 4. Left-zone selector switches Track Inspector / VST and MIDI. Centre is a track view (notation is not shown).

## MuseScore today
- `MixerPanel` (channel strips, sound and instrument selectors, audio FX per channel, aux sends).
- `TimelinePanel` (the closest thing to a track overview).
- Playback settings and the "Playback setup" in the instruments panel.
- `PianoKeyboardPanel` is unrelated.

## Design
- Left zone, **Track Inspector**: show the selected instrument's mixer channel (reuse the mixer channel-strip QML for one channel) plus Insert Effects (the strip's FX slots). Routing section: sound and instrument selectors only; port/channel/endpoint fields omitted.
- Left zone, **VST and MIDI**: stub (MuseScore exposes plugins through the mixer, not as instances).
- Centre: `TimelinePanel` content as the "track overview" stand-in: ruler, tracks per instrument, mute/solo. If embedding the timeline as `central` is hard, use the notation view with the mixer open (decide while implementing, note it as-built).
- Lower zone: Mixer (reuse), Key Editor (stub), same two-tab component as spec 54.
- Cmd-7 and Cmd-8 toggle the zones; there is no right zone, Cmd-9 does nothing.

## Deferred / unsupported
VST/MIDI instance management, endpoint setup, Independent Voice Playback, playback templates, Key Editor, expression maps, track heights and visibility buttons (tempo, markers, chords tracks).

## As built
- **Centre**: `PlayPage` overrides `central` with `TrackOverview.qml` + `TrackOverviewModel` (our own; MuseScore's `Timeline` was too small, had no playhead and its instrument names selected nothing). One track per instrument of the **master score** (also when a part is open), a bar ruler, a block per run of bars with notes (`internal/trackblocks.h`), and a playhead from `IPlaybackState::playbackPositionChanged` (no interpolation). Fits the piece to the width, at least 8 px per quarter, and scrolls to follow the playhead. Clicking a track header or row shows it in the Track Inspector; the page relays this (`trackPicked` / `inspectedPartId`) because dock content and `central` are separate components. Not built: mute/solo in track headers, click-to-seek in the ruler, zoom, track heights.
- **Left zone** (`playLeftPanel`) with a two-button selector:
  - **Track Inspector**: `TrackInspectorModel` (`qml/MuseScore/Dorico/trackinspectormodel.{h,cpp}`) is a one-row list over its own `MixerPanelModel`, holding the channel of the selected instrument (first selected element's part, matched by `MixerChannelItem::instrumentTrackId().partId`). It keeps the last instrument when nothing is selected and starts on the first one; `partId` is writable so the track overview can pick a track. MuseScore's mixer section components render it unchanged:
    - Routing = sound picker (`MixerSoundSection`).
    - Insert Effects = audio FX slots (`MixerFxSection`).
    - Channel = balance, volume, fader, mute/solo.
  - **VST and MIDI**: stub.
- **Lower zone**: `LowerZonePanel` with `tabs: ["mixer", "keyEditor"]` (Key Editor is the stub).
- Zones: `zoneDocks(Play)` = playLeftPanel / lowerZonePanel, and no right zone, so Cmd-9 does nothing.
- `dorico_qml` adds playback's header dirs to its include path and links `playback_qml` (for `MixerChannelItem`).
- Transport works on Play because of the page-gating fix (playback commands used to be disabled outside Write).

## Verification status
Built and unit-tested (`dorico_tests` 41/41), QML linted. Manual check in the app pending.
Mixer sync: `MixerPanelModel` (upstream, `setupConnections`) now also applies `controlParamsChanged` / `masterControlParamsChanged` and refreshes every track's mute/solo state, so the inspector and the lower-zone mixer (two models over the same tracks) agree on volume, pan, mute and solo, not only on FX.

## Acceptance criteria
- [ ] Play page matches `dorico-ui.md` 4 in zones.
- [ ] Track inspector follows the selected instrument and changes its sound / effects.
- [ ] Mixer works and playback plays from the transport.
- [ ] Stubs show "Not available yet".

## Tests
Manual: select an instrument, change its sound, add an effect, play.
