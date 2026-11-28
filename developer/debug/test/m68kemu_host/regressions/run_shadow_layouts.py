#!/usr/bin/env python3
"""Check that the generated shadow layouts take their native side from the build."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# No native SDK, NDK, CPU backend or application binary is needed.
import argparse
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess  # nosec B404 - this developer test runs a selected compiler
import sys
import tempfile

HERE = Path(__file__).resolve().parent
SAMPLE = 'shadow_layouts_sample'
# A lock of 16 bytes and one of 128, and a value of 4 bytes and one of 8.
LOCKS = (16, 128)
VALUES = (('unsigned int', 4), ('unsigned long long', 8))
# The sample fields behind the lock: they move with its size.
BEHIND = ('sb_Count', 'sb_Flags', 'sb_Data', 'sb_Value')


def run(command, **options):
    return subprocess.run(command, check=True, **options)  # nosec B603 - explicit argument list


def generate(source, offsets):
    script = source / 'rom/m68kemu/gen_shadow_layouts.py'
    return run([sys.executable, str(script), str(offsets)], capture_output=True).stdout


def offsets_of(path):
    """The mapped fields of an offsets file: (struct, member) -> (m68k offset, m68k size)."""
    fields = {}
    for line in path.read_text().splitlines():
        parts = line.rstrip('|').split('|')
        if parts[0] == 'OFF' and len(parts) == 6 and int(parts[4]) in (1, 2, 4):
            fields[(parts[1], parts[5])] = (int(parts[3]), int(parts[4]))
    return fields


def check_checked_in(source):
    """The checked-in header is what the generator writes, and holds no native number."""
    module = source / 'rom/m68kemu'
    header = (module / 'm68kemu_shadow_layouts.h').read_bytes()
    try:
        made = generate(source, module / 'm68k_offsets.txt')
    except subprocess.CalledProcessError as error:
        raise SystemExit('gen_shadow_layouts.py does not write the header from m68k_offsets.txt alone:\n'
                         + error.stderr.decode()) from error
    if made != header:
        ours, theirs = made.decode().splitlines(), header.decode().splitlines()
        at = next((i for i, (a, b) in enumerate(zip(ours, theirs)) if a != b), min(len(ours), len(theirs)))
        raise SystemExit('m68kemu_shadow_layouts.h is not the output of gen_shadow_layouts.py, '
                         'from line %d on' % (at + 1))
    rows = fields = 0
    table = None
    for line in header.decode().splitlines():
        start = re.match(r'static const struct M68KFieldMap shadow_fields_(\w+)\[\] = \{$', line)
        if start:
            table = start[1]
        elif table and line == '};':
            table = None
        elif table:
            rows += 1
            field = re.match(r'    SHADOW_FIELD\( *\d+, [124], (\w+), [\w.]+\),$', line)
            if field:
                assert field[1] == table, line
                fields += 1
            else:
                assert line == '    {    0,    0, SF_END  , 0, 0, NULL }', \
                    'a row of %s is neither SHADOW_FIELD() nor the end entry: %s' % (table, line)
    mapped = len(offsets_of(module / 'm68k_offsets.txt'))
    assert fields == mapped, (fields, mapped)
    # tc_UserData belongs to the task: the m68k one is the program's and the
    # native one is not for it. Its lines name no member, so no sync writes it.
    own = [line.rstrip('|').split('|') for line in (module / 'm68k_offsets.txt').read_text().splitlines()
           if line.startswith('OFF|') and line.split('|')[2] == 'tc_UserData']
    assert sorted(parts[1] for parts in own) == ['Process', 'Task'], own
    assert all(parts[3:] == ['88', '4'] for parts in own), 'a tc_UserData line names a member: %s' % own
    assert 'tc_UserData' not in header.decode(), 'm68kemu_shadow_layouts.h has a row for tc_UserData'
    print('tc_UserData: its %d lines name no member and the header has no row for it' % len(own))
    print('checked-in header: %d fields, each a SHADOW_FIELD() row; it is the generator\'s output' % fields)


def check_extractor(source, output):
    """The extractor names the member it probes and the header of the struct."""
    ndk = output / 'ndk'
    (ndk / 'exec').mkdir(parents=True)
    shutil.copy(HERE / (SAMPLE + '.h'), ndk / 'exec')
    script = source / 'rom/m68kemu/gen_offset_extractor.py'
    text = run([sys.executable, str(script)], capture_output=True, text=True,
               env=dict(os.environ, NDK=str(ndk))).stdout
    probes = dict(re.findall(r'"\.ascii \\"OFF\|(\w+\|\w+)\|%0\|%1\|([\w.]*)\|\\""', text))
    sizes = dict(re.findall(r'"\.ascii \\"SIZ\|(\w+)\|%0\|([\w./]*)\|\\""', text))
    wanted = {name + '|' + member.rsplit('.', 1)[-1]: member
              for (name, member) in offsets_of(HERE / (SAMPLE + '.txt'))}
    for field, member in wanted.items():
        assert probes.get(field) == member, (field, member, probes.get(field))
    for name in ('SampleBase', 'SampleLibrary', 'SampleNode'):
        assert sizes.get(name) == 'exec/' + SAMPLE + '.h', (name, sizes.get(name))
    print('extractor: %d probes name their member, 3 structs their header' % len(wanted))


def build_and_run(args, output, lock, value):
    binary = output / ('test_shadow_layouts_%d_%d' % (lock, value[1]))
    command = shlex.split(args.cc) + [
        '-std=c99', '-Wall', '-Wextra', '-Werror', '-O1', '-g',
        '-I' + str(output), '-I' + str(HERE),
        '-DSAMPLE_LOCK_BYTES=%d' % lock, '-DSAMPLE_VALUE_TYPE=' + value[0],
        str(HERE / 'test_shadow_layouts.c'), '-o', str(binary)]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    run(command)
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    done = subprocess.run([str(binary)], env=env, capture_output=True, text=True)  # nosec B603
    text = done.stdout
    if done.returncode:
        raise SystemExit('lock %d, value %d:\n%s%s' % (
            lock, value[1], ''.join(line + '\n' for line in text.splitlines() if not line.startswith('FIELD ')),
            done.stderr))
    build = re.search(r'^BUILD pointer (\d+) value (\d+) lock (\d+)$', text, re.M)
    assert build and int(build[2]) == value[1] and int(build[3]) == lock, text
    fields = {}
    for name, member, m68k_off, native_off, kind in re.findall(
            r'^FIELD (\w+) ([\w.]+) (\d+) (\d+) (SF_\w+)$', text, re.M):
        fields[(name, member)] = (int(m68k_off), int(native_off), kind)
    print(text.splitlines()[-1], '(lock %d, value %d)' % (lock, value[1]))
    return int(build[1]), fields


def check_sample(args, source, output):
    """One generated header, four layouts of the same structures."""
    shadow = (source / 'rom/m68kemu/m68kemu_shadow.h').read_text()
    types = [re.search(pattern, shadow) for pattern in (
        r'enum\s*\{[^}]*\bSF_END\b[^}]*\};',
        r'struct M68KFieldMap \{[^}]*\};',
        r'struct M68KStructLayout \{[^}]*\};')]
    assert all(types), 'm68kemu_shadow.h: the field types or the two structs were not found'
    (output / 'shadow_types.inc').write_text(
        'struct M68KStructLayout;\n' + '\n'.join(found[0] for found in types) + '\n')
    (output / 'sample_layouts.h').write_bytes(generate(source, HERE / (SAMPLE + '.txt')))

    wanted = offsets_of(HERE / (SAMPLE + '.txt'))
    builds = {}
    for lock in LOCKS:
        for value in VALUES:
            pointer, fields = build_and_run(args, output, lock, value)
            assert set(fields) == set(wanted), sorted(set(fields) ^ set(wanted))
            for key, (m68k_off, m68k_size) in wanted.items():
                assert fields[key][0] == m68k_off, (key, fields[key])
                if m68k_size != 4:
                    assert fields[key][2] == {1: 'SF_BYTE', 2: 'SF_WORD'}[m68k_size], (key, fields[key])
            builds[(lock, value[1])] = fields
            # a 4-byte field is a pointer exactly where the native member has 8 bytes
            wide = 'SF_PTR' if value[1] == 8 else 'SF_LONG'
            assert fields[('SampleBase', 'sb_Value')][2] == wide, fields[('SampleBase', 'sb_Value')]
            assert fields[('SampleBase', 'sb_Flags')][2] == 'SF_LONG'
            assert fields[('SampleBase', 'sb_Data')][2] == ('SF_PTR' if pointer == 8 else 'SF_LONG')
    # The same header: what lies behind the lock moves by the size of the lock,
    # and nothing else does.
    step = LOCKS[1] - LOCKS[0]
    for value in VALUES:
        small, large = builds[(LOCKS[0], value[1])], builds[(LOCKS[1], value[1])]
        for key in wanted:
            moved = large[key][1] - small[key][1]
            behind = key[0] == 'SampleBase' and key[1] in BEHIND
            assert moved == (step if behind else 0), (key, small[key], large[key])
    print('sample: %d fields in 4 builds; the %d behind the lock move by %d bytes with it'
          % (len(wanted), len(BEHIND), step))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()

    def checks(output):
        assert not output.exists(), 'Use a fresh build directory'
        output.mkdir(parents=True)
        check_checked_in(args.source_root)
        check_extractor(args.source_root, output)
        check_sample(args, args.source_root, output)

    if args.build_dir:
        checks(args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-shadow-layouts-') as path:
            checks(Path(path) / 'build')


if __name__ == '__main__':
    main()
