<!-- markdownlint-disable -->
# ClaudeUIControl

A small helper app that lets Claude drive **Dorico 6** and **this fork's MuseScore build**: press shortcuts, click, drag, type and read window positions. It is used to compare the two UIs side by side and to test UI changes. Screenshots are taken separately with macOS's `screencapture` (Screen Recording permission).

## What it can and cannot do

| Can | Cannot |
|---|---|
| Send keys, clicks, drags and typing to **Dorico 6** (`com.steinberg.dorico6`) and **our build** (`org.musescore.MuseScoreStudio5Development`) | Touch any other app: keys are refused unless one of the two is frontmost; clicks, drags and moves are refused unless they land inside one of that app's windows (main window or its popups) |
| Bring one of the two apps to the front | Read keystrokes, screen contents, files or anything else |
| Report the position and size of their main window | Run in the background: it starts per command and exits |

The app list is `ALLOWED_APPS` in `main.swift`. Changing it requires a rebuild, which also drops the permission.

## Install (you run these, not Claude)

1. Read `main.swift`.
2. Build it: `src/dorico/tools/uicontrol/build.sh` (creates `~/Applications/ClaudeUIControl.app`, ad-hoc signed).
3. **System Settings > Privacy & Security > Accessibility > +**, choose `~/Applications/ClaudeUIControl.app` and switch it on.
4. Remove **ClaudeCode.app** (and the terminal, if listed) from Accessibility: Claude no longer needs that permission.
5. Check: `src/dorico/tools/uicontrol/uictl.sh trusted` prints `true`.

## Why it is permanent and safe

- It runs as its own app (started with `open`), so macOS checks **its** Accessibility permission, not Claude Code's or the terminal's. Claude Code updates do not affect it.
- It is ad-hoc signed: if anyone modifies the binary, macOS stops trusting it until you switch it on again.
- The Accessibility switch turns it off at any time; deleting the app removes it completely.

## Use

```sh
src/dorico/tools/uicontrol/uictl.sh key mscore cmd+1 ";" wait 1500 ";" bounds mscore
src/dorico/tools/uicontrol/uictl.sh click 120 300 right
```

Commands: `trusted`, `prompt`, `front <app>`, `bounds <app>`, `key <app> <combo>`, `type <text>`, `click <x> <y> [right|double|cmd|shift|alt|ctrl]`, `drag <x1> <y1> <x2> <y2>`, `move <x> <y>`, `wait <ms>`. `<app>` is `dorico` or `mscore`. Coordinates are screen points, top-left origin (the same as `screencapture -R`).
