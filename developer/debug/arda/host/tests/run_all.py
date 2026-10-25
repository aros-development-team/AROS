#!/usr/bin/env python3
"""Run every ARDA test suite against a live agent.

    run_all.py --host H --port P [--wait SECS] [--tree SYSDIR]

The Phase 4 suite needs the hosted harness conventions (tracetest run before
the agent, SYS:T/arda.cmd polling) for its live checks; without --tree /
ARDA_TREE those checks are skipped.  test_idle.py is not included because it
needs an agent started with a short IDLE= value.
"""
import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SUITES = ["test_phase1.py", "test_client.py", "test_phase3.py", "test_phase4.py", "test_phase5.py"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=os.environ.get("ARDA_HOST", "127.0.0.1"))
    ap.add_argument("--port", type=int, default=int(os.environ.get("ARDA_PORT", "7777")))
    ap.add_argument("--wait", type=int, default=0)
    ap.add_argument("--tree", default=os.environ.get("ARDA_TREE"))
    a = ap.parse_args()
    rc = 0
    for s in SUITES:
        print("=== %s" % s, flush=True)
        args = [sys.executable, os.path.join(HERE, s), "--host", a.host, "--port", str(a.port)]
        if a.wait and s == SUITES[0]:
            args += ["--wait", str(a.wait)]
        if a.tree and s == "test_phase4.py":
            args += ["--tree", a.tree]
        if subprocess.call(args) != 0:
            rc = 1
    print("=== overall: %s" % ("PASS" if rc == 0 else "FAIL"))
    return rc


if __name__ == "__main__":
    sys.exit(main())
