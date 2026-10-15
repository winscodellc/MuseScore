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
- **Centre**: `PlayPage` overrides `central` with MuseScore's `Timeline` (one row per instrument, plus tempo/time/key/rehearsal rows) as the track overview; notation is not shown, as in Dorico. Selecting in the timeline selects in the score, which the Track Inspector follows. Fallback, if the timeline misbehaves as a central dock: put `NotationView` back by deleting the `central:` override.
- **Left zone** (`playLeftPanel`) with a two-button selector:
  - **Track Inspector**: `TrackInspectorModel` (`qml/MuseScore/Dorico/trackinspectormodel.{h,cpp}`) is a one-row list over its own `MixerPanelModel`, holding the channel of the selected instrument (first selected element's part, matched by `MixerChannelItem::instrumentTrackId().partId`). It keeps the last instrument when nothing is selected and starts on the first one. MuseScore's mixer section components render it unchanged:
    - Routing = sound picker (`MixerSoundSection`).
    - Insert Effects = audio FX slots (`MixerFxSection`).
    - Channel = balance, volume, fader, mute/solo.
  - **VST and MIDI**: stub.
- **Lower zone**: `LowerZonePanel` with `tabs: ["mixer", "keyEditor"]` (Key Editor is the stub).
- Zones: `zoneDocks(Play)` = playLeftPanel / lowerZonePanel, and no right zone, so Cmd-9 does nothing.
- `dorico_qml` adds playback's header dirs to its include path and links `playback_qml` (for `MixerChannelItem`).
- Transport works on Play because of the page-gating fix (playback commands used to be disabled outside Write).

## Verification status
Built and unit-tested (`dorico_tests` 34/34), QML linted. Manual check in the app pending. Things to look at: that the timeline renders as the central dock, and that two mixer models (inspector + lower zone) stay in sync (they share the audio tracks, so they should).

## Acceptance criteria
- [ ] Play page matches `dorico-ui.md` 4 in zones.
- [ ] Track inspector follows the selected instrument and changes its sound / effects.
- [ ] Mixer works and playback plays from the transport.
- [ ] Stubs show "Not available yet".

## Tests
Manual: select an instrument, change its sound, add an effect, play.
