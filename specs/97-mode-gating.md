<!-- markdownlint-disable -->

# 97 - Mode gating (what each mode lets you edit)

Status: todo - Depends on: 95 (single page; the gate reads one mode state), 94 - Dorico refs: `topics/dorico-concepts/modes-in-dorico.md`, `topics/setup-mode.md`, `topics/write-mode.md`, `topics/engrave-mode.md`, `topics/play-mode.md`, `topics/print-mode.md`

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

The audit (spec 94) did not test per-key gating, so capture it first. In Dorico Pro, check which keys and menu items are enabled in each mode, for example Cmd-C, Cmd-A, Undo, Alt-arrow (graphic nudge) and Shift-Alt-arrow in Engrave, and check how a blocked key responds (a beep or a status message). Launch Dorico Pro with the steps in `.claude/agents/ui-tester.md`, then record the table here.

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

## Deferred / unsupported

- The Play-mode Key Editor (spec 88). Until then, Play allows nothing in the score.
- Graphic offsets in Engrave that MuseScore cannot represent (system- or frame-relative). Use MuseScore's offsets.

## Acceptance criteria

- [ ] Setup: clicking the score selects nothing. Shift-D, Delete, letter keys and Cmd-V do nothing. Panels still work.
- [ ] Write: note input, popovers, delete and paste work. Dragging a dynamic does not change its graphic offset (save and check `<offset>` in the .mscx).
- [ ] Engrave: Delete, A-G pitch keys, Shift-N, popovers, Cmd-V and palette clicks do nothing. Dragging a dynamic changes its offset. Alt-arrow nudges graphically.
- [ ] Play: the track view only. No score edit commands are enabled.
- [ ] Print: no edits through keys either (today keys are still enabled).
- [ ] Menu items reflect the mode: the Edit menu in Engrave shows Delete disabled.
- [ ] Switching modes updates the enabled states at once (no stale enabled items).
- [ ] An upstream build without the dorico module still compiles and behaves as before.

## Tests

- GoogleTest `DoricoModePolicy*`: a table of (mode, command) → allowed, covering one command per category per mode.
- Manual, run with the `ui-tester` agent: the acceptance list, in each mode.
