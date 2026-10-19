#!/usr/bin/env python3
"""Host regression for the production capture routines; no AROS runtime needed.

Checks all six formats, successive buffers, nonzero offset and double step.
On a 64-bit host also prove the old pointer-size stride fails the assertions.
"""
from pathlib import Path
import os, re, resource, struct, subprocess, tempfile
root = Path(__file__).resolve().parent
source = (root.parent / "devsupp.c").read_text(encoding="latin-1")
source = re.sub(r"^#include[^\n]*\n", "", source, flags=re.MULTILINE)
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
with tempfile.TemporaryDirectory(prefix="ahi-capture-") as temp:
    out = Path(temp)
    for original in (False, True):
        if original and struct.calcsize("P") == 4:
            continue
        text = source.replace("sizeof(struct sample)", "sizeof(struct sample *)") if original else source
        (out / "ahi-functions.c").write_text(text)
        binary = out / "capture-test"
        subprocess.run([os.environ.get("CC", "cc"), "-std=gnu11", "-Wall", "-Wextra",
                        "-I" + str(out), str(root / "capture-frames.c"), "-o", str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if original:
            assert result.returncode != 0, "Original stride unexpectedly passed"
            print("Original pointer-sized stride: expected failure")
        else:
            result.check_returncode()
            print(result.stdout, end="")
