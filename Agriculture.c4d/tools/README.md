# Agriculture pack tooling

Two guard-rail scripts. Run from the repo root.

```bash
# Assert pack ids are unique and don't collide with sibling packs
python3 content/Agriculture.c4d/tools/check_ids.py

# Lint every DefCore.txt for required fields
python3 content/Agriculture.c4d/tools/check_defcore.py
```

Both exit non-zero on any violation.
