<!-- markdownlint-disable -->
# 20 — Popover framework
Status: pending review · Depends on: 00, 10 (key preset file exists), 11 (note-input commands) · Dorico refs: [popover concept](https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/program_concepts/program_concepts_popovers_c.html), [Notations toolbox](https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_toolbox_r.html), [popover navigation](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_navigation_r.html)

Reference: `specs/reference/dorico-popovers.md` (B0, B1), `specs/reference/musescore-internals.md` (§3, §6).

## Goal
One reusable overlay (text box anchored above the caret or selection) plus a registry of per-popover handlers. Specs 21–36 each add one handler without touching the framework. After this spec: a `Shift-<letter>` opens an empty popover with the right icon, Return applies, Esc cancels, and a dummy handler proves the whole path.

## Dorico behaviour
Everything below is stated by the manual (see reference B1) unless marked *(ours)*.

| Aspect | Behaviour |
|---|---|
| Availability | Write mode only; only "during note input or when at least one note/item is selected". |
| Position | Above the top staff on which the caret is active or an item is selected, at the caret x or the x of the earliest selected item. |
| Anatomy | Drag handle (moves it vertically), icon at left (same icon as the matching Notations toolbox button), entry field. Dynamics, key sig, time sig, bars/barlines, chord symbols, figured bass also show a Local/Global indicator. |
| Confirm | **Return** applies and closes (global/default variant). |
| Local variant | **Opt-Return** applies the local variant (see each spec; table in reference B1). |
| Cancel | **Esc** closes without applying. |
| Reopen to edit | With a single item of a popover type selected, **Return** reopens that popover pre-filled with the item's current entry text. Entering a new key sig / time sig / barline over an existing one replaces it. |
| Range / single / caret | Caret: item goes at the caret on all staves the caret spans; open-ended items (`p<`, `8va`, `vibrato->`) are created once the caret advances and keep extending until stopped. Single selection: item goes at the earliest selected item. Range: items with duration span the range; clefs add a restorative clef at the end; `|:` adds a matching `:|` at the end. |
| Insert position pref | "Create items at position of first selected note" (default on): item goes at the first selected *note*, not the first selected rest. |
| Suggestions | Documented only for tempo, playing techniques, pedal lines, cues, jump bar. Up/Down chooses, Return applies. |
| Errors | Undocumented. *(ours)*: neutral while the text is a valid prefix; red border when invalid; Return is a no-op while red. |
| Chord-symbol popover | Stops note input when opened (spec 32). |

## MuseScore today
- No popover exists. Closest precedents:
  - `SearchPopup.qml` (`src/notationscene/qml/MuseScore/NotationScene/internal/SearchPopup.qml`, hosted in `NotationView.qml:166,344`): a `TextInputField` whose text drives a model.
  - `DynamicPopup.qml` + `AbstractElementPopupModel` (`.../abstractelementpopupmodel.h`): anchored to an element via `itemRect = fromLogical(item->canvasBoundingRect())`. Its type enum is closed, so do **not** add popover types there (~4 upstream edits).
  - `toggleDynamicPopup` (`src/notation/internal/notationinteraction.cpp:5827`) is MuseScore's Ctrl-D, an in-place text edit. It stays; Shift-D gets the new popover.
- Key dispatch: `NotationViewInputController::shortcutOverrideEvent/keyPressEvent` (`notationviewinputcontroller.cpp:1441,1465`) handles Enter (starts text edit on the selected element) and Esc **before** shortcuts are consulted. Return-to-reopen must therefore either be handled there (one tiny upstream hook) or the Dorico preset must make Enter a `dorico` command, which then needs verifying that the override does not swallow it (see Design, "Return conflict").
- Command system: palette module is the template (`src/palette/palettemodule.cpp`, `palettecommands.h`, `paletteactionscontroller.cpp:53-65`).

## As built (wins over the Design section below where they differ)
- **No upstream edit at all.** The overlay is `src/dorico/qml/MuseScore/Dorico/Popover.qml`, created by a `Loader` in our own `DoricoPage.qml` (Write page only), parented to `notationView.paintView`, so it works in view pixels. `PopoverModel.viewMatrix` is bound to the paint view's `matrix` (a `QTransform`); the canvas anchor is mapped in C++, so scroll/zoom move the box. No `NotationView.qml` Loader, no `fromLogical` invokable.
- **Return on a selected text item** (dynamic, tempo, ...) never reaches `command://dorico/return`: `NotationViewInputController::shortcutOverrideEvent` claims it and dispatches `command://notation/screen-edit-text`. `PopoverController` vetoes that in `ICommandDispatcher::preDispatch` when a popover owns the item, and opens the popover instead. Non-text items reach `dorico/return`, whose handler (`DoricoNoteInputController::onReturn`) now calls `IPopoverController::openForSelection()` when the selection is not a note/rest.
- **No `popover/reopen` or `popover/close` command.** Reopen is the existing `dorico/return`; Esc is handled inside the field (binding Esc to a command would clash with `notation/cancel` in `check_keymap.py`).
- **Field**: a plain `TextInput` with `Keys.onShortcutOverride: event.accepted = true`, so no score shortcut fires while typing (MuseScore's `TextInputField` lets Up/Down through). Return → apply; Opt-Return (`Qt.AltModifier`) → local; Esc → cancel; Up/Down/Tab swallowed. Losing focus (click elsewhere) cancels, so a popover never acts on a selection made after it opened. Empty text + Return closes *(ours)*.
- **Types**: `PopoverDef` (`popover/popovertypes.h`) has `itemTypes` (a list of `ElementType`) instead of a `handlesItem` lambda; `apply(text, scope, INotationPtr)` acts on the current selection (when reopened, the item is still selected). `ParseResult<T>` is left to each parser spec. `suggest` moved to spec 26, `PendingSpanner` to spec 21 (first users).
- **Registry**: `popover/popoverregistry.{h,cpp}`, `makePopoverRegistry()` lists all 16 popovers. Each spec 21-36 replaces its `placeholder(...)` line (empty → Incomplete, anything else → Unsupported). `temporaryDynamics()` accepts MuseScore dynamic names (`p`, `mf`, `sfz`, ...) to prove the path; spec 21 replaces it.
- **Editing an existing item**: opening a popover with an item of its type selected, or on a note/rest that already has one in its segment's annotations on that staff (Shift-D on a note with a dynamic), pre-fills the item's text; on apply that item is selected first, so the entry replaces it instead of adding a second one (MuseScore's `ChordRest::drop` would just add). `// ponytail:` only annotations are searched; specs whose items live elsewhere (clefs, key signatures, fingering) extend `PopoverController::existingItem`.
- **Undo**: `INotationInteraction` has no `startEdit()/apply()`. `PopoverController::accept` records `INotationUndoStack::currentStateIndex()`, calls `apply`, then `mergeTransactions(start)` if the handler made more than one transaction: one undo step per Return.
- **Availability**: checked in the command handler (Write mode, project open, not editing text/lyrics, selection or note input), so `DoricoCommandsState` needs no selection subscription; the commands stay enabled and are no-ops otherwise.
- **Anchor**: caret rect during note input; otherwise the selection's canvas rect (or the earliest item of a list selection), its top raised to the top line of the item's staff (`System::staffCanvasYpage`). The box sits above it, clamped to the view, flipped below if there is no room above.
- Key rows: `shortcuts_dorico.json` binds Shift+B/C/D/F/G/H/I/K/L/M/O/P/Q/R/T and `;`; `dorico_keys.json` rows changed from `reserved:NN` to the command ids (owner 20). `Shift+;` stays reserved for spec 31.

## Design

### Files (all new, inside `src/dorico/`)
```
popover/popovertypes.h          ParseResult<T>, ParseStatus, ApplyScope, PopoverDef
popover/popoverregistry.{h,cpp} id -> PopoverDef; registered in DoricoModule::registerExports
popover/popovercontroller.{h,cpp}  open/close/apply, owns current state, reacts to commands
popover/parsers/<name>parser.{h,cpp}   PURE parsers, std/Qt-core only, no engraving includes
popover/handlers/<name>handler.{h,cpp} apply() + prefill() against INotationInteraction
qml/MuseScore/Dorico/Popover.qml       the overlay
qml/MuseScore/Dorico/PopoverModel.{h,cpp}  QML_ELEMENT, Q_PROPERTYs below
tests/<name>parser_tests.cpp            GoogleTest, one file per parser
```
Java/TS analogy for the C++ reader: a `.h` is the interface/type declaration, the `.cpp` the implementation; `PopoverDef` is a plain record of `std::function`s (like a TS object with callbacks), not a class hierarchy.

### Core types (`popovertypes.h`)
```cpp
enum class ParseStatus { Ok, Incomplete, Unsupported, Invalid };
// Incomplete = valid prefix of something (don't show red, Return is a no-op)
// Unsupported = valid Dorico syntax we deliberately do not implement (see each spec's Deferred list)
template<class T> struct ParseResult { ParseStatus status; std::optional<T> value; std::string message; };
enum class ApplyScope { Default, Local };            // Return / Opt-Return

struct PopoverDef {
    std::string id;                  // "dynamics"
    std::string iconName;            // reuse an existing MuseScore icon, never a Dorico asset
    bool supportsLocal = false;      // shows/accepts Opt-Return
    std::function<ParseStatus(const std::string&)> validate;       // wraps the typed parser, for live red/neutral state
    std::function<bool(const std::string&, ApplyScope, INotationPtr)> apply;   // true = applied
    std::function<std::string(const EngravingItem*)> prefill;      // text for Return-to-edit, "" if item not owned
    std::function<bool(const EngravingItem*)> handlesItem;         // which selected item types reopen this popover
    std::function<std::vector<std::string>(const std::string&)> suggest;  // may be empty
};
```
Each handler registers a `PopoverDef` whose `validate`/`apply` call its own typed parser. Parsers return a handler-specific struct (e.g. `DynamicEntry`); they never include `engraving/`, so they are unit-testable without a score.

### Opening: commands, not key handlers
- Register one command per popover: `command://dorico/popover/<id>` (`dynamics`, `timesig`, `keysig`, `clef`, `bars`, `tempo`, `holds`, `ornaments`, `techniques`, `repeats`, `tuplets`, `chordsymbols`, `lyrics`, `fingering`, `notetools`, `figuredbass`).
- Plus `command://dorico/popover/reopen` (bound to Return) and `command://dorico/popover/close` (Esc, only enabled while open).
- `CommandInfo` uses `Availability::Shortcut | Dispatch`. Handlers are bound in `DoricoModule` with `commandDispatcher()->onRequest(this, CMD, ...)` exactly like `paletteactionscontroller.cpp:53-65`.
- **State** (`ICommandsState`, registered in the context setup like palette): enabled iff a score is open, **not** text-editing, and (selection non-empty **or** note input active). Spec 10 owns the key file; this spec supplies the command ids it binds (`Shift+B/C/D/F/G/H/I/K/L/M/O/P/Q/R/T`, `;` for tuplets, Return, Esc-while-open). Cues (Shift-U) and MIDI trigger regions (Shift-0) are not registered (Deferred).
- Opening a popover while another is open replaces it (Dorico: one popover at a time).
- Popover is **not** opened in Setup-like contexts: Setup-mode meanings of Shift-I/P/F/L are out of scope for the fork.

### Overlay (`Popover.qml` + `PopoverModel`)
- Hosted by **one new `Loader` in `NotationView.qml` next to `SearchPopup` (~L344)** — this is the framework's only upstream QML edit. It sources `MuseScore.Dorico.Popover`.
- `PopoverModel` Q_PROPERTYs: `open`, `text`, `iconSource`, `anchorRect` (view coords), `status` (enum mirrors `ParseStatus`), `showLocalIndicator`, `scopeLocal`, `suggestions`, `currentSuggestion`. Q_INVOKABLEs: `accept(bool local)`, `cancel()`, `textEdited(QString)`, `moveSuggestion(int)`.
- **Anchor**: controller computes a logical rect:
  - note input active → `INotationNoteInput::cursorRect()`;
  - else → `INotationSelection::canvasBoundingRect()` of the earliest selected item (respect the "first selected note" preference: skip rests when a note is in the range).
  Convert with the paint view's `fromLogical` (`AbstractNotationPaintView`). *Open point to verify*: `fromLogical` may not be callable from QML. Preferred fix is to pass the view into the model (`PopoverModel { view: notationView }` hosted in `NotationView.qml`); if `fromLogical` is not `Q_INVOKABLE`, adding it is a second, tiny upstream edit. Place the box with its bottom edge above the **top staff's** top line (`staff top y` from the same rect expanded to the staff), left edge at anchor x; clamp inside the view; flip below the staff only if there is no room above.
- Drag handle: purely visual vertical offset kept in the model for the current popover; reset on next open.
- Focus: open with `PopoverView`/`TextInputField` focus like `SearchPopup`. **Verify** how `SearchPopup` stops single-letter shortcuts (e.g. `P`, `Q`) from firing while typing; copy that mechanism. If it relies on the text-input focus check in `ShortcutsController`, nothing else is needed; otherwise the commands' enabled state must be false while `PopoverModel.open` (add `popoverOpen` to the module's commands state).
- Keys inside the field: Return → `accept(false)`; Opt-Return → `accept(true)` (macOS Option+Return arrives as `Qt::AltModifier`); Esc → `cancel()`; Up/Down → suggestions when `suggestions` is non-empty, else ignored; Tab/Shift-Tab ignored (chord-symbol/lyric popovers handle their own navigation in their specs).
- Appearance: use `muse::ui` theme tokens, no hard-coded colours; red border for `Invalid`/`Unsupported`; `Unsupported` also shows a one-line "not supported yet" label under the box so a user knows it is intentional.

### Apply pipeline (`PopoverController::accept`)
1. `def.validate(text)`: only `Ok` proceeds. (`Incomplete` stays open, `Invalid`/`Unsupported` stay open and red.)
2. `INotationInteraction::startEdit(...)` … `def.apply(text, scope, notation)` … `apply()` — one undo step per Return. (`startEdit()/apply()` wrap an undo transaction; see `notationinteraction.cpp`.) If `apply` returns false, roll back with `INotationUndoStack::rollbackChanges()` and keep the popover open with an error message.
3. Close the popover. Keep note input active if it was active (except chord symbols). Restore keyboard focus to the score view.
4. After a successful apply while **not** in note input, the new item becomes the selection (so Return reopens it for editing).

### Return conflict (must be resolved in this spec)
- Today Return on a selected element starts the in-place editor in `notationviewinputcontroller.cpp:1441-1465`, before shortcuts run.
- Dorico: Return = start note input if a note/rest is selected; open the item's popover if the selected item has one; else (text) edit it; during note input stops it.
- Implement a single command `command://dorico/return` (bound to Return) that branches: note-input active → stop input; selection is note/rest → start note input; `registry.findByItem(item)` → open that popover; otherwise call the existing `INotationInteraction::startEditElement`.
- Verify first whether `shortcutOverrideEvent` lets a bound `Return` through. If it does not, the minimal upstream change is one early `return false` in that function when the focused view has a `dorico` Return binding; list it in `specs/README.md` under upstream edits. Spec 11 owns note-input start/stop on Return; coordinate by calling the same `INotationNoteInput::startNoteInput()`/`stopNoteInput()`.

### Open-ended items at the caret (shared helper, used by 21, 24, 28, 29)
- `PendingSpanner { type, startTick, track, payload }` held by the module while note input is active.
- Set when a handler applies an open-ended entry (`p<`, `8va`, `vibrato->`, `ped`).
- Finalised (spanner created from `startTick` to the current input tick) when: the stop token is entered (`?`, `|`/`stop`, `*`), another item of the same family is entered, a bar of nothing is created, or note input ends. Observe via `INotationNoteInput::stateChanged()`.
- `// ponytail: the line is created when stopped, not live-extended while notes are entered; add live extension if the missing preview annoys.`

### Mapping to MuseScore (shared)
Handlers pick one of, in this order of preference (smallest code first):
1. Build the element with `Factory::create*` and call `INotationInteraction::applyPaletteElement(el, modifiers)` (`notationinteraction.cpp:2224`; handles list vs range selection and note-input). Qt modifier → local variant where the element supports it (documented per spec).
2. Dedicated API (`insertClef` :8392, `addHairpinsToSelection` :5467, `addOttavaToSelection` :5419, `addBoxes` :4916, `toggleOrnament` :8374, `addTimeSignature` :6359).
3. Text path `addTextToItem(TextStyleType, item)` (:6204) then set text and `endEditText` (:4373) — only for items that really are text.

## Deferred / unsupported
- Cues popover (Shift-U) and MIDI trigger regions (Shift-0): no MuseScore model.
- Drag-handle persistence between sessions.
- Live preview of open-ended items while notes are being entered (see `PendingSpanner`).
- Multi-staff popover anchoring for the extended caret (use the top staff of the span).
- Keyboard-language-dependent popover keys (US layout only).
- "Create items at position of first selected note" (skip rests): the anchor and `applyPaletteElement` use the selection as is.
- Suggestions (spec 26) and `PendingSpanner` (spec 21) are built by their first users.

## Acceptance criteria
- [x] Module registers `command://dorico/popover/<id>` for all 15 letter popovers plus `;`; reopen uses `dorico/return`, close is Esc in the field (see As built); `cmake -P build.cmake build install` succeeds.
- [ ] With a note selected, Shift-D opens an overlay whose bottom edge is above the top staff and whose left edge is at the note's x; with note input active, at the caret. *(manual)*
- [ ] Overlay stays on screen when the note is near the view edge and follows view scroll/zoom. *(manual)*
- [ ] Return applies in **one** undo step; Cmd-Z restores the score exactly. *(manual, temporary dynamics)*
- [x] Opt-Return calls `apply(..., ApplyScope::Local)`; handlers with `supportsLocal = false` get `Default` (`PopoverController::accept`).
- [ ] Esc closes without changing the score, and focus returns to the score (arrow keys move selection again). *(manual)*
- [ ] Typing letters such as `p`, `q`, `i` inside the box does not trigger score shortcuts. *(manual)*
- [ ] Invalid text → red border, Return does nothing; valid prefix → neutral; `Unsupported` → red plus label. *(manual)*
- [ ] Select an item owned by a handler (`itemTypes`), press Return → popover reopens with `prefill()` text; apply replaces the item (no duplicate). *(manual: a dynamic)*
- [ ] Shift-D on a note that already has a dynamic shows that dynamic; applying replaces it. *(manual)*
- [ ] No popover opens with nothing selected and no note input, outside Write mode, or while a text element is being edited. *(manual)*
- [x] No upstream edits beyond the 4 standard module edits (spec 00); `specs/README.md` updated.
- [x] GoogleTest `popoverregistry_tests.cpp`: every popover command has a def, duplicate id rejected, `findByType`, placeholder statuses, temporary dynamics validation (47/47 `dorico_tests` pass).

## Tests
- Unit: `src/dorico/tests/popoverregistry_tests.cpp`.
- Manual script: open score with 1 staff; (1) select a note, Shift-D, type `p`, Return → dynamic below note (the temporary dynamics def; Cmd-Z must remove it in one step); (1b) select that dynamic, Return → popover shows `p`, type `f`, Return → same dynamic becomes `f`; (2) Shift-N, Shift-D, Esc → note input still active; (3) scroll while the popover is open → it follows; (4) open a popover on the last measure at a high zoom → stays on screen.
