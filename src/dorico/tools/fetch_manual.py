#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# Mirrors the Dorico Pro 6.2 manual (steinberg.help content API) into specs/reference/manual/ as Markdown + PNGs.
# The folder is gitignored: Steinberg owns the copyright, it is a local reference only.
# Run from the repo root:  python3 -I src/dorico/tools/fetch_manual.py [--no-images] [--limit N]
# Resumable: existing files are skipped. One request at a time, with a short delay.
#
#   manual/index.md           breadcrumb tree, grep this first
#   manual/topics/<path>.md   one file per topic
#   manual/images/<name>.png

import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
from html.parser import HTMLParser

HOST = "https://www.steinberg.help"
MAP = HOST + "/api/khub/maps/BYbh2hvBDwUCjSijIhHcqA"
OUT = "specs/reference/manual"
DELAY = 0.05
HEADER = "<!-- markdownlint-disable -->\n"


def get(url, retries=4):
    for i in range(retries):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req, timeout=60) as r:
                return r.read()
        except (urllib.error.URLError, TimeoutError) as e:
            if i == retries - 1:
                raise
            time.sleep(2 * (i + 1))


def slug(s):
    return re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-")[:60] or "x"


class ToMd(HTMLParser):
    """Light HTML -> Markdown: headings, lists, tables, images, bold/italic, code. Everything else is flattened."""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.out = []
        self.images = []
        self.list_stack = []  # "ul" | "ol"
        self.ol_n = []
        self.row = None
        self.cell = None
        self.rows = []
        self.skip = 0

    def w(self, s):
        if self.cell is not None:
            self.cell.append(s)
        else:
            self.out.append(s)

    def handle_starttag(self, tag, a):
        a = dict(a)
        cls = a.get("class", "")
        if tag in ("script", "style") or "excluded-from-rendering" in a.get("data-ft-warning", ""):
            pass
        if tag in ("script", "style"):
            self.skip += 1
        elif tag in ("h1", "h2", "h3", "h4", "h5", "h6"):
            self.w("\n\n" + "#" * int(tag[1]) + " ")
        elif tag in ("p", "div", "section", "dl"):
            self.w("\n\n")
        elif tag == "br":
            self.w("  \n")
        elif tag == "ul":
            self.list_stack.append("ul")
            self.w("\n")
        elif tag == "ol":
            self.list_stack.append("ol")
            self.ol_n.append(0)
            self.w("\n")
        elif tag == "li":
            ind = "  " * (len(self.list_stack) - 1)
            if self.list_stack and self.list_stack[-1] == "ol":
                self.ol_n[-1] += 1
                self.w(f"\n{ind}{self.ol_n[-1]}. ")
            else:
                self.w(f"\n{ind}- ")
        elif tag == "dt":
            self.w("\n\n**")
        elif tag == "dd":
            self.w("\n")
        elif tag in ("strong", "b"):
            self.w("**")
        elif tag in ("em", "i"):
            self.w("*")
        elif tag in ("code", "kbd") or "userinput" in cls or "uicontrol" in cls and False:
            self.w("`")
        elif tag == "table":
            self.rows = []
        elif tag == "tr":
            self.row = []
        elif tag in ("td", "th"):
            self.cell = []
        elif tag == "img":
            name = a.get("data-ft-asset-display-name") or os.path.basename(a.get("src", "")) or "image.png"
            self.images.append(name)
            self.w(f"\n\n![{a.get('alt', '').strip()}](../images/{name})\n\n")

    def handle_endtag(self, tag):
        if tag in ("script", "style"):
            self.skip = max(0, self.skip - 1)
        elif tag in ("ul", "ol"):
            if self.list_stack:
                if self.list_stack.pop() == "ol":
                    self.ol_n.pop()
            self.w("\n")
        elif tag == "dt":
            self.w("**\n")
        elif tag in ("strong", "b"):
            self.w("**")
        elif tag in ("em", "i"):
            self.w("*")
        elif tag in ("code", "kbd"):
            self.w("`")
        elif tag in ("td", "th"):
            if self.cell is not None and self.row is not None:
                self.row.append(re.sub(r"\s+", " ", "".join(self.cell)).strip().replace("|", "\\|"))
            self.cell = None
        elif tag == "tr":
            if self.row:
                self.rows.append(self.row)
            self.row = None
        elif tag == "table":
            if self.rows:
                n = max(len(r) for r in self.rows)
                rows = [r + [""] * (n - len(r)) for r in self.rows]
                lines = ["| " + " | ".join(rows[0]) + " |", "|" + "---|" * n]
                lines += ["| " + " | ".join(r) + " |" for r in rows[1:]]
                self.out.append("\n\n" + "\n".join(lines) + "\n\n")
            self.rows = []

    def handle_data(self, d):
        if not self.skip:
            self.w(re.sub(r"\s+", " ", d))

    def result(self):
        s = "".join(self.out)
        s = re.sub(r"[ \t]+\n", "\n", s)
        s = re.sub(r"(?m)^(\s*(?:-|\d+\.))\n+\s*", r"\1 ", s)  # <li><p>: keep the text on the bullet line
        s = re.sub(r"\n{3,}", "\n\n", s)
        return s.strip() + "\n"


def main():
    args = sys.argv[1:]
    images = "--no-images" not in args
    limit = int(args[args.index("--limit") + 1]) if "--limit" in args else None
    os.makedirs(f"{OUT}/topics", exist_ok=True)
    os.makedirs(f"{OUT}/images", exist_ok=True)

    topics = json.loads(get(MAP + "/topics"))
    if limit:
        topics = topics[:limit]
    used = set()
    tree = []
    need_images = set()
    for n, t in enumerate(topics, 1):
        crumbs = t.get("breadcrumb") or [t["title"]]
        base = "/".join(slug(c) for c in crumbs[:-1])
        name = slug(crumbs[-1])
        rel = f"{base}/{name}.md" if base else f"{name}.md"
        if rel in used:  # same title twice in one chapter
            rel = rel[:-3] + "-" + t["id"][:6] + ".md"
        used.add(rel)
        path = f"{OUT}/topics/{rel}"
        tree.append((crumbs, rel, t["title"]))
        if os.path.exists(path):
            continue
        html = get(HOST + t["contentApiEndpoint"]).decode("utf-8", "replace")
        p = ToMd()
        p.feed(html)
        need_images.update(p.images)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w") as f:
            f.write(f"{HEADER}# {t['title']}\n\n")
            f.write(f"Source: {HOST}{t['readerUrl']}  \nBreadcrumb: {' > '.join(crumbs)}\n\n")
            f.write(p.result())
        time.sleep(DELAY)
        if n % 100 == 0:
            print(f"topics {n}/{len(topics)}", flush=True)

    with open(f"{OUT}/index.md", "w") as f:
        f.write(HEADER + "# Dorico Pro 6.2 manual (local mirror)\n\n")
        prev = []
        for crumbs, rel, title in tree:
            depth = len(crumbs) - 1
            f.write(f"{'  ' * depth}- [{title}](topics/{rel})\n")

    if images:
        res = json.loads(get(MAP + "/resources"))
        for i, r in enumerate(res, 1):
            dest = f"{OUT}/images/{r['filename']}"
            if os.path.exists(dest):
                continue
            with open(dest, "wb") as f:
                f.write(get(r["contentUrl"]))
            time.sleep(DELAY)
            if i % 200 == 0:
                print(f"images {i}/{len(res)}", flush=True)
        missing = [n for n in need_images if not os.path.exists(f"{OUT}/images/{n}")]
        if missing:
            print(f"{len(missing)} referenced images not in the resource list, e.g. {missing[:5]}")
    print("done")


if __name__ == "__main__":
    main()
