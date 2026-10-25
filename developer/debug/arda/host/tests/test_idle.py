#!/usr/bin/env python3
"""Idle timeout and loopback-default checks.

    test_idle.py --host H --port P [--idle SECS] [--loopback-port P2]

Expects the agent on --port started with IDLE=<--idle> (default 5).  When
--loopback-port is given, a second agent is expected to run on AROS with that
port and no ADDRESS= (loopback default); connecting from the network must fail.
"""
import argparse
import os
import socket
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_phase1 import Conn, check, results, wait_for_agent  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    ap.add_argument("--idle", type=int, default=5)
    ap.add_argument("--loopback-port", type=int, default=0)
    ap.add_argument("--wait", type=int, default=0)
    a = ap.parse_args()
    if a.wait and not wait_for_agent(a.host, a.port, a.wait):
        return 1

    c = Conn(a.host, a.port, timeout=a.idle + 10)
    check("PING before idling", c.command("PING") == ["OK PONG"])
    t0 = time.time()
    r = c.response()                       # nothing sent: wait for the agent's verdict
    dt = time.time() - t0
    check("idle timeout -> ERR 408", r == ["ERR 408 idle timeout, closing"], repr(r))
    check("idle timeout after about IDLE seconds", a.idle - 1.5 <= dt <= a.idle + 3, "%.1fs" % dt)
    check("connection closed after idle timeout", c.closed_by_peer())
    c.close()
    c = Conn(a.host, a.port)
    check("agent serves again after the timeout", c.command("PING") == ["OK PONG"])
    c.command("QUIT")
    c.close()

    if a.loopback_port:
        try:
            s = socket.create_connection((a.host, a.loopback_port), timeout=5)
            s.close()
            check("default-bound agent not reachable from the network", False, "connection succeeded")
        except OSError as e:
            check("default-bound agent not reachable from the network", True, str(e))

    failed = [n for n, ok, _ in results if not ok]
    print("\n%d tests, %d failed" % (len(results), len(failed)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
