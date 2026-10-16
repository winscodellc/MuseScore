<!-- markdownlint-disable -->
# Dorico Pro 6.2 reference: key commands

Reference extracted from Dorico Pro 6.2 manual (paraphrased facts: key bindings and syntax tokens). macOS first.

## Sources and how they were collected

The steinberg.help pages are built with JavaScript. The research used the site's public content API (`https://www.steinberg.help/api/khub/maps/<mapId>/topics/<topicId>/content`) to pull article HTML for:
- **Dorico Pro Help 6.2** (map `BYbh2hvBDwUCjSijIhHcqA`): all 2,384 topics were fetched and every key-command sentence extracted.
- **Dorico Popover Reference 6.2** (map `iuRC2znMppy_r3SiZ6eo_w`): a separate manual, the source for popover syntax (see `dorico-popovers.md`).

Section sources use two URL prefixes. Join the prefix to the path given:
- **[M]** = `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/`
- **[P]** = `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/`

---

## A0. Conventions and keyboard layout

**Notation.** "Ctrl/Cmd-Alt/Opt-Down" means Ctrl-Alt-Down on Windows and Cmd-Opt-Down on macOS. When the keys differ entirely between systems, the Windows key comes first: "# (Windows) or \ (macOS)".
Source: [M]`introduction/key_command_conventions_c.html`

**Keyboard layout dependence.**
- "The default key commands in Dorico depend on your keyboard language setting, which follows your application language setting by default."
- You can change it in Preferences (Cmd-,) > Key Commands > **Keyboard language** menu. "Default" follows the application language. The change applies to the current project and all future projects.
- The English documentation lists key commands for the English keyboard language.
- Sources: [M]`program_concepts/program_concepts_key_commands_in_dorico_r.html`, [M]`user_interface/user_interface_keyboard_layout_changing_t.html`

**No full default table exists in the manual.**
- The complete list is only available through Help > Key Commands, or Preferences > Key Commands > Print Summary. Both open an "Interactive Dorico key commands map" in a browser.
- That map is generated locally by the running app. It shows a virtual keyboard, highlights keys by modifier, lists commands in global and mode-specific groups, and has a search box.
- The tables below are therefore built from every key command the manual mentions.
- Source: [M]`user_interface/user_interface_key_commands_interactive_map_r.html`

**Design grammar, quoted from the manual.** (Source: [M]`program_concepts/program_concepts_key_commands_in_dorico_r.html`)
- Arrows alone move the selection.
- **Alt/Opt** modifies selected items:
  - Write mode, Up/Down: transpose by staff position.
  - Write mode, Left/Right: move rhythmically.
  - Engrave mode: nudge graphically.
- **Shift-Alt/Opt** changes duration in Write mode. Shift-Alt-Right lengthens by the rhythmic grid.
- **Ctrl/Cmd** increases the amount:
  - Cmd-Alt-Up/Down transposes by an octave.
  - Cmd-Shift-Alt-Right doubles the duration.
- **Shift + letter** opens popovers in Write mode.
- **Ctrl/Cmd-Shift + letter** opens options dialogs.

---

## A1. Global, window, modes and views

| macOS (Windows) | Action | Source [M] |
|---|---|---|
| Cmd-1 / 2 / 3 / 4 / 5 | Setup / Write / Engrave / Play / Print mode | `user_interface/user_interface_modes_switching_between_t.html` |
| Cmd-Opt-1 / 2 / 3 | Page view / Galley view / Fill view | `user_interface/user_interface_view_type_changing_t.html` |
| Cmd-7 | Hide/show left zone (Notes panel) | `user_interface/user_interface_zones_hiding_showing_t.html` |
| Cmd-9 | Hide/show right zone (Notations panels) | same |
| Cmd-8 | Hide/show lower zone (Properties panel lives here) | same; `properties/properties_panel_r.html` |
| Cmd-0 | Hide/restore all zones | same |
| Opt-8 | Focus the Properties panel Search field | `properties/properties_panel_r.html` |
| Cmd-6 | Toolbar (hide/show) | Toolbar topic |
| Cmd-= or Z | Zoom in | `write_mode/write_mode_navigation/write_mode_zooming_in_out_t.html` |
| Cmd-- or X | Zoom out | same |
| Cmd-scroll | Zoom | same |
| Shift-Opt-] / Shift-Opt-[ | Next / previous layout | `user_interface/user_interface_layouts_switching_between_t.html` |
| W | Counterpart layout (switch between part and score) | same |
| Cmd-T / Cmd-W | New tab / close tab | Opening new tabs; Closing tabs |
| Ctrl-Tab / Ctrl-Shift-Tab | Cycle tabs | Switching between tabs |
| Cmd-Shift-T | New project window | |
| Cmd-N / Cmd-I | New project / Project Info | |
| Cmd-, | Preferences | |
| Cmd-Shift-L / N / E / P / I / V | Layout / Notation / Engraving / Playback / Note Input / View Options | Options dialog topics |
| Cmd-/ (Windows: F1) | Contextual help overlay | |
| J | Jump bar (Setup, Write, Engrave, Play) | `user_interface/user_interface_jump_bar_r.html` |
| Ctrl-1 (Windows: Alt-C) | Jump bar: Commands mode | same |
| Ctrl-2 (Windows: Alt-G) | Jump bar: Go To mode | same |
| Cmd-G | Go To Bar dialog | `write_mode/write_mode_navigation/write_mode_navigation_go_to_bar_t.html` |
| Cmd-Z / Cmd-Shift-Z or Cmd-Y | Undo / Redo | `write_mode/write_mode_editing/write_mode_undo_redo_t.html` |
| Opt-T | Hide/show system track | `write_mode/write_mode_selecting/write_mode_system_track_hiding_showing_t.html` |
| Hold ` (Windows: hold \\) | Temporarily hide non-printing elements | `write_mode/write_mode_visual_elements/write_mode_non_printing_elements_hiding_t.html` |
| Opt-H | Switch click-drag tools (status bar) | Status bar topic |
| Opt-C / Opt-R | Add comment / reply to comment | Comments topics |
| Shift-Opt-M | Add Marker dialog | Inputting markers/timecodes |
| F2 / F3 / F4 | Transport window / Mixer window / Video window | `mixer/mixer_window_r.html` and related topics |
| Home / End, Page Up / Page Down | Previous / next page (End and Home for horizontal arrangements, Page Down and Page Up for vertical) | `write_mode/write_mode_navigation/write_mode_navigation_dragging_pages_t.html` |
| Cmd-Home / Cmd-End | First / last page | same |

**Setup-mode letters.** These reuse the same letters as Write-mode popovers, but do different things:
- Shift-I adds an instrument (instrument picker).
- Shift-P adds a single player. Shift-Opt-P adds a section player.
- Shift-E opens the ensemble picker.
- Shift-F adds a flow.
- Shift-L adds a custom score layout.

---

## A2. Starting and stopping note input; the caret

Sources: [M]`write_mode/write_mode_caret/write_mode_caret_activating_deactivating_t.html`, `..._caret_moving_manually_t.html`, `..._caret_extending_other_staves_t.html`, `..._caret_r.html`

| Key | Action |
|---|---|
| **Shift-N** | Start note input. Select an item first; with no selection, input starts at the earliest position on the top staff in view. Pressing it again stops note input. |
| **Return** | Start note input when a note or rest is selected. If a notation item (for example a dynamic) is selected, Return instead **opens that item's popover for editing**. During note input, Return stops note input. |
| **Esc** | Stop note input |
| Double-click on staff | Start note input at that rhythmic position |
| Right / Left Arrow | Move the caret by the rhythmic grid or to the next/previous note or rest, whichever is closer |
| **Space** | Advance the caret by the selected note duration. Implicit rests fill the gap. Pressing it repeatedly with a whole note in 4/4 adds bars. |
| Cmd-Right / Cmd-Left | Caret to the next / previous bar |
| Up / Down | Caret to the staff above / below |
| Cmd-Up / Cmd-Down | Caret to the top / bottom staff of the system |
| **Shift-Up / Shift-Down** | Extend the caret to the staff above / below (multi-staff input) |
| Opt-] / Opt-[ | Rhythmic grid resolution: decrease / increase |

**Caret visuals (relevant to the UI spec).**
- Normal caret: vertical orange line with a note symbol showing the voice's stem direction. A voice number appears for second and later voices, and a "+" at bottom-left when the voice is new.
- Chord mode: "+" at top-left.
- Insert mode: V and inverted-V shapes at top and bottom, plus dotted lines on staves affected by the Insert scope.
- Lock Duration: dashed caret.
- Grace notes: shorter caret.
- Percussion kits and tablature: much smaller caret.
- Multi-staff: one caret spans all included staves.

---

## A3. Durations, dots, rests, ties, grace notes, tuplets

Sources: [M]`write_mode/write_mode_durations/write_mode_duration_selecting_t.html`, `write_mode/write_mode_notes_toolbox_r.html`, `..._rests_inputting_t.html`, `..._ties_inputting_t.html`, `..._grace_notes_inputting_t.html`, `..._tuplets_inputting_t.html`, `..._rhythm_dots_inputting_t.html`

| Key | Action |
|---|---|
| **1–9** (number row; the manual never mentions the numeric keypad for durations) | Duration. The manual states 4 = 16th, 5 = eighth, 6 = quarter, 7 = half. "Smaller numbers for smaller durations, larger for larger." By extension (inference, not stated in the manual) 8 = whole and 9 = double whole; the beat-unit table in the popover reference uses the same numbering (2 = 64th, 3 = 32nd, 4 = 16th … 9 = double whole), so 1 is presumably 128th, but the manual never names it. |
| Same number pressed twice quickly | Dotted version, e.g. 6 6 = dotted quarter |
| **.** | Toggle rhythm dot (Dotted Notes) |
| Opt-. | Cycle 1, 2, 3, 4 dots |
| **,** | Toggle rest input |
| Y, or any letter A–G, while in rest input | Input a rest. A shadow rest follows the mouse. |
| **O** | Force Duration on/off. Needed for explicit rests and forced note values. |
| **T** | Tie. During input it ties the next note to the previous note of the same pitch. On a selection of one note it ties to the next same-pitch note; on two notes it ties them. Tie cannot be deactivated; use Scissors to remove ties. |
| **U** | Scissors: split at the caret during input; outside input, delete ties in tie chains |
| Opt-U | Split notes by rhythmic grid resolution, or at the caret |
| **/** | Toggle grace-note input |
| Opt-/ | Toggle slashed / unslashed grace notes (also changes the type of selected grace notes) |
| **;** | Open the tuplets popover |
| **:** (Shift-;) | Stop tuplet input. Press it once to stop the inner tuplet of a nested pair, twice to stop both. Moving the caret with the arrows also stops tuplet input. |
| Shift-Opt-Right / Left | Lengthen / shorten selected notes by the grid. For items: by the grid or to the next notehead, whichever is closer. |
| Cmd-Shift-Opt-Right / Left | Double / halve note duration. For items (hairpins, lines): extend to the next / previous notehead. |
| 1–9 with notes selected | Change the duration of existing notes |

**Tablature exceptions.** = selects the next longer duration and - the next shorter, because digits are fret numbers. Two digits pressed quickly enter frets 10 and above. Source: [M]`write_mode/write_mode_note_input/write_mode_note_input_inputting_notes_tablature_t.html`

---

## A4. Pitch, accidentals, octave, chords and modes

Sources: [M]`..._note_input_register_selection_c.html`, `..._accidentals_inputting_t.html`, `..._chord_mode_c.html`, `..._note_input_inputting_chords_t.html`, `..._inputting_insert_mode_t.html`, `write_mode/write_mode_insert_mode/write_mode_insert_mode_scope_r.html`, `..._pitch_before_duration_t.html`, `write_mode/write_mode_transposing_tools/write_mode_notes_pitch_changing_rhythm_locked_t.html`

| Key | Action |
|---|---|
| **A–G** | Input a pitch. Dorico picks the octave closest to the previous note (F then A gives the A a 3rd above). The key signature is applied automatically (F in G major gives F♯). |
| **Shift-Opt-[letter]** | Force the note **above** the previous note. In Pitch Before Duration mode, repeated presses go up further octaves. |
| **Ctrl-[letter]** on macOS (**Ctrl-Alt-[letter]** on Windows) | Force the note **below** the previous note. In Chord mode it inputs below the lowest note at the caret. |
| **-** / **=** / **0** | Flat / sharp / natural for the next note only. With notes selected, it changes their accidental. Pressing the same key deletes that accidental (0 deletes naturals, - flats, = sharps). Source: [M]`notation_reference/notation_reference_accidentals/notation_reference_accidentals_deleting_t.html` |
| **Q** | Chord mode on/off. The caret does not advance; notes stack above the highest note. A letter repeated in a chord goes an octave up. Chord mode also affects copy/paste and duration edits so that they merge rather than overwrite. **It cannot be on together with Insert.** |
| **I** | Insert mode on/off |
| Opt-I | Cycle Insert scope: Voice, Player, Global, Global Adjustment of Current Bar |
| Shift-Opt-I | Set / delete an Insert mode stop position |
| **L** | Lock Duration: repitch existing notes while keeping their rhythm. The caret jumps from note to note and switches off automatically at the last note. |
| **K** | Pitch Before Duration on/off. Select pitch (and accidental, dot, articulation) first; the duration key then commits the note and shows a shadow notehead. |
| **V** | Next existing voice |
| **Shift-V** | Create a new voice (press repeatedly for further voices). With notes selected, moves them to a new voice. |
| Shift-Opt-V | Create a new slash voice |
| Shift held while hovering with the mouse | Move the shadow notehead chromatically instead of diatonically |

**Concert vs. transposed input pitch.** There is no default key command. It is set with the menu Write > Input Pitch > Written Pitch / Sounding Pitch. Source: [M] "Changing the input pitch setting".

**Preference: accidentals, dots and articulations before or after the note.** Preferences > Note Input and Editing > "Specify accidental, rhythm dot and articulations": *Before inputting note* or *After inputting note*. Source: [M]`write_mode/write_mode_input_preparation/write_mode_note_input_note_based_notation_input_setting_changing_t.html`

---

## A5. Articulations, slurs and hairpins

Source: [M]`write_mode/write_mode_notations_input/write_mode_articulations_inputting_methods_key_commands_r.html` (verbatim table)

| Articulation | Key (macOS) | Windows |
|---|---|---|
| Accent | [ | [ |
| Marcato | ' | ' |
| Stressed | { | { |
| Unstressed | " | @ |
| Staccato | ] | ] |
| Tenuto | \ | # |
| Staccatissimo | } | } |
| Tenuto + staccato | \| | ~ |

- During note input, articulations stay on (they latch) until you toggle them off.
- With notes selected, they toggle on those notes.
- Incompatible pairs are not allowed (staccato and staccatissimo together).

Slurs and hairpins (sources: [M]`..._slurs_inputting_t.html`, `..._dynamics_inputting_popover_t.html`):
- **S**: start a slur. During input it starts from the currently selected note, not the caret, and extends automatically as you input notes, even across rests.
- **Shift-S**: end the slur on the current note during note input. In Engrave mode, Shift-S inserts a system break.
- **S on a selection**: slur the selected notes.
- **<** / **>**: crescendo / diminuendo hairpin directly, without a popover.
- **Shift-Opt-,** / **Shift-Opt-.**: messa di voce hairpin `<>` / `><`.

---

## A6. Selection, navigation and editing (outside note input)

Sources: [M]`write_mode/write_mode_navigation/write_mode_navigation_items_music_area_t.html`, `write_mode/write_mode_selecting/write_mode_selections_extending_t.html`, `..._selections_types_r.html`, `..._selecting_more_t.html`, `..._notes_notation_selecting_deselecting_t.html`, `write_mode/write_mode_arranging_tools/*`, `write_mode/write_mode_transposing_tools/write_mode_notes_pitch_changing_t.html`, `..._notes_respelling_t.html`

| Key | Action |
|---|---|
| Any arrow with nothing selected | Select the first note, rest or item on the top staff |
| Right / Left | Next / previous item in the same voice |
| Up / Down | Closest note above / below |
| Cmd-Right / Cmd-Left | Note or rest at the start of the next / previous bar |
| Cmd-Up / Cmd-Down | Top / bottom staff of the system |
| Tab / Shift-Tab | Cycle through items. In Engrave mode, Tab cycles handles. |
| **Shift-Right / Shift-Left** | Extend the selection by one item. During note input this selects notes before/after the last input note without leaving input. |
| **Cmd-Shift-Right / Cmd-Shift-Left** | Extend the selection to the end / start of the bar |
| Shift-Up / Shift-Down, with a whole staff selected | Add the staff above / below |
| **Cmd-A** | Select all, for example the whole flow |
| **Cmd-Shift-A** | Select More. Repeated presses expand the selection to more items of the same type in the selected voices and staves. |
| Cmd-D | Deselect all |
| Cmd-click / Shift-click | Add to the selection / select a range |
| Shift-Opt-click | Select an item hidden behind another |
| Shift + double-click a note | Select that note and all later notes of the same pitch in the voice |
| Opt-Right / Opt-Left | Move notes or items rhythmically by the **nearest note or position**. Chord mode (Q) makes them overlap; Insert mode (I) pushes material. |
| Cmd-Opt-Right / Cmd-Opt-Left | Move rhythmically by the **rhythmic grid** |
| **Opt-Up / Opt-Down** | Pitch up / down one staff position (diatonic) |
| **Shift-Opt-Up / Shift-Opt-Down** | Up / down one octave division (a half-step in 12-EDO) |
| **Cmd-Opt-Up / Cmd-Opt-Down** | Up / down an octave |
| Opt-= / Opt-- | Respell using the note name above / below |
| Opt-N / Opt-M | Move notes or items to the staff above / below |
| N / M | Cross-staff notes to the staff above / below. On tablature, move to the string above / below. |
| F | Flip stem direction / placement (beams, tuplets, lines, fingerings) |
| R | Repeat the selection immediately after itself |
| Cmd-C / Cmd-V | Copy / paste. Opt-click pastes at the clicked position; Opt-drag copies. |
| Backspace / Delete | Delete. With Insert mode on, later notes close the gap; otherwise implicit rests appear. Selection moves to the nearest logical item. |
| Shift-B, then `rest` | Bar rest (see the bars popover) |
| Opt-Q / Shift-Opt-Q | Cycle chord-diagram shapes / open Choose Chord Diagram |
| Shift-A | Input a rehearsal mark directly (no popover) |
| Opt-drag notes up/down | Add notes above / below |

**Engrave mode nudging.** Opt+arrow moves an item 1/8 space (beams 1/4 space). Shift+Opt+arrow moves 1/2 space. Cmd+Opt+arrow moves 1 space. Cmd+Shift+Opt+arrow moves 1/32 space. Source: [M]`engrave_mode/engrave_mode_items_moving_graphically_t.html`

---

## A7. Playback

Sources: [M]`play_mode/play_mode_playing_back_music_t.html`, `play_mode/play_mode_playhead_moving_t.html`, `play_mode/play_mode_playback_scrubbing_t.html`

| Key | Action |
|---|---|
| **P** | Play from the selection. Also stops playback. |
| **Space** or Enter | Play / stop from the playhead |
| Shift-Space | Play from the last start position |
| Shift-Opt-Space | Play from the start of the flow |
| Num 0 | Stop |
| Num + or F9 / Num - or F7 | Fast forward / rewind |
| Num . | Rewind to the start of the flow |
| Cmd-Num ± or Cmd-F9 / Cmd-F7 | Move the playhead by frames |
| Opt-P | Move the playhead to the selection |
| Hold Opt-Space (Windows: Ctrl-Space) | Scrub all staves |
| Hold Shift-Opt-Space | Scrub soloing the instrument under the mouse |
| Cmd-R / Cmd-Opt-R | MIDI record / retrieve played notes |
| Opt-S / Shift-Opt-S | Solo selected instruments / clear all solos |

---

## Gaps and conflicts (not in the 6.2 manual)

- **Alt/Opt + number for intervals.** Not mentioned anywhere in the 6.2 manual, which was fully crawled. In 6.2 the documented route for adding intervals is Shift-I followed by an entry such as `3` or `-4`. Older Dorico versions had Alt-number key commands; confirm in the app's key command map before cloning that behavior.
- **"Select to end of staff / flow."** Not in the manual's key commands. Only Cmd-Shift-Right / Left (to the end / start of the bar), Cmd-Shift-A (Select More) and Cmd-A are documented. (The Edit menu has Select To End/Start Of System/Flow, with no default key; see `dorico-editing.md`.)
- **Popover error or invalid-input visuals and generic Esc behavior.** Undocumented. The manual only shows Esc closing the lyrics popover and the text editor.
- **Hide/show items.** No default key command is documented.
- **Same letter, different modes.** These Shift-letters do something else outside Write mode, so a clone needs per-mode key contexts:
  - Shift-I: instrument picker in Setup mode.
  - Shift-P: add player in Setup mode.
  - Shift-F: add flow in Setup mode; frame break in Engrave mode.
  - Shift-L: new score layout in Setup mode.
  - Shift-S: system break in Engrave mode.
