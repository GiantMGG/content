#!/usr/bin/env python3
"""Lint every DefCore.txt in the Agriculture pack — assert required fields
(id, Name, Category) are present and well-formed."""

import re
import sys
from pathlib import Path

PACK_ROOT = Path(__file__).resolve().parent.parent
REQUIRED = ["id", "Name", "Category"]

ID_RE = re.compile(r"^id\s*=\s*[A-Z0-9_]{1,8}\s*$", re.MULTILINE)
NAME_RE = re.compile(r"^Name\s*=\s*\S.+\s*$", re.MULTILINE)
CAT_RE = re.compile(r"^Category\s*=\s*C4D_", re.MULTILINE)


def lint(defcore: Path):
    text = defcore.read_text(encoding="latin-1")
    errs = []
    if not ID_RE.search(text):
        errs.append(f"missing or malformed `id=` (must be uppercase, <=8 chars)")
    if not NAME_RE.search(text):
        errs.append(f"missing or malformed `Name=`")
    if not CAT_RE.search(text):
        errs.append(f"missing or malformed `Category=` (must start with C4D_)")
    return errs


def main():
    errors = []
    count = 0
    for defcore in PACK_ROOT.rglob("DefCore.txt"):
        count += 1
        for e in lint(defcore):
            errors.append(f"{defcore}:\n  {e}")

    if errors:
        for e in errors:
            print(e, file=sys.stderr)
        print(f"\nFAIL: {len(errors)} DefCore problem(s) in {count} files", file=sys.stderr)
        sys.exit(1)
    print(f"OK: {count} DefCore files linted clean")


if __name__ == "__main__":
    main()
