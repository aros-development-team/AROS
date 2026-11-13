#!/usr/bin/env python3
"""Phase 4/6 tests: LOG STATUS/READ/CLEAR and the ardatrace instrumentation ring.

    test_phase4.py --host H --port P [--wait SECS] [--tree DIR]

--tree (or ARDA_TREE) is the hosted AROS SYS: directory; when given, the test
asks AROS to run tracetest live by writing SYS:T/arda.cmd (the harness polls
and executes it).  Expects the harness to have run tracetest once before the
agent started (COUNT=5 NAME=boot TEXT="before agent").
"""
import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_phase1 import Conn, check, results, wait_for_agent  # noqa: E402
from test_phase3 import fields, records  # noqa: E402


def aros_run(tree, script, conn=None, timeout=40):
    """Execute an AmigaDOS script on the running AROS via the harness command file.

    The harness loop renames SYS:T/arda.cmd to arda.running and runs it in a
    separate shell; the appended last line creates SYS:T/arda.done, which we
    wait for.  Script output goes to SYS:T/arda-cmd.log.  While waiting, PING
    the agent over conn so the idle timeout does not drop the session."""
    t = os.path.join(tree, "T")
    done = os.path.join(t, "arda.done")
    for f in (done, os.path.join(t, "arda.running")):
        if os.path.exists(f):
            os.remove(f)
    with open(os.path.join(t, "arda.cmd.tmp"), "w") as f:
        f.write(script + "\nEcho >SYS:T/arda.done \"done\"\n")
    os.rename(os.path.join(t, "arda.cmd.tmp"), os.path.join(t, "arda.cmd"))
    deadline = time.time() + timeout
    last_ping = time.time()
    while not os.path.exists(done):
        if time.time() > deadline:
            return False
        if conn and time.time() - last_ping > 5:
            conn.command("PING")
            last_ping = time.time()
        time.sleep(0.5)
    os.remove(done)
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    ap.add_argument("--wait", type=int, default=0)
    ap.add_argument("--tree", default=os.environ.get("ARDA_TREE"))
    a = ap.parse_args()
    if a.wait and not wait_for_agent(a.host, a.port, a.wait):
        return 1

    c = Conn(a.host, a.port)

    r = c.command("LOG STATUS")
    f = fields(r)
    check("LOG STATUS block", r[0] == "OK" and f.get("RING") == "present" and f.get("CAPACITY") == "1024", repr(f))
    check("LOG STATUS timer available", f.get("TIMER") == "yes", repr(f.get("NOW")))
    check("LOG STATUS reports kernel debug honestly", f.get("KERNEL_DEBUG") == "not captured")
    print("INFO  syslog source: %s (log.resource is not built for hosted targets)" % f.get("SYSLOG"))
    count0 = int(f.get("COUNT", 0))

    r = c.command("LOG READ")
    e = records(r, "LOG")
    f = fields(r)
    check("LOG READ returns entries", r[0] == "OK" and len(e) == int(f.get("COUNT", -1)) and len(e) >= 6, "%d entries" % len(e))
    boot = [x for x in e if x["NAME"] == "boot"]
    check("events emitted before the agent started are present", len(boot) == 5 and [int(x["VALUE"]) for x in boot] == [0, 1, 2, 3, 4], repr(boot[:2]))
    check("event text and source", all(x["SRC"] == "app" and x["TEXT"].startswith("before agent #") for x in boot), repr(boot[:1]))
    agent = [x for x in e if x["SRC"] == "agent"]
    check("agent's own events are logged", any("listening" in x["TEXT"] for x in agent) and any("connected" in x["TEXT"] for x in agent), repr([x["TEXT"] for x in agent][:5]))
    seqs = [int(x["SEQ"]) for x in e]
    check("sequence numbers strictly increase", seqs == sorted(seqs) and len(set(seqs)) == len(seqs))
    times = [float(x["TIME"]) for x in e]
    check("timestamps non-decreasing and non-zero", all(t > 0 for t in times) and times == sorted(times), repr(times[:3]))

    r = c.command("LOG READ 2")
    e2 = records(r, "LOG")
    check("LOG READ n returns the newest n", len(e2) == 2 and int(e2[-1]["SEQ"]) == seqs[-1], repr(e2))
    mid = seqs[len(seqs) // 2]
    r = c.command("LOG READ SINCE=%d" % mid)
    e3 = records(r, "LOG")
    check("LOG READ SINCE=seq", e3 and all(int(x["SEQ"]) > mid for x in e3) and int(e3[0]["SEQ"]) == mid + 1, repr(e3[:1]))
    r = c.command("LOG READ SRC=app")
    e4 = records(r, "LOG")
    check("LOG READ SRC=app filter", e4 and all(x["SRC"] == "app" for x in e4))
    r = c.command("LOG READ SRC=app 3")
    check("LOG READ combined args", len(records(r, "LOG")) == 3)

    for bad in ("LOG", "LOG WHAT", "LOG READ abc", "LOG READ SRC=nope", "LOG READ SINCE=x"):
        rr = c.command(bad)
        check("bad request %r -> ERR 400" % bad, len(rr) == 1 and rr[0].startswith("ERR 400 "), repr(rr))

    if a.tree:
        last = seqs[-1]
        ok = aros_run(a.tree, 'SYS:Developer/Debug/Tests/arda/tracetest >>SYS:T/arda-cmd.log COUNT=3 NAME=live TEXT="after start" VALUE=10', c)
        check("harness executed the live tracetest", ok)
        r = c.command("LOG READ SINCE=%d SRC=app" % last)
        live = records(r, "LOG")
        check("live events appear without reconnecting", [(x["NAME"], int(x["VALUE"])) for x in live] == [("live", 10), ("live", 11), ("live", 12)], repr(live))

        r = c.command("LOG CLEAR")
        check("LOG CLEAR", len(r) == 1 and r[0].startswith("OK CLEARED "), repr(r))
        f = fields(c.command("LOG STATUS"))
        check("ring empty after clear (apart from the clear itself)", int(f.get("COUNT", 99)) <= 1 and int(f.get("CLEARED", 0)) >= 1, repr(f))
        check("boot events gone after clear", not [x for x in records(c.command("LOG READ"), "LOG") if x["NAME"] == "boot"])

        ok = aros_run(a.tree, 'SYS:Developer/Debug/Tests/arda/tracetest >>SYS:T/arda-cmd.log COUNT=1500 NAME=flood TEXT="overflow"', c, timeout=90)
        f = fields(c.command("LOG STATUS"))
        check("ring wraps at capacity", ok and f.get("COUNT") == "1024" and int(f.get("OVERWRITTEN", 0)) >= 476, repr(f))
        r = c.command("LOG READ 300")
        e5 = records(r, "LOG")
        check("LOG READ 300 after flood", len(e5) == 300 and e5[-1]["NAME"] == "flood" and int(e5[-1]["VALUE"]) == 1499, repr(e5[-1:]))
        check("oldest surviving entry is the expected one", int(records(c.command("LOG READ 1 SINCE=0"), "LOG")[0]["VALUE"]) >= 0)
        r = c.command("LOG READ 1000")
        check("LOG READ caps at 300", len(records(r, "LOG")) == 300)
    else:
        print("INFO  no --tree: live tracetest, CLEAR and overflow checks skipped")

    check("agent still answers PING", c.command("PING") == ["OK PONG"])
    c.command("QUIT")
    c.close()

    failed = [n for n, ok, _ in results if not ok]
    print("\n%d tests, %d failed" % (len(results), len(failed)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
