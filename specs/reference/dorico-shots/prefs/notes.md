<!-- markdownlint-disable -->
# Preferences audit: Dorico 6 vs our build (2026-10-21)

## Ours (Cmd-, in our build)

Dialog: separate window "Preferences", 880 x 672 pt, not resizable, macOS traffic-light title bar. Left column 220 pt wide, dark (#1f2428-ish), list of 15 pages, each with an icon, selected row highlighted. Content area right of it scrolls (small flat sections separated by 1 px divider lines, bold section title, labels left, controls in a column at ~x+220). Bottom bar: `Reset preferences` (left), `Cancel`, `OK` (right, blue). No category search, no Cmd-F page search (except "Search shortcut" on Shortcuts and "Search advanced" on Advanced).

Pages (in order) and sections / options (labels only):
1. **General**: Language (dropdown System default, button Check for language updates); Program start (radios Start empty / Continue last session / Start with new score / Start with score + path + folder button); Welcome screen (checkbox Show welcome screen when MuseScore Studio launches); Automatic updates (toggle Check automatically for updates, toggle Download and install updates automatically, privacy text).
2. **Appearance**: Theme (Light / Dark radios with thumbnails, checkboxes Follow system theme, Enable high-contrast, Accent color swatches 2 x 9); Page (toggle Invert score colors, checkbox Only invert colors in dark theme, Engraving color swatch, Page color radio+swatch, Image radio+path); Background (Color radio+swatch, Image radio+path); App text (Typeface dropdown, Body text size spinner); button Reset appearance to default.
3. **Canvas**: Zoom (Default zoom dropdown + percent spinner, Mouse zoom precision spinner); Scroll pages (radios Horizontal/Vertical, checkbox Limit scroll area to page borders); Miscellaneous (Proximity for selecting elements spinner).
4. **Save & publish**: Auto save (checkbox + Auto save every N min spinner); Save to cloud (Generate MP3 audio for private cloud scores: Never / Always / Every N saves); Publish to MuseScore.com (checkbox Always prompt to share on Audio.com after publishing).
5. **Note input**: Note input (Default input mode, Apply accidentals/augmentation dots/articulations, Input by duration mode cursor: 3 dropdowns); MIDI devices (toggle Enable MIDI input, checkbox When pressing a key begin note input..., expander Real-time input modes); Note preview (toggle Hear playback when adding/editing/selecting, Playback duration spinner, checkboxes Play chord when editing, Play chord symbols and Nashville numbers, Play when setting pitch, Play preview notes with score dynamics, Play MIDI input, Play MIDI notes with velocity and duration); Voice assignment (radios All voices on the instrument / Only the voice they are applied to); Fretboard diagrams (checkbox Never autofill fretboard diagrams); Note colors (checkboxes Color notes outside of usable pitch range, Color guitar bends and hammer-ons/pull-offs outside playable range).
6. **Score**: Default style (Style for full score path + folder, Style for parts path + folder).
7. **Audio & MIDI**: Audio (Audio driver, Audio device, Buffer size dropdowns); MIDI (MIDI input, MIDI output dropdowns, checkbox Produce MIDI 2.0 output); Sound equalizer (checkbox warmer instrument sound); Mixer (checkbox Toggle track mute by showing/hiding instruments in Layout panel); Online sounds (2 checkboxes + Show processing visualization dropdown).
8. **MIDI mappings**: checkbox MIDI remote control; table ACTION/STATUS (Rewind, Toggle loop playback, Play, Stop, Toggle note input, Set duration x6); buttons Assign MIDI mapping..., Clear, Clear all.
9. **Video**: Video converter (FFmpeg library version, path field, Locate existing installation, Download FFmpeg).
10. **Percussion**: Triggering the percussion panel (radios: When an unpitched percussion staff is selected / When inputting notation on an unpitched staff / Never; checkbox Close the panel automatically).
11. **Import**: Style used for import (Built-in / Use style file); Character set (Overture import character set dropdown); MusicXML (Import layout, Import system and page breaks, Apply default typeface (Edwin), Infer text type); MIDI (Shortest note dropdown, Round tempos to nearest whole number); MEI (Import MEI layout); MNX (Require exact schema validation).
12. **Shortcuts**: Presets dropdown (DORICO) + reset, buttons Define / Clear, Search shortcut field, table ACTION/SHORTCUT grouped by APP, DIAGNOSTICS, DORICO...; buttons Import, Export, Reset to default.
13. **Folders**: 7 path rows with folder buttons (Scores, Styles, Instruments and score orders, Templates, Plugins, SoundFonts, Musical symbol fonts).
14. **Advanced**: Reset to default, Search advanced field, table PREFERENCE/VALUE (Palette scale, Smooth panning, colour swatches: All voices, Displayed default, Formatting, Frame, Invisible, Desynchronized, Voice 1-4 colours, ...).
15. **Braille**: Enable/disable (Show braille panel); Advanced options (Braille table for lyrics, Interval direction; disabled until enabled).

## Dorico 6.2 Pro: layout and chrome

Window "Preferences": 1130 x 708 pt, resizable-looking macOS title bar (centred "Preferences", traffic lights greyed), black outer border. **Left column 203 pt** (18% of width) flat black list: top **"Search categories..."** field (blue 2 px focus ring, auto-focused), then a single-level list: Colors, General (expandable: General, Window, View, Files), Key Commands, MusicXML, Note Input and Editing, Play, Proofreading, Remote, VST Plug-ins. Rows 27 pt high, text 13 px, selected row lighter grey + 3 px blue left bar; no icons. **Right pane**: page title bar (e.g. "General", lighter band, 16 px) then sub-titles as dark full-width bands (bold 13 px: "Instrument Names", "Comments"...), each followed by a **rounded dark card** (radius ~12 px, colour ~#16191c on pane ~#1e2125) holding label+control rows. Controls: grey dropdown with chevron, segmented toggle buttons (selected = bright blue #2da0f5 with black bold text), checkboxes (square, blue tick when on), spinners, "Choose..." buttons, blue round "?" help icons, italic grey help text. Vertical scrollbar on the right and a horizontal scrollbar at the bottom of the pane. Group pages scroll as one: clicking "General" shows General + Window + View + Files stacked (sub-rows are anchors). Bottom bar: `Reset to Factory` (left, grey), `Apply` (disabled, right), `Close` (blue-outlined, right). OK/Cancel do not exist: changes apply immediately/on Apply.

Colours: pane ~#1e2125, card ~#16191c, header band ~#2b2f35, accent blue ~#2da0f5, text white-grey.

## Dorico pages (labels + control type)

### General > General
Card: Language [dropdown English]; Preferred unit of measurement [dropdown Millimeters (mm)]; Default paper size type [dropdown Automatic]; Default text font family [dropdown Academico]; Middle C (note 60) [segmented C3|C4|C5]; Default piano roll display [segmented Played Durations|Notated Durations].
**Instrument Names**: [check] Default language for instrument names + [dropdown English].
**Comments**: Author name for comments [segmented User Name|Custom Name] + help icon; Full name [text]; Initials [text].
**Crash Reporting**: [check] Show prompt to send crash reports to Steinberg + help text + link Privacy Policy...
**Suppressed Warnings**: [button Reset Warnings].
### General > Window
Theme [dropdown Dark]; [check] Open new windows maximized; When the last window is closed [dropdown Open Hub window]; [check] Close side panels when lower panel is opened; [check] When switching to Engrave mode, activate the Graphical Editing tool.
### General > View
Default zoom [dropdown 200%]; Default view type [dropdown Page View]; Overscroll amount [dropdown Small]; [check] Show tab bar in new projects; [check] When switching tab or window, bring selection into view; [check] When switching to Play mode, bring selection into view. **Ruler**: [check] Continue ruler across spreads.
### General > Files
**Saving Projects**: Default project folder [path + Choose...]; [check] Show complete path for recent files; [check] Auto-save every [spinner] minutes + help; Project backup folder [path + Choose...]; Number of backups per project [spinner]; [check] Warn when open project is updated outside Dorico, moved, or deleted.
**Opening Projects**: [check] Warn when projects use fonts not installed on this computer; [check] Warn when opening projects last saved in an earlier or later version; [check] Prompt to save projects from an earlier or later version to a new location.
**Exporting Files**: Filenames for exporting graphics [button Edit...]; [check] Omit excluded players and layouts when exporting flows; [check] Open text CSV in default application after export; [check] Warn when printing full or custom score layouts with suspect transposition + Suspect transposition for score layouts [segmented Concert Pitch|Transposed Pitch] + italic help; Format for text items copied to clipboard [dropdown HTML] + italic help.

### Search bars
- **Search categories...** (top of left column): filters the category list. Not exercised beyond focus.
- **Cmd-F** opens a page search bar across the top of the right pane: field "Search page..." (blue focus border, ~120 pt), `<` and `>` previous/next buttons, and a close X at the far right. It highlights matches in the page.
- Colors has sub-rows: Page Colors, Background Colors, Voice Colors, Other Colors (anchors in one page). Key Commands, Remote, VST have none.

### Colors
**Page Colors** (rounded cards, hex field + swatch + reset-arrow button): Full score layouts #ffffff; Part layouts #fef2da; Custom score layouts #ffffff; [check] Invert colors for music (white on black) + Inverted page background #000000 + help text.
**Background Colors**: Write mode and Engrave mode cards: Use [segmented Gradient|Single Color]; Preset [dropdown (Custom)]; Stop 1-4 [hex + swatch each]; gradient preview bar + reset.
**Voice Colors**: Voice 1-8 hex+swatch+reset (#008ae6 #df0771 #00a83b #8f0ae8 #d64000 #2a56ab #c72cba #009e90) + help text.
**Other Colors**: Selected items, Linked selected items, Shadow notehead, Cautionary accidental, Contradictory accidental, Forced accidental, Out of standard range, Out of advanced range, Caret and rhythmic grid, Playhead, Playhead when recording, Active cycle locators, Inactive cycle locators, Unplayable in tablature, Minor grid lines, Major grid lines (all hex+swatch+reset). **Crosshair**: Color hex+swatch+reset; Thickness [spinner 2].

### Key Commands
Left: Search field + tree (Application, Edit, Engrave, File, Filter, Help, Jump Bar, Note Editing, Note Input, Options Dialogs, Page Layout, Play, Print, Proofreading, Script, ScrubPlayback, Selection and Navigation, Setup, Text Editor, Transformations, UI, View...). Right: "Key Commands" title + help icon; [Press shortcut field] [Add Key Command]; Keyboard language [dropdown Default]; list of the command's shortcuts; [Remove Key Command] [Print Summary] [Reset Key Commands]. **MIDI Learn**: [piano button] [field] [Add MIDI Command] [Remove MIDI Command]. **Jump Bar Alias**: [field] [Add Alias] [Remove Alias].

### MusicXML (sub-pages Import, Export; all checkboxes with an italic explanation, laid out in 2-column cards)
**Import / Notes and Chords**: Note durations, Stem directions, Tie placement and appearance, Beam groups, Accidental visibility, Articulation placement, Tuplet placement and bracket visibility; Advanced Options (expander). **Rests**: Rest durations, Rest positions, Rest visibility. **Other Notations**: Dynamics placement, Glissando line style and text, Slur direction, Text items (+ Import font style information), Explicit laissez vibrer ties only, Figured bass appearance, Import other-technical playing techniques, Ambiguous arpeggio lines span all staves, Import system and frame breaks, Include instrument octave transpositions at concert pitch; Advanced Options.
**Export**: Include suppressed system and frame breaks; Items at the end of the bar naturally relative to barline (+ Gap after barline spinner, spaces); Split ongoing items at system and frame breaks; Use same vertical position for all systems for lines crossing breaks.

### Note Input and Editing (sub-pages Note Input, Editing)
**Pitch and Duration**: By default, specify pitch [segmented Before Duration|After Duration]; Specify accidental, rhythm dot and articulations [segmented After inputting note|Before inputting note] + help. **Lock Duration**: [check] Jump over rests to next note only if closer than [16] quarters. **Force Duration**: [check] Preserve Force Duration state when stopping and restarting note input. **Auditioning**: Play notes during note input and selection; Play changes of shadow notehead pitch...; Play all notes in chord when any is selected; Use fixed volume to play selected notes [100]. **Mouse Input**: By default, enable note input using the mouse; Play shadow notehead pitch; Enable note pitch and rhythmic position editing using the mouse; Middle mouse button changes selection. **Breaks**: Snap system and frame breaks to barlines when creating; Lock source and destination systems when moving bars; Create system breaks when using Lock Layout (all with italic help). **Percussion Input**: Input onto kit or grid [segmented Use percussion map|Use staff position]; Interpret as [Treble G clef|Bass F clef]; Input techniques from MIDI key [spinner+MIDI-learn button]; Number of keys for techniques; Input onto single-line staves [segmented]; Interpret pitches relative to [60+learn]; (same two again). **Copying Music**: Include MIDI CC, pitch bend and dynamics from Key Editor when copying music. **Properties**: When starting a new project, set local properties [segmented Locally|Globally] + help; When selecting multiple items of different types [segmented Show all properties|Show only common properties].
**Editing**: Selection priority when clicking [Prioritize noteheads|Prioritize beams and stems]; Default click-drag behavior [Hand Tool|Marquee Select Tool]; Drag threshold [slider]; Double-click in Setup or Engrave mode switches to Write mode; Double-click on a chord symbol in Engrave mode opens [Kerning Pairs Editor|Appearance Editor]; Follow selection changes on undo and redo; Allow multiple items to be created with the mouse + Creating items with the mouse [Create item at selection|Load pointer with item]; Create items at position of first selected note (+ Create clefs at position of first selected note); Create separate trills for each selected note; Link dynamics and slurs to existing items when pasting; End gradual dynamics at start of last note in multiple selections; Cutaways changes on top staves apply to entire instrument by default; Direction of rhythmic movement when converting to grace notes [Automatic|Forwards|Backwards]; Open Project Info dialog when editing text frames containing tokens.

### Play
**Playback Template**: Default playback template [help, check Use 'Auto' playback template, field]; Default HALion plug-in [Use best available|Use preferred] + Preferred HALion plug-in [field]; [check] Update to expression maps and percussion maps with higher versions; Default stage template [dropdown]; Space template override [dropdown]. **Project Activation**: Activate project when only one project [Ask|Always|Never]; Activate subsequent project on load [Ask|Always|Never]. **Playhead**: Follow playhead during playback; Show playhead when stopped; Play from selection only plays selected staves; Play button in mini transport plays from selection, if any. **Mixer**: Default output level [slider + dB spinner]. **MIDI Input Devices**: checkbox list of devices. **Quantization**: Quantization unit [5 note-value toggle buttons] + help; Detect tuplets; Quantization unit for tuplets [5 buttons]; Detect grace notes; Fill gaps. **Recording**: Enable MIDI Input; Suspend when application is in background; Enable MIDI thru; Filter out MIDI controllers; Preserve note velocities; Preserve note positions; Import CC64 as pedal lines (+ Snap pedal lines to previous beat); Detect slurs; Detect tremolos; Detect trills; Add articulations to notes; MIDI input latency compensation [spinner ms]. **Transport Display**: Default mini transport display [Elapsed Time|Timecode|Bar.Beat.Ticks]. **Controllers**: Reset controllers and send 'all notes off' when stopping playback. **Audio Device**: [button Audio Device Setup...]; Advanced Options (expander).

### Proofreading (sub-pages Proofreading, Categories)
**General**: [check] Enable proofreading. **Stringed Instruments**: Maximum stretch between tip of index and pinky fingers [spinner 162 millimeters]. **Categories**: 19 checkboxes: Clefs, Octave clefs, Key signatures, Hidden time signatures, Repeated time signatures, Incorrect bar lengths, Pick-up bars, Repeat structures, Dynamics, Suggested dynamics, Playing techniques, Unplayable playing techniques, Suggested playing techniques, Instrument changes, Stretch and double-stopping, String harmonics, Snap pizzicato, String fingering, Instrument ranges, Empty percussion legends.

### Remote
Card: [check] Enable remote control; Port [spinner 4560]; client list; [Remove selected clients] [Clear all clients].

### VST Plug-ins
[check] Open VST plug-in windows when opening projects. **Allowed VST2 Plug-ins**: two lists Allowed / Blocked with [+] [Block All] [>] [<] [Allow All]. **Audio Engine Data**: [Clear Audio Engine Cache] (+ help), [Reset Audio Engine Data] (+ help), note "Restart Dorico for these changes to take effect."

## Mapping Dorico -> ours

| Dorico (category / section / option) | Ours (page) |
|---|---|
| General / General: Language | General: Language (also "Check for language updates") |
| General: Preferred unit of measurement, Default paper size type, Default text font family, Middle C, Default piano roll display | missing (units/paper are in Score style/Page settings) |
| General: Instrument Names default language | missing |
| General: Comments author name / full name / initials | missing |
| General: Crash Reporting prompt | missing (Advanced has no equivalent; Welcome/Privacy only) |
| General: Suppressed Warnings / Reset Warnings | missing |
| Window: Theme | Appearance: Theme (Light/Dark, follow system, high contrast, accent colour: ours has more) |
| Window: Open new windows maximized, last window closed action, side panels, Graphical Editing tool | missing |
| View: Default zoom | Canvas: Zoom > Default zoom |
| View: Default view type (Page/Galley) | missing (view type is per-score toggle, no default) |
| View: Overscroll amount | missing (Canvas: Scroll pages Horizontal/Vertical, Limit scroll area is the closest) |
| View: bring selection into view options, Show tab bar | missing |
| View: Ruler continue across spreads | missing |
| Files: Default project folder | Folders: Scores |
| Files: Auto-save every N minutes (+ help) | Save & publish: Auto save every N min |
| Files: Project backup folder / number of backups | missing |
| Files: Warn when project updated outside / fonts not installed / earlier-later version prompts | missing |
| Files: Exporting (graphics filenames, omit excluded players, CSV, suspect transposition, clipboard text format) | missing |
| Files: Show complete path for recent files | missing |
| Colors: Page Colors (full/part/custom layouts, inverted background) | Appearance: Page (Page colour, Engraving colour, Invert score colors) - partial |
| Colors: Background Colors (gradient + single colour, preset, stops, per mode) | Appearance: Background (Color or Image) - no gradient, no per-mode |
| Colors: Voice Colors 1-8 | Advanced: Voice 1-4 colour rows (table of swatches), no Voice 5-8 |
| Colors: Other Colors (selected, linked, shadow notehead, playhead, accidentals, range, grids) | Advanced: All voices, Displayed default, Formatting, Frame, Invisible, Desynchronized colours; Note input: note colours checkboxes - partial |
| Colors: Crosshair | missing |
| Key Commands (tree, add/remove, language, Print Summary, Reset) | Shortcuts (Presets dropdown, Define, Clear, Search shortcut, table, Import/Export) - ours is a flat table, no per-command editor, no keyboard-language, no print summary |
| Key Commands: MIDI Learn | MIDI mappings (table + Assign MIDI mapping, Clear, Clear all) |
| Key Commands: Jump Bar Alias | missing |
| MusicXML Import (notes, rests, other notations, advanced) | Import: MusicXML (Import layout, Import system and page breaks, Apply default typeface, Infer text type) - ours has 4 options, Dorico ~25 |
| MusicXML Export | missing |
| Note Input: Pitch before/after duration | Note input: Default input mode (by note name / duration / rhythm) - different concept |
| Note Input: accidental/dots/articulations before or after note | Note input: Apply accidentals, augmentation dots, and articulations (To next note entered / To previous) |
| Note Input: Lock Duration, Force Duration | missing |
| Note Input: Auditioning | Note input: Note preview (Hear playback when adding..., Play chord when editing, Play preview notes with dynamics, ...) |
| Note Input: Mouse Input (enable by default, shadow notehead) | missing |
| Note Input: Breaks (snap system and frame breaks, lock layout) | missing |
| Note Input: Percussion Input (map / staff position, MIDI key) | Percussion: Triggering the percussion panel (different) |
| Note Input: Copying Music, Properties (local/global, common properties) | missing |
| Editing: selection priority, click-drag, drag threshold | Canvas: Miscellaneous > Proximity for selecting elements (partial) |
| Editing: voice / dynamics reach | Note input: Voice assignment (All voices on the instrument / Only the voice) - closest |
| Editing: other behaviours (follow selection on undo, multiple items with mouse, trills, grace notes, Project Info) | missing |
| Play: Playback template, HALion, stage template | missing (Audio & MIDI has driver/device only) |
| Play: Project Activation, Playhead follow/show, mini transport | missing (Canvas has no playhead options; playback follow is a toolbar toggle) |
| Play: Mixer default output level | Audio & MIDI: Mixer (Toggle track mute checkbox) - different |
| Play: MIDI Input Devices | Audio & MIDI: MIDI (MIDI input, MIDI output dropdowns) and Note input: MIDI devices (Enable MIDI input, begin note input on key press) |
| Play: Quantization (unit, tuplets, grace notes, fill gaps) | Import: MIDI > Shortest note (partial) |
| Play: Recording (MIDI thru, velocities, pedal lines, latency compensation) | Note input: Real-time input modes (collapsed); no latency compensation |
| Play: Transport Display | missing |
| Play: Audio Device Setup | Audio & MIDI: Audio (driver, device, buffer size) |
| Proofreading (enable, stretch, 19 categories) | missing (no proofreading module) |
| Remote (enable remote control, port) | missing |
| VST Plug-ins (open windows, allowed/blocked, engine data) | missing (Audio & MIDI > Online sounds / extensions are different) |
| Reset to Factory button | Reset preferences button (same idea) |
| (none) | Ours only: Welcome screen, Automatic updates, Save to cloud, Publish to MuseScore.com, Video (FFmpeg), Folders (7 paths), Score default style files, Braille, MEI/MNX import, Advanced raw prefs table, Online sounds |

## Biggest structural differences
1. Dorico: 1130 x 708 resizable dialog, 203 pt category list WITH search field, Cmd-F page search, no OK/Cancel (Reset to Factory / Apply / Close). Ours: 880 x 672 fixed, 220 pt icon list, no search, Reset / Cancel / OK.
2. Dorico pages are long single scrolls of rounded dark cards with explanatory italic text; groups (General, Colors, MusicXML, Note Input and Editing, Proofreading) show sub-rows that scroll to anchors. Ours: flat sections with divider lines and no help text.
3. Dorico control vocabulary: segmented toggle buttons in accent blue, hex+swatch+reset rows, help "?" icons. Ours: radios, toggles, dropdowns.
4. Missing in ours (largest gaps): Colors (all but page/background), Key Commands per-command editor, Note Input/Editing behaviours, Play (playhead, quantization, recording), Proofreading, Remote, VST, Files warnings/backups/exports, Comments author.
