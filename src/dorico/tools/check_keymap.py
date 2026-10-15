#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# Checks the Dorico shortcut preset (src/app/configs/data/shortcuts_dorico.json).
# Run from the repo root:  python3 -I src/dorico/tools/check_keymap.py
#
# 1. Emulates CommandShortcutsRegister::reload() (base -> shortcuts_mac -> preset diff).
# 2. Reports key clashes between commands that can be active together.
# 3. Checks src/dorico/tools/dorico_keys.json against the merged map.
# 4. Checks every command in the preset is defined somewhere.

import fnmatch
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DATA = ROOT / "src/app/configs/data"
KEYS = ROOT / "src/dorico/tools/dorico_keys.json"
COMMAND_HEADERS = [
    ROOT / "src/notationscene/notationcommands.h",
    ROOT / "src/palette/palettecommands.h",
    ROOT / "src/dorico/doricocommands.h",
]

# Scopes whose commands can be live at the same time (shortcuts v2: scope never disables a command).
ACTIVE_SCOPES = {"NOTATION", "NOTATION_PAGE", "NOTATION_NOTE_INPUT", "PLAYBACK", "PROJECT", "APP"}
# Command families that are mutually exclusive by state or focus, so sharing a key is fine.
CLASH_ALLOW = ["command://*/edittext-*", "command://*/editlyrics-*", "command://*-tab", "command://*/enter-fret-*"]

MODS = {"ctrl", "alt", "shift", "meta", "num"}


def load(path):
    out = []
    for block in json.loads(path.read_text()):
        for sc in block["shortcuts"]:
            out.append({"scope": block["scope"], "command": sc["command"], "sequences": list(sc["sequences"])})
    return out


def merge_shortcuts(shortcuts, defaults):
    """Mirror of CommandShortcutsRegister::mergeShortcuts (shortcuts is modified in place)."""
    need_add = []
    for d in defaults:
        for_add = {**d, "sequences": list(d["sequences"])}
        found = False
        for sc in shortcuts:
            if sc["command"] == d["command"]:
                sc["scope"] = d["scope"]
                found = True
            elif sc["scope"] == d["scope"]:
                for_add["sequences"] = [s for s in for_add["sequences"] if s not in sc["sequences"]]
        if not found:
            need_add.append(for_add)
    shortcuts.extend(need_add)


def apply_diff(path, shortcuts):
    diff = load(path)
    merge_shortcuts(diff, shortcuts)
    return diff


def norm(seq):
    mods, rest = [], seq
    while True:
        m = re.match(r"([A-Za-z]+)\+(.+)", rest)
        if m and m.group(1).lower() in MODS:
            mods.append(m.group(1).lower())
            rest = m.group(2)
        else:
            break
    return "+".join(sorted(mods) + [rest.lower()])


def ignored(cmd):
    # NONE_COMMAND is a placeholder that only reserves a key; the CLASH_ALLOW families are exclusive by state.
    return cmd == "command://none" or any(fnmatch.fnmatch(cmd, p) for p in CLASH_ALLOW)


def main():
    merged = load(DATA / "shortcuts.json")
    merged = apply_diff(DATA / "shortcuts_mac.json", merged)
    merged = apply_diff(DATA / "shortcuts_dorico.json", merged)

    by_key = defaultdict(list)
    for sc in merged:
        if sc["scope"] not in ACTIVE_SCOPES:
            continue
        if ignored(sc["command"]):
            continue
        for s in sc["sequences"]:
            by_key[norm(s)].append(sc["command"])

    failures = 0

    print("== Conflicts")
    conflicts = 0
    for key, cmds in sorted(by_key.items()):
        if len(cmds) > 1:
            conflicts += 1
            print(f"  {key}: {', '.join(c.replace('command://', '') for c in cmds)}")
    print(f"  {conflicts} conflict(s)")
    failures += conflicts

    print("== Coverage")
    rows = json.loads(KEYS.read_text())
    bound = reserved = deferred = 0
    unmapped = []
    for r in rows:
        key, cmd = norm(r["key"]), r["command"]
        have = by_key.get(key, [])
        if cmd.startswith("command://"):
            if have == [cmd]:
                bound += 1
            else:
                failures += 1
                print(f"  FAIL {r['key']} ({r['dorico']}): want {cmd}, have {have}")
        else:
            if cmd.startswith("reserved"):
                reserved += 1
            else:
                deferred += 1
            if have:
                failures += 1
                print(f"  FAIL {r['key']} ({r['dorico']}): {cmd} but still bound to {have}")
            else:
                unmapped.append(r["key"])

    print("== Unknown commands")
    known = set()
    for h in COMMAND_HEADERS:
        known.update(re.findall(r'"(command://[^"]+)"', h.read_text()))
    base_cmds = {sc["command"] for sc in load(DATA / "shortcuts.json")}
    for sc in load(DATA / "shortcuts_dorico.json"):
        c = sc["command"]
        if c not in known and c not in base_cmds:
            print(f"  FAIL unknown command {c}")
            failures += 1
    for r in rows:
        c = r["command"]
        if c.startswith("command://") and c not in known and c not in base_cmds:
            print(f"  {'warn' if '/dorico/' in c else 'FAIL'} unknown command in dorico_keys.json: {c}")
            failures += 0 if "/dorico/" in c else 1

    print(f"== Summary: {bound} bound, {reserved} reserved, {deferred} deferred, {conflicts} conflicts, {failures} failure(s)")
    print(f"   unbound (reserved/deferred) keys: {len(unmapped)}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
