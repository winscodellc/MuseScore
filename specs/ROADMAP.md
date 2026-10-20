<!-- markdownlint-disable -->
# Roadmap: Dorico Pro 6 feature gap check

Source of the feature list: Steinberg's compare-editions page (Dorico Pro 6 column, 94 rows, fetched 2026-10-20). Status of each row was checked against `src/` and `specs/done/`. Status values:
`done (spec NN)` implemented by this fork · `MuseScore native` upstream already has it · `partial` some exists · `missing` nothing yet · `deferred` listed in the README's Global deferred list · `out of scope` bundled content, not a feature.

The compare table is marketing-level. Section 2 (manual pass) lists what it omits.

## 1. Compare-table pass

### Engraving, Customization, Printing, Playback, Import and export

| Group | Feature | Dorico value | Status | Evidence / gap |
|---|---|---|---|---|
| Engraving | Add page and system breaks | yes | done (spec 56) | Formatting panel: toggle-system-break, toggle-page-break |
| Engraving | Dedicated Engrave mode | yes | done (spec 56) | EngravePage.qml: toolbox, Formatting/Frames panels, page view only |
| Engraving | Edit vertical spacing | yes | partial | Style page/staff distance and spacers only; Staff Spacing tool deferred (56) |
| Engraving | Edit rhythmic spacing | yes | partial | Measure stretch + style spacing; Note Spacing tool deferred (56) |
| Engraving | Tweak individual items graphically | yes | MuseScore native | Drag/edit mode + Properties offsets; nudge keys deferred (README) |
| Engraving | Customizable engraving options | yes | MuseScore native | Format > Style (`src/engraving/style/styledef.cpp`); no Dorico-style dialog layout |
| Engraving | Set defaults for beaming, voices, etc. | yes | partial | Beam/voice style settings exist; no per-flow note-grouping defaults |
| Engraving | Use other text fonts | Fully customizable | MuseScore native | Per-text-style font in Style; system fonts |
| Engraving | Use other music fonts | yes | MuseScore native | Bravura/Leland/Petaluma etc., SMuFL font selection in Style |
| Engraving | Automatic collision avoidance | yes | MuseScore native | Autoplace in `src/engraving/dom` and style |
| Engraving | Multiple staff sizes | yes | MuseScore native | Staff/part scale (`src/engraving/dom/staff.cpp`) |
| Engraving | Note spacing settings | Fully customizable | partial | Style spacing ratio/min distance; no per-bar note-spacing editor (56 deferred) |
| Engraving | Staff and system spacing | Fully customizable | MuseScore native | Style page: staff/system distance, min/max; no graphic tool |
| Customization | Change page and staff size | yes | MuseScore native | Page Settings dialog; staff scale; Page Setup in Print (spec 58) |
| Customization | Custom page layout | yes | partial | Margins/frames exist; Pages panel templates, flow headings, page swaps stubbed (56) |
| Customization | Custom note spacing | Full control | partial | Style/stretch only; Note Spacing tool deferred (56) |
| Customization | Custom key signatures | yes | MuseScore native | Custom key signature dialog in palette/keysig |
| Customization | Custom accidentals | yes | partial | Microtonal accidentals and custom keysig symbols; no accidental-set editor |
| Customization | Custom playing techniques | yes | partial | Custom staff text + playback sound; no playing-technique editor/library |
| Customization | Custom noteheads | yes | partial | Notehead scheme/group via Properties; no custom notehead set editor |
| Customization | Custom text fonts | Fully customizable | MuseScore native | Text styles in Format > Style > Text styles |
| Customization | Custom music fonts | yes | MuseScore native | SMuFL font install/select |
| Customization | Custom chord symbols | yes | MuseScore native | Chord symbol style files + editor |
| Customization | Notation Options | Customizable | partial | Scattered Style/Properties; no Dorico Notation Options dialog |
| Customization | Engraving Options | Customizable | partial | Format > Style covers content; different dialog structure |
| Customization | Playback Options | Customizable | partial | Preferences > Playback + Mixer; no Dorico Playback Options dialog |
| Printing | Print multiple copies | yes | partial | System print dialog copies; Print panel Copies/Collate deferred (58) |
| Printing | Booklet, 2-up, spreads | yes | missing | Booklet/2-up deferred in spec 58; spreads deferred (README) |
| Playback | Iconica Sketch | yes | out of scope | Bundled sound content |
| Playback | Included sounds | 2000+ (13GB library) | out of scope | Bundled sound content (Muse Sounds is the MuseScore equivalent) |
| Playback | Effects processors | 32 | out of scope | Bundled content (MuseScore has VST3 FX slots) |
| Playback | SuperVision | (blank) | out of scope | Bundled product |
| Playback | General MIDI sounds | yes | MuseScore native | MuseSounds/FluidSynth soundfonts in `muse/framework/audio` |
| Playback | HALion Sonic SE production sounds | yes | out of scope | Bundled sound content |
| Playback | HALion Symphonic Orchestra | yes | out of scope | Bundled sound content |
| Playback | Soundiron Olympus Choir Micro | yes | out of scope | Bundled sound content |
| Playback | Keda Indian drum sounds | yes | out of scope | Bundled sound content |
| Playback | VST Expression Maps | yes | missing | Deferred in spec 57; MuseScore uses sound profiles instead |
| Playback | Supports third-party VSTs | yes | MuseScore native | `muse/framework/vst`; Play mode mixer routing (57) |
| Playback | Interpretation of notations | Customizable | partial | Sound profiles / Properties playback tab; no playback template editor |
| Playback | Swing playback | Customizable | partial | Swing/tripletfeel in engraving dom; swing in tempo popover deferred (README) |
| Playback | Playback of repeats | yes | MuseScore native | Repeat expansion in `src/engraving/playback` |
| Playback | Playback of dynamics | yes | MuseScore native | `src/engraving/playback` dynamics rendering |
| Playback | Playback of tempo changes | yes | MuseScore native | Tempo track row deferred (57) |
| Playback | Edit automation data | yes | missing | Only mixer faders; no Key Editor / lanes (57) |
| Import and export | Import MusicXML | yes | MuseScore native | `src/importexport/musicxml` |
| Import and export | Export MusicXML | yes | MuseScore native | `src/importexport/musicxml` |
| Import and export | Export audio (WAV, MP3) | yes | MuseScore native | `src/importexport/audioexport` |
| Import and export | Export graphics | yes | MuseScore native | `src/importexport/imagesexport`; Graphic Slices panel stub (56) |
| Import and export | Import MIDI | yes | MuseScore native | `src/importexport/midi` |
| Import and export | Export MIDI | yes | MuseScore native | `src/importexport/midi` |

### Notations, Input and editing

| Group | Feature | Dorico value | Status | Evidence / gap |
|---|---|---|---|---|
| Notations | Maximum number of players in project | Unlimited | MuseScore native | No instrument cap in `src/engraving/dom/score.cpp` |
| Notations | Included templates | 40 | MuseScore native | 43 templates in `share/templates` |
| Notations | Accidentals | Normal, double, microtonal, custom | partial | Quarter-tone in `accidental.cpp`; no custom accidental editor |
| Notations | Bar numbers | Multiple formats | partial | Interval/placement styles exist; no Roman or other numbering formats |
| Notations | Bar repeats | yes | done (spec 30) | `measurerepeat.cpp`; Shift-R popover |
| Notations | Barline types | Comprehensive | done (spec 25) | Shift-B popover |
| Notations | Clefs | 26 presets | done (spec 24) | 42 ClefTypes; Shift-C popover |
| Notations | Custom note and rest groupings | yes | MuseScore native | Time-signature beam groups; spec 22 popover |
| Notations | Custom brackets and braces | yes | MuseScore native | `bracket.cpp`, `bracketitem.cpp` |
| Notations | Chord diagrams | Full | MuseScore native | `src/engraving/dom/fret.cpp` |
| Notations | Chord symbols | 9 presets, fully customizable | done (spec 32) | Shift-Q popover; `chordlist.cpp` |
| Notations | Condensing | yes | missing | No model; only a dropped Engrave menu item (59) |
| Notations | Cues | yes | deferred (README) | No cue model; Cues panel is a stub (53) |
| Notations | Divisi for divided string sections | yes | missing | Only manual staff text or voices |
| Notations | Drum set notation | yes | MuseScore native | `drumset.cpp`, drum input panel |
| Notations | Dynamics | yes | done (spec 21) | Shift-D popover |
| Notations | Fanned beams and stemlets | yes | partial | Feathered beams native; no stemlets |
| Notations | Figured bass | yes | done (spec 36) | Shift-G popover |
| Notations | Guitar tablature | Full | MuseScore native | TAB staff types; spec 34 fingering popover |
| Notations | Harp pedaling | Full | partial | `harppedaldiagram.cpp` native; harp panel and Calculate Harp Pedals deferred (53) |
| Notations | Key signatures | Fully customizable | partial | Custom keys native; popover spec 23; custom-key panel deferred (53) |
| Notations | Holds and pauses | yes | done (spec 27) | Shift-H popover |
| Notations | Lines | Fully customizable | MuseScore native | Line/spanner palette; Lines panel in spec 53 |
| Notations | Lyrics | yes | done (spec 33) | Shift-L popover |
| Notations | Noteheads | 30 presets, fully customizable | partial | Notehead groups native; no custom noteheads editor |
| Notations | Octave lines | Fully customizable | done (spec 24) | Ottava via Shift-C popover |
| Notations | Ornaments | yes | done (spec 28) | Shift-O popover |
| Notations | Ossia staves | yes | missing | No ossia in `src/engraving`; only manual small or linked staves |
| Notations | Change no. staves for instrument | yes | MuseScore native | Instruments panel add/remove staff |
| Notations | Page numbers | Customizable | MuseScore native | Header/footer page number style |
| Notations | Pedal lines | Fully customizable | MuseScore native | `pedal.cpp`, pedal line styles |
| Notations | Playing techniques | Customizable | done (spec 29) | Shift-P popover; text-based techniques |
| Notations | Rehearsal marks | Customizable | MuseScore native | `rehearsalmark.cpp`; Dorico popover deferred (53) |
| Notations | Repeat endings | yes | done (spec 30) | Volta via Shift-R popover |
| Notations | Rhythm slashes | yes | done (spec 30) | Fill-with-slashes command |
| Notations | Single- and multi-note tremolos | yes | done (spec 30) | Shift-R popover |
| Notations | Tempo markings | yes | done (spec 26) | Shift-T popover; swing and feel entries deferred |
| Notations | Unpitched orchestral/band percussion | Customizable | partial | Drumset and percussion panel; no Dorico percussion kits or technique sets |
| Input and editing | Insert mode | yes | partial | TIMEWISE insert (spec 11); Opt-I scopes deferred (README) |
| Input and editing | Multi-paste | yes | done (spec 42) | Repeated Opt-click paste |
| Input and editing | Explode and Reduce | yes | done (spec 47) | Gaps: cue removal, over 4 rhythms, multi-voice source |
| Input and editing | Paste into Voice | yes | done (spec 41) | Voice 1-4 and New Voice; single-voice clipboard only |
| Input and editing | Filters | Full | done (spec 45) | Some categories lack a model |

Totals over the 94 rows: 35 MuseScore native, 23 partial, 21 done, 8 out of scope, 6 missing, 1 deferred.

## 2. Manual pass (what the compare table omits)

Chapters of `specs/reference/manual/index.md`, checked section by section against `src/` and `specs/done/`. Sections that are `done` or `MuseScore native` are summarised per chapter; every `partial`, `missing` and `deferred` section is listed. Skipped: Glossary, Introduction, Dorico concepts. Sections already in section 1 are not repeated.

### Fully or mostly covered

| Chapter | Covered by |
|---|---|
| Notation reference | Articulations, fingering, grace notes, trills, arpeggios, glissandi, jazz articulations, time signatures, tuplets, bars (specs 22, 25, 28, 31, 34); rests, multi-bar rests, slurs, ties, stems, tablature, text, voices, bracketed noteheads (native) |
| Notations input / Note input | Popover framework (20) and popovers 21-36; note input and caret (11-13); accidentals, rests, chords, tuplets, durations |
| Project and file handling | Hub, new/open project, version migration, templates, auto-save, backups, Project Info (all native) |
| File import and export | MusicXML, MIDI and audio import/export (native) |
| Selecting and editing music | Selection (44), Filters (45), Explode/Reduce (47), transposing (35), Insert mode (11) |
| Navigation, Jump bar | Spec 46; view moving and zoom (native, 51) |
| Project window, Write, Play, Print, Engrave, Setup modes | Specs 50-58 (stubs listed below) |
| Page formatting | Page size, margins, staff size, empty staves, casting off, cutaways, page numbers, system breaks (native, 56) |
| Mixer | Panel, channels, volume, pan, FX, inserts, scrolling (57, native) |
| Playback | Playhead, play back, repeats, cycle, mute/solo (57, native) |
| Library | Music fonts, text formatting, chord diagrams and chord symbol appearance (native) |
| Tokens, Properties | Page-number, Project Info and `$:key:` tokens; Inspector panel and numeric fields (native) |

### Gaps by chapter (`partial` / `missing` / `deferred`)

**Notation reference, Notations input, Note input**

| Section | Status | Gap |
|---|---|---|
| Beaming, note and rest grouping | partial | Stemlets missing; no per-flow defaults |
| String indicators | partial | `string N` popover entry unsupported (29) |
| Tonality systems (EDO) | missing | No model; Notations panel section stubbed (53) |
| Notes (notehead sets, string assignment) | partial | No custom notehead sets |
| Harmonics | partial | Element native; `harm ...` popover entries unsupported (29) |
| Guitar bends, guitar techniques (notation and input) | partial / missing | Elements exist (`guitarbend.cpp`, `tapping.cpp`...); no input path |
| Coordination lines | deferred (56) | No model |
| Markers, Timecodes, Video | missing | `marker.cpp` is segno/coda only; Video panel stub (53) |
| Numbered bar regions | deferred (30) | Popover `number bars` entry |
| Staves (extra, ossia) | partial | No ossia / extra-staff model; cutaways native |
| Staff labels | partial | No player-group or numbering options |
| Chord symbols input | partial | No Generate Chord Symbols dialog or chord regions |
| Lines (vertical) | partial | Vertical / coordination lines missing |
| Text items | partial | No Insert Music Text dialog |
| Rehearsal marks | partial | Popover deferred (53) |
| Cues, MIDI trigger regions | deferred (README) | No model |
| Caret (multi-staff), slash voices, multiple staves, tablature | deferred (README) | See Global deferred list |
| Insert mode | partial | Opt-I scopes deferred |
| Unpitched percussion, kits | partial | No kit editor |
| Ties, grace notes | partial | Percussion grace runs missing |
| MIDI recording | deferred (README) | Only `realtime-auto` / `realtime-advance` |

**Setup, Engrave, Print**

| Section | Status | Gap |
|---|---|---|
| Players, layouts, flows | partial | Flows panel is a read-only stub |
| Ensembles, Player groups | deferred (55) | No model |
| Flows, Flow names and titles | missing | No flow model |
| Layouts with clef/transposition overrides | missing | No per-part override |
| Percussion kits, Capos, Layouts, names | partial | No kit editor; no layout-name tokens |
| System / Frame fullness indicators | missing | |
| Engrave rulers, grids | deferred (56) | |
| Tucking index properties | missing | |
| Moving items graphically | partial | Nudge keys deferred (README) |
| Print: page ranges, page arrangements, duplex, annotations | deferred (58) | |
| Print: page vs paper size, graphics formats | partial | |

**Page formatting, Page templates, Flow headings, Frames, Graphic slices**

| Section | Status | Gap |
|---|---|---|
| Page template sets, types, editor, page format changes | missing | Only header/footer styles |
| Flow headings (all 9 sections) | missing | No flows; title frame is the nearest analogue |
| Hiding blank staves after flows, multiple flows per page | missing | No flows |
| Inserting / Deleting pages, left-hand start, First page template | partial | Page-break driven only |
| Staff spacing, Note spacing | partial | Style only; tools deferred (56) |
| Frame breaks, Front matter | partial | Page break stands in |
| Tacets, Condensing, Part formatting propagation | missing | |
| Frames (input, move, resize, copy) | partial | Vbox/hbox/text frames only |
| Frame handles, Music frames | missing | |
| Graphics frames | partial | `image.cpp` item only |
| Frame constraints, Graphic slices (9 sections) | deferred (56) | Stubs |

**Library, Tokens, Properties, Comments, Proofreading**

| Section | Status | Gap |
|---|---|---|
| Library Manager, Custom instruments, Custom tonality systems | missing | |
| Layout / Notation / Note Input / Engraving / Playback Options dialogs | partial | Scattered Style and Preferences pages |
| Custom notehead sets, playing techniques, lines, page sizes, Music symbols | partial | |
| Tokens: placeholders, player / instrument / staff-label / character-style / flow tokens | missing | |
| Tokens: project, layout, music symbol, date and time | partial | |
| Properties: local vs global scope, changing scope | missing | Reset button only |
| Comments (10 sections) | missing | No review-comment model |
| Proofreading (7 sections) | missing | No engine |

**Playback, Playback setup, Mixer, Key Editor, Play mode**

| Section | Status | Gap |
|---|---|---|
| Key Editor (12 sections) | missing | Stub tab (54), deferred (57) |
| Playback templates, Expression maps | missing | Sound profiles instead (57) |
| Endpoints, Percussion maps, Playback techniques | partial | |
| Swing playback | partial | Tempo-popover swing deferred (README) |
| Independent voice playback, Transport window, MIDI trigger regions | deferred (57, 50, README) | |
| Tracks | partial | Tempo / chord rows, tools deferred (57) |
| Mixer: hide/show channels, rename, Stage templates, Space templates | missing | |
| Mixer: window, toolbar, channel height, reverb plug-in | partial | |

**Project window, Write mode, Files, Editing, Preferences**

| Section | Status | Gap |
|---|---|---|
| Signposts, System track | missing | |
| Contextual help overlay, Tab bar, Transport window | deferred (50) | |
| Instrument filters, View setup, Toolbar | partial | |
| Missing Fonts dialog, Project Statistics dialog | missing | |
| Project access on multiple devices | partial | Cloud projects only |
| Importing / exporting flows, Splitting flows, Going to flows | missing / deferred | No flows |
| Groove Agent drum patterns, Exporting tempo tracks | missing | |
| Importing tempo tracks, Exporting text | partial | MIDI import only; lyrics only |
| Attachment lines, mouse editing toggle | missing | |
| Musical transformations | partial | Parsed in the Shift-I popover, unsupported |
| Undo / Redo | partial | Selection not in history (44) |
| Jump bar aliases | partial | No Preferences UI |
| Preferences: user settings export / import | missing | |

## 3. Proposed specs (61+)

One line of scope each; each gets a full spec in its own session, per `specs/README.md`. Ordered by value and effort inside each tier. "Size" is relative: S = a few days of agent work, M = one big session, L = new engine model.

### Tier A: builds on finished specs, no new model

| # | Title | Scope | Size |
|---|---|---|---|
| 61 | [Guitar bends, techniques, harmonics, string indicators](done/61-guitar-techniques.md) | Popover entries for `bend`, `hammer`, `pull`, `tap`, vibrato bar, `harm ...`, `string N`; wire to `guitarbend.cpp`, `hammeronpulloff.cpp`, `tapping.cpp`, `harmonicmark.cpp` (28, 29 follow-up) | M |
| 62 | Print panel parity | Copies / collate, paper size, orientation, scale, page and flow ranges, duplex | M |
| 63 | Mixer parity | Channel filter bar, rename, 190 px strips, icon toolbar, reverb picker (57 polish) | S |
| 64 | Stage and Space templates | Preset pan / reverb layouts applied to all channels | S |
| 65 | [Musical transformations](done/65-musical-transformations.md) | Retrograde, inversion, rotate (already parsed in Shift-I, 35) | S |
| 66 | Swing and playback options | Swing playback, rhythmic feel in the tempo popover (26), Playback Options dialog | M |
| 67 | Tokens | Player / instrument / layout / flow / date tokens, placeholder show / hide | S |
| 68 | Project dialogs | Project Statistics, Missing Fonts, settings export / import, Jump bar alias editing | S |
| 69 | Properties scope | Local vs global property scope in the Properties panel, reset all | M |
| 70 | Pages | Insert / delete page, start on left-hand page, First page template switch | S |

### Tier B: moderate new code

| # | Title | Scope | Size |
|---|---|---|---|
| 71 | Spacing tools | Staff Spacing and Note Spacing tools in Engrave (56 deferred) | M |
| 72 | Options dialogs | Dorico-structured Engraving / Notation / Note Input / Layout Options over Format > Style | M |
| 73 | Small notation gaps | Numbered bar regions (30), coordination lines, stemlets, bar-number formats | M |
| 74 | Harp pedals | Harp Pedals panel, popover entries, Calculate Harp Pedals (`harppedaldiagram.cpp`) | S |
| 75 | Generate chord symbols | From-selection dialog and chord regions (32 follow-up) | M |
| 76 | Tempo tracks and text export | Standalone tempo-track import / export, Export Text | S |
| 77 | MIDI recording | Cmd-R record with click, count-in, requantize on top of `realtime-auto` | M |
| 78 | Library editors | Library Manager shell, custom instruments, accidentals, noteheads, playing techniques | L |
| 79 | Proofreading | Panel of checks (range, missing clef / time signature, overlaps) with ignore / filter / resolve | M |
| 80 | Comments | Review-comment model, panel and dialog, export | M |
| 81 | Graphic slices | Region-based PNG / SVG export replacing the Engrave stub (56) | M |

### Tier C: new engine model (large)

| # | Title | Scope | Size |
|---|---|---|---|
| 82 | Cues | Cue-note model, Cues panel, **Shift-U** popover (lifts the README deferral) | L |
| 83 | Condensing | Merge same-rhythm players into one staff with `a2` labels; needs 82 | L |
| 84 | Divisi | Split a staff with `div.` / `unis.` markers, Change Divisi dialog | L |
| 85 | Ossia and extra staves | Ossia over a bar range, hide when empty | L |
| 86 | Flows | Flow model, Flows panel, flow names, import / export / split / go to | L |
| 87 | Flow headings and page templates | Template sets and editor, flow heading editor, Pages panel (56 stub); needs 86 | L |
| 88 | Key Editor and automation | Piano roll in the Play lower zone: velocity, dynamics, pitch bend, CC, tempo lanes (54, 57 stubs) | L |
| 89 | Playback templates, endpoints, expression maps | Per-instrument technique mapping over sound profiles | L |
| 90 | Tonality systems | EDO / custom tonality systems, custom key panel (53 stubs) | L |
| 91 | Markers, timecodes, Video | Timecode model, Video panel | L |
| 92 | Frames | Free music and graphics frames, handles, constraints, fullness indicators, tucking index | L |
| 93 | Multi-staff entry | Multi-staff caret, Opt-I insert scopes, slash voices (README deferred list) | L |

Bundled sound content (Iconica, HALion, Olympus, Keda, effects, SuperVision) stays out of scope.
