<!-- markdownlint-disable -->
# Modes audit (Cmd-1..5): Dorico Pro 6 vs our build

Dorico data are from a Dorico PRO launch (first run was SE, discarded and recaptured; Pro licence consumed in application.log, Engrave tab present).
Window sizes (points): Dorico 2924x1225 (title bar incl.), ours 3440x1409. Shots are 1 px = 1 pt. Different scores open (Dorico: a temp copy of a user project, ours: "Dawn"), so score content differs; compare chrome only.
Files: `dorico-<n>-<mode>.png`, `ours-<n>-<mode>.png` (full), crops `*-write-{toolbar,lefttoolbox,righttoolbox,bottom}.png`, `*-setup-left.png`, `*-print-right.png`, `dorico-write-toolbar-right.png`.

## Mode switch timing
Both apps switch within one screenshot latency (~0.3-0.5 s); the "immediately after key" shot already equals the final one (RMSE of downscaled diff 1-4% in both, same noise floor). No visible lag in either. (Our first Cmd-3 etc. not measured with a finer clock.)

## Global chrome
| Region | Dorico | Ours | Gap |
|---|---|---|---|
| Mode tabs | 4+Engrave = 5 tabs (Setup Write Engrave Play Print), group x=8..229 (SE-less Pro: wider), tab bg #36414D, active #295B82, active Write tab 56 wide, height ~29 (y 36..~65) | 5 tabs, group x=12..274, tab bg #3D4A58, active #2C678F, active tab ~52 wide, height ~16-18 | ours tabs smaller and lighter bg; active blue brighter/greener; tab strip not visually grouped in a dark box |
| Toolbar bg | #272D33 | #1F2429 (same as panels) | Dorico toolbar one step lighter than panels; ours blends in |
| Toolbar height (below title bar) | ~37 (title bar 0-28 #222222) | ~28 (title bar 0-32 #1D2024) | ours shorter, title bar taller |
| Toolbar contents | layout combo "Full score", eye, window icon; right: video, mixer, power (blue), transport readout 1.1.1.000, rewind, record-ish, play (green), record (red), metronome, tempo/click, volume, 3-dot, undo, redo, help | combo "Score", eye, window; mixer, ...; play, loop, metronome, time 0:00:0, 1.1, tempo, then undo/redo/help at far right (disabled) | ours lacks power/audio-engine button, red record, volume, 3-dot; transport grouped differently/not in dark pill; combo label "Score" vs "Full score" |
| Status bar (bottom) | Left: note-input dropdown, "Concert Pitch | Transposed Pitch" pill buttons, "No selection"; right: Follow playhead, view mode icons, Page/Galley/Fill view, zoom (362%) | Left: "Grid: 1/4" + Concert/Transposed (wide, 130 px each, lower zone); right: page icons, Page/Galley/Fill, zoom 100% | ours Concert/Transposed buttons much wider, placed in a split bar; no "No selection" text; no Follow playhead toggle; Dorico bar has mid collapse chevron at bottom centre (lower-zone toggle) |
| Zone collapse tabs | small `<` `>` tabs at left/right window edges (Setup mode) and chevron bottom centre | none visible (Setup shows no edge tabs) | missing collapse tabs |
| Music area bg | blue gradient #09203F (dark navy) around white page; grey #A0A0A4 in Print | flat #2C4D6F strip around pages; grey in Print | Dorico darker, gradient |
| Panel/strip bg | toolbox strip #272D33, flyout panel #1F2429, panel #1F2429 | strip == panel == #1F2429 | strip lacks separate shade |

## Write mode
| Region | Dorico | Ours | Gap |
|---|---|---|---|
| Left toolbox strip | x=1..44 wide (#272D33), 7ish tool icons pitch ~38 (panel toggle, select, note input, ...), icons #DBE0E6 | x=1..46, icons pitch ~41, dimmer icons (#6F7479 grey), strip same colour as panel | icon colour brighter in Dorico; strip shade |
| Left flyout (note durations) | x=46..137 (~92 wide), 2 columns, big note glyphs, up/down chevrons | x=47..~188 (~140 wide), 2 columns, note glyphs, chevrons | ours flyout ~50% wider, glyph spacing larger |
| Right toolbox | 1 column of ~20 icons, active (palette) rounded square #?blue, x~2884..2924 | similar column of ~19 icons, active blue rounded square | close; Dorico keyboard icon, ours missing the second dark-framed active-state style; icon sizes similar |
| Music area | page centred, vertical scrollbar at right, small filter bar top-left (Pro) | pages in horizontal multi-page view, no scrollbar | different default view |
| Lower zone | none by default (mixer closed) | mixer open by default in Write (13 strips) | ours opens mixer lower zone; Dorico hides it |

## Setup mode
| Region | Dorico | Ours | Gap |
|---|---|---|---|
| Left panel | "Players" with 6 players; rows with chevron expand arrow, person icon, name; selected row #40ACFF (bright azure) with "+" button; unselected #36414D; panel bg #1F2429; panel has left rail #272D33 | "Players" with "Tempos" + 12 players; no expand chevron; unselected rows #3D4A58; selected: none shown; **names show `&amp;` (e.g. "Flutes 1 &amp; 2") - escaping bug**; "..." menu top right | missing chevron, "+" and bright selection; HTML-escape bug; extra "Tempos" row |
| Right panel | "Layouts": Full score, Flute, many "Empty part" rows with checkbox on the right, expand chevrons, scrollbar | "Layouts": Full score + one per player, icon on left, no checkboxes | no checkbox column, no empty-part placeholders |
| Bottom panels | Flows panel with blue flow card ("Flow 1" + count) and side buttons; below left: Concert/Transposed + "Order: Orchestral" | Flows panel similar blue card; bottom bar "Order" label | close; Dorico has "Order:" dropdown, ours only label; Dorico adds Concert/Transposed pills here |
| Bottom-left icon row | 7 small action icons (add player etc.) and trash | similar 7 icons | close |
| Central area | score in Galley view | score pages | n/a |

## Engrave mode (ours-3-engrave / dorico-3-engrave)
| Region | Dorico | Ours | Gap |
|---|---|---|---|
| Left toolbox | 5 icons in strip (panel toggle, system/frame tools, ...) | 5 icons (panel toggle, tools) | similar count |
| Left panel | Sections: Move Bars Between Systems, Format Systems, Format Music Frames, Lock Layout, Coordination Lines, Bracketing (grids of bracket icons, two rows); buttons with icons, outlined dark | Same section list and order; Bracketing grid is a single row of many cells; extra "Change Baritone Joins" button | very close structurally; Dorico buttons taller (~23 vs ~14), bracketing grid 2 rows vs 1 row |
| Right panel | Pages (thumbnail "1" + toolbar row), Page Templates (Current set dropdown, Default/First thumbnails, toolbar), Flow Headings, Page Template Sets (collapsed) | Pages ("Not available yet"), Page Templates ("Not available yet"), Flow Headings, Page Template Sets | ours panels are stubs |
| Central | Galley/Page view of score with frame overlay (green frame borders) | pages, no frame overlay | no frame edit overlay |
| Lower zone | none (status bar only) | mixer open | ours shows mixer |

## Play mode
| Region | Dorico | Ours | Gap |
|---|---|---|---|
| Central | track view: ruler bars 1-8, Tempo and Chords tracks, player track headers (coloured edge, name, m/s), note-event blocks showing mini piano-roll dashes | track view: ruler 1-19, no Tempo/Chords tracks, player rows with m/s, solid colour blocks with no note dashes | missing Tempo/Chords lanes, no note-content preview in blocks, header strip narrower |
| Left panel | Flow dropdown, Track Inspector / VST and MIDI tabs, Routing (instrument "02 - HALion Sonic", Port/Ch, Ex. map, "Enable Independent Voice Playback"), Insert Effects, Channel | Same structure: Flow, Track Inspector / VST and MIDI, Routing (MS Basic), Insert Effects, Channel; Flow field is disabled | structurally matching; ours Flow disabled |
| Track tools | row above headers: icons (sort, metronome-toggle, ...) | none | missing track toolbar |
| Lower zone | mixer (strips with pan, level, name pill coloured; only players used, Reverb, Output); Show: Instruments/MIDI/Video/FX/Unused, Deactivate all, FX buttons | mixer with Inserts/EQ/Sends sections per strip, 13 strips, Aux 1, Output; same Show row | ours strips much taller/busier (inserts/sends always expanded); Dorico strips compact |
| Zoom | slider bottom of track view | slider bottom-left of track view | ok |

## Print mode
| Region | Dorico | Ours | Gap |
|---|---|---|---|
| Left panel | Layouts list (Full score, Flute, many Empty part), expand chevrons, scrollbar; "System Dialogs:" with Page Setup... / Print... buttons at bottom | Layouts list with icons (Full score selected), "System Dialogs:" + same two buttons | list items differ (no chevrons); selection colour ours #40ACFF-like vs Dorico #40ACFF (match) |
| Right panel | Destination (Printer/Graphics tabs, printer dropdown, Copies, Collate checkbox ticked), Job Type, Print range (All/Pages/Flows), Page Setup (Letter/ANSI A, orientation icons, "Print odd final page on", Fit to Paper/Custom Scale, Scale factor), Duplex Printing, Annotations, Print button | Same sections and order; Page Setup shows text "210 x 297 mm, portrait, 4.0 pt", "Layout page size" dropdown, Portrait/Landscape text buttons, Fit to Paper/Custom Scale, Page Setup... button | Dorico orientation = icons, ours text buttons; ours lacks "Print odd final page"/Scale factor; ours Collate unchecked |
| Central | grey #A0A0A4 bg, single white page with drop shadow, small preview | grey bg with multi-page spread, large | Dorico shows one page |
| Status bar | Page Width / Whole Page toggles | same | match |

## Top gaps (ranked)
1. `&amp;` appears literally in player names in Setup mode (bug).
2. Mixer lower zone is open in Write/Engrave/Play with fully expanded strips; Dorico shows no lower zone in Write/Engrave and compact strips in Play.
3. Toolbar: shorter/narrower tabs, wrong bg shades (#1F2429 vs #272D33, tabs #3D4A58 vs #36414D, active #2C678F vs #295B82), missing power/record/volume/3-dot buttons, combo says "Score".
4. Play mode lacks Tempo/Chords tracks and note-preview in event blocks.
5. Setup: players lack expand chevron, "+" and bright selection; Layouts lack checkboxes and empty-part rows.
6. Engrave right panels (Pages, Page Templates) are "Not available yet" stubs.
7. Status bar: Concert/Transposed buttons oversized, no "No selection"/Follow playhead; no edge collapse tabs.
8. Music area: flat blue vs dark gradient; write-mode flyout ~50% wider.
