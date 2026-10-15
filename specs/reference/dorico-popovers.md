# Dorico Pro 6.2 reference: popovers

Reference extracted from Dorico Pro 6.2 manual (paraphrased facts: key bindings and syntax tokens). macOS first.

## Sources

Syntax tables come from the separate **Dorico Popover Reference 6.2** (map `iuRC2znMppy_r3SiZ6eo_w`, 85 topics, all fetched in full); behaviour from the **Dorico Pro Help 6.2** manual (map `BYbh2hvBDwUCjSijIhHcqA`). Section sources use two URL prefixes. Join the prefix to the path given:
- **[M]** = `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/`
- **[P]** = `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/`

Key bindings that are not popovers are in `dorico-key-commands.md`.

---

## B0. Shortcut letters (verified)

Sources: [P] reference pages and [M]`write_mode/write_mode_notations_toolbox_r.html`

| Key | Popover | Notes |
|---|---|---|
| **Shift-B** | Bars and barlines | |
| **Shift-C** | Clefs **and octave lines** | Octave lines share this popover; there is no separate octave-line popover |
| **Shift-D** | Dynamics | |
| **Shift-F** | Fingerings | |
| **Shift-G** | Figured bass | |
| **Shift-H** | Holds and pauses | |
| **Shift-I** | **Note tools**: add intervals, transpose, invert, retrograde, rotate, repeat, pitch/scale mapping | In Setup mode, Shift-I is the instrument picker instead |
| **Shift-K** | Key signatures | |
| **Shift-L** | Lyrics | |
| **Shift-M** | Time signatures (meter) | |
| **Shift-O** | Ornaments: trills, arpeggios, glissandi, jazz articulations, guitar techniques | |
| **Shift-P** | Playing techniques, including pedal lines, harp pedaling and harmonics | |
| **Shift-Q** | Chord symbols | |
| **Shift-R** | Repeats and tremolos: endings, D.C./D.S./coda, tremolos, slash, bar-repeat and numbered-bar regions | |
| **Shift-U** | Cues (Pro only) | |
| **Shift-0** (zero) | MIDI trigger regions | |
| **;** | Tuplets | |
| Shift-T | Tempo (see B2 Tempo; the tempo popover letter is Shift-T) | |
| Shift-X / Shift-Opt-X | **Not a popover.** Opens the in-place **text editor** for staff text / system text. | |
| Shift-A | Rehearsal mark, input directly (no popover) | |

**Does not exist in 6.2:** an intervals popover separate from Shift-I, a lines popover (lines are panel-only), a notes popover, or anything on Shift-N (Shift-N is start note input).

---

## B1. Popover UI behavior (everything the manual states)

Sources: [M]`program_concepts/program_concepts_popovers_c.html`, `write_mode/write_mode_notations_toolbox_r.html`, `write_mode/write_mode_notations_input/write_mode_dynamics_popover_r.html`, `write_mode/write_mode_notations_input/write_mode_repeats_popover_r.html`, `write_mode/write_mode_transformations/write_mode_note_tools_popover_r.html`, and the "Inputting … with the popover" pages

**Availability**
- Popovers work in Write mode only.
- A popover is available only "during note input or when at least one note/item is selected".

**Position**
- It opens "above the top staff on which the caret is active or an item is selected, and at the caret position or the rhythmic position of the earliest selected item".

**Anatomy** (from the labelled figures)
- **Popover handle**: drag it to move the popover vertically.
- **Popover icon** on the left: matches the corresponding button in the Notations toolbox.
- **Entry field.**
- **Local/Global indicator**, on the dynamics, key signatures, time signatures, bars and barlines, chord symbols and figured bass popovers.
- Lyrics adds lyric line controls and an above/below indicator. Fingerings adds a hand indicator. Chord symbols adds chord-symbol line controls.

**Confirm and cancel**
- **Return** confirms and closes.
- **Esc** closes. The manual only states this explicitly for lyrics ("Press Return or Esc to close the popover"); for text it is Esc or Cmd-Return.

**Alt/Opt-Return (local variant)**

| Popover | Return | Alt/Opt-Return |
|---|---|---|
| Key / time signatures, pick-up bars, bars, barlines | All staves | Only the selected staves, or the staves the caret spans |
| Dynamics | All voices and staves of the instrument | Only the caret's voice (voice-specific) |
| Octave lines | All voices | Current voice only |
| Chord symbols, figured bass | The type set in the popover | The opposite type (local ↔ global) for this one entry. Opt-L / Opt-G set the popover to local / global. |
| Rhythmic feel (tempo popover) | All staves | Selected instrument only |

**Editing an existing item**
- Select a dynamic, tempo mark, clef, octave line, key signature, time signature, hold, ornament, playing technique, chord symbol, lyric, repeat marker, slash region, bar repeat, cue or MIDI trigger region, then press **Return**. The matching popover reopens for that item.
- Entering a new key signature, time signature or barline with an existing one selected replaces it.
- Text items reopen in the text editor with Return or a double-click.
- The tuplets popover, when opened on existing notes, is pre-filled with a suggested ratio.

**Suggestion menus** (only these popovers are documented as having them)
- **Tempo**: shows matching suggestions; free text is also allowed.
- **Playing techniques** and **pedal lines**: a menu of matching *valid* entries. Entries must match a known technique.
- **Cues**: a candidate instrument menu *below* the popover. Choose with the arrow keys, then Return. A full instrument name typed by hand needs Return twice.
- **Jump bar**: Up/Down to choose a suggestion.

**Error state.** The manual does not document an error or invalid-input state anywhere (no red border or error message is described).

**Selection range vs. single item vs. caret**
- **Caret:** the item is input at the caret on all staves the caret spans. Open-ended items (`p<`, `8va`, `vibrato->`) are created only once the caret advances (Space or more notes) and keep extending until stopped:
  - `?` stops gradual dynamics and playing techniques.
  - `|` or `stop` stops octave lines.
  - `*`, `s*` and `u*` stop the sustain, sostenuto and una corda pedal lines.
  - Gradual tempo changes get a default length of one quarter note and do not extend.
- **Single selection:** the item goes at the earliest selected item.
- **Range:**
  - Items with duration (hairpins, trills, octave lines, gradual tempo, techniques with duration) span the range.
  - Clefs add a *restorative* clef at the end of the range.
  - `|:` with a range adds a matching `:|` at the end.
- **Item input position preference:** "Create items at position of first selected note" (on by default) decides whether items go at the first selected note or the first selected item, which can be a rest. Source: [M]`write_mode/write_mode_input_preparation/write_mode_item_input_position_setting_changing_t.html`

**Chord symbols popover stops note input when opened.**

**Bracket conventions**
- `( … )` = parenthesized: dynamics, tempo, playing techniques, chord symbols, fingering alternatives, figured bass.
- `[ … ]` = hidden: tempo, playing techniques. For fingering, square brackets mean editorial.

**Navigation inside a popover** (chord symbols, figured bass, fingerings). Source: [P]`dorico_popovers_navigation_r.html`

| Key | Action |
|---|---|
| Space | Next beat (chord symbols, figured bass) / next note or chord (fingering) |
| Shift-Space | Previous beat / note |
| Tab | Start of the next bar |
| Shift-Tab | Start of the current bar if mid-bar, otherwise the previous bar |
| Right / Left | Next/previous note, rest, grid position or character, whichever is closest |
| Cmd-Right / Cmd-Left | Next / previous existing chord symbol or figure |

---

## B2. Entry syntax tables

These are verbatim from the Popover Reference. Each section lists its own [P] page.

### Bars and barlines (Shift-B)

**Bars** — [P]`dorico_popovers_bars_barlines_bars_r.html`

| Entry | Effect |
|---|---|
| `+2` or `2` | Add bars |
| `-6` | Delete bars |
| `rest` | Add a bar rest |
| `trim` | Delete empty bars at the end of the flow |

**Beats** — [P]`dorico_popovers_bars_barlines_beats_r.html`

| Entry | Effect |
|---|---|
| `+2q`, `+2-6`, `+2 6`, `+2/4` | Add beats |
| `-2q`, `-2-6`, `-2 6`, `-2/4` | Delete beats |
| `trim` | Delete empty beats at the end of the flow |

**Barlines** — [P]`dorico_popovers_bars_barlines_barlines_r.html`

| Barline | Entries |
|---|---|
| Single | `\|`, `single`, `normal` |
| Double | `\|\|`, `double` |
| Triple (Pro) | `\|\|\|`, `triple` |
| Dashed | `:`, `dash`, `dashed` |
| Final | `\|]`, `fin`, `final` |
| Tick (top) (Pro) | `'`, `tick` |
| Tick (bottom) (Pro) | `tickbottom` |
| Short (center) (Pro) | `,`, `short` |
| Short (top) (Pro) | `shorttop` |
| Thick (Pro) | `thick` |
| Double thick | `doublethick`, `thickdouble` |
| Start repeat | `\|:`, `start` |
| End repeat | `:\|`, `end` |
| End/start repeat | `:\|:`, `:\|\|:`, `end-start`, `endstart` |

Behavior from [M]`write_mode/write_mode_notations_input/write_mode_barlines_inputting_popover_t.html`:
- A repeat barline entered where one already exists combines into end/start.
- A final barline at the end can trim the flow.
- Barlines on single staves only work if those staves have an independent time signature.

### Clefs and octave lines (Shift-C)

**Clefs** — [P]`dorico_popovers_clefs_octave_lines_clefs_r.html`

| Clef | Entries |
|---|---|
| Treble G | `g`, `g2`, `sol`, `treble` |
| Bass F | `f`, `f4`, `fa`, `bass` |
| Tenor C | `ct`, `c4`, `ut4`, `tenor` |
| Alto C | `ca`, `c3`, `ut3`, `alto` |
| Treble 8vb | `g8ba`, `g8d`, `treble8ba`, `treble8d` |
| Treble 15mb (Pro) | `g15ba`, `g15d`, `treble15ba`, `treble15d` |
| Treble 8va (Pro) | `g8va`, `g8u`, `treble8va`, `treble8u` |
| Treble 15ma (Pro) | `g15ma`, `g15u`, `treble15ma`, `treble15u` |
| Alto 8vb (Pro) | `ca8ba`, `ca8d`, `alto8ba`, `alto8d` |
| Tenor 8vb (Pro) | `ct8ba`, `ct8d`, `tenor8ba`, `tenor8d` |
| Bass 8vb (Pro) | `f8ba`, `f8d`, `bass8ba`, `bass8d` |
| Bass 15mb (Pro) | `f15ba`, `f15d`, `bass15ba`, `bass15d` |
| Bass 8va (Pro) | `f8va`, `f8u`, `bass8va`, `bass8u` |
| Bass 15ma (Pro) | `f15ma`, `f15u`, `bass15ma`, `bass15u` |
| Percussion | `perc` |
| 4-string tab | `tab4` |
| 6-string tab | `tab6` |
| French violin (Pro) | `g1` |
| Soprano (Pro) | `soprano`, `c1`, `ut1` |
| Mezzo (Pro) | `mezzo`, `c2`, `ut2` |
| Baritone C (Pro) | `baritone`, `c5`, `ut5` |
| Baritone F (Pro) | `baritonebass` |
| Sub-bass (Pro) | `subbass` |
| Invisible (Pro) | `invisible` |

**Clef octave shift** — [P]`dorico_popovers_clefs_octave_lines_clef_octave_shifts_r.html`
- `<clef>+N` / `<clef>-N`: shift by N octaves, for example `g+2`.
- Append `c` for concert pitch or `t` for transposed pitch, for example `treble+1t`.

**Octave lines** — [P]`dorico_popovers_clefs_octave_lines_octave_lines_r.html`

| Shift | Entries |
|---|---|
| Up 1 octave | `8va`, `8`, `8u`, `1u` |
| Up 2 octaves | `15ma`, `15`, `15u`, `2u` |
| Up 3 octaves | `22ma`, `22`, `22u`, `3u` |
| Down 1 octave | `8ba`, `8vb`, `8d`, `1d` |
| Down 2 octaves | `15ba`, `15vb`, `15d`, `2d` |
| Down 3 octaves | `22ba`, `22vb`, `22d`, `3d` |
| Loco | `loco` |
| Stop during note input | `\|`, `stop` |

### Dynamics (Shift-D)

Pages: [P]`dorico_popovers_dynamics_dynamics_r.html`, `..._gradual_r.html`, `..._modifiers_r.html`; behavior from [M]`write_mode/write_mode_notations_input/write_mode_dynamics_inputting_popover_t.html`

**Immediate and combined dynamics**

| Entry | Result |
|---|---|
| `pppppp` … `p` | Soft dynamics |
| `mp`, `mf` | Medium dynamics |
| `f` … `ffffff` | Loud dynamics |
| `fp` | Fortepiano, no separator |
| `f-p`, `f:p`, `f/p` | Fortepiano with a separator |
| `sfz` | Sforzando |
| `rfz` | Rinforzando |
| `(f)` | Parenthesized dynamic |

**Gradual dynamics**

| Entry | Result |
|---|---|
| `<` | Crescendo hairpin |
| `cresc`, `crescendo` | "cresc." text |
| `cresc...`, `crescendo...` | cresc. with continuation line |
| `>` | Diminuendo hairpin |
| `dim`, `diminuendo` | "dim." text |
| `dim...`, `diminuendo...` | dim. with continuation line |
| `decresc`, `decrescendo` | "decresc." text |
| `decresc...`, `decrescendo...` | decresc. with continuation line |
| `<>` / `><` | Messa di voce: one hairpin with a shared centre handle |
| `< >` / `> <` | Two separate grouped hairpins |
| `o<`, `>o` | Niente hairpin with a circle |
| `n<`, `>n` | Niente hairpin with the letter "n" |
| `?` | Stop a gradual dynamic during note input |

**Modifiers.** Must be combined with an immediate dynamic, separated by a space (`f molto`, `p espressivo`). Free text is allowed.

| Entry | Modifier |
|---|---|
| `subito`, `sub`, `sub.` | subito |
| `possibile`, `poss`, `poss.` | possibile |
| `poco` | poco |
| `molto` | molto |
| `piu`, `più` | più |
| `meno` | meno |
| `mosso` | mosso |

Some modifiers, such as molto, are automatically placed *before* the dynamic.

**Phrases**
- Entries like `p<f>p` or `f>` create a grouped phrase.
- During note input, each hairpin in a phrase lasts a quarter note.
- An open-ended entry (`p<`) extends as the caret advances.

### Fingerings (Shift-F)

**Non-fretted instruments** — [P]`dorico_popovers_fingering_non_fretted_r.html`

| Entry | Meaning |
|---|---|
| `1` … `7` | Single fingering (also valve numbers and trombone slide positions) |
| `12` | Valves 1 and 2 |
| `1,3,5` | One fingering per chord note. Keyboard order is automatic: right hand for the upper staff, left hand for the lower. |
| `L`, `G`, `S`, `I` or `H` + n | Left hand, e.g. `L2` |
| `R`, `D` or `M` + n | Right hand, e.g. `R5` |
| `T` | Thumb |
| `2343` | Sequence on one note (for ornaments) |
| `1,1` | Same finger on two notes |
| `2(3)` | Alternative fingering |
| `[4]` | Editorial fingering |
| `1-3` | Finger substitution |
| `2/` | String shift indicator |

**Fretted instruments** — [P]`dorico_popovers_fingering_fretted_r.html`

| Entry | Meaning |
|---|---|
| `0`–`5` | Left-hand fingers |
| `t` | Left-hand thumb |
| `1`–`5` or `p i m a e` | Right-hand fingers |
| `p`, `t`, `1` | Right-hand thumb |
| `e`, `x`, `c`, `o`, `5` | Right-hand pinky |

**Fretted popover controls.** Up switches to the left hand and Down to the right hand. Space advances to the next note. Return closes.

**Limits.** One rhythmic position at a time. At most one fingering per note at that position. One voice only (the top voice if several are selected).

### Figured bass (Shift-G)

**Figures and suspensions** — [P]`dorico_popovers_figured_bass_figures_r.html`

| Entry | Meaning |
|---|---|
| `1`–`19` | Figures, auto-sorted ascending |
| `6#42`, `6,#4,2` | Multiple figures; use commas to avoid ambiguity |
| `(#643)`, `#64(3)`, `(#)643` | Bracketed figures or accidentals |
| `<3>`, `{3}` | Hidden figures |
| Chord-symbol syntax (`Em7`, `Amaj7`, `G/B`) | Converted to figures automatically |
| `tasto`, `ts` | tasto solo |
| `4->3`, `4_3`, `4~3` | Suspension with resolution |
| `r=2`, `r=n` / `r=note`, `r=1/2n` | Suspension duration: quarter notes / bass-note length / fraction of the bass note |
| `d=2`, `d=n`, `d=1/2n` | Hold duration, same forms |
| `R`, `r`, `V`, `v` or `?` at the start | Follow Engraving Options for this figure |
| `O`, `o` or `!` at the start | Follow input literally |

**Accidentals and alterations** — [P]`dorico_popovers_figured_bass_alterations_r.html`

| Entry | Meaning |
|---|---|
| `#`, `s` | Sharp; the figure can be omitted for a raised 3rd |
| `b`, `f` | Flat |
| `n`, `N` | Natural |
| `x`, `##`, `ds` | Double sharp |
| `x#`, `#x`, `###`, `ts` | Triple sharp |
| `bb`, `db` | Double flat |
| `bbb`, `tb` | Triple flat |
| `+` / `-` | Raise / lower a half-step |
| `d` | Diminished |
| `u` | Unaltered |

### Holds and pauses (Shift-H)

Pages: [P]`dorico_popovers_holds_pauses_fermatas_r.html`, `..._breath_marks_r.html`, `..._caesuras_r.html`

**Fermatas**

| Entry | Fermata |
|---|---|
| `fer`, `fermata`, `pause` | Fermata |
| `verylongfermata`, `verylongfer` | Very long |
| `longfermata`, `longfer` | Long |
| `shortfermata`, `shortfer` | Short |
| `veryshortfermata`, `veryshortfer` | Very short |
| `shorthenzefermata`, `shorthenzefer` | Short (Henze) |
| `longhenzefermata`, `longhenzefer` | Long (Henze) |
| `curlew` | Curlew (Britten) |

**Breath marks**

| Entry | Breath mark |
|---|---|
| `breathmarkcomma`, `comma`, `,` | Comma |
| `breathmarktick` | Tick-like |
| `breathmarkupbow` | Upbow-like |
| `breathmarksalzedo` | Salzedo |

**Caesuras**

| Entry | Caesura |
|---|---|
| `caes`, `caesura`, `//` | Caesura |
| `caesurathick`, `thickcaesura`, `thick//` | Thick |
| `caesuracurved`, `curvedcaesura`, `curved//` | Curved |
| `caesurashort`, `shortcaesura`, `short//` | Short |
| `caesurasingle`, `singlecaesura`, `single//` | Single-stroke |

**Placement**
- Fermatas go on all staves.
- Breath marks go on the selected or caret staves, to the right of the note.
- Caesuras go on all staves, to the left of the note.

### Note tools (Shift-I)

Requires selected notes, including during note input. Pages: [P]`dorico_popovers_note_tools_*_r.html`. The manual examples come from [M]`write_mode/write_mode_note_input/write_mode_notes_adding_above_below_t.html` and `write_mode/write_mode_transposing_tools/write_mode_notes_transposing_existing_popover_t.html`.

**Intervals**

| Entry | Meaning |
|---|---|
| `1`–`15` | Interval degree |
| `M`, `maj`, `major` | Major (`M` is case-sensitive) |
| `m`, `min`, `minor` | Minor (`m` is case-sensitive) |
| `perf`, `per`, `p` | Perfect |
| `diminished`, `dim`, `d` | Diminished |
| `augmented`, `aug`, `a` | Augmented |
| `diatonic`, `diat` | Diatonic |
| `8 div` (`divisions`, `div`, `d`) | Octave divisions |
| `8 st` (`semitones`, `st`, `s`) | Half-steps |
| `8 qt` (`quartertones`, `qt`, `q`) | Quarter tones (24-EDO) |

Without a quality, the interval is diatonic in the current key: `3` above D in C major gives F♮.

**Add notes**

| Entry | Meaning |
|---|---|
| `3`, `3rd` | Add above |
| `-4`, `-4th` | Add below |
| `5 8 div`, `5 8 st`, `5 8 qt` | Add by octave divisions, spelled as the given degree |
| `-6 9 div` etc. | Same, below |
| `3,6`, `-3,3,4` | Several notes |
| `top`, `bottom` | Apply only to the top / bottom note of chords, e.g. `-M2,m3 bottom` |

**Transpose**

| Entry | Meaning |
|---|---|
| `t3` | Up a third |
| `t-6`, `t-min6` | Down |
| `t 5 8 div`, `t -6 9 qt` | By octave divisions |
| `t top -8` | Top notes of chords down an octave |
| `t bottom 8` | Bottom notes of chords up an octave |

**Invert** (not in SE)

| Entry | Meaning |
|---|---|
| `inv`, `invert`, `inversion`, `mirror` | Invert |
| `C4`, `eb4`, `F5`, `g#6` | Central pitch |
| `d1/24 4`, `A-3/24 5` | Central octave division |
| `root`, `first`, `top`, `bottom` | Central pitch: key root / first note / top or bottom of the first chord |
| `diatonic`, `diat` / `chromatic`, `chrom` | Inversion type |

**Reverse** (not in SE)

| Entry | Meaning |
|---|---|
| `retrograde`, `ret`, `reverse`, `rev` | Reverse pitches and rhythms |
| `pitch`, `pt` | Pitches only |
| `rhythm`, `rm` | Rhythms only |
| `ret invert`, `rev inv` | Reverse and invert |
| `player` | Include other items of the same player |
| `voice` | Only items in the same voice |

**Rotate** (not in SE)

| Entry | Meaning |
|---|---|
| `rotate`, `rot` | Rotate pitches and rhythms |
| `pitch`, `pit` | Pitches only |
| `rhythm`, `rhy`, `rm` | Rhythms only |
| `n` | n steps forward |
| `-3`, `backwards 3`, `bw 3`, `prev 3` | Backward |

**Repeat** (not in SE): `repeat 2 pitches`, `rep 2 pit`.

**Pitch mapping** (not in SE)

| Entry | Meaning |
|---|---|
| `c4=eb4,f4=g4,a4=bb4` | Source=destination pairs |
| `*` after the note | Include enharmonic equivalents |
| `up` / `down` | Destination is above / below the source |

**Scale mapping** (not in SE)
- Structure: `[source] to [destination]`, e.g. `c M to c m`, `Fmin to Dmaj`.
- `snap`, then `up`/`higher` or `down`/`lower`: snap notes outside the source scale.
- `spell Cmaj`: respell to a scale. `snap cmin`: snap to a scale.
- Roots: `c`, `Db`, `e`, `f#` … Alterations: `#4`, `b6`.

| Scale | Entries |
|---|---|
| Major | `M`, `maj` |
| Minor | `m`, `min` |
| Ionian major | `ion maj`, `ion M` |
| Harmonic minor | `harm m`, `hmc min` |
| Melodic minor | `me m`, `mdc min` |
| Dorian | `dor`, `drn` |
| Phrygian | `phryg`, `phry`, `phr` |
| Lydian | `lyd` |
| Mixolydian | `mixlyd`, `mlyd`, `mixo`, `mix`, `ml` |
| Aeolian / natural minor | `aeol m`, `aeo m`, `aln min`, `nat min` |
| Locrian | `loc` |
| Super Locrian | `super loc`, `sup loc`, `spr loc` |
| Major pentatonic | `pent maj`, `5tonic M`, `5ton M` |
| Minor pentatonic | `pent min`, `5tonic m`, `5ton m` |
| Major blues | `blue maj`, `bls M` |
| Minor blues | `blue min`, `bls m` |
| Whole tone | `wholetone`, `wtone`, `whole`, `wt` |
| Half-whole diminished | `halfwhole dimin`, `hw dim`, `halftone dimin`, `half dim`, `ht dim` |
| Half-whole octatonic | `halfwhole octa`, `hw oct`, `halftone octa`, `half oct`, `ht oct` |
| Whole-half diminished | `wholehalf dimin`, `wh dim`, `wholetone dimin`, `wt dim` |
| Whole-half octatonic | `wholehalf octa`, `wh oct`, `wholetone octa`, `whole oct`, `wt oct` |
| Locrian ♭♭7 | `loc maj mode7`, `harm M mod7` |
| Super Locrian ♭♭7 | `sup loc min mode7`, `spr loc m mod7` |

### Lyrics (Shift-L)

Pages: [P]`dorico_popovers_lyrics_r.html`, [M]`write_mode/write_mode_notations_input/write_mode_lyrics_inputting_t.html`

| Key | Action |
|---|---|
| Space | End the word and advance; shows a gap or extender line |
| - | End the syllable and advance; shows a hyphen |
| Space or - repeated | Skip notes (melisma) |
| Right / Left | Move the cursor one character; crossing into the next/previous lyric moves the popover there |
| Opt-Right / Opt-Left | Move to the next / previous syllable |
| Shift-Opt-Space | Non-breaking space |
| Opt-- | Literal hyphen without advancing |
| _ | Elision slur |
| Up / Down | Lyric line number. Up from line 1 gives the chorus line ("c"). |
| Shift-Up / Shift-Down | Lyrics above / below the staff |
| Opt-Down | Translation line |
| Return or Esc | Close |

- The popover closes automatically at the last note in the voice.
- Cmd-V pastes word by word: paste, then Space or -, repeated.

### Key signatures (Shift-K)

Page: [P]`dorico_popovers_key_signatures_r.html`

| Entry | Meaning |
|---|---|
| `open`, `atonal` | Open / atonal key |
| `C`, `D`, `E` … (upper case) | Major keys |
| `c`, `d`, `e` … (lower case) | Minor keys |
| `#` or `s` / `b` or `f` | Sharp / flat in the name (`Eb`, `f#`) |
| `3#`, `3s` | Major key with 3 sharps |
| `4b`, `4f` | Major key with 4 flats |

- Custom key signatures are panel-only.
- Return applies to all staves; Opt-Return only to the selected or caret staves.
- During note input the key signature goes at the caret, even mid-bar.

### Time signatures (Shift-M)

Pages: [P]`dorico_popovers_time_signatures_time_sigs_r.html`, `..._pick_up_bars_r.html`

| Entry | Meaning |
|---|---|
| `2/2`, `3/4`, `6/8`, `5/4` … | Simple time signatures |
| `c` | Common time |
| `cutc`, `cut`, `¢` | Cut time |
| `x`, `X` | Open meter shown as X |
| `open` | Open meter with no indication |
| `3+2+2/8` | Additive, grouping shown |
| `[2+3+2]/8` | Grouping set but not shown |
| `6/8 + 3/4` | Alternating (spaces required) |
| `2/4\|6/8` | Aggregate with dashed barlines |
| `2/4:6/8` | Aggregate without dashed barlines |
| `2/4 (6/8)`, `2/4 / 6/8`, `2/4 = 6/8`, `2/4 – 6/8` | Interchangeable; spaces are required around the separators and before `(` |

**Pick-up bars:** `[timesig],[n]`, where n counts multiples of the denominator unit:

| Entry | Pick-up |
|---|---|
| `4/4,1` | Quarter |
| `4/4,1.5` | Dotted quarter |
| `4/4,0.25` | 16th |
| `3/4,0.75` | Dotted eighth |
| `6/8,2` | Two eighths |
| `2/2,0.5` | Quarter |

- Beats are only added to fill bars if Insert mode (I) is on.
- Opt-Return applies to the selected staves only.

### Ornaments (Shift-O)

Pages: [P]`dorico_popovers_ornaments_*_r.html`

**Ornaments**

| Entry | Ornament |
|---|---|
| `trill`, `tr` | Trill |
| `tr m3` (space required) | Trill with interval |
| `shorttr` | Short trill |
| `mordent`, `mor` | Mordent |
| `turn` | Turn |
| `invertedturn`, `invturn` | Inverted turn |

**Trill intervals**

| Entry | Meaning |
|---|---|
| `1`–`8` | Degree |
| `M`, `maj`, `major` | Major |
| `m`, `min`, `minor` | Minor |
| `perf`, `per`, `p` | Perfect |
| `diminished`, `dim`, `d` | Diminished |
| `augmented`, `aug`, `a` | Augmented |

**Jazz ornaments:** `brassbend`; `flip`; `jazz`/`shake` (jazz turn); `smear`.

**Jazz articulations:** `plop`; `plopsmooth`; `scoop`; `doit`; `doitsmooth`; `fall`; `fallsmooth`.

**Arpeggio signs**

| Entry | Sign |
|---|---|
| `arpeggioup`, `arpup`, `arp` | Up |
| `arpeggiodown`, `arpdown` | Down |
| `nonarpeggio`, `nonarp` | Non-arpeggio |
| `slurarp` | Curved |

**Glissando:** `gliss` (default style), `glissstraight`, `glisswavy`.

**Guitar techniques**

| Entry | Technique |
|---|---|
| `bend` | Guitar bend |
| `vibbend` | Vibrato bar dive and return |
| `vibscoop` | Vibrato bar scoop |
| `vibdip` | Vibrato bar dip |
| `wbar`, `w/bar` | Vibrato bar line |
| `hammer`, `ho` | Hammer-on |
| `pull`, `po` | Pull-off |
| `hammerpull`, `hopo`, `hp`, `ligado`, `lig` | Hammer-on/pull-off (ligado) |
| `tap` | Right-hand tapping |
| `tappull` | Right-hand tapping with pull-off |
| `lhtap` | Left-hand tapping |
| `lhtappull` | Left-hand tapping with pull-off |

During note input, a trill's length comes from the next note input or a Space advance.

### Playing techniques (Shift-P)

Pages: [P]`dorico_popovers_playing_techniques_*_r.html`, `dorico_popovers_pedal_lines_r.html`, `dorico_popovers_harp_pedaling_r.html`, `dorico_popovers_harmonics_r.html`

**General syntax**
- A suggestion menu of valid techniques appears as you type; entries must match a known technique.
- `(arco)` = parenthesized. `[pizz]` = hidden.
- `vibrato->` = with duration (open-ended during note input). `?` ends it with a duration line.
- Entering a new technique ends the open one with a continuation line.
- Outside note input, `A->B` creates two techniques, automatically grouped.
- Only one technique per entry during note input.
- Techniques are voice-specific: placed above for up-stem voices and below for down-stem voices.

**Common techniques** (the manual says "many further valid" entries exist)

| Entry | Technique |
|---|---|
| `string 1`…`9` | String indicator outside the staff |
| `nat` | Naturale |
| `vibrato` / `senza vibrato` | Vibrato / senza vibrato |
| `legato` | Legato |
| `o` | Harmonic (o) |
| `con sord` / `senza sord` | Muted / unmuted |
| `stopped` | Stopped (text) |
| `+` | Stopped or muted (+) |
| `downbow` / `upbow` | Bowing |
| `strong air pressure` | Strong air pressure |
| `double-tongue` | Double-tongue |
| `fluttertongue` | Flutter-tongue (f.t.) |
| `flt` | Flutter-tongue (flt.) |
| `flz` | Flatterzunge (flz.) |
| `sul pont` / `sul tasto` / `pst` | Sul ponticello / sul tasto / poco sul tasto |
| `pizz` / `spicc` / `arco` | Pizzicato / spiccato / arco |
| `tongue click` / `finger click` | Stockhausen clicks |
| `motor on` / `motor off` | Vibraphone motor |
| `open` / `damp` / `damp large` | Open / damp |
| `full barre` / `half barre` | Barré |
| `strum up` / `strum down` | Strums |
| `lh` / `rh` | Left / right hand |

**Pedal lines**

| Entry | Meaning |
|---|---|
| `ped` | Sustain pedal line |
| `1/4`, `1/2`, `3/4`, `1` | Sustain pedal level (Pro) |
| `^`, `notch`, `retake` | Retake |
| `nonotch` | Remove retake |
| `*` | Stop sustain pedal during note input |
| `sost` / `s*` | Sostenuto line / stop |
| `unacorda` / `u*` | Una corda line / stop |

**Harp pedaling**
- After a note name: `^` or `b` = flat; `-` or nothing = natural; `v` or `#` = sharp.
- Example: `--^|^---`, `--^^---`, `BbEb` or `DCBbEbFGA` all set D C B♭ E♭ F G A.

**Harmonics.** The entry must start with `harm`, `harmonic` or `harmonics`. Further tokens are separated by spaces:

| Entry | Meaning |
|---|---|
| `nat`, `natural` / `art`, `artificial` | Harmonic type |
| `nh`, `hh` / `ah` / `ph` | Natural / artificial / pinch harmonic |
| `p=4` (`p`, `partial=`) | Partial |
| `n=2` (`n`, `node=`) | Node |
| `s=3`, `s=D4` (`s`, `string=`) | String by number or by pitch |
| `circle`, `circleabove` | Circle above the staff (natural harmonics) |
| `diamond`, `diamondnotehead`, `sounding` | Diamond notehead (natural harmonics) |
| `white`, `whitediamondnotehead`, `whitediamond`, `touched` | White diamond for touched pitches (natural harmonics) |
| `normal` | White diamond for touched pitches (artificial harmonics) |
| `pinch` | Pinch artificial harmonic |
| `sounding`, `singlesounding` | White diamond for sounding pitches (artificial harmonics) |
| `stopped`, `singlestopped` | White diamond for stopped pitches (artificial harmonics) |
| `open` / `noopen` | Show / hide open-string pitch |
| `paren` / `noparen` | Show / hide parenthesized sounding pitch |
| `noharm` | Delete the harmonic |

Example: `harm nat n1 s1 p4 diamond`.

### Chord symbols (Shift-Q)

Pages: [P]`dorico_popovers_chord_symbols_*_r.html`, [M]`write_mode/write_mode_notations_input/write_mode_chord_symbols_inputting_t.html`

**Roots**

| System | Examples |
|---|---|
| English | `C`, `Db`, `F#`, `B` |
| German | `C`, `Des`, `Fis`, `H` |
| Fixed-do | `do`, `reb`, `fa#`, `ti` |
| Nashville | `1`, `2b`, `4#`, `7` |

**Accidentals:** `#`; `b`; `x` or `##`; `x#`, `#x` or `###`; `bb`; `bbb`.

**Qualities**

| Quality | Entries |
|---|---|
| Major | `M` (case-sensitive), `maj`, `ma`, or nothing |
| Minor | `m` (case-sensitive), `min`, `mi` |
| Diminished | `dim`, `di`, `o` |
| Augmented | `aug`, `au`, `ag`, `+` |
| Half-diminished | `half-dim`, `halfdim`, `hd` |
| 6/9 | `6/9`, `69`, `%` |

**Intervals:** `1`, `2`, `3` …; major 7th = `^7` or `^`; major 9th = `^9`, `maj9`, `9maj7`.

**Alterations**

| Entry | Meaning |
|---|---|
| `+9`, `#9` | Sharpened degree |
| `-5`, `b5` | Flattened degree |
| `add+11`, `addb9`, `addF#`, `addBb` | Added note |
| `sus4`, `sus9` | Suspension |
| `omit3`, `no3` | Omission |

**Modal chords:** `ionian`, `dorian`, `phrygian`, `lydian`, `mixolydian`, `aeolian`, `locrian`, `melodic minor`, `harmonic minor`, `whole tone`, `diminished half whole` / `diminished semitone tone`, `octatonic half whole` / `octatonic semitone tone`, `diminished whole half` / `diminished tone semitone`, `octatonic whole half` / `octatonic tone semitone`.

**Other**

| Entry | Meaning |
|---|---|
| `Cmajb5/Eb`, `Cmajb5,Eb` | Altered bass |
| `Cmaj7\|D`, `Cmaj7;D` | Polychord |
| `N.C.`, `NC`, `no chord`, `none` | No chord |
| `reg` | Chord symbol region |
| `(Cmaj7)` | Parenthesized |

**Popover controls**
- Opt-L / Opt-G: popover set to local / global.
- Up / Down: chord-symbol line.
- MIDI or Keyboard panel input is accepted.
- Space: next beat. Return: input. Opt-Return: opposite local/global for this entry.
- Opening this popover stops note input.

### Repeats and tremolos (Shift-R)

Pages: [P]`dorico_popovers_repeats_*_r.html`

**Endings:** `end` or `ending` adds a whole ending. The first segment covers the selected bars; a second segment is created automatically, plus an end repeat barline. `add` adds another segment.

**Markers**

| Entry | Marker |
|---|---|
| `da capo`, `dc`, `D.C.` | D.C. |
| `DC al Fine`, `dcalf`, `D.C. al Fine` | D.C. al Fine |
| `DC al Coda`, `dcalc`, `D.C. al Coda` | D.C. al Coda |
| `dal segno`, `ds`, `D.S.` | D.S. |
| `DS al Fine`, `dsalf`, `D.S. al Fine` | D.S. al Fine |
| `DS al Coda`, `dsalc`, `D.S. al Coda` | D.S. al Coda |
| `to coda`, `toc`, `tc` | to Coda |
| `segno`, `seg`, `s` | Segno. The procedure page also gives `$`: [M]`write_mode/write_mode_notations_input/write_mode_repeat_markers_inputting_popover_t.html` |
| `fine`, `fin`, `f` | Fine |
| `coda`, `co`, `c` | Coda |

End-of-section markers (Fine, D.C. al Coda and so on) right-align to the position.

**Single-note tremolos**

| Entry | Meaning |
|---|---|
| `/`, `\`, `1` | 1 stroke |
| `//`, `\\`, `2` | 2 strokes |
| `///`, `\\\`, `3` | 3 strokes |
| `////`, `\\\\`, `4` | 4 strokes |
| `z`, `zonstem` | Buzz roll (Z on stem) |
| `//rel`, `//r`, `2rel`, `2r` | Tremolo with release |
| `//att`, `//a`, `2att`, `2a` | Tremolo with attack |
| `0`, `clear` | Remove all tremolos |

**Multi-note tremolos**

| Entry | Meaning |
|---|---|
| `/2`, `\2`, `12` | 1 stroke |
| `//2`, `\\2`, `22` | 2 strokes |
| `///2`, `\\\2`, `32` | 3 strokes |
| `////2`, `\\\\2`, `42` | 4 strokes |
| `0`, `clear` | Remove all tremolos |

**Regions**

| Entry | Region |
|---|---|
| `slash` | Slash region |
| `number bars`, `numbars`, `num`, `nb` | Numbered bar region |
| `%1` or `%` | Repeat last bar |
| `%2` / `%4` | Repeat last 2 / 4 bars |
| `%1,2` / `%1,4` | Repeat last bar, grouped in 2 / 4 |
| `%2,4` | Repeat last 2 bars, grouped in 4 |

### Tempo (Shift-T)

Pages: [P]`dorico_popovers_tempo_*_r.html`

**General**
- A suggestion menu appears as you type; free text is allowed.
- `(q=72)` = parenthesized. `[q=96]` = hidden.
- During note input a gradual change gets a fixed quarter-note length and does not extend. With a range selected, it spans the range.
- Picking a suggestion syllabifies gradual changes (rit-ar-dan-do).

**Absolute tempo:** `Adagio`, `Allegro`, `più`, `meno`, `Rubato`, `Presto q=176` (text plus metronome mark).

**Metronome marks:** `q=176`; `(q=52)`; range `q=96-112`.

**Gradual changes:** `rit.`, `ritardando`, `rit....` (with continuation line); `rall.`, `rallentando`, `rall....`; `accel.`, `accelerando`, `accel....`.

**Relative changes:** `Mosso`, `Lento`.

**Tempo equations:** `e=e.`, `q=e`, `q.=h`. Tuplet durations are not supported yet.

**Rhythmic feel (swing)**

| Entry | Feel |
|---|---|
| `Light swing 16ths` / `Light swing 8ths` | Light swing |
| `Medium swing 16ths` / `Medium swing 8ths` | Medium swing |
| `Heavy swing 16ths` / `Heavy swing 8ths` | Heavy swing |
| `Straight (no swing)` | Straight |
| `2:1 swing 16ths (fixed)` / `2:1 swing 8ths (fixed)` | Triplet, fixed |
| `3:1 swing 16ths (fixed)` / `3:1 swing 8ths (fixed)` | Dotted, fixed |

Opt-Return applies a rhythmic feel to one instrument only.

### Beat units

Shared by the tempo, tuplets and bars popovers. Page: [P]`dorico_popovers_beat_units_r.html`

| Unit | Entries |
|---|---|
| 64th | `z`, `2` |
| 32nd | `y`, `3` |
| 16th | `x`, `4` |
| Eighth | `e`, `5` |
| Quarter | `q`, `6` |
| Half | `h`, `7` |
| Whole | `w`, `8` |
| Double whole | `2w`, `9` |
| Dot | `.` |

### Tuplets (;)

Page: [P]`dorico_popovers_tuplets_r.html`

| Entry | Tuplet |
|---|---|
| `3`, `3:2`, `3/2`, `3 2` | Triplet (3 in the time of 2) |
| `3:4`, `3/4`, `3 4` | 3 in the time of 4 |
| `4:3`, `4/3`, `4 3` | Quadruplet |
| `5:2` / `5:4` / `5:6` (and the `/` and space forms) | Quintuplets |
| `6:4` | Sextuplet |
| `7:4` | Septuplet |
| `9:8` | Nonuplet |
| `2:3` | Duplet |
| `5:4q.`, `5/4q.`, `5 4q.`, `5:4-6.`, `5:4 6.` | Quintuplet of dotted quarters |

- `3:2e` = eighth-note triplets. `5:4x` = 16th-note quintuplets.
- A numeric beat unit must be separated from the ratio by a space or hyphen.
- Without a unit, the selected duration is used.
- Input continues as tuplets until `:`, an arrow-key caret move, or the end of note input.
- Opening another tuplet without stopping the first one creates a nested tuplet.

### Cues (Shift-U, Pro only)

Page: [P]`dorico_popovers_cues_r.html`
- Type to filter source instruments; a candidate menu appears below the popover.
- Choose with the arrow keys, then Return.
- A full instrument name typed by hand needs Return twice.

### MIDI trigger regions (Shift-0)

Page: [P]`dorico_popovers_midi_trigger_regions_r.html`

| Entry | Meaning |
|---|---|
| `C0`, `Db1`, `F#3` | Notes by name; follows the middle-C numbering preference |
| `12`, `60` | MIDI note numbers |
| `C4,E4,G4`, `60,64,67` | Chords |
| `(0)`–`(127)` after the last note | Velocity, e.g. `C4,E4,G4(112)` |

A MIDI keyboard can also be played into this popover.

### Text (Shift-X / Shift-Opt-X): text editor, not a popover

Pages: [M]`write_mode/write_mode_notations_input/write_mode_text_adding_t.html`, `..._text_editing_t.html`, `..._text_editor_options_pop_up_r.html`

| Key | Action |
|---|---|
| Shift-X | Staff text |
| Shift-Opt-X | System text |
| Return | Line break inside the text |
| `XXXX` then Opt-X | Insert a Unicode character by hex code |
| Cmd-B / Cmd-I / Cmd-U | Bold / italic / underline |
| Cmd-Shift-. / Cmd-Shift-, | Font size up / down |
| **Esc or Cmd-Return** | Commit and close |
| Return or double-click on an existing text item | Edit it |

---

## Gaps and conflicts relevant to popovers (not in the 6.2 manual)

- **Popover error or invalid-input visuals and generic Esc behavior.** Undocumented. The manual only shows Esc closing the lyrics popover and the text editor.
- **Alt/Opt + number for intervals.** Not in the 6.2 manual; the documented route is Shift-I followed by an entry such as `3` or `-4`. Older Dorico versions had Alt-number key commands.
- **Same letter, different modes.** Shift-I (instrument picker), Shift-P (add player), Shift-F (add flow; frame break in Engrave), Shift-L (new score layout) are Setup-mode actions; Shift-S is system break in Engrave mode. A clone needs per-mode key contexts so these popover letters stay Write-mode only.
- **Shift-T for tempo.** B0's verified-letter list from the research did not include a row for tempo; the tempo popover section ("Tempo (Shift-T)") gives the letter. Confirm in the app's key command map.
