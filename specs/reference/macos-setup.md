<!-- markdownlint-disable -->
# macOS setup (the user's previous machine)

Moved out of `CLAUDE.md` when the user switched to Linux. Read this only when working on macOS again.

## Build
- macOS Intel, Xcode 26.5. Needs AppleClang >= 21, CMake >= 3.31, Ninja (required on macOS), Qt >= 6.8.
- Qt lives in `~/Qt/6.11.2/macos`, installed with aqtinstall from the venv `~/.venvs/aqt` (modules qt5compat, qtnetworkauth, qtshadertools, qtwebsockets).
- `build_overrides.cmake` (gitignored, repo root) sets `QTDIR` and `GENERATOR=Ninja`, so no env vars are needed.
- Build dir: `builds/Mac-Qt6.11.2-macos-Ninja-RelWithDebInfo/`, install tree `install/` inside it.
- `brew install ccache` was attempted and abandoned: it tried to rebuild cmake from source. Retry later if rebuild times hurt.
- Logs: `~/Library/Application Support/MuseScore/MuseScoreStudio5Development/logs/`.

## Driving the UI: ClaudeUIControl (Dorico 6 + our build)
Claude can see and control **Dorico 6** and **our MuseScore build** through `~/Applications/ClaudeUIControl.app` (user-built, holds the Accessibility permission). Use it when the user asks, and **offer it** whenever a change needs checking in the real UI or a Dorico behaviour must be compared side by side (ask before driving the apps). Docs: `src/dorico/tools/uicontrol/README.md`.
- Same `uictl.sh` / `shot.sh` commands as on Linux, plus `trusted`, `front|bounds <dorico|mscore>`. Screen points, top-left origin. Run `uictl.sh front <app>` before a shot if another window covers it.
- It only acts on those two apps (bundle-ID allowlist). **Never rebuild it** (`build.sh`): a rebuild drops the permission and only the user can re-grant it.
- Our build may hold the user's open scores: test on temp copies of `demos/` scores, quit with Cmd-Q, and never answer a save dialog (click Cancel and ask).
