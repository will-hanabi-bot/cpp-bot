#!/usr/bin/env python3
"""Re-point `file:line` citations in the docs after a source file was edited.

Usage: scripts/shift_citations.py <source-file> [--base REV] [--apply]

Reads `git diff -U0 REV -- <source-file>` (REV defaults to HEAD), builds the
old-line -> new-line map for every line outside a changed hunk, and rewrites
each citation of that file in the repo's .md files (and in code comments under
src/ and include/; tests/ is skipped, since editing an existing test needs approval). A citation is `<path>:A` or `<path>:A-B` where <path>
ends with the file's name; a bare `name.cpp:N` is resolved by the directory
the citing doc lives in when two files share a name (e.g. the tiiah and the
reactor0 `interpret_reactive.cpp`).

A citation with an endpoint INSIDE a changed hunk cannot be mapped mechanically;
it is listed as NEEDS REVIEW and left alone. Without --apply nothing is written.

Run it BEFORE writing any new citation into the docs: it reads every citation as
pointing at the base revision, so one already written against the new lines would
be shifted a second time.
"""
import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def line_map(path, base):
    diff = subprocess.run(["git", "diff", "-U0", base, "--", path], cwd=ROOT,
                          capture_output=True, text=True, check=True).stdout
    hunks = []  # (old_start, old_len, new_start, new_len)
    for m in re.finditer(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@", diff, re.M):
        os_, ol, ns, nl = m.groups()
        hunks.append((int(os_), int(ol) if ol is not None else 1,
                      int(ns), int(nl) if nl is not None else 1))

    def mapped(old):
        shift = 0
        for os_, ol, ns, nl in hunks:
            # An insertion (ol == 0) sits AFTER old line os_.
            first = os_ if ol > 0 else os_ + 1
            last = os_ + ol - 1
            if ol > 0 and first <= old <= last:
                return None  # inside a changed region
            if old >= first if ol == 0 else old > last:
                shift += nl - ol
        return old + shift

    return mapped, bool(hunks)


def owner_dir(rel):
    return rel.replace("\\", "/").rsplit("/", 1)[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source")
    ap.add_argument("--base", default="HEAD")
    ap.add_argument("--apply", action="store_true")
    a = ap.parse_args()
    src = a.source.replace("\\", "/")
    name = os.path.basename(src)
    mapped, changed = line_map(src, a.base)
    if not changed:
        print("no changes to", src)
        return 0
    # Other files with the same basename, to disambiguate bare citations.
    same_name = []
    for d, _, files in os.walk(os.path.join(ROOT, "src")):
        for f in files:
            if f == name:
                same_name.append(os.path.relpath(os.path.join(d, f), ROOT).replace("\\", "/"))
    for d, _, files in os.walk(os.path.join(ROOT, "include")):
        for f in files:
            if f == name:
                same_name.append(os.path.relpath(os.path.join(d, f), ROOT).replace("\\", "/"))
    ambiguous = len(same_name) > 1
    src_dir = owner_dir(src)

    pat = re.compile(r"([A-Za-z0-9_./-]*" + re.escape(name) + r"):(\d+)(?:-(\d+))?")
    docs = []
    for d, dirs, files in os.walk(ROOT):
        # tests/ is left alone: an edit to an existing test, even a comment, is
        # listed and approved first (CLAUDE.md "Test changes").
        dirs[:] = [x for x in dirs if x not in (".git", "build", ".claude", "logs", "_deps",
                                                  "Testing", "runs", "tests")]
        for f in files:
            if f.endswith((".md", ".cpp", ".h", ".py")):
                docs.append(os.path.join(d, f))
    review = []
    total = 0
    for doc in docs:
        rel = os.path.relpath(doc, ROOT).replace("\\", "/")
        if rel == src:
            continue
        text = open(doc, encoding="utf-8", newline="").read()
        doc_dir = owner_dir(rel)

        def repl(m):
            nonlocal total
            cited, a1, a2 = m.group(1), int(m.group(2)), m.group(3)
            # Does this citation mean OUR file?
            if "/" in cited:
                if not src.endswith(cited.lstrip("./")) and not cited.endswith(src):
                    return m.group(0)
            elif ambiguous:
                # A bare name: the doc's own directory decides, else skip.
                if not doc_dir.startswith(src_dir) and not src_dir.startswith(doc_dir):
                    return m.group(0)
            n1 = mapped(a1)
            n2 = mapped(int(a2)) if a2 else None
            if n1 is None or (a2 and n2 is None):
                review.append(f"{rel}: {m.group(0)}")
                return m.group(0)
            new = f"{cited}:{n1}" + (f"-{n2}" if a2 else "")
            if new != m.group(0):
                total += 1
            return new

        out = pat.sub(repl, text)
        if out != text and a.apply:
            open(doc, "w", encoding="utf-8", newline="").write(out)
        elif out != text:
            print("would update", rel)
    print(f"{total} citation(s) {'updated' if a.apply else 'to update'}")
    for r in review:
        print("NEEDS REVIEW", r)
    return 0


if __name__ == "__main__":
    sys.exit(main())
