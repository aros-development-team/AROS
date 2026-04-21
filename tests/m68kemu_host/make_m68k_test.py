#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# Author: Fabian Schmieder
"""Generate minimal m68k Amiga hunk test binaries."""
import struct, sys

def be32(v): return struct.pack('>I', v)
def be16(v): return struct.pack('>H', v)

def make_hunk(code_bytes):
    """Wrap m68k code in a minimal Amiga hunk executable."""
    # Pad code to longword boundary
    while len(code_bytes) % 4:
        code_bytes += b'\x00'
    longs = len(code_bytes) // 4

    hunk  = be32(0x000003F3)  # HUNK_HEADER
    hunk += be32(0)           # no resident libs (name count = 0)
    hunk += be32(1)           # 1 hunk
    hunk += be32(0)           # first hunk = 0
    hunk += be32(0)           # last hunk = 0
    hunk += be32(longs)       # hunk 0 size in longwords
    hunk += be32(0x000003E9)  # HUNK_CODE
    hunk += be32(longs)       # code size in longwords
    hunk += code_bytes        # the actual code
    hunk += be32(0x000003F2)  # HUNK_END
    return hunk

# Test 1: MOVEQ #42,D0; RTS — returns 42
test_return42 = make_hunk(
    b'\x70\x2A'   # MOVEQ #42,D0
    b'\x4E\x75'   # RTS
)

# Test 2: MOVEQ #0,D0; RTS — returns 0 (success)
test_return0 = make_hunk(
    b'\x70\x00'   # MOVEQ #0,D0
    b'\x4E\x75'   # RTS
)

# Test 3: A program that calls a library function via A6
# This tests the A-line trap mechanism:
#   MOVE.L  4.W,A6       ; get ExecBase (from address 4)
#   JSR     -552(A6)     ; call FindTask(NULL) — LVO 49, offset -294
#                         ; actually let's use a simpler one
#   MOVEQ   #7,D0        ; return 7
#   RTS
test_execbase = make_hunk(
    b'\x2C\x78\x00\x04'   # MOVEA.L $4.W,A6  (get ExecBase)
    b'\x70\x07'            # MOVEQ #7,D0
    b'\x4E\x75'            # RTS
)

if __name__ == '__main__':
    tests = {
        'return42': test_return42,
        'return0': test_return0,
        'execbase': test_execbase,
    }
    name = sys.argv[1] if len(sys.argv) > 1 else 'return42'
    outfile = sys.argv[2] if len(sys.argv) > 2 else f'test_{name}'
    data = tests[name]
    with open(outfile, 'wb') as f:
        f.write(data)
    print(f'Wrote {len(data)} bytes to {outfile}')
