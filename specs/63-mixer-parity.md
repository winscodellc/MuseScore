<!-- markdownlint-disable -->

# 63 - Mixer parity

Status: todo - Depends on: 57 - Dorico refs: `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/mixer/mixer_panel_r.html`, `.../mixer_toolbar_r.html`, `.../mixer_channel_strips_r.html`, `.../mixer_channels_types_r.html`, `.../mixer_channels_renaming_t.html`, `.../mixer_volume_tracks_changing_t.html`, `.../mixer_tracks_panning_changing_t.html`, `.../mixer_channels_height_changing_t.html`, `.../mixer_channels_scrolling_t.html`, `.../mixer_channels_hiding_showing_t.html`, `.../mixer_reverb_sound_changing_t.html`, `.../mixer_inserts_enable_bypass_t.html`

Local mirror: `reference/manual/topics/mixer/` (`mixer-panel.md`, `mixer-toolbar.md`, `mixer-channels/mixer-channel-strips.md`, `mixer-channels/types-of-mixer-channels.md`, `renaming-channels.md`, `changing-the-volume-of-channels.md`, `panning-channels.md`, `changing-the-height-of-channels.md`, `scrolling-through-channels.md`, `adding-fx-channels.md`, `changing-the-reverb-plug-in.md`, `loading-inserts-into-channels/`). Pictures: `reference/manual/images/mixer_panel_desktop.png` (whole panel), `mixer_height_example.png` (split line), `mixer_inserts*.png`, `mixer_sends.png`, `mixer_button_*.png`, `toolbar_mixer.png`.

## Goal

Finish the Dorico Mixer that spec 57 started (`DoricoMixer.qml`, `DoricoChannelStrip.qml`, used in the lower zone of every mode). The goal is the full Dorico layout and editing. Add channel strips (Inserts, Sends) above the channels, with a split line between them. Channels get a fixed 190 px height. Add channel renaming, typed values for volume and pan, sideways scrolling, the click channel first, and working Add and Remove Effects Channel. The toolbar gets Dorico's look. Everything sits on MuseScore's existing mixer model, with no upstream edits.

## Dorico behaviour

### Layout (`mixer_panel_desktop.png`)

| Part | Behaviour |
|---|---|
| Toolbar (top) | Show: Instruments / MIDI / Video / FX / Unused toggles, "Deactivate all:" m / s, FX+ / FX− (Add / Remove Effects Channel), Live Stage, Live Space, Resize Lower Zone at the far right. Icon buttons about 30 px, the group centred. |
| Channel strips (middle) | One per channel: **Inserts** (4 slots), **EQ** (4 bands), **Sends** (one slot per FX channel). Shown only when the Mixer is tall enough. Clicking a section header expands / collapses that section for **all** channels. |
| Channels (bottom) | Pan box, fader value, m / s, fader, meter, name on the instrument colour. About 190 px tall. FX channels green, Output red and always shown, pinned right. |
| Split line | Between strips and channels: drag up / down, the pointer is a split arrow (`mixer_height_example.png`). |
| Edges | When more channels exist than fit, the channels at the left / right edge are drawn faded. |

### Channel types

| Type | Dorico | Notes |
|---|---|---|
| Instruments | One per instrument / voice track, plus "DoricoBeep" (click) **first** | |
| MIDI, Video | MIDI volume / pan per VST; video audio | |
| FX | "Reverb" by default (REVerence as insert), green | Add / Remove Effects Channel; Remove takes the rightmost |
| Output | Master, red, always shown | |

### Inserts (`mixer_channel_strips_r`, `mixer_inserts_enable_bypass_t`)

| Control | Behaviour |
|---|---|
| Indicator | Off: no inserts loaded. Blue: at least one loaded, enabled. Yellow: all loaded inserts bypassed. Clicking it enables / bypasses all inserts of that channel. |
| Slot | Empty: menu "---". Loaded: Enable Insert (power), name, Edit Insert (opens the plug-in window), insert menu (choose another plug-in). |
| Reverb | Changing the reverb = choosing another plug-in in the FX channel's insert slot. |

### Sends

One slot per FX channel: Enable Send (power) and a gain slider (post-fader).

### Editing channels

| Action | Behaviour |
|---|---|
| Rename | Double-click the channel name, type, Return. Deleting the name resets it to the default. |
| Volume | Drag the fader. Opt-click the fader value, type a value, Return. `inf` = maximum, `-inf` / `-oo` = minimum. Cmd-click or double-click the value resets it to the default. |
| Pan | Click a position in the pan control or drag sideways. Opt-click the value, type, Return. Range `-100` to `100`. |
| Mute / solo lit | Mute yellow, solo red (`mixer_button_mute_activated.png`, `mixer_button_solo_activated.png`). |
| Scroll | Mouse wheel up / down (or a touchpad swipe) scrolls sideways. Click and drag outside the faders also scrolls. |
| Hide / show | Toolbar type buttons, in any combination. |

## MuseScore today

Paths: PB = `src/playback/qml/MuseScore/Playback`.

- **Our mixer** (spec 57): `src/dorico/qml/MuseScore/Dorico/DoricoMixer.qml` has the toolbar in `headerComponent` (left-aligned text buttons; FX+ / FX− / Live Stage / Live Space disabled). It buckets channels by type in `updateColorSlots` (`:54-71`) and pins the master on the right. Its channel ListView has a scrollbar and no wheel mapping. `DoricoChannelStrip.qml` is the channel: pan drag + double-click, fader drag + double-click, m / s lit with the accent colour, and a `label` property that is already the shown name. Strips stretch to the full zone height (about 390 px).
- **Model**: `MixerPanelModel` / `MixerChannelItem`. `volumeLevel` (dB, -60..12, `volumeLevelMin/Max`) and `balance` (int, -100..100, `balanceMin/Max`) are writable (`PB/mixerchannelitem.h:62-67`). Model order: instruments, chord-symbol tracks, metronome, visible auxes, master (`mixerpanelmodel.cpp:150-174`). `resolveInsertIndex` (`:400-460`) depends on this order, so **do not reorder the model**; order in QML.
- **Titles**: `MixerChannelItem.title` is read-only in QML. `setTitle` (`mixerchannelitem.cpp:452`) is in memory only. Aux titles are computed (`resolveAuxTrackTitle`, `src/playback/internal/playbackcontroller.cpp:77-88`): "Reverb" when the only FX is MuseReverb, else the FX id, else "Aux N". The project's `audiosettings.json` (`src/project/internal/projectaudiosettings.cpp:321-336`) is rebuilt on save, so extra keys would be dropped. Do not store names there.
- **Aux (FX) channels**: fixed at `AUX_CHANNEL_NUM = 2` (`src/playback/playbacktypes.h:33-34`), both always exist (`playbackcontroller.cpp:1670-1671`). Only visibility changes, and it is an **app-wide** setting: `IPlaybackConfiguration::isAuxChannelVisible` (default false) / `isAuxSendVisible` (default true for aux 0 only), `playbackconfiguration.cpp:135-150`. MuseScore's mixer context menu toggles them (`PB/mixerpanelcontextmenumodel.cpp:141-193`). Aux 0 gets MuseReverb when the project has no saved aux params (`playbackcontroller.cpp:63-72, 1278-1281`).
- **Insert FX**: `MixerChannelItem.outputResourceItemList` = up to 4 `OutputResourceItem`s (`OUTPUT_RESOURCE_COUNT_LIMIT`, `mixerchannelitem.cpp:45`), with a blank slot appended automatically. Each item has `title`, `isBlank`, `isActive` (writable = bypass), `hasNativeEditorSupport`, `requestToLaunchNativeEditorView()`, `requestAvailableResources()` (vendor menu) and `handleMenuItem(id)` (`PB/abstractaudioresourceitem.h:39-55`, `PB/outputresourceitem.cpp`). QML: `PB/internal/AudioResourceControl.qml`, wired in `PB/internal/MixerFxSection.qml:53-82`. The Track Inspector's Insert Effects (spec 57) already reuses this.
- **Sends**: `MixerChannelItem.auxSendItemList` = `AuxSendItem`s (`PB/auxsenditem.h:29-61`: `title`, `isActive`, `audioSignalPercentage` 0-100), visible sends only. QML: `PB/internal/AuxSendControl.qml` (used by `MixerAuxSendsSection.qml:50-54`).
- **EQ**: none in MuseScore.
- **Per-project data in the score**: spec 62 keeps settings in a score metaTag and marks the project unsaved (`src/dorico/qml/MuseScore/Dorico/printmodel.cpp:49-52, 221-239`; notes in `reference/musescore-internals.md`, "Printing without the system dialog").

## Design

All new code goes in `src/dorico/qml/MuseScore/Dorico/`, and new files are added to its CMake list. There are no upstream edits.

1. **Channel strips** - `DoricoChannelStripSections.qml`, one per channel, above its `DoricoChannelStrip` in the same delegate (and above the pinned Output strip).
   - **Inserts**: header button with the indicator (none / blue / yellow from `outputResourceItemList`; click toggles `isActive` on all loaded items), then the slots as `AudioResourceControl`s (wiring copied from `MixerFxSection.qml:53-82`).
   - **EQ**: header only, disabled, tooltip "Not available" (stub).
   - **Sends**: header, then one `AuxSendControl` per `auxSendItemList` entry.
   - Section open / closed state lives in `DoricoMixer` (`insertsOpen`, `sendsOpen`), so one click affects all channels.
   - Restyle the reused controls only through their properties. If they cannot be made compact enough, write a small Dorico slot that uses the same item API, and note that as built.
2. **Split line** - `DoricoMixer` gets a strip area (top) and a channel area (bottom, default 190 px) split by a 4 px handle (`Qt.SplitVCursor`). Drag changes `channelHeight` (clamped, kept for the session). Hide the strip area when the space above is below the height of the section headers.
3. **Channels 190 px** - the channel area's height drives `DoricoChannelStrip`. Re-measure against `mixer_panel_desktop.png` and make sure the fader still has travel.
4. **Toolbar** - restyle `headerComponent`: buttons about 30 px, the group centred in the tab row, icons from MuseScore / Lucide / Bravura (spec 60 rules, never Dorico's). FX+ / FX− become real (item 8). Live Stage / Live Space stay unavailable (spec 64).
5. **Lit colours** - mute lit = yellow, solo lit = red (colours measured from the manual pictures), in `DoricoChannelStrip` and the toolbar's Deactivate all buttons.
6. **Rename** - double-click the name label to open an inline `TextInputField` (Return commits, Esc cancels, empty resets).
   - Store the names in a score metaTag `doricoMixerNames`: a JSON object keyed by `partId/instrumentId` (from `instrumentTrackId()`), `aux:<n>` or `metronome`. Write it on the master score only.
   - Follow `printmodel.cpp`: `setMetaTag`, then `INotationProject::markAsUnsaved()`. This needs a small C++ helper (for example `MixerNamesModel`, QML_ELEMENT) with `name(key)` / `setName(key, text)` and a `namesChanged` signal. Reload it when the current project changes.
   - `DoricoChannelStrip.label` = the stored name or `channelItem.title`. The Track Inspector's Channel section shows the same name. Output cannot be renamed.
7. **Typed values** - Opt-click (`Qt.AltModifier`) on `valueBox` / `panBox` opens an inline field.
   - Volume accepts a number, `inf` (= `volumeLevelMax`) and `-inf` / `-oo` (= `volumeLevelMin`), clamped.
   - Pan accepts `-100..100`, plus `C`, `L<n>` and `R<n>`, clamped to `balanceMin/Max`.
   - Cmd-click (`Qt.ControlModifier` on mac = Cmd in Qt terms; check) or double-click on the value resets volume to 0 dB and pan to C.
   - Clicking a position in the pan box sets the pan there; dragging keeps working.
   - Parse in one small JS function in a `.js` file so the rules sit in one place.
8. **Add / Remove Effects Channel** - FX+ makes the first hidden aux visible together with its send: `IPlaybackConfiguration::setAuxChannelVisible(i, true)` + `setAuxSendVisible(i, true)` through a tiny C++ invokable on the helper from item 6, or MuseScore's mixer context-menu model if it can be reused. FX− hides the rightmost visible aux. FX+ is disabled with both shown and FX− with none. This is app-wide, unlike Dorico's per project: note it under "As built".
9. **Scrolling** - a `WheelHandler` on the channel area maps vertical wheel and swipe to `contentX`. Dragging on the background (not on a fader, pan or button) scrolls. Edge fades are 24 px gradients over the first / last visible column when there is more to scroll. Strips and channels scroll together (one ListView with both parts in its delegate).
10. **Click first** - in `DoricoMixer`, show the `MixerChannelItem.Metronome` channel first: a separate pinned-left delegate or a QML proxy order. Do not reorder `MixerPanelModel`. Label it with its model title (MuseScore's "Metronome").

## Deferred / unsupported

| Dorico feature | Why |
|---|---|
| EQ section | MuseScore has no channel EQ; the header is a disabled stub |
| MIDI, Video, Unused channel types | No MuseScore channels of those kinds; buttons stay unavailable |
| More than two FX channels; per-project FX channel list | MuseScore has exactly two auxes and stores their visibility app-wide |
| Mixer window (F3 / Window > Mixer) | F3 toggles the lower-zone Mixer (spec 54); no separate window |
| Live Stage, Live Space | Spec 64 |
| Play > Apply Default Output Level, default-level preference (-6 dB) | Not in our Play menu (spec 59); MuseScore's default is 0 dB. A reset-all is a loop over `volumeLevel = 0` when wanted |
| Resize Lower Zone toolbar button | The dock edge already resizes the zone; add it if it turns out cheap |

## Acceptance criteria

- [ ] With the lower zone tall enough, channel strips show above the channels with Inserts, EQ (disabled) and Sends headers. With the zone short, only the channels show.
- [ ] Clicking Inserts (or Sends) on any channel expands / collapses that section on every channel.
- [ ] From an instrument's strip: load an insert, bypass it (indicator turns yellow), enable it (blue), open its editor, remove it (indicator off).
- [ ] On the FX (Reverb) strip, the insert menu replaces MuseReverb with another effect; playback uses it.
- [ ] A send's level and on / off change the reverb heard on that instrument.
- [ ] Dragging the split line changes the strip / channel heights; channels default to about 190 px.
- [ ] Double-click an instrument name and an FX name, rename, Return: the names show (mixer and Track Inspector), survive save + reopen, and an empty name restores the default.
- [ ] Opt-click the fader value, type `-6`, Return: the fader moves to -6 dB. `inf` and `-inf` go to the ends. Opt-click pan, type `L30`, then `100`: the pan follows. Cmd-click / double-click the value resets.
- [ ] Clicking in the pan box sets the pan to that position.
- [ ] The mouse wheel scrolls the channels sideways; dragging on the background scrolls; the edges fade when more channels exist.
- [ ] The click (metronome) channel is the first channel; Output stays pinned right.
- [ ] FX+ adds the second FX channel (and its send slot on each strip), FX− removes it; the buttons disable at the limits.
- [ ] Mute lit is yellow, solo lit is red; the toolbar is centred icon buttons.
- [ ] Write, Engrave and Play all show the same Mixer.

## Tests

- Manual script for `ui-tester` on a temp copy of a `demos/` score, one check per acceptance box (visible display for plug-in editors and sound checks).
- No GoogleTest unless the value parser moves into C++. The JS parser gets an assert-based self-check run with `qmltestrunner` only if one already exists in the module; otherwise it is checked through the UI script.
