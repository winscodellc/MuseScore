<!-- markdownlint-disable -->
# Specs: turning this MuseScore fork into a Dorico Pro 6 clone

One spec = one implementation session. Read this file, the reference files it links, then the spec you were asked to do.

## Goal

Make this fork (MuseScore Studio 5.0-dev) behave as close as possible to **Dorico Pro 6.2** for three feature areas:

1. **Key commands**: Dorico's default macOS key set replaces MuseScore's.
2. **Popovers**: the Shift+letter text-entry boxes (dynamics, time signatures, ...).
3. **Editing**: copy / paste / paste special / duplicate / repeat / delete / selection / filters / search (Jump bar).
4. **UI shell**: Dorico's window layout (mode tabs, toolboxes, zones, status bar, menus) for all five modes (specs 50-59), and its look: colors and icons (spec 60).

The Dorico manual is the behavioural reference: <https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/introduction/introduction_c.html> (popover syntax is in the separate *Dorico Popover Reference 6.2*).

## Decisions already made by the user

| Question | Decision |
|---|---|
| Spec granularity | Small phased specs, each sized for one session |
| MuseScore shortcuts | **Only Dorico's keys are bound** (decided 2026-10-15; replaces "MuseScore-only commands keep their key if it is free"). Every MuseScore binding that is not a Dorico key is removed; the command stays reachable from menus and Preferences. Exempt: keyboard navigation inside panels/dialogs (`navigation/*`, except the F6 / backtick section jumps) and keys that only work in the text / lyrics / chord-symbol editor or on TAB staves, until later specs replace them. Enforced by `check_keymap.py` (policy 5); `check_keymap.py --fix` unbinds what upstream merges add. |
| Keyboard layout | **US English, macOS.** Windows is out of scope for now (Dorico's Windows set differs, e.g. Ctrl-Alt-letter instead of Ctrl-letter for "note below") |
| Location | `specs/` at the repo root, committed |

## Hard constraints (from `CLAUDE.md`)

- **No Dorico code, assets, icons or proprietary file formats.** Work from MuseScore's code plus the *publicly documented behaviour*. Key bindings, entry-syntax tokens and color values are facts, so they are fine. Icons are MuseScore's, Bravura SMuFL glyphs (OFL) or Lucide (ISC); never Dorico's.
- Fork stays **GPLv3**. Keep all copyright / SPDX headers. New files get the same SPDX header as their neighbours (copy it from `src/palette/palettemodule.cpp`).
- **Keep fork code isolated**: new features go in `src/dorico/`. Edits to upstream files must be tiny and obvious (each spec lists them under "upstream edits"). Merge upstream regularly (`git fetch upstream && git merge upstream/main`), then run `python3 -I src/dorico/tools/check_keymap.py --fix` to unbind any new MuseScore keys.
- If distributed it needs its own name (not "MuseScore"). Not in scope for these specs.
- The framework in `muse/` (submodule) is not edited.
- User is a Java/Scala/TS developer with minimal C++: explain C++ idioms briefly when they matter (headers vs .cpp, smart pointers, Qt signals/slots, `Q_PROPERTY`), using Java/TS analogies.

## Cross-cutting design (every spec follows this)

### 1. One fork module: `src/dorico/`
Namespace `mu::dorico`, commands `command://dorico/<name>`. It is created in spec 00 by copying the layout of `src/palette/`. Upstream touch points are the 4 standard ones in `CLAUDE.md` ("Adding a new module") plus:
- (no edit) the popover overlay lives in our own `DoricoPage.qml`, parented to `notationView.paintView` (spec 20); the Jump bar (spec 46) can be hosted the same way. The planned `NotationView.qml` Loader was not needed. Return on a selected text item is redirected by a `preDispatch` veto of `screen-edit-text`, not by editing `notationviewinputcontroller.cpp`;
- `shortcuts.cfg` + `app.qrc` entries for the `dorico` shortcut preset (spec 10).
- `appshell/qml/MuseScore/AppShell/appmenumodel.{h,cpp}`: one optional `IDoricoMenuArranger` hook in `load()` (spec 59);
- the "is the score page open" checks, so commands also work on the `musescore://dorico/*` mode pages (fix after spec 50): `notationscene/internal/notationcommandsstate.cpp`, `playback/internal/playbackcommandsstate.cpp`, `appshell/internal/appshellcommandsstate.cpp` (`isProjectPageOpened`) and `context/internal/uicontextresolver.cpp` (legacy shortcut context). Each is marked `// Dorico fork:`.
- Setup-mode layout deletion (spec 55): `notation/internal/masternotation.cpp` (`updatePotentialExcerpts` skips the `deletedPartLayouts` meta tag), `notation/iexcerptnotation.h` + `internal/excerptnotation.{h,cpp}` (`initialPartId()`), and `engraving/dom/excerpt.cpp` (plain-text part names).
- Note-input articulation latch (spec 11): `engraving/dom/articulation.cpp` `updateArticulations` toggles any articulation, not only accent / marcato / tenuto / staccato.
- Play mode (spec 57): `playback/qml/MuseScore/Playback/mixerpanelmodel.cpp` applies control / mute / solo changes from other models, so two mixers over the same tracks stay in sync.
- Note-input caret (spec 12): `notationscene/qml/MuseScore/NotationScene/abstractnotationpaintview.{h,cpp}` lets an optional `IDoricoCaret` paint the caret and grid ruler.
- Appearance (spec 60): `app/configs/dark.cfg` and `light.cfg` hold Dorico's measured colors (values only).
- Notes panel articulations (spec 52): `notationscene/internal/notationcommandsstate.cpp` also refreshes the articulation toggles on selection change; `engraving/dom/chord.cpp` `updateArticulations` removes any articulation (not only the basic four), `articulation.cpp` knows stress / unstress placements; test in `engraving/tests/note_tests.cpp`.
- Print zoom (spec 58): `notationscene/qml/MuseScore/NotationScene/abstractnotationpaintview.{h,cpp}` `load()`: the main view being shown takes the view commands (zoom, Whole Page). The Dorico pages stay alive and Print's preview is read-only, so MuseScore's "register editable views once, when created" left zoom on the wrong view.
- Duration keys (spec 13): `notationscene/qml/MuseScore/NotationScene/notationviewinputcontroller.cpp` `shortcutOverrideEvent` lets (Cmd-)Shift-Opt-Left/Right reach the shortcuts when a line or text is selected.
- Opt-click / Opt-drag copy (spec 42): `notation/internal/notationinteraction.{h,cpp}` `dropRange` pastes a copy through an optional `IDoricoPaste` (Chord / Insert rules) when one is registered; `notationscene/qml/MuseScore/NotationScene/notationviewinputcontroller.cpp` `mousePress_considerStartPasteRangeOnRelease` turns a selected note / chord into a range first, so Opt-click copies it.
- Dorico pages hide MuseScore's score/part tabs (spec 50): `notationscene/qml/MuseScore/NotationScene/NotationView.qml` `showNotationTabs`.

### 2. Changing behaviour = new commands, not overriding old ones
The command dispatcher allows **one handler per command** (`muse/framework/rcommand/internal/commanddispatcher.cpp:135` refuses duplicates), so a module cannot replace `command://notation/paste`. Instead:
- Our key map points the key at a new `command://dorico/...` command, whose handler calls the public `INotationInteraction` / `INotationNoteInput` API (`src/notation/inotationinteraction.h`, `src/notation/inotationnoteinput.h`).
- `ICommandDispatcher::preDispatch` (veto hook) is a fallback only, for menu items that still call the old command.

### 3. Mode-dependent keys are ONE branching command
Shortcut **scopes do not gate anything** (`src/context/internal/shortcutresolver.cpp`: scope only sorts candidates). What decides whether a command fires is its **enabled state** (`ICommandsState`). Two enabled commands on one key: the resolver picks by scope priority, which is fragile.
Rule: a key whose meaning depends on note-input state (Return, Space, Esc, A-G, F, Q, ...) binds to **one** `dorico/...` command that checks `INotationNoteInput::isNoteInputMode()` and branches inside.

### 4. The key map is a preset diff
`src/app/configs/data/shortcuts_dorico.json` is applied on top of `shortcuts.json` (+ `shortcuts_mac.json`). Specs 11-13 and 20-46 each add their own rows to it (each spec lists "Adds to `shortcuts_dorico.json`"). Spec 10 creates the file, the unbinding, and the preset plumbing. Details in [10-keymap-preset.md](done/10-keymap-preset.md).

### 5. Fidelity rule
Each spec has a **Dorico behaviour** section copied from the reference files, with manual URLs. Anything MuseScore cannot do yet goes under **Deferred / unsupported**. Nothing is silently dropped. When Dorico documents nothing (for example popover error state), the spec says "our choice" and picks the simplest behaviour.

### 6. Parsers are pure functions
Every popover is a pure parser (no UI, no score) plus a handler that applies the result. The parser is unit-tested with GoogleTest. Same for the Jump bar and `SearchCommandsParser` extensions.

### 6a. Popover cheat sheet (read this instead of the framework code)
Everything is under `src/dorico/`. The framework is spec 20, and 21-32 are worked examples.
- **Parser** `popover/parsers/<x>parser.h/.cpp`: `XParse parseX(const std::string&)` returns `{ ParseStatus status; ...entry }`. `ParseStatus` is one of:
  - `Ok`
  - `Incomplete`: a valid prefix, neutral border, Return does nothing. Empty text is `Incomplete`.
  - `Unsupported`: valid Dorico we don't implement, red border plus "Not supported yet".
  - `Invalid`: red border.

  Put helpers in a **named** namespace, not an anonymous one: the unity build compiles files together.
- **Handler** `popover/handlers/<x>handler.h/.cpp`: `PopoverDef xPopover()`. Register it in `popover/popoverregistry.cpp` (`makePopoverRegistry()`), replacing its `placeholder(...)` line. The command (`DORICO_POPOVER_COMMANDS` in `doricocommands.h`, id = the URI tail: `lyrics`, `fingering`, `notetools`, `figuredbass`) and the Shift-key (`shortcuts_dorico.json`) already exist.
- **`PopoverDef`** (`popover/popovertypes.h`):
  - `validate(text)`.
  - `apply(text, ApplyScope, notation)`: acts on the selection or the note-input caret; returns true if applied.
  - `prefill(item)`: the entry text when editing an existing item.
  - `itemTypes`: Return on a selected item of these types reopens this popover. With `editsAttached`, opening on a note that already has one edits it.
  - Optional: `supportsLocal` (Opt-Return), `selectionPrefill`, `noteInputEnded`, `suggest` (suggestion menu).
  - `applyAt(text, PopoverPosition, notation)` makes a **navigating** popover. It sits at a tick and staff instead of the selection, and opening it stops note input. Space / Shift-Space / Tab / Shift-Tab / Cmd-arrows enter the text and move on (`popover/popovernavigation.h`: `movePosition`, `itemAt`, `positionRect`). Beats by default; `stepsByChord` (spec 34, fingering) makes Space / Shift-Space step to the next / previous chord of the note's voice instead (`moveChord`, `chordAt`).
  - `openInline(notation)` (spec 33, lyrics): the Shift-key runs it instead of opening a box, for popovers typed straight into the score with MuseScore's own text editor. Their keys are `NOTATION_TEXT_EDITING` rows in `shortcuts_dorico.json` (that scope wins over the others); a `dorico/*` command meant for the editor must be enabled while editing (`DoricoCommandsState`, see `DORICO_LYRICS_*`).
- **Apply pattern** (see `handlers/chordsymbolshandler.cpp`, the smallest, or `tempohandler.cpp` for selection, caret and range):
  1. Get the score with `Score* score = notation->elements()->msScore()`.
  2. Open a transaction with `notation->undoStack()->prepareChanges(muse::TranslatableString("undoableAction", "Add ..."))`.
  3. Make the edits with `score->undoAddElement` / `undoRemoveElement` / `item->undoChangeProperty`.
  4. `commitChanges()`, or `rollbackChanges()` on failure.
  5. Select the new item, so Return reopens it.

  The controller merges several transactions into one undo step per entry. Items entered at the caret that need a note first: `handlers/pendingatcaret.h`.
- **Controller / UI**: `internal/popovercontroller.cpp` (open, accept, move, cancel). Its `existingItem()` only looks at segment annotations, so fingering (on the Note) does not use it: its chord is the edited item via `stepsByChord` (lyrics use `openInline` and don't need it). Key handling lives in `qml/MuseScore/Dorico/Popover.qml` and `popovermodel.*`.
- **Tests**: `tests/<x>parser_tests.cpp`, listed in `tests/CMakeLists.txt`. Build with `cmake -P build.cmake -DMUSE_ENABLE_UNIT_TESTS=ON build`, then run `<build dir>/src/dorico/tests/dorico_tests --gtest_filter='Dorico<X>*'`.
- **MuseScore internals already looked up**: `reference/musescore-internals.md` section 3 (APIs per item type, chord symbols). Add what you find there before ending the spec.

### 7. UI shell (specs 50-59)
- One `DockPage` per mode in `src/dorico/qml/MuseScore/Dorico/`. `WritePage` keeps `uri: "musescore://notation"` (so the Score tab and project opening need no change); the other four use `musescore://dorico/<mode>`. Switching mode = loading a page; the dock framework saves each page's layout separately.
- The only upstream edit for the shell is in `WindowContent.qml` (import `MuseScore.Dorico`, list the five pages, hide the Home/Score/Publish bar on Dorico pages). Spec 59 adds one hook in `appmenumodel.cpp`.
- Shared parts (`DoricoTopBar`, `DoricoStatusBar`, toolbox/panel button grid) are reused by all pages; existing MuseScore panels (palettes, instruments, properties, mixer, piano keyboard, timeline, percussion) are embedded, not copied.
- Dorico panels with no MuseScore backing are **stubs** ("Not available yet", disabled buttons with a tooltip) and are listed under each spec's Deferred section; nothing is silently dropped. Icons are MuseScore's, Bravura glyphs or Lucide (spec 60), never Dorico's.

## Spec template (every `NN-*.md`)

```
# NN - Title
Status: todo | in-progress | pending review | done  -  Depends on: NN, NN  -  Dorico refs: <urls>
## Goal
## Dorico behaviour            exact keys / syntax / edge cases, as tables
## MuseScore today             file:line of what exists and what is missing
## Design                      files to add/edit, functions to reuse, upstream edits called out
## Deferred / unsupported
## Acceptance criteria         checkbox list, each manually or unit testable
## Tests                       GoogleTest parser tests and/or a manual script
```

## Reference files

| File | Content |
|---|---|
| [reference/dorico-key-commands.md](reference/dorico-key-commands.md) | Full macOS key table by context, design grammar, caret visuals, gaps in the manual |
| [reference/dorico-popovers.md](reference/dorico-popovers.md) | Popover UI behaviour, Return / Opt-Return table, every entry-syntax table |
| [reference/dorico-editing.md](reference/dorico-editing.md) | Copy/paste/Paste Special, R, moves, delete, selection, Select More, filters, Jump bar, caret, grid, transpose/respell |
| [reference/dorico-ui.md](reference/dorico-ui.md) | Dorico UI inventory: toolbar, transport, status bar, zones, every toolbox / panel / menu per mode, with manual URLs |
| `reference/manual/` (gitignored) | Full local mirror of the Dorico Pro 6.2 manual: `index.md` (topic tree), `topics/**.md`, `images/*.png`. Built by `src/dorico/tools/fetch_manual.py` |
| [ROADMAP.md](ROADMAP.md) | Gap check of Dorico Pro 6 features vs this fork, proposed specs 61+ |
| [reference/musescore-internals.md](reference/musescore-internals.md) | Shortcuts v2 pipeline, note-input model, popup infrastructure, item-adding APIs, paste/selection/search code locations |

## Status table

| # | Spec | Status | Depends on |
|---|---|---|---|
| 00 | [Module skeleton](done/00-module-skeleton.md) | done | none |
| 10 | [Keymap preset](done/10-keymap-preset.md) | done | 00 |
| 11 | [Note-input modes](done/11-note-input-modes.md) | done | 00, 10 (land with 10) |
| 12 | [Caret and navigation](done/12-caret-and-navigation.md) | done | 00, 10, 11 |
| 13 | [Duration and rhythmic moves](done/13-duration-and-rhythmic-moves.md) | done | 00, 10, 12 (for the grid) |
| 20 | [Popover framework](done/20-popover-framework.md) | done | 00, 10 |
| 21 | [Popover: dynamics (Shift-D)](done/21-popover-dynamics.md) | done | 20 |
| 22 | [Popover: time signatures (Shift-M)](done/22-popover-time-signatures.md) | done | 20 |
| 23 | [Popover: key signatures (Shift-K)](done/23-popover-key-signatures.md) | done | 20 |
| 24 | [Popover: clefs and octave lines (Shift-C)](done/24-popover-clefs-octave-lines.md) | done | 20 |
| 25 | [Popover: bars and barlines (Shift-B)](done/25-popover-bars-barlines.md) | done | 20 |
| 26 | [Popover: tempo (Shift-T)](done/26-popover-tempo.md) | done | 20 |
| 27 | [Popover: holds and pauses (Shift-H)](done/27-popover-holds-pauses.md) | done | 20 |
| 28 | [Popover: ornaments (Shift-O)](done/28-popover-ornaments.md) | done | 20 |
| 29 | [Popover: playing techniques (Shift-P)](done/29-popover-playing-techniques.md) | done | 20 |
| 30 | [Popover: repeats and tremolos (Shift-R)](done/30-popover-repeats-tremolos.md) | done | 20 |
| 31 | [Popover: tuplets (;)](done/31-popover-tuplets.md) | done | 20, 11 |
| 32 | [Popover: chord symbols (Shift-Q)](done/32-popover-chord-symbols.md) | done | 20 |
| 33 | [Popover: lyrics (Shift-L)](done/33-popover-lyrics.md) | done | 20 |
| 34 | [Popover: fingering (Shift-F)](done/34-popover-fingering.md) | done | 20 |
| 35 | [Popover: note tools (Shift-I)](done/35-popover-note-tools.md) | done | 20 |
| 36 | [Popover: figured bass (Shift-G)](done/36-popover-figured-bass.md) | done | 20 |
| 40 | [Copy / paste](done/40-copy-paste.md) | done | 00, 10, 11 |
| 41 | [Paste Special](done/41-paste-special.md) | done | 40 |
| 42 | [Repeat (R) and Opt-click](done/42-repeat-and-alt-click.md) | done | 40 |
| 43 | [Delete](done/43-delete.md) | done | 11 |
| 44 | [Selection](done/44-selection.md) | done | 12 |
| 45 | [Filters](done/45-filters.md) | done | 44 |
| 46 | [Jump bar and Go To Bar](done/46-jump-bar.md) | done | 20 (overlay), 10 |
| 47 | [Explode / Reduce](done/47-explode-reduce.md) | done | 41 |
| 50 | [UI shell and modes](done/50-ui-shell-and-modes.md) | done | 00, 10 |
| 51 | [Status bar](done/51-status-bar.md) | done | 50 |
| 52 | [Write: Notes toolbox and panel](done/52-write-notes-toolbox-and-panel.md) | done | 50, 11 |
| 53 | [Write: Notations toolbox and panels](done/53-write-notations-toolbox-and-panels.md) | done | 50, 20 |
| 54 | [Write: lower zone](done/54-write-lower-zone.md) | done | 50 |
| 55 | [Setup mode](done/55-setup-mode.md) | done | 50 |
| 56 | [Engrave mode](done/56-engrave-mode.md) | done | 50 |
| 57 | [Play mode](done/57-play-mode.md) | done | 50 |
| 58 | [Print mode](done/58-print-mode.md) | done | 50, 55 |
| 59 | [Menus](done/59-menus.md) | done | 50 |
| 60 | [Appearance: colors, score canvas, icons](done/60-appearance.md) | done | 50 |

Specs 20-36 and 40-47 are listed here but written in a later pass. Specs 50-59 (UI shell) are all written; **implement 50 first**, then the rest in any order (58 after 55). Suggested model: Opus for 50, 53, 59; Sonnet is enough for 51, 52, 54-58. A spec file that does not exist yet is just not started.

### Dependency order

```
00 -> 10 -> 11 -> 12 -> 13
       |      \-> 31 (also needs 20)
       |      \-> 40 -> 41 -> 47
       |            \-> 42
       |      \-> 43
       |      12 -> 44 -> 45
       \-> 20 -> 21..36 (any order)
       \-> 20 -> 46

00 -> 10 -> 50 -> 51, 52 (needs 11), 53 (needs 20), 54, 55 -> 58, 56, 57, 59
```

Land **10 and 11 back to back**: spec 10 unbinds MuseScore's Shift+letter "add note to chord" keys and bare-letter behaviour; spec 11 gives them back as Chord mode (Q).

## Global deferred list (not planned; nothing is silently dropped)

| Dorico feature | Why deferred |
|---|---|
| Cues, **Shift-U** popover | MuseScore has no cue model |
| MIDI trigger regions, **Shift-0** popover | No equivalent model |
| Per-mode letters (Shift-I instrument picker, Shift-P add player, Shift-S system break, ...) and Cmd-1..5 key actions beyond switching mode | Modes themselves are now spec 50; the per-mode letter keys stay **reserved (unbound)** so nothing else claims them |
| Insert scopes beyond Voice (**Opt-I**: Player / Global / Global Adjustment), stop position (**Shift-Opt-I**) | Needs a score-wide insert engine. Insert mode (I) works as MuseScore's `TIMEWISE` for the current voice |
| Multi-staff caret (**Shift-Up/Down** during note input), caret across staves | Large `InputState` change |
| Slash voices (**Shift-Opt-V**), voice column index | No slash-voice model parity |
| Pitch before duration with Shift-held chromatic shadow notehead | Mouse feature, later |
| Rhythmic feel / swing entries in the tempo popover | Playback model change |
| Tablature special cases (`=`/`-` as duration keys, two-digit frets) | MuseScore TAB uses its own command set; left as is |
| Transposing-instrument input pitch toggle (Write > Input Pitch) | Menu only, no key |
| Engrave-mode nudging (Opt/Shift-Opt/Cmd-Opt + arrows) and graphic editing | MuseScore edit-mode differs |
| Home/Setup features: instruments panel flows (Shift-F add flow, Shift-E ensemble, Shift-L layout) | Out of the three requested areas |
| Page/Galley/Fill exact parity, Spreads / horizontal page arrangements | Spec 51 maps Page/Galley/Fill to `mode-page` / `mode-continuous` / `mode-float`; the rest is approximated or stubbed |
| Windows key set | Mac only for now |
| Scrubbing (hold Opt-Space), MIDI record (Cmd-R), solo (Opt-S) | No MuseScore equivalent yet |

## How to run a spec in a new session

1. Start in the repo root on branch `main` (or a feature branch if you prefer; the fork is personal).
2. Tell Claude: *"Implement `specs/NN-name.md`."* Claude must first read, in this order: `CLAUDE.md`, this `README.md`, the reference files the spec links, then the spec.
3. Check the spec's **Depends on** list. Every dependency must have `Status: done`. If not, stop and say so.
4. Set the spec's `Status:` line to `in-progress`.
5. Implement. Keep upstream edits to the ones the spec names. Add the new files to `src/dorico/CMakeLists.txt`.
6. Build and test: `cmake -P build.cmake build install`, run with `cmake -P build.cmake run`. For parser tests: `cmake -P build.cmake -DMUSE_ENABLE_UNIT_TESTS=ON build`, then run the `dorico_tests` binary from the build dir (created in spec 00).
7. Tick every box under **Acceptance criteria**. If one cannot be met, do not tick it; write the reason under **Deferred / unsupported** instead and tell the user.
8. Set `Status: done` in the spec **and** in the status table above. If every acceptance box is ticked, `git mv` the spec into `specs/done/` and fix links to it (table row, other specs' **Depends on**, its own `reference/` links become `../reference/`). Commit (docs and code separately if large). Commit messages end with the Co-Authored-By line given by the session.
9. If anything in a spec turned out wrong (a command id that does not exist, a Dorico behaviour that differs), fix the spec in the same commit.
10. Add any MuseScore internals you had to dig out (exact functions, what does not work and why) to `reference/musescore-internals.md` in a few lines, so the next spec does not read the same code again.
