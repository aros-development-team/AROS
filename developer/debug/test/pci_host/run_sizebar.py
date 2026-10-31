#!/usr/bin/env python3
"""Compile the real sizePCIBaseReg() verbatim at -m64 and -m32 and run its behaviour table.

The fixture does not copy this function's implementation and the table does
not repeat it: the definition is extracted from the selected source root
between its signature line and the matching closing brace.  Both widths
then print their own size table; the runner compares the two tables, so a
32-bit answer may differ from the 64-bit one only where that size does not
fit in 32 bits.
"""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.

import argparse
import os
import re
from pathlib import Path
import shlex
import subprocess  # nosec B404 - this developer test invokes the compiler it was given
import sys

TEST_DIR = Path(__file__).resolve().parent


def extract_function(path, name):
    """Return the verbatim text of function `name` in `path`, #line-tagged.

    Raises when the definition is not found exactly once.  Comments and
    string literals are blanked (newlines kept) before searching, so a
    name mention inside a comment does not count.
    """
    source = path.read_text()
    masked = re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda match: ''.join('\n' if c == '\n' else ' ' for c in match[0]),
        source,
    )
    matches = list(re.finditer(
        r'(?m)^[^\n;{}]*\b' + re.escape(name) +
        r'\s*\([^;{}]*\)[^;{}]*\{', masked))
    if not matches:
        raise ValueError(f'{path}: definition of {name} not found')
    if len(matches) > 1:
        raise ValueError(f'{path}: found {len(matches)} definitions of {name}')
    m = matches[0]
    end = m.end()
    depth = 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    line = source[:m.start()].count('\n') + 1
    return f'#line {line} "{path.name}"\n' + source[m.start():end] + '\n'


def compose_tu(source_root):
    pciutil = source_root / 'rom/hidds/pci/pciutil.c'
    hw = source_root / 'rom/hidds/pci/include/hardware/pci.h'
    for path in (pciutil, hw):
        if not path.is_file():
            raise SystemExit(f'source root {source_root} lacks {path.relative_to(source_root)}')
    test = TEST_DIR / 'test_sizebar.c'
    fixture = TEST_DIR / 'fixture.h'
    fixture_text = fixture.read_text()
    return (
        source_root,
        f'#include "{hw}"\n' + fixture_text + '\n'
        + extract_function(pciutil, 'sizePCIBaseReg') + '\n'
        + f'#line 1 "{test}"\n' + test.read_text() + '\n'
    )


def parse_rows(lines):
    rows = []
    for text in lines:
        m = re.fullmatch(r'([0-9a-f]{8}) ([0-9a-f]{8}) -> ([0-9a-f]{16})', text)
        if not m:
            raise SystemExit(f'unparsable table row: {text!r}')
        rows.append(m.groups())
    return rows


def compare_tables(rows64, rows32):
    """32-bit sizes may differ from 64-bit sizes only when they do not fit 32 bits.

    The corpus can produce the same input more than once; each distinct
    input is compared once.
    """
    if len(rows64) != len(rows32):
        raise SystemExit(f'corpus sizes differ: -m64 {len(rows64)} rows, -m32 {len(rows32)}')
    mismatches = []
    seen = set()
    for (k64, k64b, r64), (k32, k32b, r32) in zip(rows64, rows32):
        if (k64, k64b) != (k32, k32b):
            mismatches.append((k64, k64b, r64, r32, 'inputs differ'))
            continue
        if (k64, k64b) in seen:
            continue
        seen.add((k64, k64b))
        a, b = int(r64, 16), int(r32, 16)
        want = a if a < 2**32 else 0
        if b != want:
            mismatches.append((k64, k64b, r64, r32, f'width rule: -m32 want {want:016x}'))
    return mismatches, len(seen)


def build_and_run(cc, width, build_dir, tu_text):
    tu = build_dir / f'sizebar{width}.c'
    exe = build_dir / f'sizebar{width}'
    tu.write_text(tu_text)
    cmd = shlex.split(cc) + [width, '-O2', str(tu), '-o', str(exe)]
    done = subprocess.run(cmd, text=True, capture_output=True)
    if done.returncode != 0:
        return 'FAIL', 0, [], 0, 'build log tail: ' + (done.stderr or done.stdout)[-400:], []
    table = subprocess.run([str(exe)], text=True, capture_output=True)
    corpus = subprocess.run([str(exe), '--corpus'], text=True, capture_output=True)
    fails = [line for line in table.stdout.splitlines() if line.startswith('FAIL')]
    rows = parse_rows(corpus.stdout.splitlines())
    ok = table.returncode == 0 and corpus.returncode == 0
    return ('PASS' if ok else 'FAIL'), len(fails), fails, len(rows), '', rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path,
                        default=TEST_DIR.parents[3])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--allow-skip-m32', action='store_true',
                        help='accept a SKIPPED -m32 width (host without multilib gcc)')
    args = parser.parse_args()
    source_root = args.source_root.resolve()

    scratch = args.build_dir.resolve() if args.build_dir else None
    if scratch:
        scratch.mkdir(parents=True, exist_ok=True)

    def run_widths(build_dir):
        _, tu_text = compose_tu(source_root)
        results = {}
        empty = build_dir / 'multilib_probe.c'
        empty.write_text('int main(void) { return 0; }\n')
        for width in ('-m64', '-m32'):
            # Link, not just compile: a host can have the 32-bit headers
            # without the 32-bit C library.
            probe = subprocess.run(shlex.split(args.cc) +
                                   [width, str(empty), '-o',
                                    str(build_dir / 'probe')],
                                   text=True, capture_output=True)
            if probe.returncode != 0 and width == '-m32':
                results[width] = ('SKIPPED', 0, [], 0, 'probe log tail: ' +
                                  (probe.stderr or probe.stdout)[-400:], [])
                continue
            results[width] = build_and_run(args.cc, width, build_dir, tu_text)
        return results

    def verdict(results):
        for width in ('-m64', '-m32'):
            status, n_fails, fails, n_rows, note, _ = results[width]
            print(f'{width}: {status} fails={n_fails} rows={n_rows}'
                  + (f' note: {note}' if note else ''))
            for line in fails:
                print(f'  {width} {line}')
        r64 = results['-m64'][5]
        r32 = results['-m32'][5]
        if r64 and r32:
            bad, n_inputs = compare_tables(r64, r32)
            print(f'differential: mismatches={len(bad)} / inputs={n_inputs}')
            for k, k2, a, b, why in bad[:8]:
                print(f'  {k} {k2} -> 64:{a} 32:{b}  ({why})')
            return bad
        print('differential: not run (a width did not complete)')
        return None

    if args.build_dir:
        results = run_widths(scratch)
        bad = verdict(results)
    else:
        import tempfile
        with tempfile.TemporaryDirectory(prefix='pci-sizebar-') as path:
            results = run_widths(Path(path))
            bad = verdict(results)
    ok = all(r[0] == 'PASS' for r in results.values()) and bad == []
    if results['-m32'][0] == 'SKIPPED' and all(
            r[0] == 'PASS' for w, r in results.items() if w != '-m32'):
        if args.allow_skip_m32:
            print('SIZEBAR RESULT: PASS with -m32 SKIPPED (--allow-skip-m32)')
            sys.exit(0)
        else:
            print('SIZEBAR RESULT: FAIL (-m32 SKIPPED; this host has no multilib — '
                  'that is not a pass, pass --allow-skip-m32 only when intended)')
            sys.exit(1)
    print(f'SIZEBAR RESULT: {"PASS" if ok else "FAIL"}')
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
