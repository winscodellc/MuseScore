<!-- markdownlint-disable -->
# MuseScore Studio 5.0-dev internals relevant to the Dorico-clone specs

> **Line numbers are as of main commit `ac156123d8` and must be re-verified with `grep` before relying on them.** The `muse/` submodule moves independently of this repo.

Findings from an exploration of the checkout. Paths are relative to the repo root (`/Users/winsvu/Projects/MuseScore/`) unless noted; `muse/` is the framework submodule.

Summary: this checkout uses **Shortcuts v2** (`CMakeLists.txt:198`, `set(MUSE_MODULE_SHORTCUTS_V2 ON)`). The live default shortcuts are the JSON files using `command://` ids, not the XML files or the old `notationuiactions.cpp` action ids.

## 1. Shortcuts system

**Files** (all in `src/app/configs/data/`, bundled via `src/app/app.qrc:10-16`)

| File | Status | Size |
|---|---|---|
| `shortcuts.json` | Live base file | 280 `"command"` entries, 8 scopes |
| `shortcuts_mac.json` | Live macOS diff | 3 entries |
| `shortcuts_azerty.json` | Live diff, offered as a preset | 9 entries |
| `shortcuts.xml` | Legacy v1, not read under v2 | 281 `<SC>` |
| `shortcuts_mac.xml` | Legacy v1, not read under v2 | 280 `<SC>` |
| `shortcuts_azerty.xml` | Legacy v1, not read under v2 | 275 `<SC>` |

- **Index file:** `src/app/configs/shortcuts.cfg` maps names to paths: `command_shortcuts`, `command_shortcuts_mac`, `command_shortcuts_azerty`, and `"command_shortcuts_presets": ["shortcuts_azerty"]`.
- **JSON entry format:** `[{ "scope": "NOTATION", "shortcuts": [ {"command": "command://notation/copy", "sequences": ["Ctrl+C"], "autorepeat": false} ] }]`. `autorepeat` defaults to true (`commandshortcutsregister.cpp:372`).
- **Scopes in `shortcuts.json`:** NAVIGATION (line 3), NOTATION_PAGE (21), NOTATION (55), NOTATION_NOTE_INPUT (273), NOTATION_TEXT_EDITING (282), PROJECT (295), APP (306), PLAYBACK (315).
- **Legacy XML format:** `<SC><key>action-id</key><seq>Ctrl+F1</seq><autorepeat>0</autorepeat></SC>`.

### macOS vs other
Decided at compile time with `#if defined(Q_OS_MACOS)` in `muse/framework/shortcuts_v2/internal/shortcutsconfiguration.cpp:66-82`; `defaultShortcutsName()` returns `"shortcuts_mac"` or `"shortcuts"`. On macOS, `Ctrl` in the JSON means Cmd (standard Qt behaviour).

### Loading (`muse/framework/shortcuts_v2/internal/commandshortcutsregister.cpp`)
- `reload()` at :74 does this in order:
  1. Reads the base file `commandShortcutsAppDataPath("shortcuts")` (:81).
  2. Applies the platform diff, then the current preset diff, via `applyShortcutsDiff` (:88-96, :185).
  3. Adds every registered command that has `Availability::Shortcut` but no binding, with an empty sequence (:100-118).
  4. Merges the user file over the defaults (:120-140).
- **User file:** `<userAppData>/shortcuts/<activeName>.json` (`shortcutsconfiguration.cpp:111-124`). It stores only the differences from the defaults (`makeDiff` :197, `updateShortcuts` :483).
- **Current preset:** settings key `shortcuts/currentPreset` (`shortcutsconfiguration.cpp:34`).
- **Leftover v1 path:** `shortcutsUserAppDataPath()` still returns `.../shortcuts.xml` (:61), but nothing in v2 reads it.

### Swapping the shortcut set without code edits
There is no setting for the default file path. `ConfigReader::read(":/configs/shortcuts.cfg")` reads only the embedded resource (`muse/framework/global/configreader.cpp`), with no user override.

- **Option A – zero code:** set `shortcuts/currentPreset = "dorico"` and drop a complete `<userAppData>/shortcuts/dorico.json` in place. The built-in diff lookup for an unknown name returns an empty path and is quietly skipped (:188). The user file then becomes the override set (`mergeShortcuts` :234).
- **Option B – data-only, built in:** add `"command_shortcuts_dorico": ":/configs/data/shortcuts_dorico.json"` to `shortcuts.cfg`, add the name to `command_shortcuts_presets`, and add the file to `app.qrc`. It then appears in the Preferences preset picker and can't be deleted (`canDeletePreset` :610).
- **Caveat for both:** a preset is a diff on top of the base. Base bindings survive unless the preset rebinds the command (or lists it with `"sequences": []`) or reuses the key in the same scope. So the Dorico file must list every command whose MuseScore key should disappear.

### Conflicts
- `canShortcutsConflict()` is just `scope1 == scope2` (`muse/framework/shortcuts_v2/shortcutstypes.h:126`).
- When merging, a default sequence that a user/preset shortcut reuses in the same scope is removed (`commandshortcutsregister.cpp:249-255`). Clashes across scopes are kept.
- `makeUnique` (:289) combines duplicate command entries.
- The Preferences editor warns about conflicts in the same scope only (`muse/framework/shortcuts_v2/qml/Muse/Shortcuts/editshortcutmodel.cpp:58,183`).

### How a key press is dispatched
1. `Shortcuts.qml` creates one QML `Shortcut { context: Qt.WindowShortcut }` per unique sequence string.
2. `ShortcutsInstanceModel::doLoadShortcuts` (`shortcutsinstancemodel.cpp:61`) forces `autoRepeat=false` if any shortcut sharing that sequence has it off.
3. `ShortcutsController::activate` (`muse/framework/shortcuts_v2/internal/shortcutscontroller.cpp:45-75`) keeps only commands whose `commandsState()->commandState(cmd).enabled` is true.
4. If exactly one remains, it fires. If several remain, `ShortcutResolver::selectOne` (`src/context/internal/shortcutresolver.cpp:86`) picks by scope priority (:54-73): NAVIGATION 1, PLAYBACK 2, NOTATION_PAGE 3, NOTATION 4 (only if the score has focus), NOTE_INPUT 5 (only in note input), TEXT_EDITING 6.

**Constraint: scope does not gate anything.** A NOTATION_NOTE_INPUT shortcut with a single match still fires outside note input, because priority is only used for sorting. Real context-dependence comes from the command's **enabled state** in `src/notationscene/internal/notationcommandsstate.cpp`:
- `NOTE_COMMANDS` (enter/add/insert-note-X) are enabled when `isNoteInputActionAllowed()` (`notationactioncontroller.cpp:3293`) is true: not playing, not editing an element, not a TAB staff.
- `TAB_COMMANDS` swap in on TAB staves. That is how `"D"` can mean both `enter-note-d` and `enter-fret-3` (`shortcuts.json:245`).
- Text and lyrics commands are enabled only while editing text.
- `NotationViewInputController::shortcutOverrideEvent` / `keyPressEvent` (`notationviewinputcontroller.cpp:1441,1465`) handle Enter (edit text), Esc, element-edit keys and percussion keys before shortcuts are consulted.

### Single letters and number keys
Plain NOTATION-scope bindings: A–G (and H) for `enter-note-*` (`shortcuts.json:92-98`), Shift+letter for `add-note-*`, Ctrl+Shift+letter for `insert-note-*`, digits 1–9 for durations (`:188-196`), 0 for rest. Letters start note input automatically through `doAddNote` → `startNoteInput()`. Durations in `shortcuts.json:186-192` (verified): 1 = 64th, 2 = 32nd, 3 = 16th, 4 = eighth, 5 = quarter, 6 = half, 7 = whole (plus `Num+N` variants); `.` = toggle-dot.

> **Spec note — digits do NOT match Dorico.** Dorico's manual says 4 = 16th, 5 = eighth, 6 = quarter, 7 = half (8 = whole and 9 = double whole by extension), i.e. every digit is shifted by one relative to MuseScore. `10-keymap-preset.md` must rebind the duration commands (set-duration-* and breve/longa) accordingly; this is a conflict-heavy block because 5/6/7 swap meaning.

### Multi-key sequences ("Ctrl+K, Ctrl+X")
- No default uses them.
- The editor records only one key combination (`editshortcutmodel.cpp:82-111` replaces `m_newSequence` on every key).
- `Shortcut::sequencesFromString` splits on `", "` (`shortcutstypes.h:70`), which collides with Qt's own multi-chord text format.
- At runtime a QML `Shortcut` *should* accept a multi-chord string, but this was not verified. A "Ctrl+K" prefix would also become ambiguous with `add-chord-text` (Ctrl+K).
- So multi-chord bindings would have to be written by hand in the JSON, and the editor/serialiser would need code changes.

### Preferences UI
- `src/preferences/qml/MuseScore/Preferences/ShortcutsPreferencesPage.qml:43` hosts `ShortcutsPage`.
- `ShortcutsModel` (`muse/framework/shortcuts_v2/qml/Muse/Shortcuts/shortcutsmodel.h:45-92`) provides presets, `currentPresetName`, `importShortcutsFromFile`/`exportShortcutsToFile` (`.cpp:201,213`), and conflict replacement (`applySequenceToCurrentShortcut` :230).
- **Bug:** the import/export file filter is still `*.xml` (`shortcutsmodel.cpp:40`), but `importFromFile` copies the file over the user `.json` and `exportToFile` writes JSON (`commandshortcutsregister.cpp:557,579`).

### Command and action ids
- **Command system (live):** `muse/framework/rcommand/`.
  - `Command` is a `Uri` (`commandtypes.h`); `CommandInfo` holds title, description, `InputSchema`, and `Availability` flags (Dispatch/Shortcut/API/MCP).
  - Commands are registered through `ICommandsRegister::reg(IModuleCommandsRegisterPtr)` and handled via `ICommandDispatcher::onRequest`.
  - Notation ids: `src/notationscene/notationcommands.h`, 437 `command://` constants, e.g. `toggle-note-input`, `set-duration-quarter`, `toggle-marcato`, `add-chord-text`, `add-dynamic`, `repeat-selection`, `paste-special`, `select-similar-in-range`, `view/show-search`.
  - Their `CommandInfo`s are in `src/notationscene/internal/notationcommandsregister.cpp:64` (`s_commandInfos`).
  - Handlers are in `NotationActionController::init()` (`src/notationscene/internal/notationactioncontroller.cpp:112-676`).
- **Legacy UiActions (menus/toolbars):** `src/notationscene/internal/notationuiactions.cpp`, about 438 `UiAction(` entries.
  - Examples: `note-input` (:47), `pad-note-4` (:2313), `add-marcato` (:2556), `chord-text` (:1476), `find` (:617), `repeat-sel` (:1747), `select-similar` (:430), `realtime-advance` (:2278).
  - They are bridged to commands through the `ActionToCommand` table (`notationactioncontroller.cpp:681-1101`; helper in `muse/framework/rcommand/actiontocommand.h`).
- **Palette commands:** `src/palette/palettecommands.h` has 11 (`toggle-master-palette`, `search`, `apply-current-element`, …), handled in `src/palette/internal/paletteactionscontroller.cpp:53-65`.

## 2. Note input model

- **State:** `NoteInputState` = `engraving::InputState` (`src/engraving/dom/input.h:63`), aliased in `src/notation/types/noteinputtypes.h:33-35`. It holds segment, track/voice, duration, rest flag, accidental, articulation ids, slur, `noteEntryMode`, `noteEntryMethod`, and `m_notes` (pending pitches for by-duration input).
- **Methods:** `NoteEntryMethod { UNKNOWN, BY_NOTE_NAME, BY_DURATION, REPITCH, RHYTHM, REALTIME_AUTO, REALTIME_MANUAL, TIMEWISE }` (`input.h:55`).
  - **Pitch before duration exists** as `BY_DURATION`. Letters only set the pending pitch (`doAddNote` → `setInputNote`, `notationactioncontroller.cpp:1431-1458`); Up/Down change it (`moveInputNotes`, :2182); the duration key places it.
  - **Insert mode** = `TIMEWISE` ("Insert mode (grow measures)", `notationnoteinput.cpp:62`), toggled by `toggle-insert-mode` (Ctrl+I) → `toggleNoteInputInsert` (:1383).
  - Real-time input uses `REALTIME_AUTO/MANUAL` plus `realtime-advance` (Enter, NOTE_INPUT scope).
- **Adding notes:** `NoteAddingMode { CurrentChord, NextChord, InsertChord }` (`noteinputtypes.h:40`) maps to enter-note / add-note (chord mode, Shift+letter) / insert-note. Shift+Alt+digit adds intervals (`add-interval-*`).
- **Interface:** `INotationNoteInput` (`src/notation/inotationnoteinput.h:36-83`): `startNoteInput`, `setNoteInputMethod`, `addNote`, `setDuration`, `setInputNote`, `moveInputNotes`, `addTuplet`, `cursorRect()`, `stateChanged()`.
- **Caret:** `NoteInputCursor` (`src/notationscene/qml/MuseScore/NotationScene/noteinputcursor.cpp`) is painted by the paint view. There is also a shadow-note popup (`ShadowNotePopup`).
- **Arrow keys:** `NotationActionController::moveWithRet` (:2071). Up/Down move lyrics, nudge text/articulations, move grips, change the pending pitch (BY_DURATION), move strings (TAB), or otherwise `movePitch` (chromatic; Ctrl = octave). Left/Right seek beats while playing, navigate text, or `moveSelectionDeprecated(Chord|Measure)`. Shift+Left/Right are swap-left/right in NOTE_INPUT scope; Shift+Up/Down are bound to `command://none` there (`shortcuts.json:273-280`).

## 3. Popups, text editing and adding items in code

### Existing popover precedent
`add-dynamic` (Ctrl+D) runs `NotationInteraction::toggleDynamicPopup` (`src/notation/internal/notationinteraction.cpp:5827`) → `addTextToItem(DYNAMICS, el)` (:6204) → `addText` (:6220). That creates an empty element and calls `startEditText`; the paint view then shows `DynamicPopup`. `Dynamic::parseDynamicText` / `endEdit` (`src/engraving/dom/dynamic.cpp:243,353`) turn typed text into a dynamic type. This is Dorico's Shift+D behaviour in all but name.

### Popup infrastructure
- `AbstractElementPopupModel` (`src/notationscene/qml/MuseScore/NotationScene/abstractelementpopupmodel.h`): a closed `PopupModelType` enum (HARP_DIAGRAM, CAPO, STRING_TUNINGS, SOUND_FLAG, STAFF_VISIBILITY, DYNAMIC, TEXT, PARTIAL_TIE, SHADOW_NOTE), plus `ELEMENT_POPUP_TYPES`, `hasElementEditPopup` (.cpp:105) and `modelTypeFromElement` (:141).
- `elementpopups/ElementPopupLoader.qml`: `componentByType` switch, positioned at `popup.elementRect`, opened with `PopupView.NoActivateFocus`.
- `AbstractNotationPaintView::showElementPopup/hide/toggle` (`abstractnotationpaintview.cpp:674-724`) emit `showElementPopupRequested`. Callers are in `notationviewinputcontroller.cpp:136-184, 1008, 1781, 1798`.
- `NotationView.qml:193-226` hosts `ElementPopupLoader`; `:166` and `:344` host `SearchPopup`.
- Adding a new popup type means editing the enum, the type map, the loader switch, and the paint view.

### SearchPopup / Find
- `internal/SearchPopup.qml` is a bottom bar with a `TextInputField`; `onTextChanged` calls `SearchPopupModel::search` (`searchpopupmodel.cpp`) → `NotationElements::search` (`notationelements.cpp:51`).
- `SearchCommandsParser` (`src/notation/internal/searchcommandsparser.cpp`) accepts:
  - a bar number `"12"` or range `"12-16"`;
  - a page `"p5"`;
  - a rehearsal mark `"rA"`, or any other text as a rehearsal mark.
- Command: `command://notation/view/show-search` (Ctrl+F, `shortcuts.json:216`) → `ViewController::showSearch`.

### Programmatic APIs
`INotationInteraction` is in `src/notation/inotationinteraction.h`; implementations are in `src/notation/internal/notationinteraction.cpp`.

| Item | Function |
|---|---|
| Any palette element (time sig, key sig, barline, repeat, fermata, clef, tempo, articulation…) | `applyPaletteElement(EngravingItem*, modifiers)` :2224 (`→ applyPaletteElementToList/Range`, `applyDropPaletteElement` :2699). Palette calls it at `src/palette/internal/paletteprovider.cpp:596`. |
| Text: staff, system, expression, tempo, rehearsal, fingering, chord symbol (HARMONY_A/ROMAN/NASHVILLE), lyrics, sticking, instrument change | `addTextToItem(TextStyleType, item)` :6204 → `Score::addText` (`src/engraving/editing/edit.cpp:640`) → `startEditText`. Controller wrapper `NotationActionController::addText` :2407. |
| Start/end text editing | `startEditText(EngravingItem*, pos)` :4141 (TextBase overload :8253), `endEditText` :4373 |
| Time signature | `addTimeSignature(Measure*, staffIdx, TimeSignature*)` :6359; engraving `EditTimeSig::addTimeSig` (`src/engraving/editing/edittimesig.h:37`) |
| Key signature | `EditKeySig::undoChangeKeySig(tx, score, staff, tick, KeySigEvent)` (`src/engraving/editing/editkeysig.h:57`), or a KeySig via `applyPaletteElement` |
| Clef | `insertClef(ClefType)` :8392; `EditClef::insertClef` (`src/engraving/editing/editclef.h:58`) |
| Articulation / ornament | `toggleArticulation(SymId)` :8367, `toggleOrnament(SymId)` :8374, `toggleArticulationForSelection` :5581; `EditChord::toggleArticulation` |
| Hairpin / slur / ottava | `addHairpinsToSelection` :5467, `addSlurToSelection` :5397, `addOttavaToSelection` :5419; `EditHairpin::addHairpin`, `EditSpanner::addSpanner` |
| Tuplets | `addTupletToSelectedChordRests(TupletOptions)` :5687 |
| Insert / remove bars | `addBoxes(BoxType::Measure, count, AddBoxesTarget)` :4916; `removeSelectedMeasures` :6423; `removeSelectedRange` :6477 |
| Breaks | `toggleLayoutBreak` :5887 |

There is no dedicated fermata, barline or repeat API; those go through `applyPaletteElement` with an element created by `Factory::create*`.

### Figured bass (found in spec 36)
- Build one without the text editor, as the MusicXML import does: `FiguredBass::addFiguredBassToSegment(segment, track, Fraction(0, 1), &isNew)` (reuses a figured bass already on the segment/staff, so `undoRemoveElement` the old one first; sets ticks up to the next chord and `onNote`), then per line `fb->createItem(i)`, `item->parse(String&)` (false = line not conformant), `setTrack`, `setOwnershipParent(fb)`, `fb->appendItem(item)`; `fb->setXmlText(lines joined with '\n' from item->normalizedText())`; `score->undoAddElement(fb)`. Not `FiguredBass::regenerateText()`: it calls `score()->startCmd` itself.
- The line grammar (`FiguredBassItem::parse`): `[(] prefix [)] [(] digits [)] suffix [)] [-|_ continuation]`. Prefix/suffix: `#`, `##`, `b`, `bb`, `h` (natural, not `n`), `+` (cross); suffix only: `\\` and `/` (a slash needs a digit). A prefix and a suffix cannot be on the same line. A bracket around a lone digit moves to the digit slot, so `(4)` works.
- A figured bass is on the first track of the staff, anchored to a segment annotation like a chord symbol; `itemAt`/`movePosition` in `popovernavigation.cpp` find it by `ElementType::FIGURED_BASS`.

### Chord symbols (found in spec 32)
- Create one in code: `Factory::createHarmony(segment)`, `setTrack`, `setHarmonyType(HarmonyType::STANDARD)`, `setHarmony(text)`, `setXmlText(harmonyName())`, then `score->undoAddElement`. That is what `FretDiagram::setHarmony` and `Harmony::endEdit` do. `harmonyName()` gives the text back for prefilling.
- A beat with no note of its own: `measure->undoGetSegment(SegmentType::ChordRest, tick)` makes an empty ChordRest segment to hang the chord on (inside a transaction).
- Validating: `Harmony::parseHarmony` and the editor's misspelling check only reject an invalid **root**; any text after a valid root passes. `ParsedChord::parse(body, &chordList)` with a default-constructed `ChordList` and `syntaxOnly = false` sets `understandable()` false on unknown tokens (`Cxyz`). With `syntaxOnly = true` it does not. The root is parsed separately by `convertNote` (`pitchspelling.cpp`).
- MuseScore's spelling quirks: a capital `M` lowercases to `m` (minor) in most token checks, so translate Dorico's `M` to `maj`; a leading `-` means minor and `+` augmented; `|` splits polychords; `(...)` sets parentheses on the Harmony itself.
- `NotationInteraction::navigateToNearHarmony`, `navigateToHarmonyInNearMeasure` and `navigateToHarmony` (`notationinteraction.cpp:7242-7420`) only act while a Harmony is being text-edited (`editedHarmony()`), so a popover cannot call them. Their beat rule (dotted quarter in compound time) is copied in `src/dorico/popover/popovernavigation.cpp`.
- `Measure::tick2pos` starts with `tck -= ticks()` (the measure length, not its start tick) and returns an x relative to the system: do not use it for canvas positions. `positionRect` in `popovernavigation.cpp` interpolates between segment canvas positions instead.

### Lyrics editing (found in spec 33)
- `add-lyrics` → `NotationActionController::addText(LYRICS_ODD)` → `Score::addText` (`edit.cpp` ~L800) always adds a **new** verse (verse = number of lyrics on the chord). To edit line 1, `select` + `startEditText` the existing lyric.
- Which keys reach shortcuts while editing: `NotationViewInputController::shortcutOverrideEvent` → `Lyrics::isEditAllowed` (`lyrics.cpp`). False = the key goes to the shortcut system. Plain/Shift `-`, `_`, Return, Up, Down always pass; Left/Right/Backspace pass only at the text edge; Space passes (`isTextNavigationKey`). Alt+arrows are kept by the text editor unless `Lyrics::isEditAllowed` releases them (fork edit).
- `ShortcutResolver::selectOne`: when two enabled commands share a key, `NOTATION_TEXT_EDITING` (6) beats note input (5) and `NOTATION` (4).
- Commands: `edittext-next-word` (Space: next note, word end), `editlyrics-next-syllable` (`-`), `editlyrics-add-melisma` (`_`), `editlyrics-next/prev-verse`, `edittext-next/prev-element` (move to next/prev lyric without changing syllabic), `editlyrics-add-verse`.
- Paste while editing a lyric (`NotationInteraction::pasteSelection` → `extractSyllable`) already pastes one syllable, advances like Space / `-` / `_`, and puts the rest back on the clipboard.
- New lyrics copy the placement of the one they came from (`navigateToLyrics`, ~L6834), so changing `Pid::PLACEMENT` on the edited lyric moves the following entries too.

## 4. Copy, paste and duplicate

Handlers are registered in `src/notationscene/internal/notationactioncontroller.cpp:424-443`; implementations are in `notationinteraction.cpp`.

| Operation | Implementation |
|---|---|
| copy | `copySelection` :5090 → `selection()->qMimeData()` |
| cut | `cutSelection` (controller :2255) |
| paste / paste-half / paste-double / paste-special | `pasteSelection(PastingType)` (controller :2276); `resolvePastingScale` :2290 (Special = current input duration × 4); then `NotationInteraction::pasteSelection(scale)` :5228 → `Paste::paste` (`src/engraving/editing/paste.cpp:547`) |
| copy-paste-swap | `swapSelection` :5289 |
| copy-lyrics | `copyLyrics` :5222 |
| repeat-selection (R) | `repeatSelection` :5115. Note-input single selection → re-add the chord; list selection → `Paste::repeatListSelection` (paste.cpp:288); range → `Paste::pasteStaff` (:78) after the selection |

- **Clipboard mime types** (`src/engraving/dom/mscore.h:77-79`): `application/musescore/symbol`, `.../symbollist`, `.../stafflist`, plus `application/musescore/richtext` (`src/engraving/editing/textedit.h:46`).
- **Paste helpers:** `pasteSymbol` :617, `pasteStaffList` :686, `pasteSymbolList` :720 (all in `paste.cpp`).
- **Mouse duplicate:**
  - Alt+click with a range selection pastes the range at the click point (`mousePress_considerStartPasteRangeOnRelease`, `notationviewinputcontroller.cpp:795`). This is the closest match to Dorico's Alt+click.
  - Ctrl+Shift+drag drags a single element out (:777).
  - In drag-and-drop, Alt forces copy instead of move (:1587, :1634).
- **Movement:**
  - `move-up/down` → `moveChordRestToStaff` (:3855; Ctrl+Shift+Up/Down).
  - `pitch-up/down` → `move()` → `movePitch` :4000.
  - `pitch-up/down-diatonic` → `movePitchDiatonic`.
  - Going up/down inside a chord: `goto-upnote/downnote/topnote/bottomnote-in-chord` (Alt+Up / Ctrl+Alt+Up …).

### Paste internals (found in spec 40)
- `Paste::pasteStaff(tx, score, xml, dstSegment, dstStaff)` (`read500.cpp:356`) takes the destination as a ChordRest **segment + staff**, not from the selection, and can be called repeatedly in one transaction. It overwrites only the voices present in the clipboard (`preparePasteDurationElement` → `makeGapVoice`), never merges into chords, never inserts time, and sets the selection to the last pasted range itself.
- `StaffList` payload root attributes: `tick`, `len` (Fraction string), `staff`, `staves`. A copied symbol (dynamic, hairpin) is a `symbol` payload, not a stafflist.
- There is no single-staff "push music right" in MuseScore (`insertChordByInsertingTime` lengthens the bar on every staff). The fork's way (spec 40): copy the tail with `rw::RWRegister::writer()->writeStaffSelection`, `Score::deleteRange`, `appendMeasures` while short, then `pasteStaff` the tail at the new tick.
- `writeStaffSelection` needs a **non-null end segment** (`writeScoreSpanners` calls `endSegment->tick()`): for a range reaching the score end pass `lastMeasure()->last()`. Its `SelectionFilter` reads the live selection, so call `selection().setRange(...)` first or `Selection::rangeContainsMultiNoteChords` asserts (not RANGE) once per note.
- `Selection::staffMimeData` is private; `mimeData()` is public. After `deleteRange` call `score->deselectAll()` (the selection holds the rebuilt segments).
- Intercepting a command (`command://notation/paste`/`cut`) with `ICommandDispatcher::preDispatch()` and setting `*allowDispatch = false` also covers the menus; leaving it true falls through to the upstream handler.

### Paste special internals (found in spec 41)
- **`pasteStaff` keeps the source voice**: the destination only shifts the staff (`setTrackOffset((dstStaff - staffStart) * VOICES)`, `read500.cpp`). Selecting a chord-rest in voice N does *not* paste into voice N.
- **Pasting into another voice** = `ExchangeVoices::exchangeVoicesInSelection(score, a, b)` (`editing/exchangevoices.h`, needs a range selection, swaps whole bars, does nothing in parts), then `pasteStaff`, then the same exchange again. Outside the pasted span the two swaps cancel; `pasteStaff` resets the selection range, so set it again before the second swap.
- **Stafflist voices**: in clipboard mode the writer emits no `<voice>` tags. Voice 0 comes first; a change of voice is a `<location><voices>N</voices>...` step *directly under `<Staff>`*, relative to the previous voice (nested `<location>`s belong to spanner ends, ignore them). Spanners (slurs) are in a trailing `<SpannerMap>` with ticks, so reading slurs from the XML is awkward: paste, read the chords and `score->spannerMap().findContained`, roll back (`pasteSpecialArticulations`).
- **Cut/copy a span without the clipboard**: `selection().setRange(...)`, then `RWRegister::writer()->writeStaffSelection(score, filter, staffStart, staffEnd, from, to, startSeg, endSeg)` with any `SelectionFilter` (voice filter: `setFiltered(VoicesSelectionFilterTypes::ALL, false)`, then `setFiltered(1u << voice, true)`). `Score::deleteRange(..., filter, true)` with the same filter deletes just those voices and leaves rests.
- `Score::addSlur(first, second, nullptr)` is undo-aware (`undoAddElement`); `Chord::updateArticulations(ids)` adds articulations and ornaments (Insert mode).

## 5. Selection

- **SelectionTarget** (`src/notation/notationtypes.h:113`) is handled in `NotationSelection::select` (`src/notation/internal/notationselection.cpp:190-290`) and `addToSelection` (:292). Targets include Similar, SimilarInStaff, SimilarInRange, NotesInChord, All, Section, Top/BottomNoteInChord, TopStaff and others.
- **Score-level helpers:** `Score::selectSimilar` / `selectSimilarInRange` / `cmdSelectAll` / `cmdSelectSection` (`src/engraving/dom/score.h:515,516,758,759`).
- **Extending the selection:** `add-to-selection-{next,prev}-{chord,measure}`, `-{above,below}-staff`, `-{begin,end}-{system,score}` (`notationactioncontroller.cpp:151-160`; `expandSelection` :3773).
- **Selection filter** (`src/engraving/dom/selectionfilter.h`):
  - `VoicesSelectionFilterTypes` (4 voices).
  - `NotesInChordSelectionFilterTypes` (TOP, 7th…2nd, BOTTOM).
  - `ElementsSelectionFilterTypes` (20): DYNAMIC, HAIRPIN, FINGERING, LYRICS, CHORD_SYMBOL, OTHER_TEXT, ARTICULATION, ORNAMENT, SLUR, TIE, FIGURED_BASS, OTTAVA, PEDAL_LINE, OTHER_LINE, ARPEGGIO, GLISSANDO, FRET_DIAGRAM, BREATH, TREMOLO, GRACE_NOTE.
  - Plus `includeSingleNotes`.
  - Interface: `src/notation/inotationselectionfilter.h`; implementation: `notationselectionfilter.cpp`.
  - UI: `SelectionFilterPanel.qml` and `src/notationscene/qml/MuseScore/NotationScene/selectionfilter/*model.cpp`.
- **More options dialogs:** `open-selection-options` → `selectdialog.cpp` / `selectnotedialog.cpp` (`src/notationscene/widgets/`).

## 6. Keeping the fork in its own module

**Template:** `src/palette/palettemodule.cpp`.
- `resolveImports()` calls `ICommandsRegister::reg(make_shared<…CommandsRegister>())`.
- The context's `resolveImports` calls `ICommandsState::reg(…)`.
- Handlers use `commandDispatcher()->onRequest(this, CMD, …)`.

A new `src/dorico/` module can therefore register its own `command://dorico/...` commands, their state, and handlers with no upstream edits. Shortcuts in a JSON file automatically pick up any command registered with `Availability::Shortcut`.

### Fingering (found in spec 34)
- A `Fingering` is a `TextBase` in `Note::el()`, parent = the note. Make it with `Factory::createFingering(note, TextStyleType)`, `setTrack(note->track())`, `setPlainText`, `Score::undoAddElement`; no text-edit session is needed. Styles: `FINGERING` (plain), `LH_GUITAR_FINGERING`, `RH_GUITAR_FINGERING`, `STRING_NUMBER`.
- `Chord::notes()` is ordered low to high. On a TAB staff `Note::drop` skips fingering unless `staffTypeForElement(note)->showTabFingering()`; the handler copies that rule.
- Fretted instrument: `staff->part()->instrument(tick)->stringData()->frettedStrings() > 0` (as `measure.cpp` ~L3511).
- A popover that steps by note sets `PopoverDef::stepsByChord`; the position keeps the voice (`trackZeroVoice` is skipped).

### Note tools: intervals and transposing (found in spec 35)
- `Note::transpose(Interval, dbl)` and `Note::transposeDiatonic(steps, keepAlterations, dbl)` (`note.cpp` ~L4191-4254) move one note, set tpc1/tpc2 for transposing instruments, record the undo step (`EditNote::undoChangePitch`, which follows linked staves but **not ties**), and return false if the pitch is out of range. `transposeDiatonic` steps in the written key; `keepAlterations` keeps the accidental (octaves).
- `Interval(diatonic, chromatic)` stores both in `int8_t`. `Interval::allIntervals` stops at one octave, which is why the spec does not go through `Transpose::transpose` (it takes an index into that table). That function also handles key signatures and chord names; the note tools do not need them.
- `Selection::noteList()` gives the notes of a list or range selection (the range version honours the selection filter). `Note::firstTiedNote()` / `tiedNotes()` give a tie chain; copy the tie code from `Score::addInterval` (`cmd.cpp` ~L602) to tie a new chain.
- `Score::addInterval(val, notes)` (`cmd.cpp:495`) and `NotationInteraction::addIntervalToSelectedNotes` only do the key-diatonic `2..9` / `-2..-9` intervals, one per command, and select the new notes.

### Hooks available without upstream edits
- `ICommandShortcutsRegister::setAdditionalShortcuts(context, list)` (`muse/framework/shortcuts_v2/icommandshortcutsregister.h:47`, impl :525) lets a module inject bindings at runtime. They are merged like user shortcuts, so same-scope defaults are removed.
- `ICommandDispatcher::preDispatch()` (`muse/framework/rcommand/icommanddispatcher.h`; sent at `commanddispatcher.cpp:66,81`) passes `bool* allowDispatch`. A module can veto `command://notation/copy`, `paste`, etc. and run its own Dorico behaviour. Nothing in `src/` uses it today.
- **Hard limit:** only one handler per command. `CommandDispatcher::reg` refuses duplicates (`commanddispatcher.cpp:135`), so a module can't replace a handler, only intercept it via preDispatch.

### Minimal upstream changes needed
1. `src/CMakeLists.txt`: `add_subdirectory(dorico)`; `src/app/CMakeLists.txt` (~:178): `add_to_link_if_exists(dorico)`; `src/app/appfactory.cpp:434` area: include plus `app->addModule(new mu::dorico::DoricoModule())`. (CLAUDE.md also lists the root `CMakeLists.txt` option and `app_config.h.in` cmakedefine.)
2. Shortcuts: either no code at all (preset name in settings plus a user JSON file), or `shortcuts.cfg` + `app.qrc` + a new `shortcuts_dorico.json` for a built-in preset. Optionally add a DORICO scope title in `shortcutresolver.cpp:108` (unknown scopes fall back to the raw upper-case name, :135).
3. Popover overlay: there's no injection point in `NotationView.qml`. The minimal change is one `Loader`/component there next to `SearchPopup` (`NotationView.qml:344`), sourced from a `MuseScore.Dorico` QML module. To reuse `ElementPopupLoader` instead, you would also need a new `PopupModelType`, a `componentByType` case, and a `showElementPopupRequested` path. The text-entry precedents are `SearchPopup` (text field to model) and `DynamicPopup` (anchored to the element via `itemRect`).
4. Behaviour that lives inside existing handlers (paste semantics, `repeatSelection`, `moveWithRet` arrow logic, `SearchCommandsParser`) can only be changed by editing `notationinteraction.cpp` / `notationactioncontroller.cpp` / `searchcommandsparser.cpp`, or by vetoing via preDispatch and re-implementing through the public `INotationInteraction` API.

### Key conflicts to plan for
Dorico's Shift+letter popovers (Shift+D, Shift+M, Shift+K, Shift+B, Shift+X, …) collide with MuseScore's `add-note-*`, articulation and other bindings on Shift+letter (33 of them, all in the NOTATION scope). The Dorico shortcut file must clear those explicitly; same-scope rebinding clears them automatically.

## 7. App shell (window, pages, toolbars, status bar)
All paths relative to `src/`. Used by specs 50-59.

### Loading chain and pages
- `app/internal/guiapp.cpp:100` loads `appshell/qml/MuseScore/AppShell/platform/<os>/Main.qml`, which contains `WindowContent` (`platform/mac/Main.qml:39`).
- `appshell/qml/MuseScore/AppShell/WindowContent.qml` is a `DockWindow`: `:49-56` `onRequestedDockPage` -> `loadPage(uri)`; `:64-98` window-level `toolBars` (one `DockToolBar` holding `MainToolBar`, the Home/Score/Publish tabs); `:100-118` the fixed `pages` list (`HomePage`, `NotationPage`, `NotationReviewPage`, `PublishPage`, `DevToolsPage`).
- `MainToolBar.qml:70-103` tabs; model `maintoolbarmodel.cpp` (URIs `:29-33`, `load()` `:100-120`, Score tab URI swap `:139-155`). `openprojectscenario.cpp:508-520` `resolveNotationPageUri()` picks notation vs review page.
- `appshell/qml/MuseScore/AppShell/NotationPage/NotationPage.qml` is a `DockPage`, `uri: "musescore://notation"`, C++ `NotationPageModel`. `mainToolBars` `:116-222` (notation, playback, extensions, undo/redo); `toolBars` `:224-254` (`noteInputBar`); side panels `:256-396` (Palettes, Layout = instruments, Properties, Selection filter, History; group `VERTICAL_PANELS`, 300 px); bottom panels `:402-568` (Mixer, Piano keyboard, Timeline, Percussion; group `HORIZONTAL_PANELS`); `central` = `NotationView` `:571-588`; `statusBar` `:590-598`.
- Panel names: `appshell/appshelltypes.h:37-59`; page model accessors `xxxPanelName()`.
- Toggle chain: actions `appshell/internal/applicationuiactions.cpp:126-188` (dock mapping `:332-342`) -> `appshellcommandscontroller.cpp:92,115-123` -> `NotationPageModel` (`notationpagemodel.cpp:69`, `toggleDock()` `:195-209`) -> `dock-toggle` / `dock-set-open` -> `muse/framework/dockwindow/qml/Muse/Dock/dockwindow.cpp:288-312`. Command URIs `appshell/appshellcommands.h:44-52`.
- Dock engine: `dockwindow.cpp` `loadPageContent` `:357`, `loadTopLevelToolBars` `:373-387` (first on top, rest to its right), `loadToolBars` `:389-401`, `loadPanels` `:404-424`, `addDock` `:480`. Layout saved and restored **per page name** (`:617-683`); a new page starts from its QML defaults.

### Toolbars
| Toolbar | QML | Model |
|---|---|---|
| Notation | `notationscene/qml/MuseScore/NotationScene/NotationToolBar.qml` | `notationtoolbarmodel.cpp:35-70` |
| Playback | `playback/qml/MuseScore/Playback/PlaybackToolBar.qml` | `playbacktoolbarmodel.cpp:44-58,101`; config via `uiState()->toolConfig` |
| Undo/redo | `notationscene/qml/MuseScore/NotationScene/UndoRedoToolBar.qml` | `internal/undoredotoolbarmodel.h:32` |
| Note input | `NoteInputBar.qml` (`GridViewSectional`, horizontal or vertical) | `noteinputbarmodel.cpp`, defaults `:102-160`, `load()` `:163` |

### Status bar
`appshell/qml/MuseScore/AppShell/NotationPage/NotationStatusBar.qml` + `notationstatusbarmodel.{h,cpp}`. Order: playback loading, accessibility info, online sounds, workspace switcher, concert pitch toggle, view mode, zoom, overflow menu. Reusable parts: `ViewModeControl.qml`, `ZoomControl.qml` (NotationScene). View modes -> commands at `notationstatusbarmodel.cpp:65-69`.

### Vertical toolbars / side strips
- Vertical `DockToolBar` is supported: each `DockPage` has holders for all four edges (`DockPage.qml:34-42,61-113`); `DockToolBarView.orientation` (`docktoolbarview.h:48`); `init()` `docktoolbarview.cpp:166-177`. **Snag:** vertical is only chosen when the toolbar is floatable with a left/right drop destination (`canChangeOrientation` `:179-192`), and `resetToDefault()` forces horizontal (`:194-199`). A toolbar that must start vertical needs `location: Location.Left` and an explicit orientation.
- No side icon strip / tab bar exists. `DockTabBar.qml` is horizontal only; KDDockWidgets auto-hide sidebars are off (`dockmodule.cpp:174-184`). Build the strip as a narrow non-floatable vertical `DockToolBar` whose buttons dispatch `dock-toggle` / `dock-set-open`.

### Can a module add pages?
No hook. Pages are QML children in `WindowContent.qml:100-118`; `pageByUri()` (`dockwindow.cpp:568`) only searches that list; `registerPageUri` (`muse/framework/interactive/iinteractiveuriregister.h:45-47`) only routes (`InteractiveProvider.qml:57-63`); AppShell pages are imported relatively and compiled into `MuseScore.AppShell`, so they can't be shadowed. Required upstream edit: import `MuseScore.Dorico` in `WindowContent.qml` and list the Dorico pages there. If the Dorico QML module imports `NotationPageModel` (AppShell), add the appshell QML target to its `IMPORTS`.

### `src/dorico/` at the start of the UI series
`IDoricoModeState` (`idoricomodestate.h:30-42`) has only Chord/Insert flags. `qml/MuseScore/Dorico/` has only `DoricoPlaceholder.qml`; its imports are `muse_ui_qml` and `muse_uicomponents_qml`. `DoricoModule` registers commands, state and the preset switch (`doricomodule.cpp:51-95`), no URIs and no UI.
