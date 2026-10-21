<!-- markdownlint-disable -->
# Menu and context-menu audit (Dorico Pro 6 vs our build)

Captured with System Events (`dump-menubar.applescript`, run as `osascript dump-menubar.applescript "Dorico 6" 2` / `mscore 3`). Raw full dumps with shortcuts, checkmarks and disabled flags: `dorico-menubar.txt`, `ours-menubar.txt`. Both were taken in Write mode with one note selected (Dawn score, galley/page view). Shortcut format `[Cmd+X]`. NB: macOS reports Dorico's menu-item shortcuts, so Dorico single-key items show e.g. `[F]`, `[V]`.

## Part 1 - menu bar

### Top-level order
- Dorico: Dorico | File | Edit | View | Write | Play | Library | Window | Help | Script  (mode menus are per mode: only the current mode's menu is shown, so Write shows no Setup/Engrave menu; Script menu exists)
- Ours: MuseScore Studio | File | Edit | Setup | Write | Engrave | Play | Library | View | Window | Help
- Differences: ours shows all four mode menus at once (Dorico only the active one); View sits after Library in ours but before Write in Dorico; no Script menu in ours; ours App menu is "MuseScore Studio" and has extra "About Qt...".

### App menu (Dorico | ours)
Dorico: About Dorico, Preferences... Cmd+, , Services, Hide/Hide Others/Show All, Quit. Ours: same plus `About Qt...`. Same order otherwise.

### File
Dorico: New Cmd+N | New From Project Template > (Band, Chamber, Choral and Vocal, Jazz, Orchestral..., Solo) | Open... Cmd+O | Open Recent > (+ Clear Menu) | --- | Close Project Shift+Cmd+W | Save Cmd+S | Save As... Shift+Cmd+S | Revert | --- | Close All | Save All | Save As Project Template... | --- | Import > (Flows..., MIDI..., MusicXML... Shift+Cmd+O, Tempo Track...) | Export > (Flows..., Audio..., MIDI..., MusicXML..., Tempo Track..., Lyrics..., Text...) | --- | Print... | --- | Project Info... Cmd+I | Project Statistics... | --- | Export User Settings... | Import User Settings...
Ours: New... Cmd+N | Open... Cmd+O | Open recent > (every entry shows Cmd+O, cosmetic bug; ends `Clear list of recent files`) | Close Cmd+W | --- | Save, Save as..., Save a copy..., Save selection..., Save to cloud... | --- | Project properties..., Export..., `Dorico: Publish` | --- | Print... Cmd+P | ---
- Missing in ours: New From Project Template, Close Project (we have "Close" Cmd+W; Dorico Cmd+W is Close Tab, Close Project is Shift+Cmd+W), Revert, Close All, Save All, Save As Project Template, Import >, Export > submenu (ours single Export...), Project Statistics, Export/Import User Settings, Project Info shortcut Cmd+I.
- Extra in ours: Save a copy, Save selection, Save to cloud, Publish ("Dorico: Publish" label), trailing empty separator, Print has Cmd+P (Dorico Print... has no key; Print is mode Cmd+5).
- Labels: "..." vs "…" (ours uses the ellipsis char), "Open recent" vs "Open Recent", "Save as…" vs "Save As...", sentence case vs Title Case.

### Edit
Dorico: Undo Select Cmd+Z (names the action) | Redo Shift+Cmd+Z | History... | --- | Cut | Copy | Paste | Paste Special > (Paste Into Voice >, ---, Reduce, Explode, ---, Move to Staff Above Opt+N, Move to Staff Below Opt+M, ---, Duplicate to Staff Above, Duplicate to Staff Below, ---, Swap, ---, Paste Articulations) | Delete | Delete Bars | --- | Remove Rests | --- | Requantize... | --- | Select All Cmd+A | Select More Shift+Cmd+A | Select To Start Of System | Select To End Of System | Select To Start Of Flow | Select To End Of Flow | Select None Cmd+D | Filter > (Select Only ✓, Deselect Only, ---, Notes and Chords, Grace Notes, Notes by Pitch >, Voices >, Notes in Chords >, Rests, ---, All/Gradual/Immediate Dynamics, ---, All/Absolute/Relative/Gradual Tempos, ---, Lyrics >, ---, ~35 notation types alphabetical, ---, All Breaks/Frame Breaks/System Breaks, ---, Suppressed Playback) | --- | Go To > (Go To Bar... Cmd+G, Page..., Rehearsal Mark..., ---, Previous/Next Flow) | --- | Concert Pitch ✓ | Transposed Pitch | --- | Condensing | --- | Flip F | Notations > (Bar Numbers, Beaming, Chord Symbols and Diagrams, Clef, Cross Staff, Dynamics, Figured Bass, Fingering, Lyrics, Notehead, Octave Line, Pedal Lines, Percussion, Playing Techniques, Slurs, Staff Size, Staff, Stem, Text, Voices) | --- | Reset Appearance | Reset Position | Propagate Properties | --- | Device Setup... | --- | AutoFill (system)
Ours: Undo | Redo (⇧⌘Z, ⌘Y) [shortcut text in the label] | History | --- | Cut | Copy | Paste | Paste Special > (Paste half duration, Paste double duration, Swap with clipboard, ---, Paste Into Voice >, Duplicate to Staff Above/Below, Move to Staff Above/Below, Swap, Explode, Reduce, Paste Articulations) | Delete (⌦, ⌫) | Delete Bars | Remove Rests | --- | Select all | Select None | Select More | Select To > (end/beginning of system, end/beginning of score) | Select section | Filter > (Select Only, Deselect Only, ---, Notes >, Voices >, Dynamics >, Tempo >, Lyrics >, Text, Chord Symbols, Figured Bass, Slurs, Ties, Ornaments, Arpeggio Signs, Glissandi, Octave Lines, Pedal Lines, Other Lines, Tuplets, Holds and Pauses, Rehearsal Marks, Playing Techniques, Clefs) | Selection filter | Show search | --- | Reset shapes and positions | Reset text style overrides | --- | ---(double) | AutoFill
- Missing in ours: Requantize..., Go To >, Concert Pitch/Transposed Pitch, Condensing, Flip, Notations >, Reset Appearance/Position (ours has differently named Reset shapes and positions/text style overrides), Propagate Properties, Device Setup..., Select To Start/End Of Flow (ours: score), Filter entries for grace notes, rests, breaks, ~15 notation types, Undo/Redo item naming, separators around Remove Rests.
- Extra in ours: Paste half/double duration, Swap with clipboard, Select section, Selection filter, Show search, Ties and Ornaments filters, Select to end of score.
- Misordered: Paste Special order (Dorico groups Reduce/Explode, Move, Duplicate, Swap with separators; ours flat), Delete group order (Dorico Delete, Delete Bars, ---, Remove Rests), Select None after Select To (Dorico) vs after Select all (ours).
- Label: Dorico keeps shortcut out of the label; ours embeds `(⇧⌘Z, ⌘Y)` and `(⌦, ⌫)` in label text.

### View
Dorico: Page View Opt+Cmd+1 | Galley View Opt+Cmd+2 ✓ | Fill View Opt+Cmd+3 | --- | Pages > (Spreads/Single Pages Horizontally/Vertically) | --- | System Track Opt+T ✓ | Instrument Filter Overlay ✓ | Cycle Locators | --- | Signposts > (Hide Signposts, ~30 types with checks, Set All Shown/Hidden) | Note And Rest Colors > | Bar Numbers > | Page Numbers | Attachment Lines | --- | Comments, Cutaway Staff Lines, Token Placeholders | --- | Highlight Bar Repeat/Chord Symbol/MIDI Trigger Regions, Cues, Slash Regions | Zoom > (Zoom In Cmd+=, Zoom Out Cmd+-, 25%..1600%, Whole Page Opt+Cmd+0, Two Pages, Page Width, Page Height, Custom Zoom... Opt+Z) | Full Screen Ctrl+Cmd+F | --- | Marquee Select Tool ✓ | Hand Tool | --- | View Options... Shift+Cmd+V | --- | Enter Full Screen
Ours: Full screen | --- | Show > (Show invisible ✓, Show formatting ✓, Show frames ✓, Show page margins, Mark irregular measures ✓, Show sound flags ✓) | --- | Navigator | Braille | Enter Full Screen
- Missing in ours: everything view-related of Dorico: Page/Galley/Fill View (in ours these are the bottom-bar Page/Galley/Fill buttons only, no menu items, no keys), Pages, System Track, Signposts, Note And Rest Colors, Bar Numbers, Zoom submenu, Marquee/Hand tool, View Options... Shift+Cmd+V, Highlight regions. Ours has Navigator, Braille, Show invisible/formatting/frames/margins (Dorico equivalents live in View Options).

### Write (Dorico Write menu | ours Write menu; ours also has Setup/Engrave menus)
Dorico: Instrument Visibility > | --- | Note Input Shift+N | Insert I | Insert Scope > (Voice ✓, Player, Global, Global Adjustment Of Current Bar, ---, Set Edit Stop Position) | Input Pitch > (Written ✓/Sounding) | --- | Rhythmic Grid > (note values, Decrease/Increase Resolution Opt+[ / Opt+]) | Respell > (Name Above Opt+=, Below Opt+-, ---, Propagate/Reset/Respell Automatically) | --- | Create Voice Shift+V | Create Slash Voice | Next Voice V | --- | Transpose... | Add Notes Above or Below... | Add Intervals Shift+I | Transform > (Pitches/Rhythm/All) | --- | Create Bar or Barline Shift+B, Chord Symbol Shift+Q, Clef Shift+C, Cue Shift+U, Dynamic Shift+D, Figured Bass Shift+G, Fingering Shift+F, Hold or Pause Shift+H, Key Signature Shift+K, Lyrics Shift+L, Marker Opt+Shift+M, MIDI Trigger Region, Ornament Shift+O, Playing Technique Shift+P, Rehearsal Mark Shift+A, Repeat Shift+R, Rhythmic Feel, System Text > (Opt+Shift+X default), Tempo Shift+T, Time Signature Shift+M, Tuplet ;, Text > (Shift+X default; ~30 paragraph styles)
Ours (Write): `Dorico: Write mode` (disabled, ✓) | Note input Shift+N | --- | Notes > (Note input, C..B, Add C..B to chord) | Intervals > (Unison..Tenth above/below) | Tuplets > | Measures > | Text > (Title, Subtitle, Composer, Lyricist, Part name, System text Opt+Shift+X, Staff text Shift+X, Dynamic, Expression text, Rehearsal mark Shift+A, Instrument change, Fingering, Sticking, Chord symbol, Roman numeral, Nashville, Lyrics, Figured bass, Tempo marking) | Lines > (Slur S, Crescendo Shift+, Diminuendo Shift+., Ottava, Note-anchored line) | Chords and fretboard diagrams > | --- | Transpose... | Respell > (sharps/flats, Enharmonic spelling >) | Transform > | Voices > (Exchange voice 1-2 ...) | Measures > (duplicate of the earlier Measures) | --- | Implode, Realize chord symbols, Fill with slashes, Toggle rhythmic slash notation, Regroup rhythms, Resequence rehearsal marks, Copy lyrics to clipboard, Remove empty trailing measures
- Structure is entirely different. Ours is the stock MuseScore Add/Notes menu with the mode-name items prefixed "Dorico:" (disabled in Write? they are shown disabled and with checkmark). Missing in ours: all `Create X` items with Dorico keys (Shift+B/Q/C/U/D/G/F/H/K/L/O/P/R/T/M/;), Insert I, Insert Scope, Input Pitch, Rhythmic Grid, Create Voice/Slash Voice/Next Voice, Add Notes Above or Below, Add Intervals Shift+I, Instrument Visibility, Respell by name above/below. Duplicate "Measures >" appears twice in our Write menu (stock upstream bug/duplication). Extra: Implode, Realize chord symbols, Fill with slashes, Regroup rhythms, Resequence rehearsal marks, Voices > exchange.
- Ours menus hold items disabled in Write mode with a note selected (e.g. every Notes > item reads disabled), whereas Dorico's are enabled.

### Setup / Engrave (ours only; Dorico only shows the menu of its current mode, captured Write mode, so Setup/Engrave menus not captured)
Ours Setup: `Dorico: Setup mode` Cmd+1, ---, Add/remove instruments..., Parts... . Ours Engrave: `Dorico: Engrave mode` Cmd+3, ---, Page settings... Shift+Cmd+L, Measures per system..., Stretch >, Frames >, ---, Reset beams, Reset entire score to default layout. (Dorico Layout Options... is Shift+Cmd+L in its Library menu.) Dorico Setup/Engrave menu contents: see Part 3 below if captured.

### Play
Dorico: Record Cmd+R | Retrospective Record Opt+Cmd+R | Fixed Tempo Mode | --- | Play From Playhead Position Space | Play From Selection P | Play From Start Of Flow | Play From Start Of Project | Play From Last Start Position Shift+Space | --- | Cycle | Locators > | --- | Scrub Playback | --- | Solo Selected Instruments Opt+S | Deactivate All Solo States | Deactivate All Mute States | --- | Key Editor Grid Resolution +/- | --- | Load Sounds For Unassigned Instruments | Playback/Stage/Space Template... | --- | Automation > | --- | Reset Playback Overrides | Apply Default Output Level | --- | Tracks >
Ours: `Dorico: Play mode` Cmd+4 | --- | Play | Rewind . | Loop toggle | Metronome toggle | --- | Playback setup | Mixer | Reload playback cache
- Almost nothing in common: ours lacks Record, Retrospective Record, play-from variants, cycle/locators, scrub, solo/mute keys, templates, automation, tracks. (Play/Rewind have no key shown in ours; Space not listed.)

### Library
Dorico: Music Fonts..., Font Styles..., Paragraph Styles..., Character Styles... | Page Sizes... | Chord Diagrams..., Chord Symbols..., Chord Symbol Kerning Pairs..., Music Symbols..., Notehead Sets..., Playing Techniques... | Lines..., Line Bodies..., Line Annotations..., Repeatable Symbols... | Expression Maps..., Percussion Maps..., Playback Techniques... | Engraving Options... Shift+Cmd+E, Layout Options... Shift+Cmd+L, Notation Options... Shift+Cmd+N, Note Input Options... Shift+Cmd+I, Playback Options... Shift+Cmd+P | Instruments..., Instrument Families..., Instrument Score Orders... | Export Library..., Library Manager...
Ours: Style... (⇧⌘N, ⇧⌘E) | Load style... | Save style... | --- | Master palette... | Palettes
- Ours has the single "Style..." item (shortcuts embedded in label) in place of 5 Options dialogs; none of the other 25 Dorico entries exist.

### Window
Dorico: Minimize Cmd+M, Zoom, Bring All To Front | --- | New Window Shift+Cmd+T, Mixer, Transport, Video, Hub | --- | Setup Cmd+1, Write Cmd+2 ✓, Engrave Cmd+3, Play Cmd+4, Print Cmd+5 | --- | Previous Mode | --- | Show Tabs | --- | Close Tab Cmd+W, New Tab Cmd+T, Next Tab Ctrl+Tab, Move Tab To New Window, Previous Tab, Close Other Tabs, Merge All Windows | --- | No Split / Vertical Split / Horizontal Split | --- | Show Toolbar Cmd+6 ✓, Show Left Zone Cmd+7 ✓, Show Lower Zone Cmd+8, Show Right Zone Cmd+9, Hide/Restore Zones Cmd+0 | --- | Next Layout Opt+Shift+], Previous Layout Opt+Shift+[, Counterpart Layout W | --- | open-project list (audit ✓)
Ours: `Dorico: Hub` | --- | `Dorico: Setup mode` Cmd+1 ... `Dorico: Print mode` Cmd+5 (Write ✓) | --- | `Dorico: toggle toolbar` Cmd+6, left zone Cmd+7, right Cmd+9, lower Cmd+8, hide/restore zones | --- | Mixer, Properties, Timeline, Piano keyboard, Percussion | --- | Workspaces > (Default ✓, Edit workspaces..., Create new workspace) | Restore default layout | --- | Extensions > | More MuseScore Commands >
- Missing in ours: Minimize/Zoom/Bring All To Front, New Window, Previous Mode, Show Tabs and tab commands, Split, Next/Previous/Counterpart Layout, project list, Transport/Video. Labels use lowercase "Dorico: toggle ..." and the "Dorico:" prefix on mode names; Dorico plain "Setup". Our mode items are disabled while a score is open. Mode-key order Cmd+6/7/9/8 in ours differs (zone order) from Dorico 6/7/8/9. Ours has "Hide/Restore Zones" with no key (Dorico Cmd+0).

### Help
Dorico: Show Help Cmd+/ | --- | Dorico Help, Guided Tour, Key Commands | --- | Support >, Community > | --- | Create Diagnostic Report, Check For Updates
Ours: Online handbook | Ask for help | Check for update | --- | About MusicXML..., Accessibility statement | --- | Clear online sounds cache | Revert to factory settings | Diagnostics > (...)
- Missing: Show Help Cmd+/, Key Commands, Guided Tour. Ours has no Script menu (Dorico Script: Run Script..., Run Last Script, macro recording, Console, Attach Debugger).

## Part 2 - score context menus (right-click)
Dorico's context menu is custom-drawn (not in the macOS accessibility tree), so it was transcribed from screenshots. Submenus open on click (hover via synthetic move does not open them). Shots: `dorico-ctx-note.png`, `ours-ctx-note.png`.

### (a) Note - Dorico Write mode
```
Cut  Cmd+X
Copy  Cmd+C
Paste Special >                      (no Paste row shown when clipboard empty)
---
Filter >
---
Bar Numbers >
Beaming >
Chord Symbols and Diagrams >
Fingering >
Notehead >
Octave Line >
Pedal Lines >
Staff Size >
Staff >
Stem >
Voices >
```
- Paste Special >: Paste Into Voice > | --- | Move to Staff Above Opt+N | Move to Staff Below Opt+M | --- | Duplicate to Staff Above | Duplicate to Staff Below | --- | Swap  (no Reduce/Explode/Paste Articulations here; Edit menu has them disabled)
- Filter >: Select Only (check) | Deselect Only | --- | Notes and Chords | Grace Notes | Notes by Pitch > | Voices > | Notes in Chords > | --- | Suppressed Playback
- Notehead >: Circled Noteheads | Default Noteheads | Large Circled Noteheads | Larger Noteheads | Slashed Noteheads (Bottom Left to Top Right) | Slashed Noteheads (Top Left to Bottom Right) | --- | Crosses > | Triangular > | Diamonds > | Slashes > | Round and Square > | --- | Scale Degree > | Pitched > | --- | Reset to Default Notehead | --- | Toggle Round Brackets | Toggle Square Brackets
- Voices >: Default Stems Up | Default Stems Down | --- | Rhythmic Slashes > | --- | Swap Voice Contents | Change Voice > | --- | Swap Voice Order
- Stem >: Force Stem Up | Force Stem Down | Remove Forced Stem | --- | Force Centered Beam | Custom Centered Beam... | Remove Centered Beam
- Beaming >: Beam Together | Split Beam | Make Unbeamed | Split Secondary Beam | Set Partial Beam Direction > | Create Fanned Beam > | Stemlets > | Reset Beaming
- Staff >: Add Staff Above | Add Staff Below | --- | Create Ossia Above | Create Ossia Below | --- | Change Divisi... | Restore Unison | --- | Create Manual Cutaway | Create Start Cutaway | Create End Cutaway | --- | Remove Staff | --- | Manual Staff Visibility...
- Bar Numbers >: Add Bar Number Change...
- Not expanded (same style): Chord Symbols and Diagrams, Fingering, Octave Line, Pedal Lines, Staff Size.
- Style (dorico-ctx-note.png): rounded ~6px corners with thin lighter border and drop shadow, dark grey ~#2d2d2d background (translucent), white-ish 12-13px system font (SF), row height ~18px (screen px, 2x display so ~9pt rows), separators 1px mid-grey full width with ~5px vertical padding, submenu arrow = right-pointing filled triangle at the right edge, check = "✓" in a left gutter (Filter), highlight = macOS accent blue (#3b78e7-ish) full-width rounded, shortcut right-aligned with symbols (⌘X, ⌥N).

### (a) Note - ours (Write)
```
Cut  Cmd+X            (scissors icon, accelerator letters underlined)
Copy  Cmd+C
Paste  Cmd+V
Swap with clipboard
Delete  (backspace/delete glyphs)
Select >
---
Edit element
---
Style...  Shift+Cmd+N, Shift+Cmd+E
```
- Select > submenu could not be opened with mouse or Right-arrow in this automation (not transcribed; stock MuseScore: More..., Similar, Similar in same staff, In this measure...).
- Dorico lacks Paste/Swap/Delete/Edit element/Style rows; ours lacks Filter, Paste Special, Bar Numbers, Beaming, Chord Symbols and Diagrams, Fingering, Notehead, Octave Line, Pedal Lines, Staff Size, Staff, Stem, Voices. Dorico shows Paste only when the clipboard is non-empty (not tested with clipboard) and has no Delete.
- Dorico Notehead/Voices/Stem/Beaming/Staff submenus above have no equivalent in ours.

### Other cases (Dorico | ours)
Selection note for Dorico: right-click does NOT change the selection in Dorico; the menu is built from the selection. So items were left-clicked first, then right-clicked. In ours right-click selects the item under the cursor.
- (b) rest. Dorico (rest selected, rest + adjacent selection leaked in): Cut, Copy, Paste Special >, --- Filter >, --- Edit Font..., --- Bar Numbers, Beaming, Chord Symbols and Diagrams, Dynamics, Fingering, Notehead, Octave Line, Pedal Lines, Staff Size, Staff, Stem... (the Edit Font and Dynamics rows appear because the selection included a dynamic/text item; caveat: selection was not purely a rest). Ours: same 8-row menu as note (Cut, Copy, Paste, Swap with clipboard, Delete, Select >, Edit element, Style...).
- (c) empty space in a bar. Dorico (clicked empty harp staff in a bar): only `Filter >` | --- | `Bar Numbers >`, `Staff Size >`. Ours (right-click inside staff gap): measure menu: Cut, Copy, Paste, Swap with clipboard, --- Clear measures (Del), Delete measures, --- Instrument / Staff properties..., --- Insert measures >, Move measures >, Create system from selection, --- Measure properties... Right-click on blank between systems in ours gives the page menu (see h).
- (d) dynamic. Dorico: Cut, Copy, Paste Special >, --- Filter >, --- Edit Font..., --- Bar Numbers >, Dynamics >, Staff Size >, Staff >. Ours: same as note menu (Cut..Style).
- (e) slur. Dorico: Cut, Copy, Paste Special >, Filter > (top rows) | Bar Numbers, Beaming, Chord Symbols and Diagrams, Fingering, Notehead, Octave Line, Pedal Lines, Slurs >, Staff Size, Staff, Stem, Voices (menu opened upward; top rows partly scrolled off, assumed same as note). Ours: not captured; my right-click on a tie arc missed it and returned the page menu (Style..., Page settings..., Load style...); likely the note menu as for dynamics.
- (f) multi-bar range (4 flute bars selected). Dorico: Cut, Copy, Paste Special >, Filter >, Edit Font..., Bar Numbers, Beaming, Chord Symbols and Diagrams, Dynamics, Fingering, Notehead, Octave Line, Pedal Lines, Staff Size, Staff, Stem, Voices. Ours (rest + shift-click note, right-click in range): measure menu as (c): Cut, Copy, Paste, Swap with clipboard, Clear measures, Delete measures, Instrument / Staff properties..., Insert measures >, Move measures >, Create system from selection, Measure properties...
- (g) tempo text. Dorico (tempo mark): Cut, Copy, Paste Special >, Filter >, Edit Font..., Bar Numbers >, Staff Size >, Staff >. Ours (tempo "Adagio" and title text): same note menu (Cut .. Style...). Neither has a text-specific menu except Dorico's Edit Font.
- (h) outside the staves. Dorico: dark area outside the page = no menu; inside page blank (Engrave): `Filter >`, `Bar Numbers >`, `Staff Size >` (same as (c)). Ours: page menu: `Style... Shift+Cmd+N, Shift+Cmd+E` | `Page settings... Shift+Cmd+L` | `Load style...` (3 rows).

### Engrave mode (Cmd-3)
- Dorico: right-click at (a) (my click missed the note, so empty selection) and (h) blank page area: Filter >, ---, Bar Numbers >, Staff Size >. Same 3-row menu as Write empty-space. (d) not captured: no dynamic visible on the page in view.
- Ours: dynamic: note menu (Cut .. Style...), identical to Write; note click missed -> page menu (Style..., Page settings..., Load style...); blank (h): page menu. Ours has no Engrave-specific context items.

### Setup mode (Cmd-1)
- Dorico player (Glockenspiel): Soloist | --- | Rename Player... | Add Instrument to Player... (Shift+I-ish glyph shown) | Add Player to Group > | --- | Flows > | Layouts > | --- | Duplicate Player | Delete Player | --- | Chord Symbols > | Chord Diagrams > | --- | Combine Instruments Into Kit | Create Empty Kit | Import Kit...
- Dorico flow (Flow 1): Rename | Notation Options... Shift+Cmd+N | Project Info... Cmd+I | --- | Layouts > | Players > | --- | Video > | --- | Duplicate Flow | Delete Flow
- Dorico layout (Full score): Rename | Renumber Layouts | Layout Options... Shift+Cmd+L | Propagate Part Formatting... | Clef and Transposition Overrides... | Flow Heading Overrides... | --- | Flows > | Players > | --- | Delete Layout | Duplicate Layout
- Ours player (Oboes 1 & 2): Hide | Settings... | --- | Remove. Ours layout rows and flow row: right-click showed NO menu (hover tooltip only for layout). Ours player names show a literal `&amp;` ("Oboes 1 &amp; 2") in the Players list in Setup mode (HTML escaping bug in Setup panel; the Layouts list shows `&`).
- Workspace tab right-click: Dorico has no tab strip visible (single window, tabs hidden); not tested. Ours has no tabs.

### Style table (dorico-ctx-note.png vs ours-ctx-note.png; sampled by eye, no pixel tool available)
| | Dorico | Ours |
|---|---|---|
| background | dark grey ~#2e2e2e, 1px lighter border, ~6px rounded corners, shadow | dark blue-grey ~#2b3038, 1px border, ~6px corners |
| font | system font (SF) ~13px, white-ish | Inter-like ~13px, light grey |
| row height | ~18px (tight; groups separated by thin lines) | ~32px (spacious) |
| icons | none | left icons for Cut/Copy/Paste/Delete |
| accelerator underline | no | yes (Cut, Copy, Paste, Delete, Style, Page settings) |
| shortcut column | right-aligned symbols (cmd glyph + letter) | right-aligned glyphs, comma-separated alternates |
| separator | thin 1px mid-grey line with ~4px padding | thin 1px line, ~8px padding |
| checkmark | check glyph in left gutter (Filter > Select Only) | n/a in captured menus |
| submenu arrow | small filled triangle at right edge | small filled triangle at right edge |
| highlight | macOS accent blue, full-width rounded | desaturated blue (#2b5278-ish), full-width |
| submenu open | on click (second click needed after another submenu is open) | on hover/click (could not open Select by synthetic events) |

## Raw data
`dorico-menubar.txt`, `ours-menubar.txt` (menu bar, 2-3 levels), `dump-menubar.applescript` (re-run: `osascript dump-menubar.applescript "Dorico 6" 2`). Shots: `dorico-ctx-note.png`, `ours-ctx-note.png`.
