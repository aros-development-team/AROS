#!/usr/bin/env python3
# Copyright 2026, The AROS Development Team. All rights reserved.
#
# galliumglue.py - generate the GalliumCoreAPI function-pointer table and
# the consumer-side trampoline glue that lets a gallium pipe driver (in
# its own hidd module) call mesa3dgl's single Mesa compiler core (NIR,
# ralloc, glsl_types, gallium auxiliary) through a versioned,
# hash-checked table instead of linking its own copy.
#
# Provider side (compiled into mesa3dgl via libgalliumcoreapi.a):
#   gallium_core_api.h   - slot enum, struct, GCA_VERSION, GCA_ABI_HASH
#   gca_table.c          - the populated table + gallium_core_get_api()
# Consumer side (compiled into the pipe driver hidd):
#   gca_bind.c           - __gca_local[] + gca_bind() (version/hash check)
#   gca_glue.S           - one ARM trampoline per imported function
#   gca_redefs.txt       - objcopy --redefine-syms map (sym -> __gca_sym)
#   private/             - extracted provider members whose DATA symbols
#                          the driver references (const tables + the
#                          util_cpu_caps write-once case). Linked
#                          privately into the hidd; safe because they are
#                          index-accessed metadata with no cross-module
#                          pointer identity.
#   gca_private.list     - the extracted objects, one path per line
#
# The import set is computed mechanically: undefined symbols of the
# consumer archive plus the private members, minus their own definitions,
# intersected with the provider archives. Function symbols are
# trampolined. Data symbols must each be covered by --private-data;
# anything else is a hard error (the class-4 guard rail).

import argparse
import os
import subprocess
import sys
import tempfile


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, check=True,
                          **kw)


def nm_lines(nm, path):
    return run([nm, "-g", path]).stdout.splitlines()


def collect_undefined(nm, paths):
    syms = set()
    for p in paths:
        for line in nm_lines(nm, p):
            parts = line.split()
            if len(parts) == 2 and parts[0] == "U":
                syms.add(parts[1])
    return syms


def collect_defined(nm, paths):
    """symbol -> nm type letter (T/W/R/D/B/...)"""
    syms = {}
    for p in paths:
        for line in run([nm, "-g", "--defined-only", p]).stdout.splitlines():
            parts = line.split()
            if len(parts) == 3:
                if parts[2] not in syms or parts[1] in ("T", "D", "R", "B"):
                    syms[parts[2]] = parts[1]
    return syms


def find_member(nm, archives, sym):
    """(archive, member) defining sym."""
    for a in archives:
        member = None
        for line in run([nm, "-g", "--defined-only", a]).stdout.splitlines():
            if line.endswith(":"):
                member = line[:-1]
                continue
            parts = line.split()
            if len(parts) == 3 and parts[2] == sym:
                return a, member
    return None, None


def fnv1a32(data):
    h = 0x811c9dc5
    for b in data.encode("utf-8"):
        h = ((h ^ b) * 0x01000193) & 0xffffffff
    return h


def atomic_write(path, content):
    d = os.path.dirname(path) or "."
    data = content.encode("utf-8")
    fd, tmp = tempfile.mkstemp(dir=d)
    try:
        os.write(fd, data)
    finally:
        os.close(fd)
    os.replace(tmp, path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--nm", required=True)
    ap.add_argument("--ar", required=True)
    ap.add_argument("--consumer", required=True, nargs="+",
                    help="pipe driver archive (libgallium_vc4.a)")
    ap.add_argument("--providers", required=True, nargs="+",
                    help="mesa3dgl core archives")
    ap.add_argument("--private-data", nargs="*", default=[],
                    help="data symbols resolved by extracting the defining "
                         "provider member into the consumer module")
    ap.add_argument("--mesa-version", required=True)
    ap.add_argument("--arch", default="arm", choices=("arm", "aarch64"),
                    help="target CPU: picks the trampoline template")
    ap.add_argument("--abi-flags", default="",
                    help="ABI-relevant compile flags; part of the hash so "
                         "layout-affecting flag changes refuse to bind")
    ap.add_argument("--abi-files", nargs="*", default=[],
                    help="files (e.g. the mesa aros diff) whose content is "
                         "digested into the hash")
    ap.add_argument("--outdir", required=True)
    args = ap.parse_args()

    os.makedirs(args.outdir, exist_ok=True)
    privdir = os.path.join(args.outdir, "private")
    os.makedirs(privdir, exist_ok=True)

    # ---- extract private members ----
    # Destination names carry the archive name: two archives may hold
    # same-named members, and a bare-basename scheme would silently reuse
    # the first extraction for both.
    private_objs = []
    for sym in args.private_data:
        arch, member = find_member(args.nm, args.providers, sym)
        if not member:
            sys.exit("galliumglue: --private-data %s: no provider defines it"
                     % sym)
        libtag = os.path.splitext(os.path.basename(arch))[0]
        dst = os.path.join(privdir, "%s__%s" % (libtag, member))
        if not os.path.exists(dst):
            run([args.ar, "x", os.path.abspath(arch), member], cwd=privdir)
            os.replace(os.path.join(privdir, member), dst)
        if dst not in private_objs:
            private_objs.append(dst)

    # ---- import set: drivers + private members, minus own definitions ----
    # Per consumer, then the union: one provider table has to serve every
    # driver, and gca_bind matches slots positionally, so all of them must
    # be generated against the same list. Subtracting one driver's own
    # definitions from another's imports would be wrong, hence per-consumer
    # subtraction before the union.
    imports = set()
    for consumer in args.consumer:
        consumer_paths = [consumer] + private_objs
        imp = collect_undefined(args.nm, consumer_paths)
        own = collect_defined(args.nm, consumer_paths)
        imports |= (imp - set(own))

    provided = collect_defined(args.nm, args.providers)

    funcs = sorted(s for s in imports if provided.get(s) in ("T", "W"))
    data = sorted(s for s in imports if s in provided
                  and provided[s] not in ("T", "W"))
    residual = sorted(s for s in imports if s not in provided)

    if not funcs:
        sys.exit("galliumglue: empty function import set - wrong archives?")
    if data:
        # class-4 guard: every data import must be a --private-data decision
        sys.exit("galliumglue: unhandled data imports (add to --private-data "
                 "after auditing identity semantics): %s" % ", ".join(data))

    # ABI hash: layout-compat scope ONLY — mesa version, ABI-relevant
    # flags, and the content of layout-affecting inputs (the aros diff).
    # Slot compatibility is deliberately NOT hashed: gca_bind verifies
    # every slot by name, so appending slots keeps older consumers
    # binding, and a slot mismatch is reported by index instead of as an
    # opaque hash failure.
    hashsrc = "mesa-%s|flags-%s" % (args.mesa_version, args.abi_flags.strip())
    for f in args.abi_files:
        with open(f, "rb") as fh:
            hashsrc += "|%s-%08x" % (os.path.basename(f),
                                     fnv1a32(fh.read().decode("utf-8",
                                                              "replace")))
    abihash = fnv1a32(hashsrc)

    guardnote = ("/* Generated by galliumglue.py - DO NOT EDIT.\n"
                 " * Consumer: %s\n * Mesa: %s, %d function slots, "
                 "%d private data objects, %d residual libc/AROS refs.\n */\n"
                 % (", ".join(os.path.basename(c) for c in args.consumer),
                    args.mesa_version,
                    len(funcs), len(private_objs), len(residual)))

    # ---------- gallium_core_api.h ----------
    h = [guardnote]
    h.append("#ifndef GALLIUM_CORE_API_H\n#define GALLIUM_CORE_API_H\n")
    h.append("#include <stdint.h>\n")
    h.append("#define GCA_VERSION    1")
    h.append("#define GCA_ABI_HASH   0x%08xu" % abihash)
    h.append("#define GCA_FUNC_COUNT %d\n" % len(funcs))
    h.append("struct GalliumCoreAPI\n{")
    h.append("    uint32_t gca_Version;")
    h.append("    uint32_t gca_Size;")
    h.append("    uint32_t gca_ABIHash;")
    h.append("    uint32_t gca_FuncCount;")
    h.append("    void   *const *gca_Func;")
    h.append("    const char *const *gca_Names;")
    h.append("};\n")
    h.append("enum\n{")
    for s in funcs:
        h.append("    GCA_%s," % s)
    h.append("};\n")
    h.append("/* Provider (mesa3dgl side). Validation is the consumer's "
             "job (gca_bind). */")
    h.append("const struct GalliumCoreAPI *gallium_core_get_api(void);\n")
    h.append("/* Consumer (driver hidd side). 0 = bound. Negative = table")
    h.append(" * rejected (-1 null/version, -2 size, -3 hash, -4 count);")
    h.append(" * positive = name mismatch at slot (value - 1), see")
    h.append(" * gca_slot_name() for what the consumer expected there. */")
    h.append("int gca_bind(const struct GalliumCoreAPI *api);")
    h.append("const char *gca_slot_name(uint32_t slot);\n")
    h.append("#endif /* GALLIUM_CORE_API_H */")
    atomic_write(os.path.join(args.outdir, "gallium_core_api.h"),
                 "\n".join(h) + "\n")

    # ---------- gca_table.c (provider) ----------
    c = [guardnote]
    c.append('#include "gallium_core_api.h"\n')
    for s in funcs:
        c.append("extern void %s(void);" % s)
    c.append("\nstatic void *const gca_funcs[GCA_FUNC_COUNT] =\n{")
    for s in funcs:
        c.append("    (void *)%s," % s)
    c.append("};\n")
    c.append("static const char *const gca_names[GCA_FUNC_COUNT] =\n{")
    for s in funcs:
        c.append('    "%s",' % s)
    c.append("};\n")
    c.append("static const struct GalliumCoreAPI gca_table =\n{")
    c.append("    GCA_VERSION,")
    c.append("    sizeof(struct GalliumCoreAPI),")
    c.append("    GCA_ABI_HASH,")
    c.append("    GCA_FUNC_COUNT,")
    c.append("    gca_funcs,")
    c.append("    gca_names,")
    c.append("};\n")
    c.append("const struct GalliumCoreAPI *gallium_core_get_api(void)")
    c.append("{")
    c.append("    return &gca_table;")
    c.append("}")
    atomic_write(os.path.join(args.outdir, "gca_table.c"),
                 "\n".join(c) + "\n")

    # ---------- gca_bind.c (consumer) ----------
    b = [guardnote]
    b.append('#include "gallium_core_api.h"\n')
    b.append("void *__gca_local[GCA_FUNC_COUNT];\n")
    b.append("static int gca_streq(const char *a, const char *b)")
    b.append("{")
    b.append("    while (*a && *a == *b) { a++; b++; }")
    b.append("    return *a == *b;")
    b.append("}\n")
    b.append("static const char *const gca_wanted[GCA_FUNC_COUNT] =")
    b.append("{")
    for s in funcs:
        b.append('    "%s",' % s)
    b.append("};\n")
    b.append("const char *gca_slot_name(uint32_t slot)")
    b.append("{")
    b.append("    return (slot < GCA_FUNC_COUNT) ? gca_wanted[slot] : \"?\";")
    b.append("}\n")
    b.append("int gca_bind(const struct GalliumCoreAPI *api)")
    b.append("{")
    b.append("    uint32_t i;\n")
    b.append("    if (!api || api->gca_Version != GCA_VERSION)")
    b.append("        return -1;")
    b.append("    if (api->gca_Size < sizeof(struct GalliumCoreAPI))")
    b.append("        return -2;")
    b.append("    if (api->gca_ABIHash != GCA_ABI_HASH)")
    b.append("        return -3;")
    b.append("    if (api->gca_FuncCount < GCA_FUNC_COUNT)")
    b.append("        return -4;")
    b.append("    /* slot identity by name: catches provider/consumer built")
    b.append("     * from different generator runs even when count+hash")
    b.append("     * happen to collide */")
    b.append("    for (i = 0; i < GCA_FUNC_COUNT; i++)")
    b.append("        if (!api->gca_Names[i] || !gca_streq(api->gca_Names[i], gca_wanted[i]))")
    b.append("            return (int)i + 1;")
    b.append("    for (i = 0; i < GCA_FUNC_COUNT; i++)")
    b.append("        __gca_local[i] = api->gca_Func[i];")
    b.append("    return 0;")
    b.append("}")
    atomic_write(os.path.join(args.outdir, "gca_bind.c"),
                 "\n".join(b) + "\n")

    # ---------- gca_glue.S (consumer trampolines) ----------
    #
    # One tail-branch per imported function, loading the target from the
    # module-local table. Argument-transparent (nothing but the ABI's
    # intra-procedure scratch register is touched), so varargs, struct
    # returns and FP arguments all pass through untouched.
    s_ = ["/* " + guardnote[3:-4] + " */\n"]
    if args.arch == "arm":
        s_.append("    .syntax unified")
        s_.append("    .arm")
        s_.append("    .text\n")
        for i, s in enumerate(funcs):
            s_.append("    .balign 4")
            s_.append("    .globl __gca_%s" % s)
            s_.append("    .type __gca_%s, %%function" % s)
            s_.append("__gca_%s:" % s)
            s_.append("    ldr ip, 0f")            # ip = &__gca_local
            s_.append("    ldr pc, [ip, #%d]" % (i * 4))
            s_.append("0:  .word __gca_local\n")
    else:   # aarch64
        s_.append("    .text\n")
        for i, s in enumerate(funcs):
            s_.append("    .balign 8")
            s_.append("    .globl __gca_%s" % s)
            s_.append("    .type __gca_%s, %%function" % s)
            s_.append("__gca_%s:" % s)
            # x16 (IP0) is the ABI's call-boundary scratch register.
            # The literal is R_AARCH64_ABS64, the reloc every AROS
            # aarch64 loader handles; no adrp/GOT needed.
            s_.append("    ldr x16, 0f")           # x16 = &__gca_local
            s_.append("    ldr x16, [x16, #%d]" % (i * 8))
            s_.append("    br x16")
            s_.append("    .balign 8")             # .quad must be aligned
            s_.append("0:  .quad __gca_local\n")
    atomic_write(os.path.join(args.outdir, "gca_glue.S"),
                 "\n".join(s_) + "\n")

    # ---------- gca_redefs.txt (objcopy map) ----------
    # Symbols defined by private members must not be renamed even if
    # imported elsewhere; they resolve locally in the consumer module.
    atomic_write(os.path.join(args.outdir, "gca_redefs.txt"),
                 "".join("%s __gca_%s\n" % (s, s) for s in funcs))

    # ---------- private object list ----------
    atomic_write(os.path.join(args.outdir, "gca_private.list"),
                 "".join(p + "\n" for p in private_objs))

    # ---------- report ----------
    rep = ["# galliumglue import-set report",
           "functions (trampolined): %d" % len(funcs),
           "private data objects:    %s" % ", ".join(
               os.path.basename(p) for p in private_objs),
           "residual (libc/AROS):    %d" % len(residual),
           ""] + ["  residual: " + s for s in residual] + [""]
    atomic_write(os.path.join(args.outdir, "gca_report.txt"),
                 "\n".join(rep))

    print("galliumglue: %d trampolines, %d private objects, %d residual, "
          "hash 0x%08x" % (len(funcs), len(private_objs), len(residual),
                           abihash))


if __name__ == "__main__":
    main()
