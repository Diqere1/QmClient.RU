#!/usr/bin/env python3
"""Validate all generated QmClient language files."""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.dirname(__file__))

import twlang_qmclient as twlang

errors = []
langs_dir = os.path.join(
    os.path.dirname(__file__), "..", "..", "data", "qmclient", "languages"
)
count = 0
for fname in sorted(os.listdir(langs_dir)):
    if not fname.endswith(".txt") or fname in ("index.txt", "README.txt"):
        continue
    fpath = os.path.join(langs_dir, fname)
    try:
        trans = twlang.translations(fpath)
        print(f"  OK: {fname} ({len(trans)} entries)")
        count += 1
    except Exception as e:
        errors.append(f"{fname}: {e}")
        print(f"  FAIL: {fname}: {e}")

print()
if errors:
    print(f"{len(errors)} files with errors!")
    sys.exit(1)
else:
    print(f"All {count} language files parse correctly!")
