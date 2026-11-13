#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>
# This file is distributed under the AROS Public License, version 1.1.
"""Exercise MetaMake argument lists with a built executable and GNU make."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('mmake', type=Path)
p.add_argument('--make', default='make')
p.add_argument('--output', type=Path)
a = p.parse_args()
mmake = a.mmake.resolve()
results = []
with tempfile.TemporaryDirectory(prefix='mmake-arguments-') as tmp:
    root = Path(tmp)
    (root / 'mmake.config').write_text(
        '[arguments]\nmaketool ' + a.make + '\n'
        'defaultmakefilename mmakefile\ndefaulttarget t000\n')
    targets = ['t%03d' % i for i in range(96)]
    (root / 'mmakefile').write_text('\n'.join(
        '#MM ' + t + ':\n.PHONY: ' + t + '\n' + t + ':\n'
        '\t@echo ' + t + ' >> result\n' for t in targets))
    env = dict(os.environ)
    env.pop('_MMAKE_TARGET', None)
    env.pop('_MMAKE_TARGETS', None)
    for name, options, selected in (
            ('targets', [], targets),
            ('make-flags', ['--no-print-directory'] * 96, targets[:1]),
            ('default-target', [], [])):
        result = root / 'result'
        result.unlink(missing_ok=True)
        run = subprocess.run(
            [str(mmake), '--srcdir=' + str(root), '--builddir=' + str(root),
             *options, *['arguments.' + t for t in selected]],
            cwd=root, env=env, capture_output=True, text=True, timeout=60)
        expected = selected or targets[:1]
        actual = result.read_text().splitlines() if result.exists() else []
        passed = run.returncode == 0 and actual == expected
        results.append({'case': name, 'returncode': run.returncode,
                        'requested': len(expected), 'executed': len(actual),
                        'result': 'PASS' if passed else 'FAIL',
                        'stdout': run.stdout, 'stderr': run.stderr})
if a.output:
    a.output.write_text(json.dumps(results, indent=2) + '\n')
for row in results:
    print(row['case'] + ': ' + row['result'])
raise SystemExit(0 if all(row['result'] == 'PASS' for row in results) else 1)
