#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
#
# Takes a token-cheap screenshot of Dorico, our build or the whole screen, e.g.
#   src/dorico/tools/uicontrol/shot.sh mscore out.png               # whole window, long edge <= 1200 px
#   src/dorico/tools/uicontrol/shot.sh mscore out.png 0,0,700,300   # x,y,w,h relative to the window, native size
#   src/dorico/tools/uicontrol/shot.sh screen out.png "" 1600       # whole screen, long edge <= 1600 px
# Claude bills an image by its pixels (about w*h/750 tokens), not its file size, so this crops and downscales.
# It prints the size, the token estimate and how to map an image pixel (ix,iy) back to screen points for `uictl.sh click`.
set -e
[ $# -ge 2 ] || { echo "usage: shot.sh <dorico|mscore|screen> <out.png> [x,y,w,h] [max]" >&2; exit 2; }
app=$1 out=$2 crop=$3 max=${4:-1200}

if [ "$app" = screen ]; then win=0,0,0,0; else win=$("$(dirname "$0")/uictl.sh" bounds "$app"); fi
case $win in *[!0-9,-]* | "") echo "$win" >&2; exit 1 ;; esac

# Window origin clamped to the screen (Dorico reports x = -1), plus the optional window-relative crop.
rect=$(echo "$win $crop" | awk -F'[ ,]' '{ x = $1 < 0 ? 0 : $1; y = $2 < 0 ? 0 : $2; w = $3; h = $4
    if ($5 != "") { x += $5; y += $6; w = $7; h = $8 }
    print x "," y "," w "," h }')
if [ "$rect" = 0,0,0,0 ]; then screencapture -x "$out"; else screencapture -x -R "$rect" "$out"; fi

dims() { sips -g pixelWidth -g pixelHeight "$out" | awk '/pixelWidth/ { w = $2 } /pixelHeight/ { h = $2 } END { print w, h }'; }
set -- $(dims)
pw=${rect#*,*,}; pw=${pw%,*}; [ "$pw" != 0 ] || pw=$1   # width in screen points (whole screen: 1x display assumed)
if [ "$1" -gt "$max" ] || [ "$2" -gt "$max" ]; then sips -Z "$max" "$out" >/dev/null; set -- $(dims); fi

echo "$1 $2 $rect $pw" | awk -v f="$out" '{ split($3, r, ","); s = $4 / $1
    printf "%s %dx%d ~%d tokens; screen = %d+ix*%.3f, %d+iy*%.3f\n", f, $1, $2, $1 * $2 / 750, r[1], s, r[2], s }'
