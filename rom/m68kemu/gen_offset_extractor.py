#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# Author: Fabian Schmieder
"""Scan NDK headers, generate C that emits offsets — recurse into embedded structs."""
import re, os

NDK = "/opt/amiga/m68k-amigaos/ndk-include"
DIRS = ["exec","dos","intuition","graphics","utility","workbench",
        "devices","libraries","resources","datatypes"]
SKIP_HEADERS = {"libraries/bsdsocket.h","libraries/miami.h","libraries/usergroup.h",
                "devices/scsidisk.h","devices/trackfile.h",
                "dos/shell.h","graphics/graphint.h",
                "libraries/keymap.h","libraries/mathlibrary.h","libraries/mathresource.h"}

SKIP_STRUCTS = {
    "AslSemaphore","CliProcList","ConsoleScrollback",
    "gpKeyGoInactive","gpKeyInput","gpKeyTest",
    "impDomainFrame","pdtObtainPixelArray","TimeRequest","TimeVal",
    # Structs with different layouts in AROS or missing fields
    "mouth_rb","narrator_rb","NSDeviceQueryResult","TDU_PublicUnit",
    "WordColorEntry","XRef","FileHandle32","IntuiWheelData",
}

SKIP_FIELDS = {
    "nxtlist","Cop2Start","Cop3Start","Cop4Start","Cop5Start",
    "ie_y","pd_p0","pd_s0","X","Y",
    "BlockPen","DetailPen","Depth","Height","Width",
    "dr_UnitInit","fo_Reserved0","reserved1","Scratch","ScratchSize",
    # Union members or macro-generated names that fail offsetof()
    "Flags","VWaitPos","DestAddr","ie_x",
    "dol_Handler","dol_Priority","dol_SegList","dol_StackSize","dol_Startup",
    "meu_Addr","meu_Reqs","nr_Port",
    # Fields in NDK but not in AROS (different struct layouts)
    "reservedlink","vlink","eb_Private05","fh_Link","fo_SpecialDrawMode",
    "OnScreen","OffScreen","Backup","resPtr1","resPtr2","resPtr5",
    "res_count","UserClipRectsCount","wordreserved","DrawPath","BitMap",
}

# Parse all struct definitions from NDK headers
structs = {}       # name -> (header, [(type_str, field_name, is_pointer, is_array)])
struct_order = []  # preserve discovery order
headers_needed = {}  # name -> header

for d in DIRS:
    path = os.path.join(NDK, d)
    if not os.path.isdir(path):
        continue
    for fn in sorted(os.listdir(path)):
        if not fn.endswith(".h"):
            continue
        hdr = f"{d}/{fn}"
        if hdr in SKIP_HEADERS:
            continue
        with open(os.path.join(path, fn), "r", errors="replace") as f:
            src = f.read()
        for m in re.finditer(r"struct\s+(\w+)\s*\{([^}]*)\}", src, re.DOTALL):
            sname = m.group(1)
            body = m.group(2)
            if sname in SKIP_STRUCTS or sname in structs:
                continue
            fields = []
            for line in body.split("\n"):
                line = line.strip()
                if not line or line.startswith("/*") or line.startswith("//") or line.startswith("#"):
                    continue
                # Strip trailing comments and semicolons
                line = re.sub(r'/\*.*?\*/', '', line).strip()
                line = re.sub(r'//.*$', '', line).strip()
                line = line.rstrip(";").strip()
                # Match: [qualifiers] type [*]name [array]
                fm = re.match(
                    r"(?:(?:unsigned|signed|const|volatile)\s+)*"
                    r"(?:(struct|enum)\s+)?(\w+)\s+(\*?)(\w+)(?:\[(\d+)\])?$",
                    line)
                if fm:
                    fname = fm.group(4)
                    if fname in SKIP_FIELDS:
                        continue
                    is_struct = fm.group(1) == "struct"
                    type_name = fm.group(2)
                    is_pointer = fm.group(3) == "*"
                    is_array = fm.group(5) is not None
                    fields.append((type_name, fname, is_pointer, is_array, is_struct))
            if fields:
                structs[sname] = (hdr, fields)
                struct_order.append(sname)
                headers_needed[sname] = hdr


def get_leaf_fields(sname, prefix, depth=0):
    """Recursively get leaf (scalar/pointer) fields with their access paths.

    For each field, yields (field_display_name, c_access_path) where:
    - field_display_name is the leaf field name (e.g. "io_Command")
    - c_access_path is the C expression for offsetof/sizeof (e.g. "io_Message.mn_Node.ln_Succ")

    Embedded structs are recursed into. Pointers, scalars, and arrays are leaf fields.
    """
    if sname not in structs or depth > 4:
        return
    _, fields = structs[sname]
    for type_name, fname, is_pointer, is_array, is_struct in fields:
        access = f"{prefix}{fname}" if prefix else fname
        if is_struct and not is_pointer and not is_array and type_name in structs:
            # Embedded struct — recurse into it
            yield from get_leaf_fields(type_name, access + ".", depth + 1)
        else:
            # Leaf field (scalar, pointer, or array) — emit it
            yield (fname, access)


# Generate C code
print('#include <stddef.h>')
headers = set(hdr for hdr, _ in structs.values())
for h in sorted(headers):
    print(f'#include <{h}>')
print('\nvoid dummy(void) {')

for sname in sorted(structs.keys()):
    seen_names = set()
    for display_name, access_path in get_leaf_fields(sname, ""):
        # Avoid duplicate field names within the same struct
        # (can happen if two embedded structs have fields with the same name)
        if display_name in seen_names:
            continue
        seen_names.add(display_name)
        print(f'__asm__ volatile (".ascii \\"OFF|{sname}|{display_name}|%0|%1|\\"" '
              f':: "n" ((int)offsetof(struct {sname}, {access_path})), '
              f'"n" ((int)sizeof(((struct {sname}*)0)->{access_path})));')
    print(f'__asm__ volatile (".ascii \\"SIZ|{sname}|%0|\\"" '
          f':: "n" ((int)sizeof(struct {sname})));')

print('}')
