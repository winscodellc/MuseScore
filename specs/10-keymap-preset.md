# 10 - Keymap preset (Dorico key commands replace MuseScore's)
Status: todo  -  Depends on: 00  -  Dorico refs: key conventions <https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/introduction/key_command_conventions_c.html>, key commands concept `.../program_concepts/program_concepts_key_commands_in_dorico_r.html`; per-row sources are in [reference/dorico-key-commands.md](reference/dorico-key-commands.md)

## Goal
Ship a built-in shortcut preset `shortcuts_dorico`, selected automatically on first run, that:
1. binds every Dorico key that has an **existing** MuseScore command to that command;
2. **unbinds** every MuseScore default that Dorico uses for something else, or reserves for something not built yet;
3. leaves a documented, machine-checked list of keys that later specs bind to new `dorico/...` commands.

Specs 11-13, 20-46 add their own rows to the same JSON file; this spec owns the file, the plumbing and the checker.

## Dorico behaviour
Notation used below (macOS, US English):

| Dorico says | JSON sequence (Qt on macOS) |
|---|---|
| Cmd | `Ctrl` |
| Opt | `Alt` |
| Ctrl (the physical Control key) | `Meta` (verify, see Design 6) |
| Shift | `Shift` |

Rule of thumb from the manual: arrows alone move the selection; **Opt** modifies selected items (up/down = pitch by staff position, left/right = move rhythmically); **Shift-Opt** changes duration; **Cmd** multiplies the amount (octave, double); **Shift+letter** opens popovers; **Cmd-Shift+letter** opens options dialogs.

### Key table
Columns: key | Dorico action | target | owner | MuseScore binding this displaces. Target `notation/x` means `command://notation/x`. **Owner `10`** = bound by this spec. **`reserved -> NN`** = this spec unbinds the key and spec NN binds it to a new `dorico/...` command. **`deferred`** = Dorico feature not built; the key is left unbound on purpose (reserved).

#### A. Application, window, view
| Key | Dorico action | Target | Owner | Displaces |
|---|---|---|---|---|
| Cmd-Z | Undo | `notation/undo` | 10 (already) | |
| Cmd-Shift-Z, Cmd-Y | Redo | `notation/redo` | 10 (adds Cmd-Y) | |
| Cmd-, | Preferences | `app/preferences` | 10 (already) | |
| Cmd-N / O / S / Shift-S / W / P / Q | New / open / save / save as / close / print / quit (standard) | `project/*`, `app/quit` | 10 (already) | |
| Cmd-I | Project Info | | deferred | `notation/toggle-insert-mode` (Ctrl+I) |
| Cmd-T / Cmd-Shift-T | New tab / new project window | | deferred | `notation/add-staff-text` (Ctrl+T), `notation/add-system-text` (Ctrl+Shift+T) |
| Cmd-1..5 | Setup / Write / Engrave / Play / Print mode | | deferred | `notation/add-duplet`..`add-quintuplet` (Ctrl+2..5) |
| Cmd-6 | Toolbar | | deferred | `notation/add-sextuplet` (Ctrl+6) |
| Cmd-Opt-1 | Page view | `notation/view/mode-page` | 10 | `notation/use-voice-1` |
| Cmd-Opt-2 | Galley view | `notation/view/mode-continuous` (closest) | 10 | `notation/use-voice-2` |
| Cmd-Opt-3 | Fill view | `notation/view/mode-float` (closest) | 10 | `notation/use-voice-3` |
| Cmd-= , Z | Zoom in | `notation/view/zoom-in` | 10 | |
| Cmd-- , X | Zoom out | `notation/view/zoom-out` | 10 | `notation/flip` (X) |
| Cmd-0 | Hide/restore all zones | | deferred | `notation/view/zoom-to-100` |
| Cmd-Shift-L | Layout Options | `notation/open-page-settings` (closest) | 10 | |
| Cmd-Shift-N, Cmd-Shift-E | Notation / Engraving Options | `notation/open-edit-style` (closest) | 10 | `notation/insert-note-e` (Ctrl+Shift+E) |
| Cmd-Shift-I | Note Input Options | | deferred | `notation/toggle-note-input-repitch` (Ctrl+Shift+I) |
| Cmd-Shift-P / V | Playback / View Options | | deferred | |
| Home / PgUp | Previous page | `notation/view/prev-page` | 10 | `notation/view/top-of-first-page` (Home), `prev-screen` (PgUp) |
| End / PgDown | Next page | `notation/view/next-page` | 10 | `notation/view/bottom-of-last-page` (End), `next-screen` (PgDown) |
| Cmd-Home / Cmd-End | First / last page | `notation/view/top-of-first-page` / `bottom-of-last-page` | 10 | `notation/goto-first-element` / `goto-last-element` |
| Opt-T | Show/hide system track | | deferred | |
| J | Jump bar | | reserved -> 46 | `notation/enharmonic-spell-both` (J) |
| Ctrl-1 / Ctrl-2 (physical Control) | Jump bar Commands / Go To mode | | reserved -> 46 | |
| Cmd-G | Go To Bar | | reserved -> 46 | `notation/add-figured-bass` (Ctrl+G) |
| W | Counterpart layout (score/part) | | deferred | `notation/set-double-duration` (W) |
| Shift-Opt-] / [ | Next / previous layout | | deferred | |
| Cmd-R | MIDI record | | deferred | `notation/reset-shapes-and-positions` (Ctrl+R) |
| Opt-S / Shift-Opt-S | Solo / clear solo | | deferred | `notation/make-into-system` (Alt+S) |
| Opt-H | Switch click-drag tool | | deferred | `notation/add-hammer-on-pull-off` (Alt+H) |

#### B. Panels
Dorico's zones are mapped by meaning, not position: left zone = Notes panel, right zone = Notations panels, lower zone = Properties.

| Key | Dorico action | Target | Owner | Displaces |
|---|---|---|---|---|
| Cmd-7 | Left zone (Notes panel) | `app/dock/toggle-noteinput` | 10 | |
| Cmd-8 | Lower zone (Properties) | `app/dock/toggle-properties` (F8 stays) | 10 | |
| Cmd-9 | Right zone (Notations) | `app/dock/toggle-palettes` | 10 | F9 |
| F3 | Mixer | `app/dock/toggle-mixer` (F10 stays) | 10 | |
| F2 / F4 | Transport / Video window | | deferred | |
| F7 / F9 | Rewind / fast-forward | | deferred | `app/dock/toggle-instruments` (F7), `toggle-palettes` (F9) |
| Opt-8 | Focus Properties search | | deferred | |

#### C. Playback
| Key | Dorico action | Target | Owner | Displaces |
|---|---|---|---|---|
| P | Play from selection (also stops) | `playback/play-selection` | 10 | `app/dock/toggle-piano-keyboard` (P) |
| Space | Play/stop from playhead (outside note input; advances caret during note input) | | reserved -> 11 | `playback/play-toggle` (Space) |
| Shift-Space | Play from last start position | | deferred | `playback/play-selection` (Shift+Space) |
| Shift-Opt-Space, Opt-Space | Play from flow start / scrub | | deferred | `playback/pause-and-select` (mac: Alt+Space) |
| Num 0 / Num + / Num - / Num . | Stop / fast-forward / rewind / to start | | deferred | |

#### D. Note input: start, stop, modes
| Key | Dorico action | Target | Owner | Displaces |
|---|---|---|---|---|
| Shift-N | Start / stop note input | `notation/toggle-note-input` | 10 | `notation/toggle-tenuto` (Shift+N) |
| Return | Start note input on a note/rest; open popover for a selected item; stop note input | | reserved -> 11 | `notation/toggle-system-break` (Return/Enter), `notation/realtime-advance` (Enter, note-input scope) |
| Esc | Stop note input / deselect | `notation/cancel` | 10 (already) | |
| K | Pitch Before Duration | | reserved -> 11 | |
| L | Lock Duration (repitch) | | reserved -> 11 | |
| I | Insert mode | `notation/toggle-insert-mode` (11 adds the Q-exclusion) | 10 | `instruments/select` (I) |
| Q | Chord mode | | reserved -> 11 | `notation/set-halve-duration` (Q) |
| O | Force Duration | | reserved -> 11 | `app/dock/toggle-percussion` (O) |
| V / Shift-V | Next voice / new voice | | reserved -> 11 | `notation/toggle-visible` (V), `notation/toggle-sforzato` (Shift+V) |
| Space | Advance caret | | reserved -> 11 | (see C) |
| , | Rest input on/off | `notation/toggle-rest` | 10 | |
| A-G | Input pitch (only during note input; F outside input = flip) | | reserved -> 11 | `notation/enter-note-a`..`g` stay until 11 replaces them with branching `dorico/pitch-*` |
| Shift-Opt-A..G | Input note above previous | | reserved -> 11 | |
| Ctrl(phys)-A..G | Input note below previous | | reserved -> 11 | |
| N / M | Cross-staff note to staff above / below | `notation/move-up` / `notation/move-down` | 10 | `notation/toggle-note-input-by-note-name` (N), `...-by-duration` (M) |
| Opt-N / Opt-M | Move selection to staff above / below (cut + paste) | | reserved -> 41 | |

#### E. Durations, dots, ties, tuplets, grace notes
**Dorico digits are MuseScore's digits + 1** (MuseScore: `5` = quarter, `1` = 64th, `9` = longa). Dorico: `6` = quarter. Evidence: manual says 4 = 16th, 5 = eighth, 6 = quarter, 7 = half; its beat-unit table has 2 = 64th, 3 = 32nd, 8 = whole, 9 = double whole. `1` = 128th is inferred (manual never names it). The earlier internals note that MuseScore's digits "already match" is wrong; every duration command is rebound.

| Key | Dorico action | Target | Owner | Displaces |
|---|---|---|---|---|
| 1 | 128th | `notation/set-duration-128th` | 10 | `set-duration-64th` |
| 2 | 64th | `notation/set-duration-64th` | 10 | `set-duration-32nd` |
| 3 | 32nd | `notation/set-duration-32nd` | 10 | `set-duration-16th` |
| 4 | 16th | `notation/set-duration-16th` | 10 | `set-duration-eighth` |
| 5 | eighth | `notation/set-duration-eighth` | 10 | `set-duration-quarter` |
| 6 | quarter | `notation/set-duration-quarter` | 10 | `set-duration-half` |
| 7 | half | `notation/set-duration-half` | 10 | `set-duration-whole` |
| 8 | whole | `notation/set-duration-whole` | 10 | `set-duration-breve` |
| 9 | double whole | `notation/set-duration-breve` | 10 | `set-duration-longa` (loses its key) |
| same digit twice quickly | dotted | | reserved -> 11 | |
| `.` | Toggle rhythm dot | `notation/toggle-dot` | 10 (already) | |
| Opt-. | Cycle 1-4 dots | | reserved -> 11 | `notation/increase-dynamic` (Alt+.) |
| `0` (and Num 0) | Natural accidental | `notation/toggle-nat` | 10 | `notation/enter-rest` (0, Num+0) |
| `-` | Flat | `notation/toggle-flat` | 10 (already) | |
| `=` | Sharp | `notation/toggle-sharp` | 10 | `notation/toggle-nat` (=) |
| T | Tie | `notation/toggle-tie` | 10 (already) | |
| U / Opt-U | Scissors / split by grid | | reserved -> 13 | |
| `/` | Toggle grace-note input | | reserved -> 11 (`notation/add-acciaccatura` stays until then) | |
| Opt-/ | Slashed/unslashed grace | | deferred | |
| `;` | Tuplets popover | | reserved -> 31 | `notation/enter-rest-tab` (`;`, TAB only) |
| `:` | Stop tuplet input | | reserved -> 31 | |

#### F. Articulations (Dorico's table is verbatim from the manual)
| Key | Articulation | Target | Owner | Displaces |
|---|---|---|---|---|
| `[` | Accent | `notation/toggle-sforzato` (verify it toggles `articAccentAbove` in `notationactioncontroller.cpp`) | 10 | |
| `'` | Marcato | `notation/toggle-marcato` | 10 | Shift+O |
| `]` | Staccato | `notation/toggle-staccato` | 10 | Shift+S |
| `\` | Tenuto | `notation/toggle-tenuto` | 10 | Shift+N |
| `{` | Stressed | | reserved -> 11 | `notation/stretch-decrease` (`{`) |
| `}` | Staccatissimo | | reserved -> 11 | `notation/stretch-increase` (`}`) |
| `"` | Unstressed | | reserved -> 11 | |
| `\|` | Tenuto + staccato | | reserved -> 11 | |
| S | Start slur | `notation/add-slur` | 10 (already) | |
| Shift-S | End slur (note input) | | deferred | `notation/toggle-staccato` (Shift+S) |
| `<` / `>` | Crescendo / diminuendo hairpin | `notation/add-hairpin` / `add-hairpin-reverse` | 10 (mac diff already maps to Shift+, / Shift+.) | |
| Shift-Opt-, / Shift-Opt-. | Messa di voce | | deferred | |

#### G. Pitch editing
| Key | Dorico action | Target | Owner | Displaces |
|---|---|---|---|---|
| Opt-Up / Opt-Down | Up/down one staff position | `notation/pitch-up-diatonic` / `pitch-down-diatonic` | 10 | `goto-upnote-in-chord` / `goto-downnote-in-chord` (Alt+Up/Down), `move-measure-to-prev/next-system` (Alt+Up/Down), old `pitch-up-diatonic` key (Alt+Shift+Up) |
| Shift-Opt-Up / Down | Up/down one octave division (semitone) | `notation/pitch-up` / `pitch-down` | 10 | (both lose bare Up/Down) |
| Cmd-Opt-Up / Down | Up/down an octave | `notation/pitch-up-octave` / `pitch-down-octave` | 10 | `goto-topnote-in-chord` / `goto-bottomnote-in-chord` (Ctrl+Alt+Up/Down), old octave keys (Ctrl+Up/Down) |
| Opt-= / Opt-- | Respell using note name above / below | | reserved -> 12 | `notation/add-beam-selected-range` (Alt+=) |
| F | Flip stem/placement (outside note input; pitch F inside) | | reserved -> 11 | `notation/flip` (X) |

#### H. Selection and navigation (full semantics in 12 and 44)
| Key | Dorico action | Target | Owner | Displaces |
|---|---|---|---|---|
| Right / Left | Next / previous item in voice | `notation/move-right` / `move-left` | 10 (already) | |
| Up / Down | Closest note above / below (then staff above/below) | | reserved -> 12 | `notation/pitch-up` / `pitch-down` (Up/Down) |
| Cmd-Right / Cmd-Left | Start of next / previous bar | `notation/move-right-quickly` / `move-left-quickly` | 10 (already) | |
| Cmd-Up / Cmd-Down | Top / bottom staff of system | | reserved -> 12 | `pitch-up-octave` / `pitch-down-octave` (Ctrl+Up/Down) |
| Tab / Shift-Tab | Cycle items at position | | deferred | |
| Shift-Right / Left | Extend selection one item | `notation/add-to-selection-next-chord` / `prev-chord` | 10 (already) | `notation/swap-right` / `swap-left` (note-input scope) |
| Cmd-Shift-Right / Left | Extend to end / start of bar | `notation/add-to-selection-next-measure` / `prev-measure` | 10 (already) | |
| Shift-Up / Shift-Down | Add staff above / below to selection | `notation/add-to-selection-above-staff` / `below-staff` | 10 (already) | |
| Cmd-A | Select all (flow) | `notation/select-all` | 10 (already) | |
| Cmd-Shift-A | Select More | | reserved -> 44 | `notation/insert-note-a` (Ctrl+Shift+A) |
| Cmd-D | Deselect all | | reserved -> 44 | `notation/add-dynamic` (Ctrl+D) |

#### I. Editing
| Key | Dorico action | Target | Owner | Displaces |
|---|---|---|---|---|
| Cmd-C / X / V | Copy / cut / paste | `notation/copy` / `cut` / `paste` | 10 (already; 40 rebinds paste) | |
| Backspace / Delete | Delete | `notation/delete` | 10 (already; 43 refines) | |
| R | Repeat selection | `notation/repeat-selection` | 10 (already; 42 refines) | |
| Opt-Right / Opt-Left | Move rhythmically (nearest position) | | reserved -> 13 | `notation/goto-next-element` / `goto-prev-element` (Alt+Right/Left) |
| Cmd-Opt-Right / Left | Move rhythmically (grid) | | reserved -> 13 | |
| Shift-Opt-Right / Left | Lengthen / shorten by grid | | reserved -> 13 | |
| Cmd-Shift-Opt-Right / Left | Double / halve duration | | reserved -> 13 | `notation/goto-next-segment-element` / `goto-prev-segment-element` (Ctrl+Alt+Shift+Right/Left) |
| Opt-[ / Opt-] | Rhythmic grid finer / coarser | | reserved -> 12 | |
| Cmd-Opt-N/M, Shift-Opt-V, Shift-Opt-I, Opt-I, Opt-Q ... | various | | deferred | |

#### J. Popovers and text (bound by specs 20-36; this spec only clears MuseScore's Shift+letter bindings)
| Key | Dorico | Target | Owner | Displaces |
|---|---|---|---|---|
| Shift-X | Staff text (text editor) | `notation/add-staff-text` | 10 | `notation/mirror-notehead` (Shift+X) |
| Shift-Opt-X | System text | `notation/add-system-text` | 10 | |
| Shift-A | Rehearsal mark (direct) | `notation/add-rehearsalmark-text` (Ctrl+M stays) | 10 | `notation/add-note-a` (Shift+A) |
| Shift-B | Bars and barlines popover | | reserved -> 25 | `notation/add-note-b` (Shift+B, Shift+H) |
| Shift-C | Clefs / octave lines | | reserved -> 24 | `notation/add-note-c` |
| Shift-D | Dynamics | | reserved -> 21 | `notation/add-note-d` |
| Shift-F | Fingering | | reserved -> 34 | `notation/add-note-f` |
| Shift-G | Figured bass | | reserved -> 36 | `notation/add-note-g` |
| Shift-H | Holds and pauses | | reserved -> 27 | (Shift+H alias of Shift+B above) |
| Shift-I | Note tools | | reserved -> 35 | |
| Shift-K | Key signatures | | reserved -> 23 | |
| Shift-L | Lyrics | | reserved -> 33 | `notation/get-location` (Shift+L) |
| Shift-M | Time signatures | | reserved -> 22 | |
| Shift-O | Ornaments | | reserved -> 28 | `notation/toggle-marcato` (Shift+O) |
| Shift-P | Playing techniques | | reserved -> 29 | |
| Shift-Q | Chord symbols | | reserved -> 32 | `notation/set-halve-duration-dotted` (Shift+Q) |
| Shift-R | Repeats and tremolos | | reserved -> 30 | |
| Shift-T | Tempo | | reserved -> 26 | |
| Shift-U | Cues | | deferred | |
| Shift-E / Shift-0 | Ensemble picker / MIDI trigger regions | | deferred | |
| Shift-W | (none in Dorico) | | unchanged | `notation/set-double-duration-dotted` keeps Shift+W |

MuseScore-only bindings that Dorico does **not** use stay (examples): Ctrl+B, Ctrl+K, Ctrl+E, Ctrl+L, Ctrl+F, Ctrl+M, Alt+Shift+T, Alt+Shift+B, Alt+1..9 / Alt+Shift+2..9 (add interval), Alt+W, Alt+B, Alt+X, Alt+L, Ctrl+Shift+Up/Down, Ctrl+Shift+Q/W/X, Ctrl+Shift+M, Ins, Ctrl+Ins, Ctrl+Return, Alt+Return, `(`, `F8`, `F10`, `F11`, `F12`, `Ctrl+F9`, `Shift+F9`, Ctrl+J, H (alias of B).
Note: Dorico 6.2's manual documents no Opt+digit interval keys (older Dorico versions had them); MuseScore's Alt+digit stay as extras, and Shift-I (spec 35) is the Dorico route.

## MuseScore today
- **Which system is live**: Shortcuts **v2** (`CMakeLists.txt:198`, `set(MUSE_MODULE_SHORTCUTS_V2 ON)`). The `.xml` files in `src/app/configs/data/` are dead under v2.
- **Files**: `src/app/configs/data/shortcuts.json` (280 `command://` entries; scopes `NAVIGATION` L3, `NOTATION_PAGE` L21, `NOTATION` L55, `NOTATION_NOTE_INPUT` L273, `NOTATION_TEXT_EDITING` L282, `PROJECT` L295, `APP` L306, `PLAYBACK` L315), `shortcuts_mac.json` (3 entries: `add-hairpin` -> `Shift+,`, `add-hairpin-reverse` -> `Shift+.`, `playback/pause-and-select` -> `Alt+Space`), `shortcuts_azerty.json`. Index file: `src/app/configs/shortcuts.cfg`; resources: `src/app/app.qrc:10-16`.
- **Load order** (`muse/framework/shortcuts_v2/internal/commandshortcutsregister.cpp:74` `reload()`): base file -> platform diff (`shortcuts_mac`, chosen at compile time in `shortcutsconfiguration.cpp:66-82`) -> current preset diff -> every registered command that has no binding gets an empty entry -> user file `~/Library/Application Support/.../shortcuts/<preset>.json` (only differences).
- **Preset path lookup**: `ShortcutsConfiguration::commandShortcutsAppDataPath(name)` returns `m_config.value("command_" + name)`. So for preset `shortcuts_dorico` the cfg key must be `command_shortcuts_dorico`. The presets list is `command_shortcuts_presets` (`["shortcuts_azerty"]`). Current preset: settings key `shortcuts/currentPreset`, API `IShortcutsConfiguration::setCurrentShortcutsPresetName(name)` (`ishortcutsconfiguration.h`).
- **Diff semantics**: a diff entry replaces the whole `sequences` list of that command; `"sequences": []` unbinds; a default sequence reused by a diff entry in the **same scope** is removed from other commands (`commandshortcutsregister.cpp:249-255`); clashes across scopes are kept. `canShortcutsConflict` is `scope1 == scope2` (`shortcutstypes.h:126`).
- **Dispatch**: `ShortcutsController::activate` (`shortcutscontroller.cpp:45-75`) keeps only commands whose state is enabled; if several remain, `ShortcutResolver::selectOne` (`src/context/internal/shortcutresolver.cpp:86`) picks by scope priority. Scope never disables a command.
- **Gotchas**: `Shortcut::sequencesFromString` splits on `", "` (`shortcutstypes.h:70`), so multi-chord sequences are not supported; the Preferences import/export dialog filter is still `*.xml` (`shortcutsmodel.cpp:40`, a bug; not ours to fix here).

## Design

### 1. Files
- **New** `src/app/configs/data/shortcuts_dorico.json` (valid JSON; comments go in `"_comment"` keys like `shortcuts.json:278`).
- **Edit** `src/app/configs/shortcuts.cfg`: add `"command_shortcuts_dorico": ":/configs/data/shortcuts_dorico.json"` and make the presets list `["shortcuts_azerty", "shortcuts_dorico"]`.
- **Edit** `src/app/app.qrc`: add `<file>configs/data/shortcuts_dorico.json</file>` after L16.
- **New** `src/dorico/tools/check_keymap.py` and `src/dorico/tools/dorico_keys.json` (Design 5).
These are the only upstream files touched (two tiny edits). The preset becomes visible in Preferences > Shortcuts as a built-in, non-deletable preset (`canDeletePreset`, `shortcutsconfiguration.cpp:610`).

### 2. What this spec writes into `shortcuts_dorico.json`
Entries go under the **same scope the command has in the base file** (so the merge cannot create a duplicate in another scope). Rows marked `10` in the table above, concretely:

```jsonc
[
 { "scope": "NOTATION", "shortcuts": [
  // durations: Dorico digit = MuseScore digit + 1
  {"command":"command://notation/set-duration-128th","sequences":["1","Num+1"]},
  {"command":"command://notation/set-duration-64th", "sequences":["2","Num+2"]},
  {"command":"command://notation/set-duration-32nd", "sequences":["3","Num+3"]},
  {"command":"command://notation/set-duration-16th", "sequences":["4","Num+4"]},
  {"command":"command://notation/set-duration-eighth","sequences":["5","Num+5"]},
  {"command":"command://notation/set-duration-quarter","sequences":["6","Num+6"]},
  {"command":"command://notation/set-duration-half",  "sequences":["7","Num+7"]},
  {"command":"command://notation/set-duration-whole", "sequences":["8","Num+8"]},
  {"command":"command://notation/set-duration-breve", "sequences":["9","Num+9"]},
  {"command":"command://notation/set-duration-longa", "sequences":[]},
  // accidentals / rest
  {"command":"command://notation/toggle-sharp","sequences":["=","+","Num++"]},
  {"command":"command://notation/toggle-nat",  "sequences":["0","Num+0"]},
  {"command":"command://notation/enter-rest",  "sequences":[]},
  {"command":"command://notation/toggle-rest", "sequences":[","]},
  // articulations
  {"command":"command://notation/toggle-sforzato","sequences":["["]},
  {"command":"command://notation/toggle-marcato", "sequences":["'"]},
  {"command":"command://notation/toggle-staccato","sequences":["]"]},
  {"command":"command://notation/toggle-tenuto",  "sequences":["\\"]},
  {"command":"command://notation/stretch-increase","sequences":[]},
  {"command":"command://notation/stretch-decrease","sequences":[]},
  // pitch editing
  {"command":"command://notation/pitch-up-diatonic",  "sequences":["Alt+Up"]},
  {"command":"command://notation/pitch-down-diatonic","sequences":["Alt+Down"]},
  {"command":"command://notation/pitch-up",  "sequences":["Alt+Shift+Up"]},
  {"command":"command://notation/pitch-down","sequences":["Alt+Shift+Down"]},
  {"command":"command://notation/pitch-up-octave",  "sequences":["Ctrl+Alt+Up"]},
  {"command":"command://notation/pitch-down-octave","sequences":["Ctrl+Alt+Down"]},
  {"command":"command://notation/goto-upnote-in-chord","sequences":[]},
  {"command":"command://notation/goto-downnote-in-chord","sequences":[]},
  {"command":"command://notation/goto-topnote-in-chord","sequences":[]},
  {"command":"command://notation/goto-bottomnote-in-chord","sequences":[]},
  {"command":"command://notation/move-measure-to-prev-system","sequences":[]},
  {"command":"command://notation/move-measure-to-next-system","sequences":[]},
  {"command":"command://notation/goto-next-element","sequences":[]},
  {"command":"command://notation/goto-prev-element","sequences":[]},
  {"command":"command://notation/goto-next-segment-element","sequences":[]},
  {"command":"command://notation/goto-prev-segment-element","sequences":[]},
  {"command":"command://notation/goto-first-element","sequences":[]},
  {"command":"command://notation/goto-last-element","sequences":[]},
  // cross-staff, note input start, insert
  {"command":"command://notation/move-up",  "sequences":["Ctrl+Shift+Up","N"]},
  {"command":"command://notation/move-down","sequences":["Ctrl+Shift+Down","M"]},
  {"command":"command://notation/toggle-note-input","sequences":["Shift+N"]},
  {"command":"command://notation/toggle-note-input-by-note-name","sequences":[]},
  {"command":"command://notation/toggle-note-input-by-duration","sequences":[]},
  {"command":"command://notation/toggle-note-input-repitch","sequences":[]},
  {"command":"command://notation/toggle-insert-mode","sequences":["I"]},
  // clear MuseScore Shift+letter / letter bindings Dorico uses
  {"command":"command://notation/add-note-a","sequences":[]},  // ... add-note-b .. add-note-g likewise
  {"command":"command://notation/insert-note-a","sequences":[]},
  {"command":"command://notation/insert-note-e","sequences":[]},
  {"command":"command://notation/mirror-notehead","sequences":[]},
  {"command":"command://notation/get-location","sequences":[]},
  {"command":"command://notation/set-halve-duration","sequences":[]},
  {"command":"command://notation/set-halve-duration-dotted","sequences":[]},
  {"command":"command://notation/set-double-duration","sequences":[]},
  {"command":"command://notation/toggle-visible","sequences":[]},
  {"command":"command://notation/flip","sequences":[]},
  {"command":"command://notation/enharmonic-spell-both","sequences":[]},
  {"command":"command://notation/add-beam-selected-range","sequences":[]},
  {"command":"command://notation/add-dynamic","sequences":[]},
  {"command":"command://notation/increase-dynamic","sequences":[]},
  {"command":"command://notation/toggle-system-break","sequences":[]},
  {"command":"command://notation/add-hammer-on-pull-off","sequences":[]},
  {"command":"command://notation/make-into-system","sequences":[]},
  {"command":"command://notation/reset-shapes-and-positions","sequences":[]},
  {"command":"command://notation/enter-rest-tab","sequences":[]},
  {"command":"command://notation/add-duplet","sequences":[]},   // ... add-triplet .. add-nonuplet likewise (Ctrl+2..9)
  {"command":"command://notation/use-voice-1","sequences":[]}, {"command":"command://notation/use-voice-2","sequences":[]}, {"command":"command://notation/use-voice-3","sequences":[]},
  // text
  {"command":"command://notation/add-staff-text","sequences":["Shift+X"]},
  {"command":"command://notation/add-system-text","sequences":["Alt+Shift+X"]},
  {"command":"command://notation/add-rehearsalmark-text","sequences":["Ctrl+M","Shift+A"]},
  // history
  {"command":"command://notation/redo","sequences":["Ctrl+Shift+Z","Ctrl+Y"]},
  // view
  {"command":"command://notation/view/mode-page","sequences":["Ctrl+Alt+1"]},
  {"command":"command://notation/view/mode-continuous","sequences":["Ctrl+Alt+2"]},
  {"command":"command://notation/view/mode-float","sequences":["Ctrl+Alt+3"]},
  {"command":"command://notation/view/zoom-in","sequences":["Ctrl+=","Ctrl++","Z"]},
  {"command":"command://notation/view/zoom-out","sequences":["Ctrl+-","Ctrl+Shift+-","X"]},
  {"command":"command://notation/view/zoom-to-100","sequences":[]},
  {"command":"command://notation/view/prev-page","sequences":["Home","PgUp","Ctrl+PgUp"]},
  {"command":"command://notation/view/next-page","sequences":["End","PgDown","Ctrl+PgDown"]},
  {"command":"command://notation/view/prev-screen","sequences":[]},
  {"command":"command://notation/view/next-screen","sequences":[]},
  {"command":"command://notation/view/top-of-first-page","sequences":["Ctrl+Home"]},
  {"command":"command://notation/view/bottom-of-last-page","sequences":["Ctrl+End"]},
  // dialogs
  {"command":"command://notation/open-page-settings","sequences":["Ctrl+Shift+L"]},
  {"command":"command://notation/open-edit-style","sequences":["Ctrl+Shift+N","Ctrl+Shift+E"]},
  // Up/Down arrows: unbound here, spec 12 binds dorico/select-up|down
  // (pitch-up / pitch-down lose Up/Down by being listed above with new sequences)
  // Cmd-Up/Down likewise (spec 12)
  // Space, Return, A-G, F, Q, V, K, L, O, Shift+letter popovers: freed here, bound by 11/12/2x
 ]},
 { "scope": "NOTATION_NOTE_INPUT", "shortcuts": [
  {"command":"command://notation/realtime-advance","sequences":[]},
  {"command":"command://notation/swap-left","sequences":[]},
  {"command":"command://notation/swap-right","sequences":[]}
 ]},
 { "scope": "NOTATION_PAGE", "shortcuts": [
  {"command":"command://app/dock/toggle-noteinput","sequences":["Ctrl+7"]},
  {"command":"command://app/dock/toggle-properties","sequences":["Ctrl+8","F8"]},
  {"command":"command://app/dock/toggle-palettes","sequences":["Ctrl+9"]},
  {"command":"command://app/dock/toggle-mixer","sequences":["F3","F10"]},
  {"command":"command://app/dock/toggle-instruments","sequences":[]},
  {"command":"command://app/dock/toggle-piano-keyboard","sequences":[]},
  {"command":"command://app/dock/toggle-percussion","sequences":[]},
  {"command":"command://instruments/select","sequences":[]}
 ]},
 { "scope": "PLAYBACK", "shortcuts": [
  {"command":"command://playback/play-toggle","sequences":[]},
  {"command":"command://playback/play-selection","sequences":["P"]},
  {"command":"command://playback/pause-and-select","sequences":[]}
 ]}
]
```
Notes the implementer must verify before writing the final file (each is a one-line grep):
- `toggle-sforzato` really is the accent (`SymId::articAccentAbove`) in `src/notationscene/internal/notationactioncontroller.cpp`. If not, use the command that is, and add `dorico/toggle-articulation` for it in 11.
- Whether `"\\"`, `"'"`, `"["`, `"]"` parse as Qt key sequences. The base file already uses `{`, `}`, `(`, `:`, `;`, `/`, `<`, `>`, `.`, `+`, `=`, `-`, so most literal characters work. If `"\\"` or `"|"` or `"\""` do not, bind them as `Shift+...` forms or leave them to 11.
- The mac platform diff (`shortcuts_mac.json`) is applied **before** the preset, so the preset wins; it does not need editing.
- `Num+` variants: the digit rows keep `Num+n`. TAB-staff duration commands (`set-duration-*-tab`) also use `Num+n`; the same-scope reuse rule removes `Num+n` from them. TAB staves keep `Shift+n`, which is acceptable (Dorico also changes tablature digits to frets).

### 3. Preset selection on first run
In `DoricoModule::onInit` (global part, spec 00):
```cpp
// ponytail: runs once; user picks a different preset later and we never touch it again
if (!muse::settings()->value(DORICO_KEYMAP_APPLIED_KEY).toBool()) {
    shortcutsConfiguration()->setCurrentShortcutsPresetName("shortcuts_dorico");
    muse::settings()->setSharedValue(DORICO_KEYMAP_APPLIED_KEY, muse::Val(true));
}
```
where `shortcutsConfiguration` is `muse::GlobalInject<muse::shortcuts::IShortcutsConfiguration>` (header: `muse/framework/shortcuts_v2/ishortcutsconfiguration.h`) and the key is `Settings::Key("dorico", "dorico/keymapApplied")`. Look at `src/palette/internal/paletteconfiguration.cpp` for the exact `settings()` calls used in this codebase. Check that `CommandShortcutsRegister::reload()` runs **after** this write (module init order); if not, call `ICommandShortcutsRegister::reload()` once. Add a Preferences-free escape hatch: picking "Default" in Preferences > Shortcuts > preset menu restores MuseScore's keys.

### 4. macOS Option dead-key risk (must be tested, not assumed)
On macOS, Option+letter produces characters and some keys are *dead keys*: Opt-N (tilde), Opt-U (umlaut), Opt-I (circumflex), Opt-E (acute), Opt-` (grave); Opt-M gives `µ`, Opt-[ gives `"`, Opt-] gives `'`, Opt-= gives `≠`, Opt-- gives `-`, Opt-. gives `≥`, Opt-, gives `≤`, Opt-/ gives `÷`, Opt-S `ß`. Qt normally matches `QKeySequence("Alt+N")` on the key code, not the produced character, but dead keys can be swallowed by the text-input path.
Affected Dorico keys: Opt-N/M (41), Opt-U (13), Opt-I (out of scope), Opt-[ / Opt-] (12), Opt-= / Opt-- (12), Opt-. (11), Opt-/ (deferred), Opt-T/H/S/Q/L/X. Test procedure, one row per key, in the app, score focused, no text field active:
1. Add a temporary binding of the key to `command://dorico/noop` (spec 00) via the preset.
2. Press it; the log must show "dorico noop".
3. Record the result in a table appended to this spec (`## Results`). For any key that fails: choose an alternative (`Meta+Alt+key` or `Ctrl+Alt+key`), bind it **in addition** in the preset, and note it.
Opt+arrow, Opt+digit and Shift-Opt+arrow are not dead keys and are expected to work (MuseScore already ships `Alt+Up`, `Alt+1`).

### 5. The checker `src/dorico/tools/check_keymap.py`
Python 3 standard library only; run as `python3 -I src/dorico/tools/check_keymap.py` from the repo root. It must:
1. **Emulate the merge**: load `shortcuts.json`, apply `shortcuts_mac.json`, then `shortcuts_dorico.json`, replicating `applyShortcutsDiff` and the same-scope reuse rule. Read `commandshortcutsregister.cpp` L74-300 first and mirror it exactly (replace-by-command, `[]` unbinds, same-scope sequence reuse removes others).
2. **Conflict check**: normalise each sequence (case-insensitive modifiers, `Num+` kept distinct). Group commands per sequence across the scopes that can be active together: `NOTATION`, `NOTATION_PAGE`, `NOTATION_NOTE_INPUT`, `PLAYBACK`, `PROJECT`, `APP`. Report any sequence with >= 2 commands, except an allowlist of mutually exclusive-by-state families: `edittext-*`, `editlyrics-*`, `*-tab`, `enter-fret-*`, and `navigation/*` (different scope, handled by focus). Exit 1 on any other conflict.
3. **Coverage check** against `src/dorico/tools/dorico_keys.json`: a list `{ "key": "Ctrl+Alt+Up", "dorico": "Cmd-Opt-Up", "command": "command://notation/pitch-up-octave" | "reserved:NN" | "deferred", "owner": "10" }`, generated by hand from the table above (every row). For `command` entries: the merged map must bind exactly that key to exactly that command. For `reserved:*` and `deferred`: the key must be **unbound** (no MuseScore command) until the owner spec lands; once a spec lands it changes its rows to concrete `dorico/...` commands.
4. **Unknown command check**: every `command://...` string in the preset must be defined in `src/notationscene/notationcommands.h`, `src/palette/palettecommands.h`, `src/dorico/doricocommands.h`, or listed in the base `shortcuts.json`. Unknown `command://dorico/*` whose owner spec is not `done` yields a warning only (pending).
5. Print a summary: keys bound, keys reserved, keys deferred, conflicts (must be 0), and a list of Dorico keys in `dorico_keys.json` that are still unmapped.

## Deferred / unsupported
See the `deferred` rows in the table. Also: Tab / Shift-Tab item cycling, Opt-Space scrubbing, Shift-Space "play from last start", numeric-keypad transport keys, hide-all-zones, per-mode letter meanings, Windows key set, Dorico's per-language key command files (only US English is specced).

## Acceptance criteria
- [ ] `python3 -I src/dorico/tools/check_keymap.py` exits 0 with 0 conflicts and every `owner: 10` row bound exactly as in the table.
- [ ] A fresh profile (delete the app's settings/shortcuts dir or use `-profile` if available) starts with preset `shortcuts_dorico` active (Preferences > Shortcuts shows it selected); choosing the default preset restores MuseScore's keys and Dorico's is not re-applied on restart.
- [ ] Manual, score open, note selected: `6` makes quarter notes (not half), `5` eighths, `8` whole, `9` double whole, `1` 128th; `0` toggles natural, `=` sharp, `-` flat; `,` rest; `]` staccato, `[` accent, `'` marcato, `\` tenuto.
- [ ] Opt-Up/Down moves a note by a step, Shift-Opt-Up/Down by a semitone, Cmd-Opt-Up/Down by an octave; Opt-Right/Left no longer jumps selection (reserved for 13).
- [ ] Shift-N starts and stops note input; Esc stops it; `N` / `M` push a note to the staff above / below.
- [ ] `P` plays from the selection; Cmd-Opt-1/2/3 switch page / continuous / float view; Cmd-7/8/9 and F3 toggle panels; Cmd-= / Cmd-- / Z / X zoom.
- [ ] Shift-X opens staff text, Shift-Opt-X system text, Shift-A rehearsal mark.
- [ ] No Shift+letter, bare letter or Space still performs a MuseScore action that Dorico assigns elsewhere (spot-check: Shift-D does nothing yet until spec 21; Space does nothing until spec 11).
- [ ] Option dead-key results table (Design 4) is filled in for every Opt key used by specs 11-13 and 41.
- [ ] Upstream diff is exactly `shortcuts.cfg`, `app.qrc` plus the new files.

## Tests
- `check_keymap.py` (above) is the automated test; run it in CI/manually after every spec that edits `shortcuts_dorico.json`.
- Manual script: the acceptance list. For regressions, `git stash` the preset and compare behaviour with MuseScore defaults.

## Results
(Option dead-key table to be filled in by the implementer.)
