#!/usr/bin/env python3
"""Assert every DefCore.txt id in the Agriculture pack is unique within
the pack and does not collide with any id in the sibling content packs."""

import re
import sys
from pathlib import Path

PACK_ROOT = Path(__file__).resolve().parent.parent  # content/Agriculture.c4d/
CONTENT_ROOT = PACK_ROOT.parent                     # content/
SIBLING_PACKS = ["Objects", "Knights", "Western", "FarWorlds", "Hazard", "Fantasy"]

ID_RE = re.compile(r"^id\s*=\s*(\S+)\s*$", re.MULTILINE)


def collect_ids(root: Path):
    """Return list of (id, defcore_path) tuples found under root."""
    out = []
    for defcore in root.rglob("DefCore.txt"):
        text = defcore.read_text(encoding="latin-1")
        m = ID_RE.search(text)
        if m:
            out.append((m.group(1), defcore))
    return out


def main():
    pack_ids = collect_ids(PACK_ROOT)
    errors = []

    # (a) no duplicate id within the pack
    seen = {}
    for id_, path in pack_ids:
        if id_ in seen:
            errors.append(
                f"Duplicate id {id_} in pack:\n"
                f"  {seen[id_]}\n  {path}"
            )
        else:
            seen[id_] = path

    # (b) no id collides with any sibling content pack
    sibling_ids = {}
    for sib in SIBLING_PACKS:
        sib_root = CONTENT_ROOT / f"{sib}.c4d"
        if not sib_root.is_dir():
            continue
        for id_, path in collect_ids(sib_root):
            sibling_ids.setdefault(id_, []).append(path)

    for id_, path in pack_ids:
        if id_ in sibling_ids:
            errors.append(
                f"Id {id_} ({path}) collides with sibling pack:\n"
                f"  {sibling_ids[id_][0]}"
            )

    if errors:
        for e in errors:
            print(e, file=sys.stderr)
        print(f"\nFAIL: {len(errors)} id problem(s)", file=sys.stderr)
        sys.exit(1)
    print(f"OK: {len(pack_ids)} pack ids verified unique and collision-free")


if __name__ == "__main__":
    main()
