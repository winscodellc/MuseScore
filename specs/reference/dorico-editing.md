<!-- markdownlint-disable -->
# Dorico Pro 6.2 reference: copy/paste, moving, deleting, selection, jump bar, caret, undo, transforms

Reference material for the specs in `specs/`. Source: Dorico Pro 6.2 manual.

**How this was gathered.** steinberg.help is a JavaScript site, so the fetch tool returned each page as a summary of the article body. Text in "quotes" is quoted directly from the page. Everything else is a close paraphrase that keeps the key commands and menu names as the page gives them.

**Key notation.** The manual writes Windows/macOS together: "Ctrl/Cmd" means Ctrl on Windows and Cmd on macOS, and "Alt/Opt" means Alt on Windows and Option on macOS. Below, macOS first, with Windows in brackets when it differs.

All URLs share this prefix, abbreviated as `{B}`:
`{B}` = `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/`

**Things asked about that the manual doesn't have.** Several assumed commands don't exist as named:
- **No Edit > Find**, no "Select Inverse" and no "Select Less". The nearest equivalent to inverse selection is the Deselect Only mode for filters (section 4.7).
- **No "Paste Into Selection" or "Paste Rhythm".** Pasting onto a selected range fills that range instead (section 1.4).
- **No "Delete Time" command.** Removing time is done with the bars and barlines popover, Edit > Delete Bars, or deleting in Insert mode.
- **No Write > Explode/Reduce.** These are under **Edit > Paste Special**.
- **No "Write > Paste Special" menu.** Paste Special is under the **Edit** menu.
- **Cmd-Alt-Up/Down transposes by an octave.** It does not move notes to another staff. Moving to another staff is **Alt/Opt-N** (staff above) and **Alt/Opt-M** (staff below).
- **Shift-Alt/Opt-Up/Down transposes by one octave division** (a semitone in 12-EDO). It does not duplicate to an octave above/below.
- **No "Repeat last command" key.** R repeats the selected *music*. The jump bar shows your most frequently used commands instead (section 5).

---

## 1. Copy / Cut / Paste

### 1.1 Copying and pasting notes/items (main procedure)
Source: `{B}write_mode/write_mode_arranging_tools/write_mode_copying_pasting_items_t.html`

- Prerequisite: "If you want to use the mouse, you have enabled mouse editing."
- Step 1: "In Write mode, select the notes/items you want to copy." Tip: use large selections and/or filters.
- Step 2 offers these ways to copy:
  - "Press Ctrl/Cmd-C, select the position to which you want to paste them, then press Ctrl/Cmd-V."
  - **Alt/Opt-click**: "Alt/Opt-click each position to which you want to paste them."
  - **Alt/Opt-drag**: "Alt/Opt-drag notes to the position and staff to which you want to paste them." Shadow noteheads show the destination, and the affected region is highlighted with the rhythmic grid shown above it.
  - **R**: "To repeat the material directly after itself, press R."
  - **Edit > Paste Special > Duplicate to Staff Above** copies to the staff above.
  - **Edit > Paste Special > Duplicate to Staff Below** copies to the staff below.
  - Both Duplicate options are also in the context menu.
- Result:
  - "copied without deleting them from their original positions."
  - "By default, any MIDI CC points in the copied range are also pasted."
  - Dynamics and slurs copied to other staves at the same rhythmic position are automatically linked by default.
  - Copying or moving chord symbol regions to other staves makes those players show chord symbols.
- Tip: Duplicate to Staff Above/Below have **no default key command**. You can assign them in Preferences > Key Commands.
- Related links on the page: Disabling automatic copying of MIDI data; Disabling automatic linking of dynamics/slurs; Enabling/Disabling mouse editing; Copying and pasting articulations; Copying and pasting lyrics; Key Editor copy/paste of points; Rhythmic grid; Large selections; Filters; Resetting notes crossed to other staves; Hiding/Showing chord symbols; Key Commands page.

### 1.2 Paste Into Voice
Source: `{B}write_mode/write_mode_arranging_tools/write_mode_copying_pasting_notes_into_different_voices_t.html`

- Notes can be pasted into a voice other than their original one, including slash voices. Example: up-stem voice on one staff pasted into a down-stem voice on another staff.
- Procedure:
  1. Select the notes.
  2. Copy with Ctrl/Cmd-C or Edit > Copy (also in the context menu).
  3. "Select the staff you want to paste into, at the position where the selection should start."
  4. **Edit > Paste Special > Paste Into Voice > [existing voice or new voice]** (also in the context menu).
- Result: notes go to the selected staff and voice, and MIDI CC is pasted by default.
- Recommendation from the related links: reset cross-staff notes before copy/paste "to avoid unexpected beaming".

### 1.3 Pasting to multiple staves
Source: `{B}write_mode/write_mode_arranging_tools/write_mode_copying_pasting_multiple_staves_t.html`

- Procedure:
  1. Copy (Ctrl/Cmd-C, Edit > Copy, or context menu > Copy).
  2. Select a target item **on each staff** that should receive the material.
  3. Paste (Ctrl/Cmd-V, Edit > Paste, or context menu > Paste).
- **Rule:** "The earliest selected item on the top selected staff sets the rhythmic position for the pasted items on all staves."
- Result: the material is copied to all selected staves. Same MIDI CC, linking and chord-symbol behaviour as in 1.1.
- Tip: "If you select a range of notes or items on each staff, the copied material is pasted repeatedly to fill the selected range."
- The manual does not describe what happens when you copy N staves and paste onto a different number of staves with plain Paste. Explode and Reduce (1.6) are the documented tools for changing the staff count.

### 1.4 Pasting to fill a selected range
Source: `{B}write_mode/write_mode_arranging_tools/write_mode_copying_pasting_across_selection_t.html`

- Items can be pasted multiple times within a selected range, for example to fill several bars with a phrase.
- **Constraint:** you can only copy items that have a duration that fills a selection. Gradual dynamics qualify; immediate dynamics do not.
- Procedure: copy, then select the destination **range**, then paste.
- Result: "copied as many times as will fit within the destination range **without extending past its end**."
- Tip: a range selected across multiple staves pastes to multiple staves.

### 1.5 Other Paste Special items

| Command | Menu path | Behaviour | Source |
|---|---|---|---|
| Paste Articulations | Edit > Paste Special > Paste Articulations (also context menu) | Copy slurs, articulations and jazz articulations, then select the **first target note**. They are pasted onto notes starting there. **Pitches are unchanged.** | `{B}notation_reference/notation_reference_articulations/notation_reference_articulations_copying_t.html` |
| Duplicate to Staff Above / Below | Edit > Paste Special > … | Copies the selection to the adjacent staff. No default key. | 1.1 |
| Move to Staff Above / Below | Edit > Paste Special > … (keys **Alt/Opt-N** and **Alt/Opt-M**) | Cut and paste to the adjacent staff (see 2.3). | 2.3 |
| Paste Into Voice > [voice] | Edit > Paste Special > Paste Into Voice | See 1.2. | 1.2 |
| Explode / Reduce | Edit > Paste Special > Explode / Reduce | See 1.6. | 1.6 |
| Swap | Edit > Paste Special > Swap | Swaps the contents of **two staves** for the selected range. The page lists no key command. | `{B}write_mode/write_mode_arranging_tools/write_mode_staves_swapping_contents_t.html` |

Swap, quoted from step 2: "Choose Edit > Paste Special > Swap. You can also choose this option from the context menu." Step 1 is to select the range of music on two staves. Result: "The contents of the selected staves for the selected duration are swapped."

### 1.6 Explode and Reduce

**Reduce** (`{B}write_mode/write_mode_arranging_tools/write_mode_arranging_reducing_t.html`)
- Prerequisite: choose the voicing approach on the **Paste Special page of Note Input Options** (open with Cmd-Shift-I [Ctrl-Shift-I]).
- Procedure: select the music, copy, select the destination staves, then **Edit > Paste Special > Reduce** (also context menu).
- Tip: reduce rhythmic-unison sections separately from sections with different rhythms.
- If rhythms match throughout the selection:
  - The music is merged into a single voice by default.
  - Unison notes are removed, so only one note of a pitch is pasted per position.
  - Clef changes, octave lines and cues are deleted.
- If rhythms differ anywhere: Dorico uses as many voices as needed to keep the rhythms.
- Distribution: each destination staff gets at least one instrument, calculated from the top down. Example with 5 instruments onto 3 staves: staff 1 gets instruments 1+2, staff 2 gets 3+4, staff 3 gets 5.

**Explode** (`{B}write_mode/write_mode_arranging_tools/write_mode_arranging_exploding_t.html`)
- Prerequisites: Paste Special settings in Note Input Options, and reset cross-staff notes.
- Procedure: copy, select the destination staves, then **Edit > Paste Special > Explode**.
- Each destination staff gets at least one note from each chord, allocated from the top staff downwards:
  - Same number of notes as staves: one note per staff.
  - Fewer notes than staves: the same note goes to several staves, and slurs/dynamics are duplicated onto each.
  - More notes than staves: notes are spread as evenly as possible. With an odd number of notes, the extras go to the upper staves.

### 1.7 Paste preferences
Both are in Preferences (Cmd-, [Ctrl-,]) > **Note Input and Editing**:
- **Editing** section: "**Link dynamics and slurs to existing items when pasting**" (on by default).
  Source: `{B}write_mode/write_mode_arranging_tools/write_mode_linking_dynamics_slurs_when_pasting_disabling_t.html`
- **Note Input** section: "**Include MIDI CC, pitch bend and dynamics from Key Editor when copying music**" (on by default; applies to all future projects).
  Source: `{B}write_mode/write_mode_arranging_tools/write_mode_midi_data_included_when_pasting_disabling_t.html`

### 1.8 Overwrite vs Chord mode vs Insert mode on paste
- **Default:** pasted notes overwrite existing notes in the target voice.
  - Implied by Inputting notes: "New notes overwrite existing notes at the caret in the voice shown…"
  - A forum report says that if the existing material is in a *different* voice, it is not clobbered.
  - Source: `{B}write_mode/write_mode_note_input/write_mode_note_input_inputting_notes_t.html`
- **Chord mode (Q):** "Pasting notes onto a staff with Chord mode active combines them with existing notes instead of overwriting them." The manual's example is four copied notes pasted onto four existing notes, which produces chords.
  - Source: `{B}write_mode/write_mode_note_input/write_mode_chord_mode_c.html`
- **Insert mode (I):** affects "Copying and pasting notes". Pasted material pushes later music along within the Insert scope. In the **Voice** scope outside note input, the scope "includes all selected voices, such as when copying and pasting across multiple staves".
  - Sources: `{B}write_mode/write_mode_insert_mode/write_mode_insert_mode_c.html` and `{B}write_mode/write_mode_insert_mode/write_mode_insert_mode_scope_r.html`
- **Insert mode and Chord mode cannot both be active.**
- Inputting vs editing (`{B}write_mode/write_mode_note_input_inputting_editing_c.html`):
  - Editing happens when the caret is hidden and acts on the selection.
  - New items are placed at the first selected note/item.
  - "Insert mode and Chord mode can change the results of some edits, including copy and paste."
- The grid sets the possible positions for "copying and pasting" (Rhythmic grid page, section 6.4).

### 1.9 Pasting lyrics
Source: `{B}notation_reference/notation_reference_lyrics/notation_reference_lyrics_copying_pasting_t.html`

1. Copy lyrics (Write mode; use lyric filters or repeated Cmd-Shift-A to select many) or text from an external editor.
2. Select the first note of the target voice.
3. Open the lyrics popover with **Shift-L**.
4. Optionally change the target line:
   - Up/Down Arrow changes the line number.
   - Shift-Up Arrow moves to a line above the staff.
   - Up Arrow moves to a chorus line.
   - **Alt/Opt-Down Arrow** moves to a translation line.
5. **Cmd-V pastes one word or syllable at a time.** The popover advances automatically; a hyphen in the source acts like pressing hyphen.
6. To extend a syllable over several notes, press Space after a whole word or final syllable, or press hyphen after a non-final syllable.
7. Close with Return or Esc. The popover also closes automatically at the last note of the voice.

Rules:
- Clipboard text may only use single spaces and single hyphens as separators.
- Each pasted syllable is **removed from the clipboard**, so copy again before pasting into another line or staff.

---

## 2. Repeat / Duplicate / Move

### 2.1 R (Repeat)
- "To repeat the material directly after itself, press R." (copy page, 1.1)
- The official Quick Reference Card lists "Repeat selection: R".
- Forum, Dorico developer (pre-release, so behaviour may have changed): R also works during note input to repeat the previous note/chord, and it does not truncate at the barline.
- Forum: you can select a note, press R, then change the pitch with Alt-Up/Down.
- Sources: https://forums.steinberg.net/t/note-input-copy-paste-repeat-note/662458 and https://forums.steinberg.net/t/use-of-the-r-shortcut-key/673739

### 2.2 Moving notes/items rhythmically
Source: `{B}write_mode/write_mode_arranging_tools/write_mode_notes_items_moving_rhythmically_t.html`

| Key | Action |
|---|---|
| **Opt-Right / Opt-Left** [Alt-] | Move right/left |
| **Cmd-Opt-Right / Cmd-Opt-Left** [Ctrl-Alt-] | Move by the current rhythmic grid resolution |
| Mouse drag | Drag left/right; shadow noteheads and a grid ruler are shown. Only one item at a time with the mouse. |

- Optional step before moving notes:
  - Turn on **Chord mode (Q)** to let them overlap existing notes.
  - Turn on **Insert mode (I)** to let them move *through* existing notes. This "can change the durations of the existing notes they move through."
- Result:
  - Most items move by the grid.
  - A single selected dynamic, lyric or pedal line jumps to adjacent noteheads, bars or barlines.
  - Single cues move by the source instrument's rhythm.
  - Multiple items move as a block by the grid.
- Overwrite rules:
  - Only one tempo mark, clef and similar item can exist per position. Passing over an item of the same type deletes or shortens it.
  - When moving multiple items, same-type items in between or passed over are deleted/shortened unless Insert mode is on.
- **Undo caveat:** items deleted in the process are restored on undo **only if you moved with the keyboard**.
- Cannot be moved: notehead-attached items (glissandi, fingerings, tremolos). Delete and re-input them.
- Arpeggio signs and vertical lines: delete and re-input rather than move. If moved onto a rest, they are deleted.
- Markers have a fixed time position. Moving one relative to the music changes the tempo; change its timecode instead.
- Signposts (for example ossia start/end) can be moved independently.

### 2.3 Moving to other staves
Source: `{B}write_mode/write_mode_arranging_tools/write_mode_notes_moving_other_staves_t.html`

- **Opt-N** [Alt-N] moves to the staff above; **Opt-M** [Alt-M] moves to the staff below.
- Menus: Edit > Paste Special > Move to Staff Above / Move to Staff Below (also context menu).
- Mouse: drag a small amount horizontally first, then vertically.
- Result:
  - The selection is cut and pasted onto the new staff.
  - Notes go into the **first voice active** on that staff.
  - Notes in tuplets stay in tuplets even if the bracket wasn't selected.
- Not applicable to pedal lines or system-attached items.
- To keep notes attached to their original staff, use cross-staff notation instead.

### 2.4 Changing pitch (Alt-Up/Down family)
Source: `{B}write_mode/write_mode_transposing_tools/write_mode_notes_pitch_changing_t.html`

| Key | Action |
|---|---|
| Opt-Up / Opt-Down [Alt-] | Up/down one staff position (diatonic step, C to D) |
| Shift-Opt-Up / Down [Shift-Alt-] | Up/down one **octave division** (semitone in 12-EDO, quarter tone in 24-EDO) |
| Cmd-Opt-Up / Down [Ctrl-Alt-] | Up/down one **octave** |
| Drag / Shift-drag / Cmd-drag | By staff position / octave division / octave |

- Figured bass updates automatically.
- On fretted instruments, an impossible fret shows "?" in tablature.
- In percussion kits (grid or 5-line presentation), Alt-Up/Down changes the staff position and therefore the instrument.

### 2.5 Adding notes (intervals) and Alt-drag duplicate in pitch
Source: `{B}write_mode/write_mode_note_input/write_mode_notes_adding_above_below_t.html`

- **Shift-I** opens the note tools popover. Other routes: Notations toolbox > Popovers > Note Tools, or **Write > Add Intervals**.
- Example entries:
  - `-M2,m3 bottom` adds a major 2nd below and a minor 3rd above the bottom notes of the chords.
  - `5 16 qt` (24-EDO).
- **Alt/Opt-drag selected notes up/down** adds notes above/below (duplicating in pitch), with shadow noteheads shown.

### 2.6 Voices
Change the voice of existing notes (`{B}write_mode/write_mode_arranging_tools/write_mode_voices_existing_notes_changing_t.html`):
- **Shift-V** moves the notes to a new voice.
- **Shift-Opt-V** [Shift-Alt-V] moves them to a new slash voice.
- **V** cycles through the existing active voices.
- Menu: **Edit > Notations > Voices > Change Voice > [Voice]** (also context menu). This can create a new voice if the staff has only one.
- Result: stem directions may change, and rests appear as required.

Swap voice contents (`{B}write_mode/write_mode_arranging_tools/write_mode_voices_swapping_contents_t.html`):
- Select notes in both voices, then **Edit > Notations > Voices > Swap Voice Contents** (also context menu). Up-stem and down-stem contents are exchanged.
- Overlapping noteheads are arranged automatically. The related links point to "Swapping the order of voices" and "Voice column index".

---

## 3. Deleting

### 3.1 Deleting notes/items
Source: `{B}write_mode/write_mode_arranging_tools/write_mode_deleting_items_t.html`

- Keys: **Backspace or Delete**, or **Edit > Delete**. Write mode only (not Setup, Engrave or Print). In the Key Editor only notes can be deleted.
- Selection rules:
  - Barlines must be selected directly, not via signposts.
  - Deleting some but not all dynamics in a linked group deletes them from all linked staves.
  - Deleting only tuplets leaves their notes, and vice versa.
- After deleting, **the selection moves to the nearest logical item**. For a note, that is first the nearest note in the same voice.
- **Insert mode on:** following notes move back to fill the gap.
- **Insert mode off:** deleted notes become implicit rests "where appropriate".

Per-type behaviour:

| Item | Behaviour when deleted |
|---|---|
| Slur end note | The slur moves to the next/previous notehead. It is deleted if only one note remains under it. |
| Holds/pauses | Not deleted when unselected. They move to the nearest note/rest, or over the bar rest. |
| Repeat ending | Its repeat barlines are kept. |
| Barline | The adjacent bars merge without changing the time signature; regrouping shown by signposts. |
| Time signature | Re-barred with the previous time signature up to the next one or the end of the flow. With none, open meter. |
| Clef / key signature | Pitches are kept and renotated with the previous clef/key. With none, the flow becomes atonal/open (not C major). |
| Octave line | Notes shown at concert or transposed pitch depending on layout. |
| Immediate dynamics next to a hairpin | The hairpin may be adjusted. |
| Dynamics group deleted from one staff | Not deleted from linked staves. |
| Rehearsal mark | Later marks are renumbered. |
| Tempo mark | Also removed from the Tempo editor/track. Playback falls back to the previous tempo or 120 bpm. |

### 3.2 Deleting the contents of bars
Source: `{B}notation_reference/notation_reference_bars/notation_reference_bars_deleting_contents_t.html`
- Select the bars, then Backspace/Delete or Edit > Delete. Barlines and bars remain.

### 3.3 Remove Rests
Source: `{B}notation_reference/notation_reference_rests/notation_reference_rests_deleting_t.html`

- Select rests (individually or a region containing them), then **Edit > Remove Rests**. There is **no default key**; assignable in Key Commands. The jump bar alias example is `rr`, which is user-assigned.
- Mechanism: Dorico sets the **"Starts voice" / "Ends voice"** properties (Properties panel, Notes and Rests group) on the notes/rests immediately right and left of the removed rests.
- To restore rests, turn those properties off.
- Works on implicit and explicit rests.
- Spacing caveat: empty positions may look narrower.
- Alternatives the page suggests:
  - Delete bars/beats to remove the time.
  - Hide bar rests.
  - Consolidate identical rests across voices via Notation Options > Rests > Rest Positioning.

### 3.4 Deleting bars/beats (the "delete time" equivalent)
Source: `{B}notation_reference/notation_reference_bars/notation_reference_bars_deleting_t.html`

- **Shift-B** opens the bars and barlines popover:
  - `-6` deletes 6 bars starting with the selected bar.
  - `-2q` deletes two quarter-note beats from the selected position.
- Or select bars, then **Edit > Delete Bars**.
- Music to the right moves left "the same way it does when Insert mode is active".
- Signposts inside the range are deleted (for example, an ossia can disappear).
- Deleting only some beats removes only their contents. To shorten a bar's duration, use a hidden time signature, a barline plus deletion, or the Insert scope **Global Adjustment of Current Bar**.
- System track variant: `{B}notation_reference/notation_reference_bars/notation_reference_bars_beats_deleting_system_track_t.html`

### 3.5 Delete in Insert mode
Source: `{B}write_mode/write_mode_insert_mode/write_mode_insert_mode_c.html`
- "Deleting notes or shortening them pulls the surrounding notes closer together, leaving no rests."
- In the Global Adjustment of Current Bar scope, deleting shortens the bar and rewrites its time signature. This is how you create or delete pick-up bars.

---

## 4. Selection

### 4.1 Individual selection
Source: `{B}write_mode/write_mode_selecting/write_mode_notes_notation_selecting_deselecting_t.html`

| Gesture | Result |
|---|---|
| Click | Select a note/item |
| **Shift-Opt-click** [Shift-Alt-click] | Select an item **behind** another item |
| Cmd-click [Ctrl-click] | Add or remove individual items |
| Shift-click | Select adjacent items (range) |
| Click a beam | All notes in the beam group |
| Click a stem | All notes in the chord |
| Click empty staff space in a bar | Everything in that bar on that staff |
| **Shift-double-click a note** | That note and **all subsequent notes of the same pitch in the same voice** |
| Marquee | See 4.4 |
| Any arrow key with nothing selected | Selects the first note/rest/item on the top staff of the layout |
| **Cmd-D** [Ctrl-D], Edit > Select None, or click outside the staves | Deselect all |

- Selected items are shown in **orange**.
- Works in Write and Engrave modes.
- Preference: Note Input and Editing > Editing, "prioritize noteheads over stems/beams", lets you click single chord notes without selecting the whole chord.

### 4.2 Large selections
Source: `{B}write_mode/write_mode_selecting/write_mode_selections_types_r.html`

- **Cmd-A** [Ctrl-A] / Edit > Select All: everything in the whole **flow**.
- One staff:
  - Click the first note, then Shift-click the last note.
  - Or select the first note, then Edit > Select To End Of System / Select To End Of Flow.
  - Or select the last note, then Edit > Select To Start Of System / Select To Start Of Flow.
- Adjacent staves: select a full staff, then **Shift-Up / Shift-Down** to extend, or Shift-click the staff at the other end.
- System range including system-attached items (chord symbols, key and time signatures): use the system track (4.6).
- **Edit > Select More = Cmd-Shift-A** [Ctrl-Shift-A] (4.3).
- Tip: assignable key commands exist to select bar numbers and chord symbols in a range.

### 4.3 Select More (Cmd-Shift-A)
Source: `{B}write_mode/write_mode_selecting/write_mode_selecting_more_t.html`

- Expands horizontally to more items of the **same types**, and to notes in the **same voices** and staves. Works in Write and Engrave (Engrave requires the Graphic Editing tool).
- Expansion stages:
  1. **First press:**
     - With one item of each type: expands to the bar boundaries. If there is nothing else in the bar, it skips to stage 2.
     - With two or more items of a type: selects everything of that type between the earliest and latest. If there is nothing in between, it skips to stage 2.
  2. **Second press:** expands to the system boundaries in page/fill view, or to the whole flow in galley view.
  3. **Third press:** expands to the whole flow in page/fill view.
- Items spanning bars or systems count in the earliest bar/system where they appear.
- Single-item rules:
  - Lyrics: only the same line number, placement and line type.
  - Dynamics: first the same group on the same staves, then other groups.
  - Text: only the same paragraph style.
  - Playing techniques: only the same category; up-bow and down-bow only expand to each other.

### 4.4 Marquee
Source: `{B}write_mode/write_mode_selecting/write_mode_selecting_marquee_t.html`
- Activate **Marquee Tool** in the status bar, then click-drag a gray rectangle.
- Only items **entirely** inside are selected, **except** notes and tie chains, which are selected if any part is inside.
- The 6.2 page does not mention a Shift-drag modifier.

### 4.5 Extending selections
Source: `{B}write_mode/write_mode_selecting/write_mode_selections_extending_t.html`
- **Shift-Right / Shift-Left**: extend to the adjacent note/item.
- **Cmd-Shift-Right / Left** [Ctrl-Shift-]: extend to the end/start of the bar.
- Edit > Select To End/Start Of System.
- Edit > Select To End/Start Of Flow.
- These Select To commands have **no default keys** in the manual.

### 4.6 System track selection
Source: `{B}write_mode/write_mode_selecting/write_mode_system_track_bars_selecting_t.html`
- Click a bar in the system track, Shift-click or drag to extend, then click the **System Track Select** icon.
- Everything on all staves is selected, including system-attached items and signposts. Deleting this selection also deletes the signposts.
- Beats variant: `{B}write_mode/write_mode_selecting/write_mode_system_track_beats_selecting_t.html`

### 4.7 Filters
Sources:
- Filters reference: `{B}write_mode/write_mode_selecting/write_mode_filters_r.html`
- Select Only / Deselect Only: `{B}write_mode/write_mode_selecting/write_mode_filters_select_deselect_changing_t.html`
- Filter Notes by Pitch dialog: `{B}write_mode/write_mode_selecting/write_mode_filter_notes_by_pitch_dialog_r.html`

- Path: **Edit > Filter > [Item] > [Item type]** (also context menu).
- **Edit > Filter > Select Only** (default): the result is only the filtered type.
- **Edit > Filter > Deselect Only**: the result is everything *except* the filtered type. This is the inverse-selection mechanism.
- "Dorico Pro includes a filter for every notation item." Examples: arpeggio signs, chord symbols, key signatures, playing techniques, note spacing changes.

Filters with sub-types:

| Filter | Sub-types |
|---|---|
| Notes | Notes, grace notes, chords; also by accidental, pitch, and **position within chords** |
| Voices | By stem direction; slash voices |
| Dynamics | All / gradual / immediate |
| Tempos | All / absolute / relative / gradual |
| Lyrics | All / line number / type / staff-relative placement |
| Text | All / by paragraph style |

- Not filterable: barlines, and things that belong to notes (fingerings, beams, articulations, tremolos).
- **The manual does not give the exact item list.** Forum sources name **Edit > Filter > Notes in Chords** with items such as "Top Notes Or Single Notes" (no default key): https://forums.steinberg.net/t/keyboard-shortcuts-for-filter/112654. Top/bottom/nth-from-top items are not enumerated on the 6.2 page.

**Edit > Filter > Notes by Pitch > Specific Pitches** opens the Filter Notes by Pitch dialog:
- Written/Sounding pitches toggle.
- List of pitch filters, with Remove Selected and Clear.
- Note name menu and Accidental menu.
- "in octave" checkbox with a value: limits matching to one octave; off means all octaves.
- Add button.

### 4.8 Keyboard navigation
Source: `{B}write_mode/write_mode_navigation/write_mode_navigation_items_music_area_t.html`

| Key | Moves selection to |
|---|---|
| Right / Left | Next/previous item or note **in the same voice** (same item type) |
| Up | Closest note above: other notes on the same staff first, then the **lowest** note/rest on the staff above. If a whole chord was selected, the bottom note stays selected. |
| Down | Mirror of Up: the **highest** note/rest on the staff below. If a whole chord was selected, the top note stays selected. |
| Cmd-Right / Left [Ctrl-] | Note/rest at the start of the next/previous bar |
| Cmd-Up / Down [Ctrl-] | Top/bottom staff in the system |
| Tab / Shift-Tab | Cycle to other item types at the same rhythmic position (not system-attached items) |

- Forward and back movement stays on the same staff.
- In Engrave mode, navigation goes to the graphically closest item.
- To walk through one item type (for example rehearsal marks), select one first.

---

## 5. Jump bar, Go To Bar, key command search

Jump bar reference: `{B}user_interface/user_interface_jump_bar_r.html`

**Opening and modes**
- Open with **J** in Setup, Write, Engrave and Play modes.
- Two modes, switched as follows:
  - **Commands**: **Ctrl-1** on macOS (Alt-C on Windows), or click "Commands".
  - **Go To**: **Ctrl-2** on macOS (Alt-G on Windows), or click "Go To".
- Separate keys for opening directly in each mode can be assigned in Key Commands.

**Commands mode** (procedure: `{B}user_interface/user_interface_jump_bar_commands_performing_t.html`)
- Type text and a menu of valid commands and options appears; pick with Up/Down.
- Selecting an option makes its setting editable inside the jump bar.
- Down Arrow on an empty field shows up to **5 most-frequently-used commands**.
- Reopening the jump bar after a command shows the previous entry pre-selected.
- Only commands valid in the current mode can run (for example, Setup actions only in Setup).
- The menu shows each command's key command or alias.
- Option editing:
  - Space toggles checkboxes and opens menus.
  - Left/Right moves between buttons.
  - Up/Down moves through menu items or changes values.
  - **Opt-Return** [Alt-Return] opens the corresponding dialog.
  - Return applies.

**Go To mode syntax** (`{B}user_interface/user_interface_jump_bar_going_to_locations_t.html`)

| Destination | Entry | Examples |
|---|---|---|
| Bar n | `b[n]` | `b32`, `b+32` |
| Rehearsal mark | `r[n]` | `rK`, `rk` |
| Page | `p[n]` | `p6`, `p+6` |
| Flow | `f[n]` | `f3`, `f+3` |
| Next flow | `fn` | |
| Previous flow | `fp` | |
| Combined | `f3b20` | bar 20 of flow 3 |

- Result: the view updates and the playhead moves to the start of the corresponding flow.
- The page doesn't explain the `+` forms (for example `b+32`).

**Aliases** (`{B}user_interface/user_interface_jump_bar_aliases_assigning_t.html`, removal: `{B}user_interface/user_interface_jump_bar_aliases_removing_t.html`)
- Type the command name, then `\=` and the alias, for example "Remove Rests\=rr".
- Return runs the command and assigns the alias; **Opt-Return** assigns without running.
- Aliases can also be set on the Key Commands page.

**Go To Bar** (`{B}write_mode/write_mode_navigation/write_mode_navigation_go_to_bar_t.html`)
- **Cmd-G** [Ctrl-G] opens a dialog with a Flow menu and a Bar field, then OK.
- Scrolls so the top staff is near the top-left, and moves the playhead to the flow start.
- In Play mode, the playhead goes to the start of the bar instead.

**Key command search** (`{B}user_interface/user_interface_key_commands_searching_t.html`)
- Preferences (**Cmd-,**) > Key Commands > Search field (filters by words) > expand the entry. Hover to see long names.
- Also: Help > Key Commands (window) and the interactive key commands map (`{B}user_interface/user_interface_key_commands_interactive_map_r.html`).

**Search inside Options dialogs** (`{B}library/library_note_input_options_dialog_r.html`)
- Cmd-L focuses the category search.
- Cmd-F opens a page search; Cmd-G / Cmd-Shift-G go to the next/previous match.

**There is no Edit > Find / Find and Select feature** documented. Filters and Shift-double-click on a note (same pitch onwards) are the closest equivalents.

---

## 6. Caret and note input basics

### 6.1 Activating and deactivating the caret
Source: `{B}write_mode/write_mode_caret/write_mode_caret_activating_deactivating_t.html`
- Activate:
  - Select an item and press **Shift-N**.
  - **Return** also starts input, but if a notation such as a dynamic is selected, Return opens its popover instead.
  - Notes toolbox > Start Note Input.
  - Double-click a rhythmic position on a staff.
- Deactivate:
  - Shift-N, **Return**, or **Esc**.
  - Click any item, if mouse input is disabled.
  - The toolbox button.
  - Switching mode.
- Toolbox rule: input starts at the selected item's position, or at the earliest position on the top visible staff if nothing is selected.

### 6.2 Moving the caret
Source: `{B}write_mode/write_mode_caret/write_mode_caret_moving_manually_t.html`

| Key | Movement |
|---|---|
| Right / Left | By the grid resolution **or** to the next/previous note/rest, **whichever is closer** |
| Space (or Advance Caret in the Keyboard/Fretboard/Drum Pads panels) | Advance by the **selected note duration**; gaps become implicit rests |
| Cmd-Right / Left | Next/previous bar |
| Up / Down | Staff above/below |
| Cmd-Up / Down | Top/bottom staff in the system |
| **Shift-Up / Shift-Down** | **Extend** the caret to the staff above/below; repeatable. Source: `{B}write_mode/write_mode_caret/write_mode_caret_extending_other_staves_t.html` |

- The caret doesn't advance automatically when Chords is on.

### 6.3 Caret appearance
Source: `{B}write_mode/write_mode_caret/write_mode_caret_r.html`
- An orange vertical line. It can sit partway through tie chains.
- Voice indicator: a note symbol showing stem direction; a voice number for the 2nd voice and above; "+" for a new voice. Slash voices have their own symbol.
- Chord mode: "+" at the top-left.
- Insert mode: V and inverted-V shapes at the top and bottom, plus dotted lines on the other affected staves.
- Lock Duration: dashed caret.
- Grace notes: shorter caret.
- Percussion kits: small caret with the instrument name above the grid.
- Tablature: small caret; behaves as if Chords is always on.

### 6.4 Rhythmic grid
Sources:
- Concept: `{B}write_mode/write_mode_rhythmic_grid/write_mode_rhythmic_grid_c.html`
- Changing the resolution: `{B}write_mode/write_mode_rhythmic_grid/write_mode_rhythmic_grid_resolution_changing_t.html`

The grid controls:
- Input and paste positions.
- How far Left/Right moves the caret.
- The lengthen/shorten step.
- The move step.
- The split duration.

It does not set note durations. **Default: eighth note.** It is shown in the status bar and as a ruler above the caret.

Changing it (quoted):
- "To **decrease** the rhythmic grid resolution, press **Alt/Opt-]**." Decreasing makes it **finer** (a shorter value).
- "To **increase** the rhythmic grid resolution, press **Alt/Opt-[**." Increasing makes it **coarser** (a longer value).
- Menus: Write > Rhythmic Grid > Decrease/Increase Grid Resolution, or a specific value.
- Or use the status bar selector.

### 6.5 Inputting notes
Source: `{B}write_mode/write_mode_note_input/write_mode_note_input_inputting_notes_t.html`

- Durations: 7 = half, 6 = quarter, 5 = eighth, 4 = 16th (larger number, longer value).
- Rhythm dots: "." (or press the duration key twice); Opt-. cycles the dot count.
- Rests: ",".
- Pitch: letters A–G give the nearest pitch.
  - **Shift-Opt-letter** [Shift-Alt]: force **above** the previous note.
  - **Ctrl-letter** on macOS [Ctrl-Alt-letter on Windows]: force **below**.
- Mouse: shadow notehead moves by staff position; hold Shift for chromatic.
- Accidentals apply to the next note only. Dots and articulations stay on until turned off.
- Overwrites the existing notes in the caret's voice.

### 6.6 Chords (Q)
Source: `{B}write_mode/write_mode_note_input/write_mode_note_input_inputting_chords_t.html`
- **Q** toggles chord input.
- Letters add a note **above the highest** note at the caret; **Ctrl-letter** (mac) [Ctrl-Alt-letter on Windows] adds **below the lowest**.
- Space advances the caret.
- With the keyboard, a repeated letter goes an octave higher. With the mouse, clicking the same line adds a duplicate pitch.
- Insert mode plus chords works only with MIDI.

### 6.7 Insert mode (I)
- **I** toggles. **Opt-I** [Alt-I] cycles the scope. Menu: **Write > Insert Scope > [Scope]**.
- Scopes (`{B}write_mode/write_mode_insert_mode/write_mode_insert_mode_scope_r.html`):

| Scope | Affects |
|---|---|
| Voice | Selected voices only (the caret's voice during input; all selected voices when editing) |
| Player | All voices and instruments of the selected players |
| Global | All players in the flow |
| Global Adjustment of Current Bar | All players; also changes the current bar's duration and rewrites its time signature |

- All scopes apply up to the stop position or the end of the flow.
- In note input, press **V** to choose the voice (Voice scope) and extend the caret for multiple staves (Voice/Player scopes). Source: `{B}write_mode/write_mode_note_input/write_mode_note_input_inputting_insert_mode_t.html`
- Insert mode also affects copy/paste, delete, duration changes and time-signature input. With Insert on, beats are inserted before existing time signatures to fill the bar.
- **Stop position** (`{B}write_mode/write_mode_insert_mode/write_mode_insert_mode_stop_position_setting_t.html`):
  - Set with **Shift-Opt-I** [Shift-Alt-I], the system track button "Set Edit Stop Position", or Write > Insert Scope > Set Edit Stop Position.
  - Only one per flow; setting a new one replaces the old.
  - Shown as a translucent red line; its top handle can be dragged.
  - **Notes pushed past it are deleted.**
  - Deleting stop positions: `{B}write_mode/write_mode_insert_mode/write_mode_insert_mode_stop_positions_deleting_t.html`
- Insert and Chord mode are mutually exclusive.

### 6.8 Notes toolbox keys
Source: `{B}write_mode/write_mode_notes_toolbox_r.html`

| Key | Tool |
|---|---|
| K | Pitch Before Duration |
| Q | Chords |
| I | Insert (Opt-I scope) |
| L | Lock Duration |
| O | Force Duration |
| Shift-V / Shift-Opt-V / V | Create Voice / Create Slash Voice / Next Voice |
| . | Dotted notes (Opt-. cycles) |
| , | Rests |

- "Select" in the toolbox disables mouse input. It has no default key ("Disable Mouse Input" is assignable).

### 6.9 Lock Duration (L)
Source: `{B}write_mode/write_mode_transposing_tools/write_mode_notes_pitch_changing_rhythm_locked_t.html`
- Repitches existing notes and keeps the rhythm. The caret jumps note to note, even across large rests.
- Turns off automatically at the last existing note, then returns to the previously selected duration.
- Max rests skipped: Preferences > Note Input and Editing > Note Input > Lock Duration.

### 6.10 Pitch Before Duration (K)
Source: `{B}write_mode/write_mode_note_input/write_mode_note_input_inputting_notes_pitch_before_duration_t.html`
- Select the pitch first (a shadow notehead shows it), then the duration key inputs the note.
- Repeating the Shift-Opt or Ctrl letter command moves by octave.
- The default order is set in Preferences > Note Input and Editing.

---

## 7. Undo / Redo / History
Sources:
- Undo/Redo: `{B}write_mode/write_mode_editing/write_mode_undo_redo_t.html`
- History dialog: `{B}write_mode/write_mode_editing/write_mode_history_dialog_r.html`

- Undo: **Cmd-Z** [Ctrl-Z], or the toolbar Undo button.
- Redo: **Cmd-Shift-Z or Cmd-Y** [Ctrl-Shift-Z / Ctrl-Y], or the toolbar Redo button.
- **Edit > History** dialog lists the session's **edits and selections**. Pick an entry and click OK to undo/redo to that point. Available in any mode.
- **Selections are part of the undo stack.**
- Caveat from 2.2: items deleted while moving are restored on undo only if moved with the keyboard.
- There is no "repeat last command" key; R repeats music. The jump bar's Down-arrow list of most-used commands is the closest equivalent.

---

## 8. Other editing commands (Edit / Write menus)

### 8.1 Note durations
Source: `{B}write_mode/write_mode_durations/write_mode_notes_duration_changing_t.html`

| Action | Key | Menu |
|---|---|---|
| Set a duration | Number key / Notes panel | — |
| Lengthen by grid | **Shift-Opt-Right** [Shift-Alt-] | Write > Edit Duration > Lengthen Duration by Grid Value |
| Shorten by grid | **Shift-Opt-Left** | Write > Edit Duration > Shorten Duration by Grid Value |
| Double | **Cmd-Shift-Opt-Right** [Ctrl-Shift-Alt-] | Write > Edit Duration > Double Note Duration |
| Halve | **Cmd-Shift-Opt-Left** | Write > Edit Duration > Halve Note Duration |
| Extend to next note (same voice; not grace notes) | — | Write > Edit Duration > Extend to Next Note |
| Extend to end of selection | Select the note plus an item at the end position | Write > Edit Duration > Extend to End of Selection |
| Shorten overlapping notes | — | Write > Edit Duration > Shorten to Next Note |

- Extended notes fill rests. Extend to End of Selection does not delete intervening notes; they combine into chords.
- Assignable commands: search "Shorten duration by", "Lengthen duration by" and "Extend to" in Key Commands.

Splitting notes (`{B}write_mode/write_mode_durations/write_mode_notes_splitting_by_duration_t.html`):
- **Opt-U** [Alt-U] or Write > Edit Duration > Split by Rhythmic Grid Resolution.
- With the caret active and extended across the staves, Opt-U splits at the caret.
- Write > Edit Duration > Split by Note Duration > [value].
- A remainder that doesn't divide evenly becomes a shorter final note.

### 8.2 Transpose
Source (procedure): `{B}write_mode/write_mode_transposing_tools/write_mode_transposing_selections_t.html`
- **Write > Transpose** opens the dialog. **No default key is listed** (a Cmd-Shift-T binding is not in the manual).
- With no selection, the whole flow containing the last selected item is transposed.
- Include key signatures, chord symbols and figured bass in the selection to transpose them.
- Key signatures that apply to all staves are transposed on every staff. Single-staff key signatures are transposed only if selected.

Transpose dialog (`{B}write_mode/write_mode_transposing_tools/write_mode_transpose_dialog_r.html`):
- Direction.
- Octaves.
- Interval + quality (choose the interval first), **or** a number of octave divisions.
- Transpose key signatures.
- Keep relative respelled notes and chord symbols.
- Respell to avoid double/triple sharps/flats (12-EDO-compatible systems only).
- **Calculate interval**: enter a from-note and a to-note, then Apply.
- Impossible notation (beyond a triple sharp, or a microtone missing from the tonality system) is blocked.

### 8.3 Respell
Source: `{B}write_mode/write_mode_transposing_tools/write_mode_notes_respelling_t.html`
- **Opt-=** [Alt-=]: Respell Using Note Name Above.
- **Opt--** [Alt--]: Respell Using Note Name Below.
- Both are also buttons in the Keyboard panel toolbar.
- **Write > Respell > Respell Notes Automatically** opens a dialog (`{B}write_mode/write_mode_transposing_tools/write_mode_respell_notes_automatically_dialog_r.html`).
- Example cycle from G♯: down gives F𝄪… (F triple-sharp per the page); up gives A♭, then B♭♭♭.
- Respelling in a score layout applies to all layouts. Respelling in a part applies only to that part.
- Respelling individual tie-chain noteheads requires Engrave mode.
- Atonal default spelling: Note Input Options > MIDI Input > Note Spelling.
- Related pages:
  - Copy spellings to other layouts: `{B}write_mode/write_mode_transposing_tools/write_mode_respelling_propagating_t.html`
  - Reset spellings: `{B}write_mode/write_mode_transposing_tools/write_mode_respelling_resetting_t.html`

### 8.4 Musical transformations (Write > Transform)
Overview: `{B}write_mode/write_mode_transformations/write_mode_transformations_c.html`
- Six transformations: Inversion, Reversal (retrograde), Rotation, Repetition, Pitch mapping, Scale mapping.
- Defaults live on the Note Input Options > **Musical Transformations** page, including whether transformations that shorten the selection are allowed.

| Command | Menu | Source |
|---|---|---|
| Invert Pitches (dialog) | Write > Transform > Pitches > Invert Pitches | `{B}write_mode/write_mode_transformations/write_mode_inverting_pitches_t.html` |
| Reverse and Invert Pitches (dialog) | Write > Transform > Pitches > Reverse and Invert Pitches | same |
| Reverse and Invert Pitches and Reverse Rhythms (dialog) | Write > Transform > All > … | same |
| Reverse Pitches | Write > Transform > Pitches > Reverse Pitches | `{B}write_mode/write_mode_transformations/write_mode_reversing_pitches_rhythms_t.html` |
| Reverse Rhythms | Write > Transform > Rhythm > Reverse Rhythms | same |
| Reverse Pitches and Rhythms | Write > Transform > All > Reverse Pitches and Rhythms | same |
| Rotate Pitches / Rhythms / Pitches and Rhythms (dialogs) | Write > Transform > Pitches / Rhythm / All > Rotate… | `{B}write_mode/write_mode_transformations/write_mode_rotating_pitches_rhythms_t.html` |
| Repeat Pitches (dialog: number of pitches) | Write > Transform > Pitches > Repeat Pitches | `{B}write_mode/write_mode_transformations/write_mode_repeating_pitches_t.html` |

- Reversal and rotation include **staff-attached items** in the selection (for example dynamics).
- Dialog reference pages:
  - `{B}write_mode/write_mode_transformations/write_mode_reverse_invert_dialog_r.html`
  - `{B}write_mode/write_mode_transformations/write_mode_rotate_pitches_dialog_r.html`
  - `{B}write_mode/write_mode_transformations/write_mode_repeat_pitches_dialog_r.html`

### 8.5 Note tools popover (Shift-I, or Write > Add Intervals)
Overview: `{B}write_mode/write_mode_transformations/write_mode_note_tools_popover_r.html`
- Works with notes selected, **including during note input**.
- Covers the functions of the Add Notes, Transpose, Reverse/Invert, Rotate, Map Pitches, Map Scale and Repeat Pitches dialogs.

Entry syntax pages (prefix `https://www.steinberg.help/r/dorico-pro/6.2/en/_shared/topics/dorico/dorico_popovers/`):

**Intervals** (`dorico_popovers_note_tools_intervals_r.html`)
- Degrees: 1–15.
- Qualities:
  - `M`, `maj`, `major` = major (`M` is case-sensitive).
  - `m`, `min`, `minor` = minor (case-sensitive).
  - `perf`, `per`, `p` = perfect.
  - `dim`, `d`, `diminished` = diminished.
  - `aug`, `a`, `augmented` = augmented.
  - `diat`, `diatonic` = diatonic.
- Octave-division forms:
  - `N div` (or `divisions`, `d`): octave divisions in any tonality system.
  - `N st` (or `semitones`, `s`): semitones.
  - `N qt` (or `quartertones`, `q`): quarter tones (24-EDO).
- Add-notes examples: `-m3,4`; `-M2,m3 bottom` (relative to the bottom notes).

**Transpose** (`dorico_popovers_note_tools_transpose_r.html`)
- `t3` up a third; `t-6` down a sixth.
- `t 5 8 div` / `t 5 8 st` / `t 5 8 qt`: the degree sets the target note name, plus the number of divisions.
- `t -6 9 div` (and the `st`/`qt` forms) for downward moves.
- `t top -8`: top notes of chords down an octave. `t bottom 8`: bottom notes up an octave.

**Invert** (`dorico_popovers_note_tools_invert_r.html`)
- Keywords: `inv`, `invert`, `inversion`, `mirror`. Default axis: the range between the highest and lowest selected notes.
- Axis options:
  - A pitch, for example `inveb4`.
  - An octave-division form, for example `d1/24 4`.
  - `root` (key root; C in open key), `first`, `top`, `bottom`.
- `diatonic`/`diat` or `chromatic`/`chrom`.

**Reverse** (`dorico_popovers_note_tools_reverse_r.html`)
- `retrograde`, `ret`, `reverse`, `rev`.
- `pitch`/`pt` for pitches only; `rhythm`/`rm` for rhythms only.
- `ret invert` / `rev inv` for retrograde inversion.
- Scope modifiers: `player`, `voice`. Example: `rev pt voice`.

**Rotate** (`dorico_popovers_note_tools_rotate_r.html`)
- `rotate`/`rot`; `pitch`/`pit`; `rhythm`/`rhy`/`rm`.
- A step count `n` rotates forward. `-n`, `backwards`, `bw` or `prev` rotate backward. Example: `rot rm bw 3`.

**Repeat** (`dorico_popovers_note_tools_repeat_r.html`)
- `repeat 2 pitches` / `rep 2 pit`.

**Other entry types** (not fetched during research):
- Pitch mapping: `dorico_popovers_note_tools_map_pitch_r.html`
- Scale mapping: `dorico_popovers_note_tools_map_scale_r.html`

(The full pitch/scale-mapping syntax is in `dorico-popovers.md`.)

### 8.6 General key command conventions
Source: `{B}program_concepts/program_concepts_key_commands_in_dorico_r.html`
- Arrows alone move the selection.
- Alt+arrows: up/down transposes by staff position; left/right moves rhythmically (Write mode). In Engrave mode they nudge graphically.
- Shift-Alt-Right lengthens.
- Cmd-Alt-Up/Down is an octave.
- Cmd-Shift-Alt-Right doubles.
- Shift+letter opens popovers (Shift-D dynamics, Shift-O ornaments, Shift-B bars, Shift-L lyrics, Shift-I note tools).
- Cmd-Shift+letter opens Options dialogs: Cmd-Shift-L Layout Options, Cmd-Shift-N Notation Options, Cmd-Shift-I Note Input Options.
- Default key commands depend on the keyboard language setting.

---

## 9. Implementation notes not stated explicitly in the manual
- **Paste anchor:** the earliest selected item on the top selected staff sets the start position. One selected item per destination staff means one copy per staff; a selected range is filled with as many whole copies as fit, with no partial copy at the end.
- **Paste conflicts:** the result depends on the mode. Default overwrites in the target voice; Chord mode merges into chords; Insert mode pushes later music along within its scope. Insert and Chord mode can't both be on.
- **Selection after delete:** moves to the nearest logical item, first the nearest note in the same voice.
- **Moves that overwrite:** single-instance items (clefs, tempo marks) passed over by a move are deleted or shortened. Undo restores them only for keyboard moves.
- **Undo stack:** selection changes are recorded in History.
