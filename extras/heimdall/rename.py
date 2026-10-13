#!/usr/bin/env python3
# Copyright (C) 2026 TheFenrisLycaon
# SPDX-License-Identifier: GPL-2.0-or-later
"""Rename VLC to Heimdall and VideoLAN to Lycaria across the tree.

Run from the repository root:  python extras/heimdall/rename.py
Running it again changes nothing. Prints a report of every "vlc" and
"videolan" left behind, grouped by why it was kept.
"""
import fnmatch
import os
import re
import subprocess
import sys
from collections import Counter

YEAR = b"2026"
AUTHOR = b"TheFenrisLycaon"

# Files that are never edited or renamed.
SKIP = [
    "COPYING", "COPYING.LIB", "AUTHORS", "THANKS",   # license and credits
    "NEWS", "ChangeLog*", "*/ChangeLog*",            # upstream history
    ".mailmap",                                      # upstream author ids
    "contrib/src/*/*.patch",                         # patches other projects
    "contrib/src/*/SHA512SUMS",                      # upstream tarball names
    "docs/plans/*",                                  # plans talk about the original VLC
    "extras/heimdall/rename.py",                     # this script
]

# Spans inside a line that are never renamed.
PROTECT = re.compile(
    rb"(?:https?|ftps?|git|ssh|svn|rsync)://[^\s\"'<>`]*"     # web and repo URLs
    rb"|git@[^\s\"'<>`]*"
    rb"|[\w.@/+:%~-]*\bvideolan\.(?:org|me|net)\b[\w./@:+%~#?=&-]*"  # hosts, mails
    rb"|VLC authors and VideoLAN|VideoLAN and VLC authors",
    re.I)

# A copyright line, and the indented lines that continue it.
COPYRIGHT = re.compile(rb"copyright\s*(?:\(c\)|\xc2\xa9|\d)|\(c\)\s*\d{4}|\xc2\xa9\s*\d{4}", re.I)
CONTINUED = re.compile(rb"^[ \t]*(?:\*|#|//|;|--|%|dnl)(?:[ \t]{2,}\S|[ \t]*all rights reserved)", re.I)
CODE_NAME = re.compile(rb"\bvlc_(?=\w)")
COMMENT = re.compile(rb"[ \t]*(?:\*|#|//|/\*|;|--|%|dnl|<!--)?[ \t]*")

ID_RULES = [
    (re.compile(rb"org\.videolan\.vlc\b", re.I), b"com.lycaria.heimdall"),
    (re.compile(rb"\borg\.videolan\.", re.I), b"com.lycaria."),
]
ORG = re.compile(rb"VIDEOLAN|VideoLAN|Videolan|VideoLan|videolan")
ORG_MAP = {b"VIDEOLAN": b"LYCARIA", b"videolan": b"lycaria"}
NAME = re.compile(rb"vlc|Vlc|VLC")
WORD = re.compile(rb"[A-Za-z0-9_]")


def heimdall(text, m):
    """Case-keeping replacement for one vlc/Vlc/VLC match."""
    hit = m.group()
    if hit == b"vlc":
        return b"heimdall"
    if hit == b"Vlc":
        return b"Heimdall"
    # VLC: all caps inside an all-caps name, else Heimdall.
    a, b = m.start(), m.end()
    while a and WORD.match(text, a - 1):
        a -= 1
    while b < len(text) and WORD.match(text, b):
        b += 1
    token = text[a:b]
    pieces = token.split(b"_")
    i = len(token[:m.start() - a].split(b"_")) - 1
    part = pieces[i]
    if part != b"VLC":                       # QVLCFrame, VLCStyle, LIBVLC
        return b"Heimdall" if re.search(rb"[a-z]", part) else b"HEIMDALL"
    others = [p for j, p in enumerate(pieces) if j != i and p]
    if any(len(p) > 1 and p.isupper() for p in others):  # VLC_TICK_FROM_MS
        return b"HEIMDALL"
    if not others and b"_" in token:         # VLC_##x token pasting
        return b"HEIMDALL"
    return b"Heimdall"                       # VLC, VLC_Init, psz_VLC


def rename_text(text):
    for rule, new in ID_RULES:
        text = rule.sub(new, text)
    text = ORG.sub(lambda m: ORG_MAP.get(m.group(), b"Lycaria"), text)
    return NAME.sub(lambda m: heimdall(text, m), text)


def rename_line(line):
    out, last = [], 0
    for m in PROTECT.finditer(line):
        out += [rename_text(line[last:m.start()]), m.group()]
        last = m.end()
    out.append(rename_text(line[last:]))
    return b"".join(out)


def copyright_lines(lines):
    """Indexes of copyright lines and their continuation lines."""
    keep, inside = set(), False
    for i, line in enumerate(lines):
        if COPYRIGHT.search(line):
            inside = True
        elif not (inside and CONTINUED.match(line)):
            inside = False
            continue
        keep.add(i)
    return keep


def add_copyright(lines, keep):
    """Add our copyright under the last one in the header."""
    head = [i for i in sorted(keep) if i < 60]
    if not head or any(AUTHOR in l for l in lines[:60]):
        return lines
    first = next(i for i in head if COPYRIGHT.search(lines[i]))
    m = COPYRIGHT.search(lines[first])
    prefix = lines[first][:m.start()]
    if not COMMENT.fullmatch(prefix):        # a string in code, not a notice
        return lines
    end = re.search(rb"\r?\n$", lines[first])
    new = prefix + b"Copyright (C) " + YEAR + b" " + AUTHOR + (end.group() if end else b"\n")
    last = head[-1]
    while last + 1 in keep:
        last += 1
    return lines[:last + 1] + [new] + lines[last + 1:]


def skipped(path):
    return any(fnmatch.fnmatch(path, p) for p in SKIP)


def is_text(data):
    return b"\0" not in data[:8000]


def process(path):
    with open(path, "rb") as f:
        data = f.read()
    if not is_text(data):
        return False
    lines = data.splitlines(keepends=True)
    keep = copyright_lines(lines)
    # Copyright lines keep their text, but code names in them still change
    # (meson's f'Copyright © @vlc_copyright_years@ ...').
    new = [CODE_NAME.sub(b"heimdall_", l) if i in keep else rename_line(l)
           for i, l in enumerate(lines)]
    if new == lines:
        return False
    if not path.startswith("po/"):
        new = add_copyright(new, keep)
    with open(path, "wb") as f:
        f.write(b"".join(new))
    return True


def report(files):
    why = Counter()
    other = []
    pat = re.compile(rb"vlc|videolan", re.I)
    for path in files:
        if not os.path.exists(path):
            continue
        with open(path, "rb") as f:
            data = f.read()
        if not is_text(data):
            continue
        lines = data.splitlines()
        keep = copyright_lines(lines)
        for n, line in enumerate(lines):
            for m in pat.finditer(line):
                if skipped(path):
                    why["skipped file"] += 1
                elif n in keep:
                    why["copyright"] += 1
                elif any(p.start() <= m.start() < p.end() for p in PROTECT.finditer(line)):
                    why["url, host or mail"] += 1
                else:
                    why["other"] += 1
                    other.append(f"{path}:{n + 1}: {line.strip()[:160].decode('utf-8', 'replace')}")
    print("Leftovers:")
    for k, v in why.most_common():
        print(f"  {k}: {v}")
    for line in other:
        print("  " + line)


def main():
    files = subprocess.run(["git", "ls-files", "-z"], capture_output=True,
                           check=True).stdout.decode().split("\0")
    files = [f for f in files if f]
    edited = sum(process(f) for f in files if not skipped(f))

    moves = []
    for f in files:
        if skipped(f):
            continue
        new = rename_text(f.encode()).decode()
        if new != f:
            moves.append((f, new))
    targets = [new for old, new in moves]
    taken = {f.lower() for f in files} - {old.lower() for old, new in moves}
    clash = [t for t in targets if t.lower() in taken or targets.count(t) > 1]
    if clash:
        sys.exit(f"Paths would clash: {clash[:10]}")
    for old, new in moves:
        os.makedirs(os.path.dirname(new) or ".", exist_ok=True)
        os.replace(old, new)
        d = os.path.dirname(old)
        while d and not os.listdir(d):
            os.rmdir(d)
            d = os.path.dirname(d)
    if moves:
        paths = "\0".join(p for m in moves for p in m)
        subprocess.run(["git", "add", "-A", "-f", "--pathspec-from-file=-",
                        "--pathspec-file-nul"], input=paths.encode(), check=True)

    print(f"Edited {edited} files, moved {len(moves)} files.")
    moved = dict(moves)
    report([moved.get(f, f) for f in files])


if __name__ == "__main__":
    sys.exit(main())
