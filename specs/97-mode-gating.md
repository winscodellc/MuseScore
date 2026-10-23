<!-- markdownlint-disable -->

# 97 - Mode gating (what each mode lets you edit)

Status: built, pending review - Depends on: 95 (single page; the gate reads one mode state), 94 - Dorico refs: `topics/dorico-concepts/modes-in-dorico.md`, `topics/setup-mode.md`, `topics/write-mode.md`, `topics/engrave-mode.md`, `topics/play-mode.md`, `topics/print-mode.md`

## Goal

Each mode allows only the edits that Dorico allows there. Today Setup, Write, Engrave and Play all allow everything. The only gate is the popover check, and Print is the only read-only view. Add one gate, driven by `IDoricoModeState::mode()`, that covers key commands, menu items, mouse editing, drag and drop, palette apply and popovers.

## Dorico behaviour (manual, quoted in `topics/dorico-concepts/modes-in-dorico.md` and each mode page)

| Mode | Allowed | Not allowed |
|---|---|---|
| Setup | Players, instruments, flows, layouts and videos in the panels. The score can be viewed, and the layout selector works. | "You cannot select or interact with anything in the music area in Setup mode." No selection, no editing keys, no popovers, no note input. |
| Write | Note input, popovers, pitch, rhythm, delete, copy / paste, properties. Alt-arrow nudges an item's **rhythmic** position. | "By design, you cannot move notes and items graphically… only possible in Engrave mode." Mouse drag of items does **not** set graphic offsets. Dragging moves items rhythmically by the grid (verify in Dorico 6 for slurs, dynamics and text). |
| Engrave | Select, drag and nudge positions graphically (graphical offsets), handles, properties, page and frame formatting, Engrave toolbox tools. | "You cannot delete any notes or items, change their rhythmic positions, or change the pitch of notes." No note input, no popovers, no paste, no Delete. |
| Play | Track view, the Key Editor and the Mixer. Notes are edited in the Key Editor. | No score editing in the music area (it shows tracks, not the score). |
| Print | Print and export settings, layout selection in the Layouts panel. | Everything in the music area. The layout selector in the toolbar is disabled. |

### Dorico Pro 6 audit (2026-10-22, ui-tester on a real project)

Where Dorico 6 differs from the manual's wording, the observed behaviour wins; unconfirmed cells follow the manual.

| Action | Write | Engrave | Setup | Play | Print |
|---|---|---|---|---|---|
| Click a note | selects | selects | nothing | (piano roll) | nothing |
| Delete | deletes | **resets engraving changes**, removes nothing | nothing (deletes a selected player) | deletes in the Key Editor | nothing |
| Letter key (C) outside note input | nothing | nothing | nothing | nothing | nothing |
| Alt-Up | pitch up | **note: pitch up; other item: graphic nudge** | reorders players (panel) | nothing | nothing |
| Shift-N | note input | toggles note input (label seen; kept blocked here, per the manual) | nothing | nothing | nothing |
| Shift-D | popover | unclear (kept blocked) | nothing | nothing | nothing |
| Cmd-C / Cmd-V | yes / yes | Copy on with notes; paste not confirmed (kept blocked, per the manual) | off | (Key Editor) | off |
| Cmd-A | yes | yes | off | (Key Editor) | off |
| Undo | yes | yes | greyed in the test (stack was empty); kept on, Setup edits are undoable | yes | - |
| Alt-Left/Right | rhythmic move | graphic nudge | - | - | - |
| Ctrl-Alt-Left/Right | nothing | nothing | - | - | - |
| Shift-Alt-Right | lengthens (slur, rhythmic) | nothing | - | - | - |
| Drag a dynamic | snaps to the next grid position | free graphic move | - | - | - |
| Drag a note up | pitch | pitch | - | - | - |
| Double-click text | text editor | text editor | - | - | - |
| Slur handles | shown; drag snaps to notes | bezier handles | - | - | - |
| System / frame breaks | no | Engrave toolbox only | no | no | no |

A blocked key gives no beep and no message: it is silently ignored.

## MuseScore today

- Mode state: `src/dorico/idoricomodestate.h:32-56`, implemented in `internal/doricomodestate.h`.
- The single gate: `PopoverController::canOpen()` (`internal/popovercontroller.cpp:200-208`).
- Notation commands: `src/notationscene/internal/notationcommandsstate.cpp:36-41,530`. Its fork patch makes `isProjectOpened()` true on Dorico pages, so all commands are enabled everywhere.
- Dorico commands: `internal/doricocommandsstate.cpp:131-186`. It already listens to `modeChanged()` (`:75`).
- Mouse editing in `src/notationscene/qml/MuseScore/NotationScene/notationviewinputcontroller.cpp`:
  - `mousePressEvent` (`:641`, the read-only gate at `:663`);
  - `startDragElements` (`:1168`);
  - `mousePress_considerDragOutgoingRange` (`:948`);
  - `mouseDoubleClickEvent` (`:1316`);
  - drag and drop (`:1568`, `:1697`).
- Palette apply: `src/palette/internal/paletteprovider.cpp:584`.
- Context menu: `abstractnotationpaintview.cpp:654`. Its content is spec 98's job; here it must only be suppressed in Setup and Print.

## Design

1. **Policy in one place**: `src/dorico/internal/modepolicy.h/.cpp`.
   - `bool isCommandAllowed(Mode, const std::string& code)`: an allow-list per mode, with categories (input, delete, pitch, rhythm, graphical, view, clipboard). A command not listed is allowed only in Write. View, navigation, playback, file and window commands are allowed everywhere.
   - `MouseCaps capsFor(Mode)`: `select`, `graphicDrag`, `rhythmicDrag`, `editText`, `drop`.
   - Unit-testable, with no UI.
2. **Commands**:
   - Notation commands: in `notationcommandsstate.cpp`, extend the existing fork patch. `doCommandState` asks an injected `IDoricoModePolicy` when one is registered (an optional `GlobalInject`, so upstream builds without the module still work). Refresh all states on `modeChanged`.
   - Dorico commands: `doricocommandsstate.cpp` applies the same policy.
   - A disabled command does nothing even when it is dispatched by a shortcut. The dispatcher already checks enabled state; confirm this.
3. **Mouse**: add `NotationViewInputController::setInteractionCaps(caps)`, a small upstream addition near `readOnly`.
   - Setup and Print set every cap to false; this is the current `readOnly` path.
   - Engrave allows select, graphic drag and grips. It blocks double-click note entry, drop and text editing (decide from the audit whether Engrave allows text edits; Dorico does allow editing text in Engrave).
   - Write: drag of items becomes a rhythmic move using the grid (reuse spec 13's move-by-grid commands). If that is too large for one session, block graphic drag in Write and list rhythmic drag as deferred.
4. **Palette apply**: refuse it unless the mode is Write, through the policy, with one guard at `paletteprovider.cpp:584`.
5. **Feedback**: when a key is blocked by mode, Dorico does nothing (check in the audit whether it beeps or shows a status message). Match that.

### Upstream edits (tiny, marked `// Dorico fork`)

- `notationcommandsstate.cpp`: one policy check.
- `notationviewinputcontroller.{h,cpp}`: a caps struct and checks at the five entry points.
- `paletteprovider.cpp`: one guard.

### As built (differs from the design above)

- Policy: `src/dorico/internal/modepolicy.{h,cpp}` (`isCommandAllowed`, `interactionCaps`, `ModeCommandsGate`). Notation commands ask it through a new optional `notationscene/inotationcommandsgate.h` (`NotationCommandsState::commandState`); Dorico commands ask it in `DoricoCommandsState`.
- Shortcuts, menus and toolbars all read the enabled state, so disabling is enough for them. The dispatcher itself does not check it, so `ModeCommandsGate` also vetoes blocked commands in `preDispatch` (double-clicking a bar dispatches note input directly). It subscribes first; the paste controller's hook skips vetoed commands. In Engrave it turns Delete into `reset-shapes-and-positions`.
- No palette guard: the palettes panel exists only in Write.
- Setup does not use `readOnly` (that paints the score as printed, hiding invisible items); it gets no mouse caps instead. Caps (`notationscenetypes.h` `InteractionCaps`) reach the view through `AbstractNotationPaintView::interactionCaps`, bound in `DoricoShellPage.qml` from `DoricoTopBarModel::interactionCaps`.
- Engrave drags must not move items to another beat: `INTERACTION_REANCHOR` is off there, which sets `EditData::keepAnchors` (checked by `MoveElementAnchors::moveElementAnchorsOnDrag` and `LineSegment::rebaseAnchors`).
- Engrave Alt-arrow: `command://dorico/nudge-{left,right,up,down}` share Alt-arrow with Write's commands; only one of each pair is enabled per mode. Up/Down on notes changes pitch (Dorico 6).

## Deferred / unsupported

- The Play-mode Key Editor (spec 88). Until then, Play allows nothing in the score.
- Rhythmic drag in Write (a dragged dynamic snapping to the grid; slur handles snapping to notes). Write blocks graphic drags of items and handles instead.
- Shift-N and paste in Engrave: Dorico 6 seemed to allow them, but the audit could not confirm it; kept blocked, per the manual.
- Graphic offsets in Engrave that MuseScore cannot represent (system- or frame-relative). Use MuseScore's offsets.

## Acceptance criteria

- [x] Setup: clicking the score selects nothing. Shift-D, Delete, letter keys and Cmd-V do nothing. Panels still work.
- [x] Write: note input, popovers, delete and paste work. Dragging a dynamic does not change its graphic offset (save and check `<offset>` in the .mscx).
- [x] Engrave: A-G pitch keys, Shift-N, popovers, Cmd-V and palette clicks do nothing. Delete resets the selected items' positions and removes nothing. Dragging a dynamic changes its offset. Alt-arrow nudges graphically (Alt-Up/Down on notes changes their pitch).
- [x] Play: the track view only. No score edit commands are enabled.
- [x] Print: no edits through keys either (today keys are still enabled).
- [x] Menu items reflect the mode: the Edit menu in Engrave shows Paste disabled (Delete stays on, as in Dorico 6).
- [x] Switching modes updates the enabled states at once (no stale enabled items).
- [x] Spec 97's upstream hooks are safe without the dorico module (reviewed 2026-10-22, not built): no dorico header or symbol in the edits, every hook is null-safe, caps default to `INTERACTION_ALL`, `keepAnchors` defaults to false. The whole fork cannot be built without the module, see Review.

## Review (2026-10-22)

- Fixed (UI-checked): Shift+arrow re-anchored dynamics, lines and slurs in modes without `INTERACTION_REANCHOR` (Engrave, and a Write selection carried into Setup/Play). One guard, `NotationViewInputController::isReanchorKeyBlocked`, covers the anchor-editing key and keys in edit mode (text editing exempt).
- Fixed (UI-checked): in Write a click on a slur or line started grip edit mode, so the arrows moved its handles. `startEditElement` on click now needs `INTERACTION_GRIPS`.
- Fixed (UI-checked): dragging one item of a list selection dragged the others regardless of caps. `startDragElements`' `isDraggable` checks each element.
- `INotationInteraction::setDragKeepsAnchors` has a default body, so the upstream mock is untouched.
- Debt: there is no `MUE_BUILD_DORICO_MODULE` option (`src/CMakeLists.txt` and `appfactory.cpp` add the module unconditionally), and two upstream headers include dorico interfaces (`abstractnotationpaintview.h`: `dorico/idoricocaret.h`, spec 12; `notationinteraction.h`: `dorico/idoricopaste.h`, spec 42).


## Tests

- GoogleTest `DoricoModePolicy*`: a table of (mode, command) → allowed, covering one command per category per mode.
- Manual, run with the `ui-tester` agent: the acceptance list, in each mode.
