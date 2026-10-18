#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
#
# Runs ClaudeUIControl commands and prints their output, e.g.
#   src/dorico/tools/uicontrol/uictl.sh key dorico cmd+1 ";" wait 1500 ";" bounds dorico
# The helper is started with `open`, so macOS checks the helper's own Accessibility permission. See main.swift.
[ "$(uname)" = Linux ] && exec "$(dirname "$0")/x11.sh" "$@"  # headless Xvfb backend
out=$(mktemp -t uictl)
open -W -n -g -a "$HOME/Applications/ClaudeUIControl.app" --stdout "$out" --stderr "$out" --args "$@"
cat "$out"
rm -f "$out"
