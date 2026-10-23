<!-- markdownlint-disable -->

# CLAUDE.md

Personal fork of **MuseScore Studio** (GPLv3), base = upstream `main` (5.0-dev, not 4.x).
Long-term goal: add features and workflows _inspired by_ Dorico Pro, implemented independently.
Remotes: `origin` = the fork, `upstream` = github.com/musescore/MuseScore.
The `muse/` submodule is our fork too: `origin` = github.com/winscodellc/muse_framework (branch `dorico`, which carries our engine edits), `upstream` = github.com/musescore/muse_framework. Commit `muse/` edits on `dorico` and push them before committing the superproject's submodule pointer. After an upstream MuseScore merge moves the pointer, merge that upstream commit into `dorico` (`git -C muse merge <commit>`).

## Hard constraints

- Fork stays **GPLv3**; keep all existing copyright / SPDX headers. New files get the same SPDX header as neighbours.
- If distributed, it needs its **own name** (not "MuseScore"). Rename points: `version.cmake` (`MUSE_APP_NAME_*`, `MUSE_APP_GUI_IDENTIFIER`), `buildscripts/packaging/macOS/Info.plist.in`, icons/logos.
- **Keep fork code isolated** so upstream merges stay easy: put new features in their own module (`src/<name>/`), keep edits to upstream obvious. Merge upstream regularly (`git fetch upstream && git merge upstream/main`).
- Upstream `AI_POLICY.md` applies if anything is ever sent upstream (disclose with an `Assisted-by:` trailer). (NEVER SEND TO UPSTREAM OR CREATE A PR. THIS COPY WILL BE A NEW PRODUCT)

## About the user

Software developer (Java/Scala/JS/TS), **minimal C++**. Explain C++ idioms (headers vs .cpp, smart pointers, Qt signals/slots, `Q_PROPERTY`) briefly when they matter, using Java/TS analogies. Works on **Linux** (Arch/Omarchy, Hyprland, 8 cores, 15 GB RAM). Previously macOS: its build and Dorico UI-control notes are in `specs/reference/macos-setup.md`.

## Environment & build

- System Qt 6.11.2 (pacman; CI uses Qt 6.11.2 too), gcc 16 / clang 22, CMake >= 3.31, Ninja.
- `build_overrides.cmake` (gitignored, repo root) sets Ninja, `CPUS 6` (8 can exhaust RAM) and `compile_commands.json`. Build dir: `builds/Linux-Qt-usr-Ninja-RelWithDebInfo/`, binary `install/bin/mscore`. Root `compile_commands.json` is a symlink into it, for the clangd-lsp plugin (project scope).
- Submodules must be initialised: `git submodule update --init --recursive` (`muse/` = framework, `muse_deps/` = prebuilt third-party libs fetched automatically).

```sh
cmake -P build.cmake build install        # build + install (incremental)
cmake -P build.cmake run                  # run the installed app
cmake -P build.cmake -DCMAKE_BUILD_TYPE=Debug build install   # separate Debug build dir
cmake -P build.cmake clean                # delete the build dir
```

- Build in the background with output to a file, then report only `grep -m20 'error:'` and `tail -5`. Never stream build output.
- Unit tests: build with `-DMUSE_ENABLE_UNIT_TESTS=ON` (the build dir already has it), then run test binaries from the build dir (GoogleTest), e.g. `src/dorico/tests/dorico_tests`.
- Formatting: uncrustify via `./hooks/install.sh` (not installed yet).
- A dev build is `org.musescore.MuseScoreStudio5Development`, so its settings are separate from an installed MuseScore 4. No rename is needed to run side by side.
- Logs: `~/.local/share/MuseScore/MuseScoreStudio5Development/logs/`.

## Driving the UI (our build, headless)

- `src/dorico/tools/uicontrol/uictl.sh start <score>` runs our build alone in a headless Xvfb (`UICTL_VISIBLE=1` shows it in a Xephyr window instead); `stop` kills it (no save dialog). Commands: `key mscore <combo>` (`cmd` is sent as Ctrl), `type <text>`, `click <x> <y> [right|double|cmd|shift|alt|ctrl]`, `drag`, `move`, `wait <ms>`. See: `src/dorico/tools/uicontrol/shot.sh mscore out.png [x,y,w,h]`. Image tokens = w*h/750, so crop to what you check. Docs: `src/dorico/tools/uicontrol/README.md`.
- The test instance uses its own profile, `~/.cache/uictl-profile`. Test on temp copies of `demos/` scores.
- No Dorico on Linux, so compare against the manual.
- **Delegate UI test runs to the `ui-tester` agent** (`.claude/agents/ui-tester.md`; pass it the test list, score and build path; spawn it **without** a `model` override, its frontmatter pins Opus; the user wants Opus everywhere). UI testing in the main session was 60-77% of a spec session's tokens. Drive directly only for a one-off check, with the action and the shot in one Bash call.

## Architecture (paths relative to `src/`)

Modules are C++ libraries wired by an IoC container (think Spring): each has a `*module.cpp` implementing `muse::modularity::IModuleSetup` (`registerExports` -> `resolveImports` -> `onInit`). Services are declared by interface (`inotation.h`) and injected with `muse::GlobalInject<IFoo>` (app-wide) or `muse::ContextInject<IFoo> foo = { this };` (per window/project). Per-window state uses `IContextSetup` (see `palette/palettemodule.cpp`). The framework lives in the `muse/` submodule (IoC, actions, UI kit, audio, mpe); do not edit it casually.

- **engraving/** score model + layout engine, no UI.
  - `dom/`: `Score`/`MasterScore` -> `Measure` -> `Segment` -> `ChordRest` -> `Chord` -> `Note`; also `Dynamic`, `TimeSig`. Create items only via `Factory` (`dom/factory.h`).
  - `editing/`: mutations + undo (`cmd.cpp` `startCmd/endCmd`, `edit.cpp` `Score::undoAddElement`, `edittimesig.h` `EditTimeSig::addTimeSig`, `transaction/`). Change properties with `item->undoChangeProperty(Pid::X, v)`. Ending a transaction re-lays-out automatically.
  - `rendering/score/` layout, `rendering/single/` single-item render, `rw/` file read/write (one reader per format version), `style/`, `types/types.h` (`ElementType`), `playback/` (score -> playback events), `api/` plugin API.
  - Tests: `engraving/tests/` (GoogleTest, `.mscx` reference data + `ScoreRW`/`ScoreComp` helpers).
- **notation/** editing API above engraving. `INotation` (one open score) -> `INotationInteraction` (impl `internal/notationinteraction.cpp`: select, drag, `applyPaletteElement` ~L2224, `startEditText` ~L4141, `startEdit()`/`apply()` wrap an undo transaction), undo stack, selection.
- **notationscene/** score canvas + UI glue. `NotationActionController` (maps action/command ids to `INotationInteraction` calls; `registerCommand(...)`), `NotationViewInputController` (mouse/keys), `AbstractNotationPaintView` (`fromLogical`/`toLogical` convert score coords <-> view pixels), `elementpopups/` (in-score popups, e.g. `DynamicPopup.qml`).
- **palette/** `PaletteTree -> Palette -> PaletteCell` (a prototype element). Click -> `UserPaletteController::applyPaletteElement` (`internal/paletteprovider.cpp`) -> `notation->interaction()->applyPaletteElement`.
- **playback/** `PlaybackController` drives framework audio; `engraving/playback/PlaybackModel` builds the events; mixer UI is QML.
- **project/ context/ instrumentsscene/ propertiespanel/ preferences/ print/ importexport/ appshell/** project files, global context (`currentNotation()`), panels, import/export, main window + menus.
- **app/** `main.cpp`, `appfactory.cpp` (module list: `app->addModule(new X())`), `configs/data/shortcuts*.xml|json`.
- **QML**: lives in `src/<module>/qml/MuseScore/<Name>/`, built with `qt_add_qml_module`; C++ models are exposed with `QML_ELEMENT`, `Q_PROPERTY`, `Q_INVOKABLE`. Score canvas: `notationscene/qml/MuseScore/NotationScene/NotationView.qml` (contains `ElementPopupLoader`, `SearchPopup`).
- **Actions**: two coexisting systems. Legacy `UiAction` lists (`notationuiactions.cpp`) + shortcuts in `shortcuts*.xml`; newer `muse::rcommand::Command("command://...")` constants (`palette/palettecommands.h`) registered through `ICommandsRegister`, shortcuts in `shortcuts*.json`.

### Adding a new module `foo` (the only upstream edits needed)

1. Root `CMakeLists.txt`: `option(MUE_BUILD_FOO_MODULE ...)` (near the other module options, ~L142).
2. `src/CMakeLists.txt`: `if (MUE_BUILD_FOO_MODULE) add_subdirectory(foo) endif()` before `app`.
3. `src/app/CMakeLists.txt` (~L160-182): `add_to_link_if_exists(foo)` (also links `foo_qml` if present).
4. `src/app/app_config.h.in`: `#cmakedefine MUE_BUILD_FOO_MODULE 1`; `src/app/appfactory.cpp`: include + `app->addModule(new mu::foo::FooModule())` (~L435).
   Inside the module copy `src/palette/CMakeLists.txt` (`muse_create_module(foo)`) and `palette/qml/MuseScore/Palette/CMakeLists.txt` for QML.

## Dorico-clone work: specs

Goal: make this fork behave as close as possible to **Dorico Pro 6.2** (keys, popovers, copy/paste/select/filter/search).
**Behavioural reference = the Dorico Pro 6.2 manual**: <https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/introduction/introduction_c.html> (popover syntax: _Dorico Popover Reference 6.2_). Check every key and behaviour against it, not memory. **Local mirror (gitignored, Markdown + PNGs): `specs/reference/manual/`; grep `index.md` first, refetch with `python3 -I src/dorico/tools/fetch_manual.py`.** The rest is the fallback. Pages are JS-heavy (curl on a page URL gets only a JS shell). Fastest: the public content API, which curl reads: `https://www.steinberg.help/api/khub/maps/BYbh2hvBDwUCjSijIhHcqA/topics/<id>/content` (all 2,384 topics). Fallback: WebFetch the page with `offset` ~420000 (the article body sits after ~400k chars of navigation).
**Only Dorico's keys are bound**: any MuseScore binding that is not a Dorico key is removed (exemptions and enforcement: `specs/README.md`, "MuseScore shortcuts"). After an upstream merge run `python3 -I src/dorico/tools/check_keymap.py --fix`.
macOS key strings are **physical keys**: write `Shift+]`, never `}` (MuseScore drops the shift, so `}` fires on bare `]`). `src/dorico/tools/check_keymap.py` flags this. Work is specified in `specs/` (start at `specs/README.md`); each numbered spec is executed in its own session. Research lives in `specs/reference/` (Dorico manual extracts + MuseScore internals). Everything goes in the `src/dorico/` module (spec 00); Dorico key map is a shortcuts preset (spec 10); popovers are specs 20-36.
