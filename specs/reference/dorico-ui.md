<!-- markdownlint-disable -->
# Dorico Pro 6.2 user interface inventory

Source: the official Dorico Pro 6.2 manual. In this file `[M]` = `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/`. Key bindings are in `dorico-key-commands.md`, popover syntax in `dorico-popovers.md`; they are not repeated here.

**Known gaps in the manual** (nothing below is guessed beyond these):
- The Notes panel's individual buttons exist only in a screenshot (section 2.2 lists the parts and the assumed content).
- The order of the lower-zone tabs per mode is only in screenshots.
- The Notes toolbox order (2.1) is the order the manual lists the buttons; it does not say outright that it is top-to-bottom.
- The menu list (0.7) is built from items that topics happen to mention. It is **not** a full menu dump.
- No "Routing panel" or "Event display" page exists for Play or Setup mode in 6.2. Routing is a section of the Track Inspector.

## 0. Project window (all modes)

### 0.1 Areas - [M]`user_interface/user_interface_windows_c.html`
| Area | Purpose |
|---|---|
| Toolbar | Modes, workspace options, Mixer, Video, mini transport, undo/redo, help |
| Tab bar | Open tabs (Setup, Write and Engrave only); each split partition gets its own |
| Central area | Project start area, then the music area; track overview in Play; print preview in Print |
| Zones | Left, right and lower panels, which change per mode |
| Toolboxes | Notes and Notations (Write), Engrave (Engrave) - [M]`user_interface/user_interface_toolboxes_c.html` |
| Status bar | View type, page arrangement, zoom, selection summary |

### 0.2 Toolbar, left to right - [M]`user_interface/user_interface_toolbar_r.html` (Cmd-6)
| Item | Purpose |
|---|---|
| Modes | Setup / Write / Engrave / Play / Print; collapse to a menu when the window is narrow |
| Layout selector | Layout shown in the current tab (disabled in Print mode) |
| View | Hide/show common visual elements (tab bar, system track...); contents depend on the mode |
| Hide/Restore Zones | Hide/show all open zones |
| Show Video | Video window; only when playback is active and the project has a video |
| Show Mixer | Mixer window |
| Mini transport | see 0.3 |
| Undo / Redo | |
| Show Help | Contextual help overlay |

### 0.3 Mini transport - [M]`user_interface/user_interface_transport_options_r.html`
| Item | Purpose |
|---|---|
| Activate Project | Playback on/off for this project |
| Time display | Click to cycle bars/beats/ticks, elapsed time, flow timecode |
| Rewind to Beginning of Flow | |
| Cycle | Cycle playback on/off |
| Play | Start/stop from the playhead or the selection |
| Record | Start/stop MIDI recording |
| Click | Metronome |
| Tempo Mode | Shows the tempo; click the beat unit for Fixed/Follow; click the number for a slider |
| Scrub Playback | |
| Transport Window | Full Transport window |

### 0.4 Tab bar - [M]`user_interface/user_interface_tab_bar_r.html`
Tabs named after their layout, a close "x" on hover, and a New Tab button. Toggled from toolbar View > Show Tabs.

### 0.5 Status bar - [M]`user_interface/user_interface_status_bar_r.html`
Not every item exists in every mode. Manual order:

| Item | Purpose |
|---|---|
| Rhythmic grid selector | Grid resolution (caret step, move step) |
| Layout transposition | Concert or transposed pitch for the open layout |
| Order | Instrument score order (Setup mode only) |
| Selection summary | Page (page view only), bar or bar range, timecode or range plus duration, and an overview (pitch and voice, or the implied chord) |
| Disclosure arrow | Hide/show the lower zone |
| Follow playhead | |
| Click-drag tools | Marquee Tool / Hand Tool (Opt-H toggles) |
| View type | Page / Galley / Fill - [M]`user_interface/user_interface_view_types_r.html` |
| Page arrangement | Spreads Horizontally, Spreads Vertically, Single Pages Horizontally, Single Pages Vertically |
| Zoom options | Custom Zoom (dialog), Set Zoom (presets), Zoom Out, Zoom In |
| Indicator/Warning | Green light = MIDI input arriving; warning icon = MIDI can't reach the audio engine |

### 0.6 Zones per mode - [M]`user_interface/user_interface_panels_r.html`
| Mode | Left zone (Cmd-7) | Right zone (Cmd-9) | Lower zone (Cmd-8) |
|---|---|---|---|
| Setup | Players | Layouts | Flows |
| Write | Notes | Notations panels | Properties, Keyboard, Fretboard, Drum Pads, Mixer, Key Editor |
| Engrave | Formatting / Frames / Graphic Slices | Pages | Properties, Mixer |
| Play | Track Inspector / VST and MIDI | n/a | Mixer, Key Editor |
| Print | Layouts | Print Options | n/a |

### 0.7 Menus (incomplete, see gaps)
Top-level: **Dorico (macOS), File, Edit, Setup, Write, Engrave, Play, Library, View, Window, Help.**

| Menu | Items the manual mentions |
|---|---|
| File | Open, Open Recent, Project Info, Project Statistics, Save As Project Template, Import (Flows, MIDI, MusicXML, Tempo Track), Export (Audio, Flows, Lyrics, MIDI, MusicXML, Tempo Track, Text), Import/Export User Settings |
| Edit | Copy, Paste, Paste Special (Paste Into Voice, Explode, Reduce, Swap, Move to Staff Above/Below, Paste Articulations), Delete, Remove Rests, Select All/None, Filter, Go To (Page, Rehearsal Mark), History, Notations > (Bar Numbers, Beaming, Chord Symbols and Diagrams, Clef, Cross Staff, Dynamics, Figured Bass, Fingering, Lyrics, Notehead, Octave Line, Pedal Lines, Percussion, Playing Techniques, Slurs, Staff, Staff Size, Stem, Text, Voices), Propagate Properties, Reset Appearance, Reset Position, Requantize, Device Setup, Preferences |
| Setup | Propagate Part Formatting |
| Write | Add Intervals, Create... (Bar or Barline, Chord Symbol, Clef, Cue, Dynamic, Figured Bass, Fingerings, Hold or Pause, Key Signature, Lyrics, MIDI Trigger Region, Ornament, Playing Technique, Repeat, Tempo, Time Signature, Tuplet), Calculate Harp Pedals, Insert Scope (> Set Edit Stop Position), Instrument Visibility (> Edit Instrument Filter Presets), Respell (Propagate/Reset Note Spellings), Rhythmic Grid (Increase/Decrease Resolution), Split Flow, Transform > Pitches, Transpose, Input Pitch |
| Engrave | Condensing Change, Note Spacing Change, Grid, Rulers, Reset Bracketing, Split Multi-bar Rest, Staff Spacing > Copy Staff Spacing |
| Play | Cycle, Fixed Tempo Mode, Scrub Playback, Playback Template, Space Template, Stage Template, Reset Playback Overrides |
| Library | Layout/Notation/Engraving/Playback/Note Input Options, Library Manager, Export Library, Character/Font/Paragraph Styles, Chord Symbols, Chord Symbol Kerning Pairs, Instruments, Instrument Families, Instrument Score Orders, Expression Maps, Percussion Maps, Playback Techniques, Playing Techniques, Lines, Line Annotations, Line Bodies, Repeatable Symbols, Notehead Sets, Page Sizes |
| View | View Options, Full Screen, System Track, Comments, Page Numbers, Attachment Lines, Cutaway Staff Lines, Cycle Locators, Token Placeholders, Highlight (Bar Repeat / Chord Symbol / Slash / MIDI Trigger Regions, Cues), Note and Rest Colors, Signposts |
| Window | Setup / Write / Engrave / Play / Print, New Tab, New Window, Show Toolbar, Show Left/Right/Lower Zone, Hide/Restore Zones, Mixer, Video |
| Help | Show Help, Key Commands |

Other pages: music area `user_interface/user_interface_music_area_r.html`; system track `write_mode/write_mode_selecting/write_mode_system_track_r.html`; View Options dialog `user_interface/user_interface_view_options_dialog_r.html`; Hub `project_file_handling/project_file_handling_hub_r.html`.

## 1. Setup mode - [M]`setup_mode/setup_mode_project_window_r.html`
Selecting an item in one panel shows checkboxes on the cards in the other two (that is how players, layouts and flows are assigned to each other).

### 1.1 Left: Players panel - [M]`setup_mode/setup_mode_players_panel_r.html`
| Element | Purpose |
|---|---|
| Player card | Disclosure arrow, player type (single/section), name, add-instruments icon, instrument labels (blue; kits green) with an instrument menu |
| Add Single Player | New player plus its part layout |
| Add Section Player | |
| Add Ensemble | Several players at once |
| Add Group | Empty, or wrapping the selected players |
| Sort Players | By score order; click-hold for None / Jazz / Orchestral / Band / custom |
| Player Settings | Rename, show chord symbols... |
| Delete Player | With a warning |

### 1.2 Right: Layouts panel - [M]`setup_mode/setup_mode_layouts_panel_r.html`
| Element | Purpose |
|---|---|
| Layout card | Disclosure arrow, type (full score / instrumental part / custom score), name, page size and orientation, space size (pt), layout number |
| Add Full Score / Instrumental Part / Custom Score Layout | |
| Sort Layouts | By type; click-hold for Layout Number or Instrument Score Order |
| Layout Options | Opens the Layout Options dialog |
| Layout Settings | Rename... |
| Delete Layout | |

### 1.3 Lower: Flows panel - [M]`setup_mode/setup_mode_flows_panel_r.html`
Horizontal list of flow cards (name, film-reel icon if it has video, start timecode, flow number). Action bar: Add Flow, Notation Options, Flow Settings, Delete Flow.

## 2. Write mode - [M]`write_mode/write_mode_window_r.html`

### 2.1 Notes toolbox (left edge) - [M]`write_mode/write_mode_notes_toolbox_r.html`
| # | Button | Purpose / click-hold options |
|---|---|---|
| 1 | Show Left Zone | Hide/show the Notes panel |
| 2 | Select | Mouse input off |
| 3 | Start Note Input | Show/hide the caret |
| 4 | Pitch Before Duration (K) | Choose pitch first; duration key commits |
| 5 | Chords (Q) | Caret doesn't advance |
| 6 | Insert (I) | Push later notes on. Scope (Opt-I): Voice / Player / Global / Global Adjustment |
| 7 | Lock Duration (L) | Change pitches, keep rhythm |
| 8 | Force Duration (O) | Exact duration, no automatic splitting |
| 9 | Create Voice | Create Voice (Shift-V) / Create Slash Voice (Shift-Opt-V) / Next Voice (V) |
| 10 | Dotted Notes (.) | One to Four Rhythm Dots (Opt-. cycles) |
| 11 | Rests (,) | |
| 12 | Tuplets | 2:3, 3:2, 4:3, 5:4, 6:4, 7:8, x:y (opens the tuplets popover) |
| 13 | Grace Notes (/) | Unslashed / Slashed (Opt-/) |
| 14 | Tie (T) | |
| 15 | Scissors (U) | Split at the caret; outside note input, removes ties |

### 2.2 Left zone: Notes panel - [M]`write_mode/write_mode_panels/write_mode_notes_panel_r.html`
| Part | Contents |
|---|---|
| Upper | Note and rest durations; common ones by default, Show/Hide All Notes arrows at the top and bottom |
| Middle | Accidentals (toggle) and Slur |
| Bottom | Articulations (toggle). Assumed to be the eight in `dorico-key-commands.md` A5: accent, marcato, stressed, unstressed, staccato, tenuto, staccatissimo, tenuto-staccato |

### 2.3 Notations toolbox (right edge) - [M]`write_mode/write_mode_notations_toolbox_r.html`
A **Panels / Popovers** switch at the top; the buttons below depend on it.

**Panel buttons** (`…` = `write_mode/write_mode_notations_input`):

| Button | Panel sections | URL |
|---|---|---|
| Clefs | Common, Uncommon, Archaic Clefs, Octave Lines | [M]`…/write_mode_clefs_panel_r.html` |
| Key Signatures, Tonality Systems, Accidentals | Tonality System (menu + New/Duplicate/Edit/Import/Export/Delete), Used in This Flow, Key Signatures (Major/Minor, More Sharps/Fewer Flats, Fewer Sharps/More Flats, Input), Custom Key Signatures, Accidentals | [M]`…/write_mode_key_signatures_tonality_systems_accidentals_panel_r.html` |
| Time Signatures | Used in This Flow, Common, Create Time Signature (Regular / Additive / Interchangeable / Aggregate / Alternating; up to 4 fields; pick-up bar of 1/2, 1, 2, 3 beats; Input) | [M]`…/write_mode_time_signatures_panel_r.html` |
| Tempo | Used in This Flow, Tap Tempo, Absolute Tempo Change, Gradual Tempo Change (+ modifiers), Relative Tempo Change (+ modifiers), Reset Tempo, Tempo Equation | [M]`…/write_mode_tempo_panel_r.html` |
| Dynamics | Immediate (+ modifiers), Gradual (+ modifiers), Force/Intensity of Attack, Combined Dynamics | [M]`…/write_mode_dynamics_panel_r.html` |
| Ornaments | Jazz, Baroque and Classical, Arpeggiation, Glissandi, Guitar | [M]`…/write_mode_ornaments_panel_r.html` |
| Repeat Structures | Repeat Endings, Repeat Jumps, Repeat Sections, Tremolos, Rhythm Slashes, Bar Repeats, Numbered Bars | [M]`…/write_mode_repeat_structures_panel_r.html` |
| Bars and Barlines | Insert Bars, Insert Bar Rest, Create Barline | [M]`…/write_mode_bars_barlines_panel_r.html` |
| Holds and Pauses | Fermatas, Breath Marks, Caesuras | [M]`…/write_mode_holds_pauses_panel_r.html` |
| Playing Techniques | Common, Wind, Brass, Unpitched Percussion, Pitched Percussion, Keyboard (incl. pedal lines), Choral, Strings, Guitar, Harp Pedals | [M]`…/write_mode_playing_techniques_panel_r.html` |
| Lines | Horizontal, Vertical | [M]`…/write_mode_lines_panel_r.html` |
| Cues | Create Cue, Suggest Cues | [M]`…/write_mode_cues_panel_r.html` |
| Video | Properties; Markers table (Timecode, Text, Imp.) with Add/Delete Marker | [M]`…/write_mode_markers_panel_r.html` |
| Comments | List (author, date, text, instrument, flow, bars); Create / Edit / Reply / Export / Delete | [M]`write_mode/write_mode_comments/write_mode_comments_panel_r.html` |
| Proofreading | Categories menu, filters, flows, show/clear ignored, issues list; button shows the issue count | [M]`proofreading/proofreading_panel_r.html` |

**Popover buttons**, in order: 1 Clefs, 2 Key Signatures, 3 Time Signatures, 4 Tempo, 5 Dynamics, 6 Ornaments, 7 Repeat Structures, 8 Bars and Barlines, 9 Holds and Pauses, 10 Playing Techniques, 11 Cues, 12 Rehearsal Marks (inputs one directly), 13 Text (opens the text editor), 14 Lyrics, 15 Chord Symbols, 16 Note Tools, 17 Fingering, 18 Figured Bass, 19 MIDI Trigger Region. Lyrics, chord symbols, fingering, figured bass and text have no panels.

### 2.4 Lower zone - [M]`write_mode/write_mode_panels/write_mode_lower_zone_r.html`
Selector: **Properties, Keyboard, Fretboard, Drum Pads, Mixer, Key Editor.**

| Panel | Contents | URL |
|---|---|---|
| Properties | Search (Opt-8), Show active status (Active/All), Show scope (Local/Global/All), Set local properties; groups per item type | [M]`properties/properties_panel_r.html` |
| Keyboard | Range strip; Advance Caret, Delete Left, Set Note Duration, Slur, Set Articulation, Respell Using Note Name Above/Below, Resize | [M]`write_mode/write_mode_panels/write_mode_keyboard_panel_r.html` |
| Fretboard | Advance Caret, Delete Left, Set Note Duration (fretted instrument only) | `…/write_mode_fretboard_panel_r.html` |
| Drum Pads | Advance Caret, Delete Left, Set Note Duration, Reorder Drum Pads, Resize | `…/write_mode_drum_pads_panel_r.html` |
| Mixer | see 4.4 | |
| Key Editor | see 4.4 | |

## 3. Engrave mode - [M]`engrave_mode/engrave_mode_window_r.html`
Always page view. The left zone hides automatically while Note Spacing or Staff Spacing is active.

### 3.1 Engrave toolbox - [M]`engrave_mode/engrave_mode_engrave_toolbox_r.html`
| Button | Purpose |
|---|---|
| Show Left Zone | |
| Graphic Editing (default) | Select/edit items; shows Formatting |
| Frames | Edit frames; shows Frames panel |
| Staff Spacing | Move staves and systems vertically |
| Note Spacing | Move notes horizontally (spacing columns) |
| Graphic Slices | Shows the Graphic Slices panel |

### 3.2 Left zone - [M]`engrave_mode/engrave_mode_left_zone_r.html`
| Panel | Sections / buttons |
|---|---|
| Formatting ([M]`engrave_mode/engrave_mode_formatting_panel_r.html`) | **Moving Bars Between Systems**: Move to Previous System (,), Next System (.). **Format Systems**: Insert System Break (Shift-S), Lock System, Make into System. **Format Music Frames**: Insert Frame Break (Shift-F), Lock Frame, Make into Frame. **Lock Layout**: Lock Layout, Reset Layout. **Coordination Lines**: Create. **Bracketing**: bracket / sub-bracket / sub-sub-bracket / brace, Change barline joins |
| Frames ([M]`engrave_mode/engrave_mode_frames_panel_r.html`) | **Insert Frames**: Music / Text / Graphics. **Constraints**: lock frame edges to page margins |
| Graphic Slices ([M]`engrave_mode/engrave_mode_graphic_slices_panel_r.html`) | Create Slice; table (name, page) with Rename/Delete; image settings; destination; Export All / Selected |

### 3.3 Right: Pages panel - [M]`engrave_mode/engrave_mode_pages_panel_r.html`
| Section | Actions |
|---|---|
| Pages | Insert Pages, Insert Page Number Change, Insert Page Template Change, Insert Flow Heading Change, Swap with Previous/Next Page, Remove Overrides |
| Page Templates | New, Edit, Rename, Import, Delete (current set menu) |
| Flow Headings | New, Edit, Rename, Delete |
| Page Template Sets | New, Rename, Import, Export, Delete |

### 3.4 Lower zone: Properties and Mixer only.

## 4. Play mode - [M]`play_mode/play_mode_project_window_r.html`
No toolbox and no right zone.

### 4.1 Left: Track Inspector - [M]`play_mode/play_mode_track_inspector_r.html`
| Section | Controls |
|---|---|
| Routing | Audio plug-in menu, Port, Channel, Endpoint Setup, Edit Instrument, Expression/Percussion map, Independent Voice Playback, "Edits apply to" (This flow / All flows) |
| Insert Effects | 4 slots (Activate, Insert menu, Edit Insert) |
| Channel | Mixer channel strip for the track |

### 4.2 Left: VST and MIDI panel - [M]`play_mode/play_mode_vst_midi_panel_r.html`
VST Instruments (Activate, name/number, menu, Endpoint Setup, Edit Instrument) and MIDI Instruments (name/number, menu, Endpoint Setup). Action bar: Add, Duplicate, Save Endpoint Configuration, Delete.

### 4.3 Centre: Track overview - [M]`play_mode/play_mode_track_overview_r.html`
Flow selector, track height, visibility buttons (Tempo, Markers, Chords), Mute/Solo, ruler, track headers, tracks, playhead.

### 4.4 Lower zone
- **Key Editor** - [M]`key_editor/key_editor_panel_r.html`; toolbar `key_editor/key_editor_panel_toolbar_r.html`: tools Select, Draw, Drumstick, Line, Transform, Sync Region, Delete; Played/Notated Durations, Lock Played Durations; Rhythmic Grid; Playing Techniques; Voice selector. Editors: piano roll, percussion, Playing Techniques, Velocity, Dynamics, MIDI Pitch Bend, MIDI CC, Tempo.
- **Mixer** - [M]`mixer/mixer_panel_r.html`; toolbar `mixer/mixer_toolbar_r.html`: channel-type buttons, Deactivate all (mute/solo), Add/Remove Effects Channel, Live Stage, Live Space. Window (F3): `mixer/mixer_window_r.html`.

### 4.5 Playback templates (dialogs): `play_mode/play_mode_playback_templates_c.html`, Apply, Edit and Auto Playback Template dialogs.

## 5. Print mode - [M]`print_mode/print_mode_window_r.html`
Layout selector disabled; view options Page Width and Whole Page. Preview shows only the first selected layout and can't be edited.

- **Left: Layouts panel** - [M]`print_mode/print_mode_layouts_panel_r.html`. Cards with type, name, page size, **page count**. Selected layouts are the ones printed or exported.
- **Right: Print Options panel** - [M]`print_mode/print_mode_print_options_panel_r.html`:

| Section | Contents |
|---|---|
| Destination | Printer (copies) or graphics file (format, colour mode, resolution, file name, folder) |
| Job Type | All pages / page range / flow range; one PDF per flow; arrangement (2-up, booklet) |
| Page Setup | Paper size, orientation, scale |
| Duplex Printing | One- or two-sided |
| Annotations | Crop marks, border |
| Print button | Print, Export, or Print and Export |

## 6. Coverage map (every section above -> a spec)
| Section | Spec |
|---|---|
| 0.2, 0.3, 0.4, 0.6 | 50 |
| 0.5 | 51 |
| 0.7 | 59 |
| 2.1, 2.2 | 52 |
| 2.3 | 53 |
| 2.4 | 54 |
| 1 | 55 |
| 3 | 56 |
| 4 | 57 |
| 5 | 58 |
