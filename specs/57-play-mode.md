# 57 - Play mode
Status: todo  -  Depends on: 50  -  Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/play_mode/play_mode_project_window_r.html`, `.../play_mode_track_inspector_r.html`, `.../play_mode_vst_midi_panel_r.html`, `.../play_mode_track_overview_r.html`, `.../mixer/mixer_panel_r.html`

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

## Acceptance criteria
- [ ] Play page matches `dorico-ui.md` 4 in zones.
- [ ] Track inspector follows the selected instrument and changes its sound / effects.
- [ ] Mixer works and playback plays from the transport.
- [ ] Stubs show "Not available yet".

## Tests
Manual: select an instrument, change its sound, add an effect, play.
