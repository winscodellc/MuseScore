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
# 5. Policy (user decision): the preset is Dorico's key map, not MuseScore's plus Dorico's. Any binding not backed by a
#    command:// row in dorico_keys.json is a failure, apart from EXEMPT (UI navigation, text/lyrics editor and TAB keys).
#    `--fix` rewrites shortcuts_dorico.json so those bindings (and keys of reserved/deferred rows) are unbound. Run it
#    after every upstream merge, since upstream keeps adding keys.

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
    ROOT / "src/playback/playbackcommands.h",
]

# Scopes whose commands can be live at the same time (shortcuts v2: scope never disables a command).
ACTIVE_SCOPES = {"NOTATION", "NOTATION_PAGE", "NOTATION_NOTE_INPUT", "PLAYBACK", "PROJECT", "APP"}
# Command families that are mutually exclusive by state or focus, so sharing a key is fine.
CLASH_ALLOW = ["command://dorico/nudge-*", "command://*/edittext-*", "command://*/editlyrics-*", "command://*-tab", "command://*/enter-fret-*"]

MODS = {"ctrl", "alt", "shift", "meta", "num"}

# Kept although Dorico does not list them: keyboard navigation inside panels and dialogs (accessibility), and keys that
# only work inside the text / lyrics / chord-symbol editor or on TAB staves, until the popover specs replace them.
EXEMPT_SCOPES = {"NAVIGATION", "NOTATION_TEXT_EDITING"}
EXEMPT_COMMANDS = CLASH_ALLOW + ["command://none"]
NOT_EXEMPT = ["command://navigation/*-section"]  # F6 / ` section jumps: MuseScore-only, ` is Dorico's hide-non-printing


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


# macOS treats a shortcut string as physical keys and drops the shift of a shifted character, so "}" fires on a bare
# "]" (macosshortcutsinstancemodel.mm). Normalise the same way so such clashes show up; bind "Shift+]" instead.
# "+" is left alone: upstream uses it ("Ctrl++") and it collapses onto "=" bindings of the same commands.
SHIFTED = dict(zip('~!@#$%^&*()_{}|:"<>?', "`1234567890-[]\\;',./"))


def norm(seq):
    mods, rest = [], seq
    while True:
        m = re.match(r"([A-Za-z]+)\+(.+)", rest)
        if m and m.group(1).lower() in MODS:
            mods.append(m.group(1).lower())
            rest = m.group(2)
        else:
            break
    rest = SHIFTED.get(rest, rest)
    return "+".join(sorted(mods) + [rest.lower()])


def exempt(sc):
    if any(fnmatch.fnmatch(sc["command"], p) for p in NOT_EXEMPT):
        return False
    return sc["scope"] in EXEMPT_SCOPES or any(fnmatch.fnmatch(sc["command"], p) for p in EXEMPT_COMMANDS)


def musescore_only(merged, rows):
    """(scope, command, sequence) bound in the merged map but not backed by a Dorico row."""
    backed = {(norm(r["key"]), r["command"]) for r in rows if r["command"].startswith("command://")}
    unbacked = {norm(r["key"]) for r in rows if not r["command"].startswith("command://")}
    out = []
    for sc in merged:
        for s in sc["sequences"]:
            if (norm(s), sc["command"]) in backed:
                continue
            if exempt(sc) and norm(s) not in unbacked:
                continue
            out.append((sc["scope"], sc["command"], s))
    return out


def dump_preset(blocks):
    """Same layout as the hand-written file: one shortcut per line."""
    lines = ["["]
    for bi, b in enumerate(blocks):
        lines += ["    {", f'        "scope": {json.dumps(b["scope"])},', '        "shortcuts": [']
        lines.append(",\n".join(f'        {{"command": {json.dumps(sc["command"])}, "sequences": {json.dumps(sc["sequences"])}}}'
                                for sc in b["shortcuts"]))
        lines += ["        ]", "    }" + ("," if bi < len(blocks) - 1 else "")]
    lines.append("]")
    return "\n".join(lines) + "\n"


def fix(merged, extra):
    """Unbinds `extra` (scope, command, sequence) by writing the remaining sequences into the preset."""
    path = DATA / "shortcuts_dorico.json"
    blocks = json.loads(path.read_text())
    remaining = {sc["command"]: sc for sc in merged}
    drop = defaultdict(set)
    for scope, cmd, s in extra:
        drop[(scope, cmd)].add(s)
    for (scope, cmd), seqs in drop.items():
        keep = [s for s in remaining[cmd]["sequences"] if s not in seqs]
        entry = next((sc for b in blocks for sc in b["shortcuts"] if sc["command"] == cmd), None)
        if entry is None:
            block = next((b for b in blocks if b["scope"] == scope), None)
            if block is None:
                block = {"scope": scope, "shortcuts": []}
                blocks.append(block)
            entry = {"command": cmd, "sequences": keep}
            block["shortcuts"].append(entry)
        entry["sequences"] = keep
    path.write_text(dump_preset(blocks))
    print(f"== Fixed: unbound {len(extra)} sequence(s) in {path.name}")


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

    print("== MuseScore-only bindings (policy 5)")
    extra = musescore_only(merged, rows)
    for scope, cmd, s in extra:
        print(f"  FAIL {s} -> {cmd.replace('command://', '')} ({scope})")
    failures += len(extra)
    if extra and "--fix" in sys.argv:
        fix(merged, extra)
        return main_after_fix()

    print(f"== Summary: {bound} bound, {reserved} reserved, {deferred} deferred, {conflicts} conflicts, {failures} failure(s)")
    print(f"   unbound (reserved/deferred) keys: {len(unmapped)}")
    return 1 if failures else 0


def main_after_fix():
    sys.argv = [a for a in sys.argv if a != "--fix"]
    print("== Re-checking")
    return main()


if __name__ == "__main__":
    sys.exit(main())
