#!/usr/bin/env python3
#
# Copyright (C) 2026 The AROS Dev Team
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
"""
mkimports.py - keep the list of what a module library uses of the core correct.

(Run through tools/netcore-update.sh, which knows which files of the stack
are core and which are module, and compiles them.)

The list is net/netcore.def: one line `NC(name)` per function or variable of
the core that a module may use.  Its order is the binary layout of
`struct netcore`, so lines are only ever appended.

  derive  --nm NM --objdump OBJDUMP --core OBJ... --module OBJ... [--def FILE]
          From object files compiled the ordinary way (as part of the stack
          program): every symbol the module's objects leave undefined and an
          object of the core defines is an import.  Reports
            - the imports, functions and variables, and which are not in the
              .def file yet (with --append they are appended);
            - lines of the .def file this module does not use (information);
            - symbols the module needs that neither it nor the core defines
              and that are not in a link library: a reference to ANOTHER
              module, which has to go through a registration instead;
            - imports whose address is used in a static initialiser (a
              relocation in a data section): the table is filled at run time,
              so these have to be assigned in the module's start function;
            - with --src/--inc: imports whose name is also a struct/union/enum
              tag, and declarations of imports written locally in a module
              source (both stop the mapping header from working).

  map     --def FILE -o HEADER
          Writes the mapping header: `#define name (*__netcore.nc_name)`.

  verify  --nm NM --def FILE --core OBJ... --module OBJ...
          For objects compiled AS A MODULE (-DNETMODULE): none of them may
          still leave a symbol of the core undefined.  Exit status 1 if one
          does.  (The link of the library fails on the same thing; this says
          which file and which symbol.)

  edits   --def FILE --src SOURCE...
          Lists, as file:line: lines, what has to change in each module
          source before it can be built into a module library: the include
          line, the local declarations of core names, the uses of a
          variable under the name that is also a structure tag.  Changes
          nothing.

No symbol is ever looked up by name when a library is loaded: this script
runs at build time and writes C source.
"""
import argparse, glob, os, re, subprocess, sys

def expand(paths):
    out = []
    for p in paths:
        if os.path.isdir(p):
            out += sorted(glob.glob(os.path.join(p, "*.o")))
        else:
            g = sorted(glob.glob(p))
            out += g if g else [p]
    return out

def nm(tool, obj):
    """-> (defined {name: kind}, undefined set); kind 'func' or 'data'."""
    defs, und = {}, set()
    for line in subprocess.run([tool, obj], capture_output=True, text=True).stdout.splitlines():
        p = line.split()
        if len(p) == 2 and p[0] == "U":
            und.add(p[1])
        elif len(p) == 3 and p[1] in "TDBRCGSV":
            defs[p[2]] = "func" if p[1] == "T" else "data"
    return defs, und

def unprefix(sym, prefix):
    return sym[len(prefix):] if prefix and sym.startswith(prefix) else sym

def read_def(path, full=False):
    """-> list of (name in the .def file, symbol in the core[, wrapper]).

    A line may end in a comment that says more:
      NC(ifnet_list)   /* = ifnet */
          the sources use `ifnet_list'; in the core that is the variable `ifnet'
      NC(bsd_malloc)   /* macro(size, type, flags) = (size) */
          the headers wrap the function in a macro of the same name; the
          mapping keeps the wrapper
    """
    out = []
    if not path or not os.path.exists(path):
        return out
    for line in open(path):
        m = re.match(r"\s*NC\(\s*(\w+)\s*\)\s*(?:/\*\s*(.*?)\s*\*/)?", line)
        if not m:
            continue
        name, note, sym, wrap = m.group(1), m.group(2) or "", m.group(1), None
        a = re.match(r"=\s*(\w+)$", note)
        w = re.match(r"macro\s*(\(.*?\))\s*=\s*(\(.*\))$", note)
        if a:
            sym = a.group(1)
        if w:
            wrap = (w.group(1), w.group(2))
        out.append((name, sym, wrap) if full else (name, sym))
    return out

def data_relocs(objdump, obj):
    """Symbols whose address is stored in a data section of obj."""
    out, sect = set(), None
    for line in subprocess.run([objdump, "-r", obj], capture_output=True, text=True).stdout.splitlines():
        m = re.match(r"RELOCATION RECORDS FOR \[(.*)\]", line)
        if m:
            sect = m.group(1)
            continue
        if sect is None or sect.startswith(".text") or sect.startswith(".debug") or sect.startswith(".eh_frame"):
            continue
        p = line.split()
        if len(p) >= 3 and re.match(r"^[0-9a-f]+$", p[0]):
            out.add(re.sub(r"[+-]0x[0-9a-f]+$", "", p[2]))
    return out

def gather(args):
    core_defs, mod_defs, mod_und = {}, {}, {}
    mods = set(os.path.basename(o) for o in expand(args.module))
    for o in expand(args.core):
        if os.path.basename(o) in mods:     # named on both sides: it belongs to the module
            continue
        d, _ = nm(args.nm, o)
        for s, k in d.items():
            core_defs[unprefix(s, args.prefix)] = (k, os.path.basename(o))
    for o in expand(args.module):
        d, u = nm(args.nm, o)
        for s, k in d.items():
            mod_defs[unprefix(s, args.prefix)] = k
        mod_und[o] = set(unprefix(s, args.prefix) for s in u)
    return core_defs, mod_defs, mod_und

def cmd_derive(args):
    core_defs, mod_defs, mod_und = gather(args)
    need = set().union(*mod_und.values()) - set(mod_defs)
    imports = sorted(s for s in need if s in core_defs)
    other = sorted(s for s in need if s not in core_defs)
    listed = read_def(args.deffile)
    alias_of = {sym: name for name, sym in listed}       # core symbol -> name in the .def file
    funcs = [s for s in imports if core_defs[s][0] == "func"]
    datas = [s for s in imports if core_defs[s][0] == "data"]
    print("imports from the core: %d (%d functions, %d variables)" % (len(imports), len(funcs), len(datas)))
    print("  variables: " + " ".join(datas))
    if args.list:
        with open(args.list, "w") as f:
            f.write("".join("%s %s %s\n" % (s, core_defs[s][0], core_defs[s][1]) for s in imports))
    missing = [s for s in imports if s not in alias_of]
    if args.deffile:
        print("not in %s: %d" % (os.path.basename(args.deffile), len(missing)))
        for s in missing:
            print("  NC(%s)\t/* %s, %s */" % (s, core_defs[s][0], core_defs[s][1]))
        unused = [name for name, sym in listed if sym not in imports]
        print("in the list, not used by this module: %d  %s" % (len(unused), " ".join(unused)))
    if args.known:
        known = set(open(args.known).read().split())
        other = [s for s in other if s not in known]
    print("needed, defined neither by the module nor by the core (link library, or ANOTHER MODULE): %d" % len(other))
    print("  " + " ".join(other))
    if args.objdump:
        st = {}
        for o in expand(args.module):
            for s in data_relocs(args.objdump, o):
                s = unprefix(s, args.prefix)
                if s in imports:
                    st.setdefault(s, []).append(os.path.basename(o))
        print("imports used in a static initialiser (assign at start instead): %d" % len(st))
        for s in sorted(st):
            print("  %-24s %s" % (s, " ".join(sorted(st[s]))))
    tags = {}
    if args.inc or args.src:
        # both names: the one in the .def file, and the core's symbol where
        # that differs (a local `extern struct ifnet *ifnet;' has to go too)
        tags = lint(sorted(set(imports) | set(alias_of.get(s, s) for s in imports)),
                    set(s for s in imports if alias_of.get(s, s) != s), args)
    if args.append and missing:
        # A variable named like a structure cannot be listed under its own
        # name (the mapping would replace the tag as well): it needs an
        # alias in its header first, and a line  NC(alias)  /* = name */
        held = [s for s in missing if s in tags]
        add = [s for s in missing if s not in tags]
        if add:
            with open(args.deffile, "a") as f:
                f.write("/* appended by mkimports.py: raise NETCORE_VERSION */\n")
                for s in add:
                    f.write("NC(%s)\n" % s)
        print("appended %d lines to %s" % (len(add), args.deffile))
        for s in held:
            print("NOT appended: %s is also a tag. Give the variable an alias in its header "
                  "and add  NC(<alias>)  /* = %s */  by hand." % (s, s))
    return 0

def short(path):
    """dir/file of a source path, as the reports name it."""
    return "/".join(path.replace("\\", "/").split("/")[-2:])

def local_decls(srcs, names):
    """Declarations of core names written in a module source, outside the
    headers: -> [(file, line number, name)]."""
    word = re.compile(r"\b\w+\b")
    local = []
    for f in srcs:
        depth = 0
        for n, line in enumerate(open(f, errors="replace"), 1):
            code = re.sub(r"/\*.*?\*/|//.*", "", line)
            is_decl = re.match(r"\s*extern\b", code) or \
                      (depth == 0 and re.match(r"^[A-Za-z_][\w\s\*]*\b(\w+)\s*\([^;{]*\)\s*;\s*$", code))
            if is_decl:
                if re.match(r"\s*extern\b", code):
                    hit = [w for w in word.findall(code) if w in names and
                           not re.search(r"\b(struct|union|enum)\s+%s\b(?!\s*[;,=\[])" % w, code.replace("*", " "))]
                    # `extern struct ifnet *ifnet;` declares the variable too: catch a name that ends the declarator
                    hit += [w for w in names if re.search(r"\b%s\s*(\[[^\]]*\])?\s*[;,]" % re.escape(w), code)]
                else:
                    m = re.match(r"^[A-Za-z_][\w\s\*]*?\b(\w+)\s*\(", code)
                    hit = [m.group(1)] if m and m.group(1) in names else []
                for w in sorted(set(hit)):
                    local.append((f, n, w))
            depth += code.count("{") - code.count("}")
    return local

def lint(imports, aliases, args):
    names = set(imports)
    files = []
    for d in args.inc or []:
        for root, _, fs in os.walk(d):
            files += [os.path.join(root, f) for f in fs if f.endswith(".h")]
    srcs = expand(args.src or [])
    tag = re.compile(r"\b(struct|union|enum)\s+(\w+)\b")
    tags = {}
    for f in files + srcs:
        try:
            text = open(f, errors="replace").read()
        except OSError:
            continue
        for m in tag.finditer(text):
            if m.group(2) in names:
                tags.setdefault(m.group(2), set()).add(m.group(1))
    # `aliases': the symbols that already have a name that is not a tag
    clash = sorted(s for s in tags if s not in aliases)
    print("imports whose name is also a tag and that have no alias yet: %d" % len(clash))
    for s in clash:
        print("  %s (%s)" % (s, "/".join(sorted(tags[s]))))
    local = ["%s:%d: %s" % (short(f), n, w) for f, n, w in local_decls(srcs, names)]
    print("local declarations of imports in module sources (remove; the header declares them): %d" % len(local))
    for l in local:
        print("  " + l)
    return tags

def cmd_map(args):
    listed = read_def(args.deffile, full=True)
    with open(args.output, "w") as f:
        f.write("/*\n * Copyright (C) 2026 The AROS Dev Team\n *\n"
                " * This program is free software; you can redistribute it and/or modify\n"
                " * it under the terms of the GNU General Public License version 2 as\n"
                " * published by the Free Software Foundation.\n */\n\n")
        f.write("/*\n * GENERATED by tools/mkimports.py from net/%s - do not edit;\n"
                " * run tools/netcore-update.sh instead.\n *\n" % os.path.basename(args.deffile))
        f.write(" * Included LAST by every source file of a module library. In a build of\n"
                " * the stack program it is empty; in a module (-DNETMODULE) each name of\n"
                " * the core becomes the matching entry of the module's copy of the\n"
                " * services table, so `m_freem(m)' compiles to one load and one indirect\n"
                " * call, checked against the real prototype.\n */\n")
        f.write("#ifndef NET_NETCORE_MAP_H\n#define NET_NETCORE_MAP_H\n\n#ifdef NETMODULE\n#include <net/netcore.h>\n\n")
        for name, sym, wrap in listed:
            if wrap:        # the headers' macro of the same name, kept, now calling through the table
                f.write("#undef  %s\n#define %s%s (*__netcore.nc_%s)%s\n" % (name, name, wrap[0], name, wrap[1]))
            elif sym != name:   # an alias the headers define for a variable named like its structure
                f.write("#undef  %s\n#define %s (*__netcore.nc_%s)\n" % (name, name, name))
            else:
                f.write("#ifdef  %s\n#error \"%s is a macro in the headers: note its wrapper in netcore.def\"\n#endif\n"
                        "#define %s (*__netcore.nc_%s)\n" % (name, name, name, name))
        f.write("\n#endif /* NETMODULE */\n#endif /* !NET_NETCORE_MAP_H */\n")
    print("%s: %d names" % (args.output, len(listed)))
    return 0

def cmd_edits(args):
    """What has to change in each module source before it can be built as
    part of a module library, as file:line: lines (nothing is changed)."""
    listed = read_def(args.deffile)
    names = set(n for n, _ in listed) | set(sym for _, sym in listed)
    alias = [(sym, name) for name, sym in listed if name != sym]    # variable -> the name that is no tag
    total = 0
    for f in expand(args.src):
        try:
            lines = open(f, errors="replace").read().split("\n")
        except OSError:
            continue
        key = short(f)
        first_body = next((i for i, l in enumerate(lines) if l.startswith("{")), len(lines))
        incs = [i for i, l in enumerate(lines[:first_body]) if l.startswith("#include")]
        if not incs:
            print("%s: no #include before its first function: not a source file to prepare" % key)
            continue
        decl = {}
        for _, n, w in local_decls([f], names):
            decl.setdefault(n, []).append(w)
        out = []
        if not any("net/netcore_map.h" in lines[i] for i in incs):
            out.append((incs[-1] + 1, "ADD after it, as the last include:  #include <net/netcore_map.h>"))
        for i, l in enumerate(lines):
            if i + 1 in decl:
                where = "" if i > incs[-1] else "  (above the last include: harmless, may stay)"
                out.append((i + 1, "REMOVE the local declaration of %s:  %s%s" %
                            (", ".join(decl[i + 1]), l.strip(), where)))
                continue
            if re.match(r"\s*(/\*|\*|//)", l):
                continue
            n = l
            for a, b in alias:
                n = re.sub(r"(?<!struct )(?<!struct\t)(?<!union )(?<!enum )(?<![\w>.\"])%s(?![\w\"])" % re.escape(a), b, n)
            if n != l:
                out.append((i + 1, "REPLACE  %s  ->  %s" % (l.strip(), n.strip())))
        for n, text in sorted(out):
            print("%s:%d: %s" % (key, n, text))
        total += len(out)
    print("%d edits" % total)
    return 0

def cmd_verify(args):
    core_defs, mod_defs, mod_und = gather(args)
    bad = 0
    for o, und in sorted(mod_und.items()):
        left = sorted(s for s in und if s in core_defs and s not in mod_defs)
        if left:
            bad += 1
            print("%s still names the core directly: %s" % (os.path.basename(o), " ".join(left)))
    print("verify: %d of %d module objects name the core directly" % (bad, len(mod_und)))
    return 1 if bad else 0

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("derive", "verify"):
        p = sub.add_parser(name)
        p.add_argument("--nm", required=True)
        p.add_argument("--objdump")
        p.add_argument("--core", nargs="+", required=True)
        p.add_argument("--module", nargs="+", required=True)
        p.add_argument("--def", dest="deffile")
        p.add_argument("--prefix", default="", help="symbol prefix of the target, if it has one")
        p.add_argument("--known", help="file of symbols that come from link libraries")
        p.add_argument("--inc", nargs="*", help="header directories to scan for tags")
        p.add_argument("--src", nargs="*", help="module source files to scan")
        p.add_argument("--append", action="store_true")
        p.add_argument("--list", help="write the imports, one per line with kind and defining object")
    p = sub.add_parser("map")
    p.add_argument("--def", dest="deffile", required=True)
    p.add_argument("-o", dest="output", required=True)
    p = sub.add_parser("edits")
    p.add_argument("--def", dest="deffile", required=True)
    p.add_argument("--src", nargs="+", required=True)
    args = ap.parse_args()
    return {"derive": cmd_derive, "map": cmd_map, "verify": cmd_verify, "edits": cmd_edits}[args.cmd](args)

if __name__ == "__main__":
    sys.exit(main())
