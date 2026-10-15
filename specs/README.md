# Specs: turning this MuseScore fork into a Dorico Pro 6 clone

One spec = one implementation session. Read this file, the reference files it links, then the spec you were asked to do.

## Goal

Make this fork (MuseScore Studio 5.0-dev) behave as close as possible to **Dorico Pro 6.2** for three feature areas:

1. **Key commands**: Dorico's default macOS key set replaces MuseScore's.
2. **Popovers**: the Shift+letter text-entry boxes (dynamics, time signatures, ...).
3. **Editing**: copy / paste / paste special / duplicate / repeat / delete / selection / filters / search (Jump bar).
4. **UI shell**: Dorico's window layout (mode tabs, toolboxes, zones, status bar, menus) for all five modes (specs 50-59).

The Dorico manual is the behavioural reference: <https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/introduction/introduction_c.html> (popover syntax is in the separate *Dorico Popover Reference 6.2*).

## Decisions already made by the user

| Question | Decision |
|---|---|
| Spec granularity | Small phased specs, each sized for one session |
| MuseScore shortcuts that conflict with Dorico | **Dorico wins.** MuseScore-only commands keep their key if it is free, otherwise they become unbound (still reachable from menus and Preferences) |
| Keyboard layout | **US English, macOS.** Windows is out of scope for now (Dorico's Windows set differs, e.g. Ctrl-Alt-letter instead of Ctrl-letter for "note below") |
| Location | `specs/` at the repo root, committed |

## Hard constraints (from `CLAUDE.md`)

- **No Dorico code, assets, icons or proprietary file formats.** Work from MuseScore's code plus the *publicly documented behaviour*. Key bindings and entry-syntax tokens are facts, so they are fine. Popover icons reuse MuseScore palette icons.
- Fork stays **GPLv3**. Keep all copyright / SPDX headers. New files get the same SPDX header as their neighbours (copy it from `src/palette/palettemodule.cpp`).
- **Keep fork code isolated**: new features go in `src/dorico/`. Edits to upstream files must be tiny and obvious (each spec lists them under "upstream edits"). Merge upstream regularly (`git fetch upstream && git merge upstream/main`).
- If distributed it needs its own name (not "MuseScore"). Not in scope for these specs.
- The framework in `muse/` (submodule) is not edited.
- User is a Java/Scala/TS developer with minimal C++: explain C++ idioms briefly when they matter (headers vs .cpp, smart pointers, Qt signals/slots, `Q_PROPERTY`), using Java/TS analogies.

## Cross-cutting design (every spec follows this)

### 1. One fork module: `src/dorico/`
Namespace `mu::dorico`, commands `command://dorico/<name>`. It is created in spec 00 by copying the layout of `src/palette/`. Upstream touch points are the 4 standard ones in `CLAUDE.md` ("Adding a new module") plus:
- a single QML `Loader` in `src/notationscene/qml/MuseScore/NotationScene/NotationView.qml` (next to `SearchPopup`, ~L344) that hosts the popover / Jump-bar overlay (spec 20);
- `shortcuts.cfg` + `app.qrc` entries for the `dorico` shortcut preset (spec 10).

### 2. Changing behaviour = new commands, not overriding old ones
The command dispatcher allows **one handler per command** (`muse/framework/rcommand/internal/commanddispatcher.cpp:135` refuses duplicates), so a module cannot replace `command://notation/paste`. Instead:
- Our key map points the key at a new `command://dorico/...` command, whose handler calls the public `INotationInteraction` / `INotationNoteInput` API (`src/notation/inotationinteraction.h`, `src/notation/inotationnoteinput.h`).
- `ICommandDispatcher::preDispatch` (veto hook) is a fallback only, for menu items that still call the old command.

### 3. Mode-dependent keys are ONE branching command
Shortcut **scopes do not gate anything** (`src/context/internal/shortcutresolver.cpp`: scope only sorts candidates). What decides whether a command fires is its **enabled state** (`ICommandsState`). Two enabled commands on one key: the resolver picks by scope priority, which is fragile.
Rule: a key whose meaning depends on note-input state (Return, Space, Esc, A-G, F, Q, ...) binds to **one** `dorico/...` command that checks `INotationNoteInput::isNoteInputMode()` and branches inside.

### 4. The key map is a preset diff
`src/app/configs/data/shortcuts_dorico.json` is applied on top of `shortcuts.json` (+ `shortcuts_mac.json`). Specs 11-13 and 20-46 each add their own rows to it (each spec lists "Adds to `shortcuts_dorico.json`"). Spec 10 creates the file, the unbinding, and the preset plumbing. Details in [10-keymap-preset.md](10-keymap-preset.md).

### 5. Fidelity rule
Each spec has a **Dorico behaviour** section copied from the reference files, with manual URLs. Anything MuseScore cannot do yet goes under **Deferred / unsupported**. Nothing is silently dropped. When Dorico documents nothing (for example popover error state), the spec says "our choice" and picks the simplest behaviour.

### 6. Parsers are pure functions
Every popover is `parse(QString) -> Result` (no UI, no score) plus `apply(Result, INotationInteraction&)`. The parser is unit-tested with GoogleTest. Same for the Jump bar and `SearchCommandsParser` extensions.

### 7. UI shell (specs 50-59)
- One `DockPage` per mode in `src/dorico/qml/MuseScore/Dorico/`. `WritePage` keeps `uri: "musescore://notation"` (so the Score tab and project opening need no change); the other four use `musescore://dorico/<mode>`. Switching mode = loading a page; the dock framework saves each page's layout separately.
- The only upstream edit for the shell is in `WindowContent.qml` (import `MuseScore.Dorico`, list the five pages, hide the Home/Score/Publish bar on Dorico pages). Spec 59 adds one hook in `appmenumodel.cpp`.
- Shared parts (`DoricoTopBar`, `DoricoStatusBar`, toolbox/panel button grid) are reused by all pages; existing MuseScore panels (palettes, instruments, properties, mixer, piano keyboard, timeline, percussion) are embedded, not copied.
- Dorico panels with no MuseScore backing are **stubs** ("Not available yet", disabled buttons with a tooltip) and are listed under each spec's Deferred section; nothing is silently dropped. Icons are MuseScore's, never Dorico's.

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
| [reference/musescore-internals.md](reference/musescore-internals.md) | Shortcuts v2 pipeline, note-input model, popup infrastructure, item-adding APIs, paste/selection/search code locations |

## Status table

| # | Spec | Status | Depends on |
|---|---|---|---|
| 00 | [Module skeleton](00-module-skeleton.md) | done | none |
| 10 | [Keymap preset](10-keymap-preset.md) | done | 00 |
| 11 | [Note-input modes](11-note-input-modes.md) | pending review | 00, 10 (land with 10) |
| 12 | [Caret and navigation](12-caret-and-navigation.md) | todo | 00, 10, 11 |
| 13 | [Duration and rhythmic moves](13-duration-and-rhythmic-moves.md) | todo | 00, 10, 12 (for the grid) |
| 20 | [Popover framework](20-popover-framework.md) | todo | 00, 10 |
| 21 | [Popover: dynamics (Shift-D)](21-popover-dynamics.md) | todo | 20 |
| 22 | [Popover: time signatures (Shift-M)](22-popover-time-signatures.md) | todo | 20 |
| 23 | [Popover: key signatures (Shift-K)](23-popover-key-signatures.md) | todo | 20 |
| 24 | [Popover: clefs and octave lines (Shift-C)](24-popover-clefs-octave-lines.md) | todo | 20 |
| 25 | [Popover: bars and barlines (Shift-B)](25-popover-bars-barlines.md) | todo | 20 |
| 26 | [Popover: tempo (Shift-T)](26-popover-tempo.md) | todo | 20 |
| 27 | [Popover: holds and pauses (Shift-H)](27-popover-holds-pauses.md) | todo | 20 |
| 28 | [Popover: ornaments (Shift-O)](28-popover-ornaments.md) | todo | 20 |
| 29 | [Popover: playing techniques (Shift-P)](29-popover-playing-techniques.md) | todo | 20 |
| 30 | [Popover: repeats and tremolos (Shift-R)](30-popover-repeats-tremolos.md) | todo | 20 |
| 31 | [Popover: tuplets (;)](31-popover-tuplets.md) | todo | 20, 11 |
| 32 | [Popover: chord symbols (Shift-Q)](32-popover-chord-symbols.md) | todo | 20 |
| 33 | [Popover: lyrics (Shift-L)](33-popover-lyrics.md) | todo | 20 |
| 34 | [Popover: fingering (Shift-F)](34-popover-fingering.md) | todo | 20 |
| 35 | [Popover: note tools (Shift-I)](35-popover-note-tools.md) | todo | 20 |
| 36 | [Popover: figured bass (Shift-G)](36-popover-figured-bass.md) | todo | 20 |
| 40 | [Copy / paste](40-copy-paste.md) | todo | 00, 10, 11 |
| 41 | [Paste Special](41-paste-special.md) | todo | 40 |
| 42 | [Repeat (R) and Opt-click](42-repeat-and-alt-click.md) | todo | 40 |
| 43 | [Delete](43-delete.md) | todo | 11 |
| 44 | [Selection](44-selection.md) | todo | 12 |
| 45 | [Filters](45-filters.md) | todo | 44 |
| 46 | [Jump bar and Go To Bar](46-jump-bar.md) | todo | 20 (overlay), 10 |
| 47 | [Explode / Reduce](47-explode-reduce.md) | todo | 41 (lowest priority) |
| 50 | [UI shell and modes](50-ui-shell-and-modes.md) | pending review | 00, 10 |
| 51 | [Status bar](51-status-bar.md) | todo | 50 |
| 52 | [Write: Notes toolbox and panel](52-write-notes-toolbox-and-panel.md) | todo | 50, 11 |
| 53 | [Write: Notations toolbox and panels](53-write-notations-toolbox-and-panels.md) | todo | 50, 20 |
| 54 | [Write: lower zone](54-write-lower-zone.md) | todo | 50 |
| 55 | [Setup mode](55-setup-mode.md) | todo | 50 |
| 56 | [Engrave mode](56-engrave-mode.md) | todo | 50 |
| 57 | [Play mode](57-play-mode.md) | todo | 50 |
| 58 | [Print mode](58-print-mode.md) | todo | 50, 55 |
| 59 | [Menus](59-menus.md) | todo | 50 |

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

1. Start in `/Users/winsvu/Projects/MuseScore` on branch `main` (or a feature branch if you prefer; the fork is personal).
2. Tell Claude: *"Implement `specs/NN-name.md`."* Claude must first read, in this order: `CLAUDE.md`, this `README.md`, the reference files the spec links, then the spec.
3. Check the spec's **Depends on** list. Every dependency must have `Status: done`. If not, stop and say so.
4. Set the spec's `Status:` line to `in-progress`.
5. Implement. Keep upstream edits to the ones the spec names. Add the new files to `src/dorico/CMakeLists.txt`.
6. Build and test: `cmake -P build.cmake build install`, run with `cmake -P build.cmake run`. For parser tests: `cmake -P build.cmake -DMUSE_ENABLE_UNIT_TESTS=ON build`, then run the `dorico_tests` binary from the build dir (created in spec 00).
7. Tick every box under **Acceptance criteria**. If one cannot be met, do not tick it; write the reason under **Deferred / unsupported** instead and tell the user.
8. Set `Status: done` in the spec **and** in the status table above. Commit (docs and code separately if large). Commit messages end with the Co-Authored-By line given by the session.
9. If anything in a spec turned out wrong (a command id that does not exist, a Dorico behaviour that differs), fix the spec in the same commit.
