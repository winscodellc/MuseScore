#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
#
# Linux backend of uictl.sh: drives our build inside its own headless X server (Xvfb, display :99),
# so it never touches the user's desktop and nothing else can receive the input. E.g.
#   src/dorico/tools/uicontrol/uictl.sh start /tmp/x/score.mscx ";" key mscore shift+n ";" type cde
#   src/dorico/tools/uicontrol/uictl.sh stop
# Commands match main.swift (cmd maps to ctrl), plus `start [score]` and `stop`. Dorico does not run on Linux.
# Needs: xorg-server-xvfb, xdotool. Env: UICTL_DISPLAY (default :99), MSCORE (default: newest builds/*/install/bin/mscore),
# UICTL_PROFILE (default ~/.cache/uictl-profile; app log in $UICTL_PROFILE/data/MuseScore/MuseScoreStudio5Development/logs).
D=${UICTL_DISPLAY:-:99}
S=${XDG_RUNTIME_DIR:-/tmp}/uictl$D
P=${UICTL_PROFILE:-$HOME/.cache/uictl-profile}  # app settings + logs of the test instance
ROOT=$(cd "$(dirname "$0")/../../../.." && pwd)
export DISPLAY=$D
mkdir -p "$S"

win() {  # largest visible window with instance name mscore (the main window, not a popup)
    for w in $(xdotool search --onlyvisible --classname mscore 2>/dev/null); do
        xdotool getwindowgeometry --shell "$w" | awk -F= -v w="$w" '/WIDTH/ { a = $2 } /HEIGHT/ { print a * $2, w }'
    done | sort -n | tail -1 | cut -d' ' -f2
}

keysym() {  # main.swift key names -> xdotool keysyms
    echo "$1" | tr '+' '\n' | while read -r k; do
        case $k in
            cmd|command|ctrl|control) echo ctrl ;; opt|option|alt) echo alt ;; shift) echo shift ;;
            return|enter) echo Return ;; esc|escape) echo Escape ;; delete|backspace) echo BackSpace ;;
            fwddelete) echo Delete ;; tab) echo Tab ;; space) echo space ;; home) echo Home ;; end) echo End ;;
            left) echo Left ;; right) echo Right ;; up) echo Up ;; down) echo Down ;;
            pageup) echo Prior ;; pagedown) echo Next ;; f[0-9]*) echo "F${k#f}" ;;
            ']') echo bracketright ;; '[') echo bracketleft ;; -) echo minus ;; =) echo equal ;; '`') echo grave ;;
            ,) echo comma ;; .) echo period ;; /) echo slash ;; ';') echo semicolon ;; "'") echo apostrophe ;;
            '\') echo backslash ;; *) echo "$k" ;;
        esac
    done | paste -sd+
}

mod() { case $1 in cmd) echo ctrl ;; shift|alt|ctrl) echo "$1" ;; esac; }

start() {
    if [ ! -S "/tmp/.X11-unix/X${D#:}" ]; then
        Xvfb "$D" -screen 0 1600x1000x24 -nolisten tcp >"$S/xvfb.log" 2>&1 &
        echo $! >"$S/xvfb.pid"
        i=0; while [ ! -S "/tmp/.X11-unix/X${D#:}" ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i + 1)); done
    fi
    bin=${MSCORE:-$(ls -t "$ROOT"/builds/*/install/bin/mscore 2>/dev/null | head -1)}
    [ -x "$bin" ] || { echo "no mscore binary (set MSCORE)"; return 1; }
    # Own settings profile, so tests never touch the user's; its session file goes, else a killed run's
    # "restore the session?" recovery mode would win over the score given here.
    rm -f "$P/data/MuseScore/MuseScoreStudio5Development/session/session.json"
    env -u WAYLAND_DISPLAY XDG_CONFIG_HOME="$P/config" XDG_DATA_HOME="$P/data" QT_QPA_PLATFORM=xcb QT_SCALE_FACTOR=1 \
        "$bin" "$@" >"$S/mscore.log" 2>&1 &
    echo $! >"$S/mscore.pid"
    i=0; while [ -z "$(win)" ] && [ $i -lt 120 ]; do sleep 0.5; i=$((i + 1)); done
    w=$(win); [ -n "$w" ] || { echo "no window after 60 s, see $S/mscore.log"; return 1; }
    sleep 1; xdotool windowfocus "$w"  # no window manager in Xvfb; the app keeps its own size (1150x800), so use `bounds`
    echo "started pid $(cat "$S/mscore.pid") on $D, log $S/mscore.log"
}

stop() {  # kill, not quit: no save dialog can appear, files on disk stay as they are
    for p in mscore xvfb; do
        [ -f "$S/$p.pid" ] || continue
        pid=$(cat "$S/$p.pid"); kill "$pid" 2>/dev/null
        i=0; while kill -0 "$pid" 2>/dev/null && [ $i -lt 30 ]; do sleep 0.1; i=$((i + 1)); done
        kill -9 "$pid" 2>/dev/null; rm -f "$S/$p.pid"
    done
    echo stopped
}

run() {
    c=$1; shift
    case $c in
        trusted) echo true ;;
        start) start "$@" ;;
        stop) stop ;;
        front) w=$(win); [ -n "$w" ] && xdotool windowraise "$w" windowfocus "$w" ;;
        bounds) w=$(win); [ -n "$w" ] || { echo "no mscore window"; return 1; }
            xdotool getwindowgeometry --shell "$w" | awk -F= '{ v[$1] = $2 } END { print v["X"] "," v["Y"] "," v["WIDTH"] "," v["HEIGHT"] }' ;;
        key) xdotool key --clearmodifiers "$(keysym "$2")" ;;
        type) xdotool type --delay 30 "$*" ;;
        move) xdotool mousemove "$1" "$2" ;;
        click) xdotool mousemove "$1" "$2"
            case $3 in
                right) xdotool click 3 ;; double) xdotool click --repeat 2 --delay 80 1 ;;
                "") xdotool click 1 ;; *) m=$(mod "$3"); xdotool keydown "$m" click 1 keyup "$m" ;;
            esac ;;
        drag) xdotool mousemove "$1" "$2" mousedown 1 sleep 0.1 mousemove "$(( ($1 + $3) / 2 ))" "$(( ($2 + $4) / 2 ))" \
            sleep 0.1 mousemove "$3" "$4" sleep 0.1 mouseup 1 ;;
        wait) sleep "$(echo "$1" | awk '{ print $1 / 1000 }')" ;;
        *) echo "unknown command: $c"; return 2 ;;
    esac
}

# Split the arguments on ";" and run each command in turn.
cmd=""
for a in "$@" ";"; do
    if [ "$a" = ";" ]; then [ -n "$cmd" ] && eval "run $cmd"; cmd=""
    else cmd="$cmd '$(printf %s "$a" | sed "s/'/'\\\\''/g")'"; fi
done
