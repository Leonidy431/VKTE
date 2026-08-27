#!/usr/bin/env python3
"""Generate context_map.json: a lightweight dependency tree of the project.

For each C source/header under firmware/ and each test under tests/, records
the local #include edges, so an agent editing a file can see what depends on
it without reading the whole tree (AUTONOMY_PLAYBOOK artifact #4).

Idempotent; run after adding/removing files or includes.
"""

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCAN_DIRS = ["firmware/src", "firmware/inc", "tests/unit_tests", "tests/inc"]
INCLUDE_RE = re.compile(r'^\s*#include\s+"([^"]+)"', re.M)


def main():
    files = {}
    for d in SCAN_DIRS:
        for p in sorted((ROOT / d).rglob("*.[ch]")):
            rel = p.relative_to(ROOT).as_posix()
            files[rel] = INCLUDE_RE.findall(p.read_text(errors="replace"))

    # Reverse edges: header basename -> files that include it
    dependents = {}
    for rel, incs in files.items():
        for inc in incs:
            dependents.setdefault(inc, []).append(rel)

    out = {
        "generated_by": "tools/gen_context_map.py",
        "includes": files,
        "dependents": dependents,
    }
    (ROOT / "context_map.json").write_text(json.dumps(out, indent=1, sort_keys=True))
    print(f"context_map.json: {len(files)} files, {len(dependents)} headers mapped")


if __name__ == "__main__":
    main()
