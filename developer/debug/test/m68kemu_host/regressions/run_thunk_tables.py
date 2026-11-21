#!/usr/bin/env python3
"""Check the thunk tables of m68kemu.library against the library interfaces."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
#
# A 68k program calls a library function by its offset (LVO). The thunk
# tables map each offset to the function that converts the call. An entry at
# the wrong offset is never reached, or converts the wrong function; a thunk
# that reads a register the function does not take passes a stale value.
#
# Every table entry must have an offset that is a positive multiple of 6.
# The entries of the dos.library table written by hand must also name, at
# their offset, the function rom/dos/dos.conf has there, and their thunks may
# read only the registers that function takes (A6 and A7 aside). dos.conf
# follows the AmigaOS dos.library layout, the one 68k programs are built for.
#
# No compiler, SDK or CPU core is needed. The register check reads the body
# of each thunk only, not the helpers it calls.
import argparse
from pathlib import Path
import re
import sys


def masked(source):
    """The source with comments and string contents blanked, lines kept."""
    return re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]),
        source)


def tables(sources):
    """{table name: (file, [(lvo, function, line)])} of every M68KThunkEntry table."""
    found = {}
    for path, text in sources.items():
        for m in re.finditer(r'(?:static\s+)?const\s+struct\s+M68KThunkEntry\s+(\w+)\s*\[\s*\]\s*=\s*\{', text):
            end = text.index('};', m.end())
            entries = []
            for e in re.finditer(r'\{\s*(-?\d+)\s*,\s*(\w+)\s*\}', text[m.end():end]):
                if e[2] == 'NULL':
                    continue
                line = text[:m.end() + e.start()].count('\n') + 1
                entries.append((int(e[1]), e[2], line))
            found[m[1]] = (path, entries)
    return found


def function_body(sources, name):
    for text in sources.values():
        m = re.search(r'(?m)^(?=\S)[^\n;{}]*\b' + re.escape(name) + r'\s*\([^;{}]*\)[^;{}]*\{', text)
        if not m:
            continue
        end = text.index('{', m.start()) + 1
        start, depth = end, 1
        while depth:
            depth += (text[end] == '{') - (text[end] == '}')
            end += 1
        return text[start:end - 1]
    return None


def registers_read(body):
    regs = set()
    for kind, n in re.findall(r'\bTHUNK_(D|A|PTR|DPTR)\s*\(\s*(\d)\s*\)', body):
        regs.add(('A' if kind in ('A', 'PTR') else 'D') + n)
    for kind, n in re.findall(r'\bM68KEmu_Get(D|A)\s*\(\s*cpu\s*,\s*(\d)\s*\)', body):
        regs.add(kind + n)
    return regs


def conf_functions(text):
    """{lvo: (name, [registers])} of a library .conf function list."""
    body = re.search(r'##begin functionlist(.*?)##end functionlist', text, re.S)[1]
    lines = [re.sub(r'\)\s*#.*$', ')', l.strip()) for l in body.split('\n')]
    lines = [l for l in lines if l and not l.startswith('#')]
    protos = [l for l in lines if not l.startswith('.')]
    # dos.conf and exec.conf list the open and close vectors themselves;
    # other libraries start after the four standard vectors
    slot = 0 if re.search(r'\b(open|OpenLib)\s*\(', protos[0]) else 4
    funcs = {}
    for l in lines:
        if l.startswith('.skip'):
            slot += int(l.split()[1])
            continue
        if l.startswith('.'):
            continue
        m = re.match(r'.*?\b(\w+)\s*\((.*)\)\s*\(([^)]*)\)$', l)
        if not m:
            raise ValueError('cannot read the .conf line %r' % l)
        slot += 1
        funcs[slot * 6] = (m[1], [r.strip().upper() for r in m[3].split(',') if r.strip()])
    return funcs


def check(sources, dos_conf):
    problems = []
    found = tables(sources)
    for name, (path, entries) in sorted(found.items()):
        for lvo, func, line in entries:
            if lvo <= 0 or lvo % 6:
                problems.append('%s:%d: %s: %s at offset %d, which is not a positive multiple of 6'
                                % (path, line, name, func, lvo))
    if 'm68kemu_thunks_dos' not in found:
        problems.append('the dos.library table m68kemu_thunks_dos was not found')
        return problems
    funcs = conf_functions(dos_conf)
    path, entries = found['m68kemu_thunks_dos']
    for lvo, func, line in entries:
        if lvo not in funcs:
            if lvo > 0 and lvo % 6 == 0:
                problems.append('%s:%d: %s at offset %d: dos.conf has no function there' % (path, line, func, lvo))
            continue
        conf_name, conf_regs = funcs[lvo]
        short = re.sub(r'^(thunk|m68k)_dos_', '', func)
        short = re.sub(r'_m$', '', short)
        if short != conf_name:
            problems.append('%s:%d: %s at offset %d: dos.conf has %s there' % (path, line, func, lvo, conf_name))
            continue
        body = function_body(sources, func)
        if body is None:
            problems.append('%s:%d: the body of %s was not found' % (path, line, func))
            continue
        extra = sorted(registers_read(body) - set(conf_regs) - {'A6', 'A7'})
        if extra:
            problems.append('%s:%d: %s reads %s, but %s takes %s' % (
                path, line, func, ', '.join(extra), conf_name, ', '.join(conf_regs) or 'no arguments'))
    return problems


SELF_TEST_CONF = """##begin functionlist
struct DosLibrary *OpenLib(ULONG version) (D0)
.private
BPTR CloseLib() ()
.private
.skip 2
BPTR Open(CONST_STRPTR name, LONG accessMode) (D1,D2)
.skip 35
void SendPkt(struct DosPacket *dp, struct MsgPort *port, struct MsgPort *replyport) (D1,D2,D3)
##end functionlist
"""

SELF_TEST_SOURCE = """
static IPTR thunk_dos_Open(struct M68KEmuContext *ctx, void *cpu) { return THUNK_D(1) + THUNK_D(2); }
static IPTR thunk_dos_SendPkt(struct M68KEmuContext *ctx, void *cpu) { return THUNK_PTR(1); }
const struct M68KThunkEntry m68kemu_thunks_dos[] = {
    { 30, thunk_dos_Open },
    { 244, thunk_dos_Open },
    { 252, thunk_dos_Open },
    { 246, thunk_dos_SendPkt },
    { 0, NULL }
};
"""


def self_test():
    """Check the checker on a small table with one fault of each kind."""
    # It must find an offset that is no multiple of 6, a function at another
    # function's offset and a register the function does not take.
    problems = check({'self-test.c': masked(SELF_TEST_SOURCE)}, SELF_TEST_CONF)
    want = ('offset 244, which is not a positive multiple of 6',
            'thunk_dos_Open at offset 252: dos.conf has no function there',
            'thunk_dos_SendPkt reads A1, but SendPkt takes D1, D2, D3')
    missing = [w for w in want if not any(w in p for p in problems)]
    if missing or len(problems) != len(want):
        print('self-test failed: expected %r, got %r' % (want, problems))
        return False
    print('self-test: the checker finds the three kinds of fault')
    return True


def main():
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=here.parents[4],
                        help='AROS source tree (default: the one holding this script)')
    args = parser.parse_args()
    root = args.source_root
    if not self_test():
        return 1
    sources = {str(p.relative_to(root)): masked(p.read_text(encoding='latin1'))
               for p in sorted((root / 'rom/m68kemu').glob('*.c'))}
    dos_conf = (root / 'rom/dos/dos.conf').read_text(encoding='latin1')
    problems = check(sources, dos_conf)
    count = sum(len(e) for _, e in tables(sources).values())
    for p in problems:
        print(p)
    print('%d thunk table entries in %d tables checked, %d problems'
          % (count, len(tables(sources)), len(problems)))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
