#!/usr/bin/env python3
"""
Two-pass case-correction for #include "..." header file names. For
cross-compiling C/C++ projects whose headers were authored on a
case-insensitive filesystem (e.g. NTFS/Windows) and need to build
cleanly on a case-sensitive one (e.g. Linux).

Pass 1: walk a header directory tree, record the real on-disk casing
of every header file, keyed by lowercased basename.

Pass 2: walk one or more source trees, and for every #include whose
basename (case-insensitively) matches a known header but doesn't
match its real casing, rewrite it to the correct casing.

Usage:
    python fix-include-casing.py <dir> [--also-scan DIR ...]
"""
import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

HEADER_EXTS = {".h", ".hpp", ".hxx"}
SOURCE_EXTS = {".c", ".cpp", ".h", ".hpp", ".hxx", ".cxx"}

INCLUDE_RE = re.compile(r'(#include\s*")((?:[^"/]*/)*)([^"/]+)(")')


def build_casing_index(header_dir: Path) -> dict[str, str]:
    """lowercase basename -> real on-disk basename. Warns on ambiguity."""
    found: dict[str, set[str]] = defaultdict(set)
    for path in header_dir.rglob("*"):
        if path.suffix.lower() in HEADER_EXTS:
            found[path.name.lower()].add(path.name)

    index = {}
    for lower, variants in found.items():
        if len(variants) > 1:
            print(f"warning: ambiguous casing for {lower!r}: {variants} "
                  f"— skipping auto-fix for this name", file=sys.stderr)
            continue
        index[lower] = next(iter(variants))
    return index


def fix_file(path: Path, index: dict[str, str]) -> bool:
    text = path.read_text(errors="ignore")

    def fix(match: re.Match) -> str:
        prefix, basename = match.group(2), match.group(3)
        correct = index.get(basename.lower())
        if correct and basename != correct:
            return f'{match.group(1)}{prefix}{correct}{match.group(4)}'
        return match.group(0)

    new_text = INCLUDE_RE.sub(fix, text)
    if new_text != text:
        path.write_text(new_text)
        return True
    return False


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("header_dir", type=Path,
                         help="Directory tree to index for real header casing")
    parser.add_argument("--also-scan", type=Path, nargs="*", default=[],
                         help="Additional directories to scan and fix "
                              "#include lines in (header_dir is always scanned)")
    args = parser.parse_args()

    header_dir = args.header_dir.resolve()
    if not header_dir.is_dir():
        parser.error(f"{header_dir} is not a directory")

    index = build_casing_index(header_dir)
    print(f"Indexed {len(index)} header name(s) from {header_dir}")

    scan_roots = [header_dir] + [p.resolve() for p in args.also_scan]
    files_changed = 0
    for scan_root in scan_roots:
        for path in scan_root.rglob("*"):
            if path.suffix.lower() not in SOURCE_EXTS:
                continue
            if fix_file(path, index):
                files_changed += 1
                print(f"fixed: {path}")

    print(f"Done. {files_changed} file(s) changed.")


if __name__ == "__main__":
    main()
