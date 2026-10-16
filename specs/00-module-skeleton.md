<!-- markdownlint-disable -->
# 00 - Module skeleton (`src/dorico/`)
Status: done  -  Depends on: none  -  Dorico refs: none (infrastructure)

## Goal
Create the empty fork module that every later spec fills in: it builds, links into the app, registers one no-op command, and has an (empty) QML module and a unit-test target. Nothing user-visible changes.

## Dorico behaviour
None. This is plumbing. Naming used by all later specs:

| Thing | Name |
|---|---|
| C++ namespace | `mu::dorico` |
| CMake target / `muse_create_module` name | `dorico` |
| Module class | `mu::dorico::DoricoModule` (global part) and `mu::dorico::DoricoContext` (per-window part) |
| Command URI prefix | `command://dorico/` |
| CMake options | `MUE_BUILD_DORICO_MODULE`, `MUE_BUILD_DORICO_QML`, `MUE_BUILD_DORICO_TESTS` |
| QML module URI | `MuseScore.Dorico` (target `dorico_qml`) |
| Test binary | `dorico_tests` |

## MuseScore today
The template to copy is `src/palette/`. It shows every piece needed:
- `src/palette/CMakeLists.txt`: `muse_create_module(palette)` + `target_sources(palette PRIVATE ...)` + `target_link_libraries(palette PRIVATE engraving Qt::Widgets)` + `if (MUE_BUILD_PALETTE_QML) add_subdirectory(qml/MuseScore/Palette) endif()`.
- `src/palette/palettemodule.h/.cpp`: `IModuleSetup` (`moduleName`, `registerExports`, `resolveImports`, `onInit`, `newContext`) and `IContextSetup` (`registerExports`, `resolveImports`, `onInit`, `onAllInited`, `onDeinit`). Think Spring: `registerExports` = publish beans, `resolveImports` = look up beans from other modules, `onInit` = `@PostConstruct`.
- `src/palette/palettecommands.h`: `inline static const muse::rcommand::Command X("command://palette/x");` constants.
- `src/palette/internal/palettecommandsregister.h/.cpp`: `IModuleCommandsRegister` returning a `static const std::vector<CommandInfo>` (title, description, `InputSchema`, `Decoration`).
- `src/palette/internal/palettecommandsstate.h/.cpp`: `IModuleCommandsState::commandState(cmd) -> CommandState(enabled, checked)`; recomputed on events and pushed via `commandStateChanged()`.
- `src/palette/internal/paletteactionscontroller.cpp:50-63`: `commandDispatcher()->onRequest(this, CMD, [this]() { ...; return muse::make_ok(); })`.
- `src/palette/qml/MuseScore/Palette/CMakeLists.txt`: `muse_create_qml_module(palette_qml FOR palette)` + `qt_add_qml_module(palette_qml URI MuseScore.Palette VERSION 1.0 ...)` + `fixup_qml_module_dependencies(palette_qml)`.
- Upstream hook points (all verified): root `CMakeLists.txt:142` (`option(MUE_BUILD_PALETTE_MODULE ...)`), `src/CMakeLists.txt:68-72` (`add_subdirectory(palette)` / stub), `src/app/CMakeLists.txt:178` (`add_to_link_if_exists(palette)`, which also links `palette_qml` if the target exists), `src/app/app_config.h.in:28`, `src/app/appfactory.cpp:261-264` (include) and `:434` (`app->addModule(new mu::palette::PaletteModule());`).
- C++ note: `GlobalInject<IFoo>` / `ContextInject<IFoo> foo = { this };` are field-style `@Inject`. `ContextInject` needs the class to derive from `muse::Contextable`. Headers (`.h`) declare, `.cpp` define; a class used as a pointer can be forward-declared in the header (`class PaletteConfiguration;`) like `palettemodule.h` does.

## Design

### New files
```
src/dorico/CMakeLists.txt
src/dorico/doricomodule.h / .cpp
src/dorico/doricocommands.h                       // Command constants
src/dorico/internal/doricocommandsregister.h/.cpp // CommandInfo list
src/dorico/internal/doricocommandsstate.h/.cpp    // enabled/checked state
src/dorico/internal/doricoactionscontroller.h/.cpp// dispatcher handlers
src/dorico/qml/MuseScore/Dorico/CMakeLists.txt    // empty QML module (one placeholder .qml)
src/dorico/qml/MuseScore/Dorico/DoricoPlaceholder.qml
src/dorico/tests/CMakeLists.txt, doricomodule_tests.cpp   // GoogleTest smoke
```
Copy the SPDX header block from `src/palette/palettemodule.cpp` into each file.

### `src/dorico/CMakeLists.txt`
```cmake
muse_create_module(dorico)
target_sources(dorico PRIVATE
    doricomodule.cpp doricomodule.h doricocommands.h
    internal/doricocommandsregister.cpp internal/doricocommandsregister.h
    internal/doricocommandsstate.cpp internal/doricocommandsstate.h
    internal/doricoactionscontroller.cpp internal/doricoactionscontroller.h
)
target_link_libraries(dorico PRIVATE engraving)
if (MUE_BUILD_DORICO_QML)
    add_subdirectory(qml/MuseScore/Dorico)
endif()
if (MUE_BUILD_DORICO_TESTS)
    add_subdirectory(tests)
endif()
```
Copy the tests wiring from `src/notationscene/tests/CMakeLists.txt` (look at how `MUE_BUILD_NOTATIONSCENE_TESTS` is consumed). Interface headers from `notation/`, `notationscene/`, `context/` are found via the include paths `muse_create_module` sets up; implementations are resolved through the IoC container at runtime, so no `notation` link is needed. Add one only if the linker complains.

### The no-op command (proves registration + shortcut visibility)
`doricocommands.h`:
```cpp
inline static const muse::rcommand::Command DORICO_NOOP_COMMAND("command://dorico/noop");
```
`DoricoCommandsRegister`: one `CommandInfo{ DORICO_NOOP_COMMAND, TranslatableString("dorico","Dorico no-op"), TranslatableString("dorico","Does nothing; proves the dorico module is wired"), InputSchema(), Decoration() }`. Check the `Availability` default in `muse/framework/rcommand/commandtypes.h`. The palette commands use the default and appear in Preferences > Shortcuts, so the default is enough.
`DoricoActionsController::init()`: `commandDispatcher()->onRequest(this, DORICO_NOOP_COMMAND, [] { LOGI() << "dorico noop"; return muse::make_ok(); });`
`DoricoCommandsState::commandState`: `CommandState(true, false)` when a project is open (copy `isProjectOpened()` from the palette state).

### `DoricoModule`
`moduleName()` returns `"dorico"`. `resolveImports()` registers `std::make_shared<DoricoCommandsRegister>()` with `muse::rcommand::ICommandsRegister` (same as `palettemodule.cpp` L82-85). `DoricoContext::resolveImports()` registers `DoricoCommandsState` with `ICommandsState`; `onInit` calls `m_actionsController->init()`. Specs 10-47 add their registers/handlers by appending to these classes (one `CommandInfo` vector, one controller `init()`); keep each feature's handlers in its own `internal/<feature>*.cpp` and call them from `init()` to avoid one giant file.

### Upstream edits (the only ones in this spec, all tiny)
1. Root `CMakeLists.txt` near L142:
   ```cmake
   option(MUE_BUILD_DORICO_MODULE "Build dorico (fork) module" ON)
   option(MUE_BUILD_DORICO_QML "Build dorico QML" ${MUE_BUILD_DORICO_MODULE})
   option(MUE_BUILD_DORICO_TESTS "Build dorico tests" ${MUE_BUILD_DORICO_MODULE})
   ```
2. `src/CMakeLists.txt` before `app`: `if (MUE_BUILD_DORICO_MODULE) add_subdirectory(dorico) endif()`.
3. `src/app/CMakeLists.txt` near L178: `add_to_link_if_exists(dorico)`.
4. `src/app/app_config.h.in`: `#cmakedefine MUE_BUILD_DORICO_MODULE 1`. `src/app/appfactory.cpp`: `#ifdef MUE_BUILD_DORICO_MODULE #include "dorico/doricomodule.h" #endif`, then after the palette line (~L435) `#ifdef MUE_BUILD_DORICO_MODULE app->addModule(new mu::dorico::DoricoModule()); #endif`. No stub module is needed because the include and registration are guarded.

### QML module
`src/dorico/qml/MuseScore/Dorico/CMakeLists.txt`: copy the palette one with URI `MuseScore.Dorico`, target `dorico_qml`, `IMPORTS TARGET muse_ui_qml TARGET muse_uicomponents_qml`, one placeholder file `DoricoPlaceholder.qml` (`import QtQuick  Item {}`). Later specs add real QML files here.

## Deferred / unsupported
- A stub module for `MUE_BUILD_DORICO_MODULE=OFF`: not needed (everything is `#ifdef`-guarded).
- Renaming the app: out of scope (see `CLAUDE.md`).

- Not verified by a build (user chose incremental verification): the clean-configure build and the `MUE_BUILD_DORICO_MODULE=OFF` build. The OFF case was checked by inspection only: the `appfactory.cpp` include and `addModule`, and the `src/CMakeLists.txt` subdirectory, are all under `MUE_BUILD_DORICO_MODULE` guards. Run both builds before relying on them.

## Acceptance criteria
- [ ] `cmake -P build.cmake build install` succeeds from a clean configure (delete the build dir first: `cmake -P build.cmake clean`).
- [x] `cmake -P build.cmake run` starts the app. The log has a line from `DoricoModule::onInit` ("dorico module init").
- [x] Preferences > Shortcuts lists "Dorico no-op" with an empty shortcut.
- [x] Dispatching `command://dorico/noop` logs "dorico noop" (assign a temporary shortcut in Preferences and press it).
- [ ] Configuring with `-DMUE_BUILD_DORICO_MODULE=OFF` still builds and runs (upstream edits are guarded).
- [x] `git diff --stat` for upstream (non-`src/dorico`) files touches exactly: `CMakeLists.txt`, `src/CMakeLists.txt`, `src/app/CMakeLists.txt`, `src/app/app_config.h.in`, `src/app/appfactory.cpp`.
- [x] `cmake -P build.cmake -DMUSE_ENABLE_UNIT_TESTS=ON build` builds `dorico_tests`, and its one smoke test passes.

## Tests
`dorico_tests` contains one GoogleTest: `TEST(DoricoModule, CommandListContainsNoop)` constructs `DoricoCommandsRegister` and asserts `commandList()` contains `DORICO_NOOP_COMMAND`. Manual: the acceptance items above.
