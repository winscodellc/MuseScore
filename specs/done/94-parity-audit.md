<!-- markdownlint-disable -->

# 94 - Dorico Pro parity audit

Status: done - Depends on: 50-63 - Dorico refs: Dorico Pro 6 itself (`/Applications/Dorico 6.app`), driven side by side with our build

## Goal

Measure our build against the real **Dorico Pro 6** (not SE, not only the manual) before any new feature spec. The user's bar is "exactly the same": colours, sizes, menus, context menus, popovers, Preferences, and which edits each mode allows. Specs 95-102 are written from this audit.

## How it was captured (2026-10-20/21)

- `ui-tester` agents drove both apps through `src/dorico/tools/uicontrol`. The two apps had different temp scores open, so the comparisons cover the chrome only.
- **Dorico must run as Pro.** Twice, Dorico started as SE, because a modifier looked held at launch (`Running as Dorico SE: (Ctrl pressed)` in `~/Library/Application Support/Steinberg/Dorico 6/application.log`). The SE shots were discarded. The launch procedure is in `.claude/agents/ui-tester.md`.
- One run sent Dorico into a runaway `Window.ShowToolbar` toggle loop after it zoomed to 1600% and entered note input. Dorico had to be killed. Don't zoom far in Dorico while automating it.
- Dorico shots are reference material only and are gitignored (`specs/reference/dorico-shots/**/*.png`). The notes and menu dumps are our own transcriptions and are committed.

## Results

| Area | Notes (in `reference/dorico-shots/`) | Fixed by |
|---|---|---|
| Five modes: chrome, colours, sizes, zones, switch lag | `modes/notes.md` | 95 (switching), 101 (look) |
| Menu bar (full text dumps of both apps) | `menus/notes.md` Part 1, `menus/dorico-menubar.txt`, `menus/ours-menubar.txt`, `menus/dump-menubar.applescript` | 102 |
| Context menus: Write, Engrave, Setup panels, style | `menus/notes.md` Part 2 | 98 |
| 16 popovers: size, anatomy, glyphs, placement, invalid state | `popovers/notes.md` | 99 |
| Preferences: every Dorico category and option, mapping to ours | `prefs/notes.md` | 100 |
| What each mode allows (from the manual) | `reference/manual/topics/dorico-concepts/modes-in-dorico.md` + per-mode pages | 97 |
| Performance (from code reading, not measured) | spec 96 table | 95, 96 |

Mode-switch lag was not visible at screenshot granularity (0.3-0.5 s) in either app. Spec 95 measures it with a timer.

## Gaps left to capture (each owning spec captures its own first)

- Context menus:
  - the Chord Symbols and Diagrams, Fingering, Octave Line, Pedal Lines, Staff Size, Dynamics and Slurs submenus;
  - Engrave right-click on a note and on a dynamic;
  - what right-click on an unselected item does (98).
- The Setup, Engrave and Print mode menus in the menu bar (102).
- The Dorico fingering popover, and whether popover size depends on zoom (99).
- Apply, Close and Reset to Factory semantics in Preferences (100).
