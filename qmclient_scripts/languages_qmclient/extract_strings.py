#!/usr/bin/env python3
"""Extract all unique Localize() literal strings from QmClient source files."""
import os
import re
import sys

def extract_localize_strings(root_dir):
    """Walk root_dir and extract all literal strings from Localize() calls."""
    strings = set()
    pattern = re.compile(r'Localize\(\s*"((?:[^"\\]|\\.)*)"')

    for dirpath, dirs, files in os.walk(root_dir):
        dirs.sort()
        for fname in sorted(files):
            if not fname.endswith(('.cpp', '.h')):
                continue
            fpath = os.path.join(dirpath, fname)
            try:
                with open(fpath, 'r', encoding='utf-8') as f:
                    content = f.read()
                for m in pattern.finditer(content):
                    s = m.group(1)
                    strings.add(s)
            except Exception as e:
                print(f"Error reading {fpath}: {e}", file=sys.stderr)

    # Also scan gameclient.cpp for QmClient-specific strings (language loading etc.)
    return strings

def main():
    os.chdir(os.path.dirname(__file__) + "/../..")

    # Extract from qmclient components
    strings = extract_localize_strings("src/game/client/components/qmclient")

    # Also extract from gameclient.cpp (language-related Localize)
    strings |= extract_localize_strings("src/game/client/gameclient.cpp")

    # Sort
    sorted_strings = sorted(strings)

    # Write output
    outpath = "qmclient_scripts/languages_qmclient/extracted_strings.txt"
    with open(outpath, 'w', encoding='utf-8') as f:
        for s in sorted_strings:
            f.write(s + '\n')

    print(f"Extracted {len(sorted_strings)} unique Localize strings to {outpath}")

    # Also print them
    for i, s in enumerate(sorted_strings):
        print(f"  {i+1:3d}. {s}")

if __name__ == '__main__':
    main()
