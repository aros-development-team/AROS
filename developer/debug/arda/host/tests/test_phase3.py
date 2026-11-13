#!/usr/bin/env python3
"""Phase 3 tests: SYSINFO, MEMORY, TASKS over a raw socket.

    test_phase3.py --host H --port P [--wait SECS]
"""
import argparse
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_phase1 import Conn, check, results, wait_for_agent  # noqa: E402


def fields(resp):
    return dict(l.split("=", 1) for l in resp[1:-1] if "=" in l and " " not in l.split("=", 1)[0])


def records(resp, tag):
    out = []
    for line in resp[1:-1]:
        if not line.startswith(tag + " "):
            continue
        rec = {}
        for m in re.finditer(r'([A-Z_0-9]+)=("([^"]*)"|\S+)', line[len(tag) + 1:]):
            rec[m.group(1)] = m.group(3) if m.group(3) is not None else m.group(2)
        out.append(rec)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    ap.add_argument("--wait", type=int, default=0)
    a = ap.parse_args()
    if a.wait and not wait_for_agent(a.host, a.port, a.wait):
        return 1

    c = Conn(a.host, a.port)

    # SYSINFO
    r = c.command("SYSINFO")
    f = fields(r)
    check("SYSINFO is a block", r[0] == "OK" and r[-1] == "END", repr(r[:3]))
    check("SYSINFO AROS keys", all(k in f for k in ("AROS_VERSION", "AROS_ARCHITECTURE", "AROS_BUILD_DATE", "EXEC_VERSION")), repr(f))
    check("SYSINFO architecture looks right", "x86_64" in f.get("AROS_ARCHITECTURE", "") and "linux" in f.get("AROS_ARCHITECTURE", ""), f.get("AROS_ARCHITECTURE"))
    check("SYSINFO CPU keys", f.get("CPU_ARCH") == "x86" and f.get("CPU_ENDIAN") == "little" and int(f.get("CPU_COUNT", 0)) >= 1, repr({k: v for k, v in f.items() if k.startswith("CPU")}))
    check("SYSINFO time", re.match(r"20\d\d-\d\d-\d\d \d\d:\d\d:\d\d$", f.get("SYSTEM_TIME", "")), f.get("SYSTEM_TIME"))

    # MEMORY
    r = c.command("MEMORY")
    f = fields(r)
    check("MEMORY block", r[0] == "OK" and r[-1] == "END")
    tot, free, larg = int(f.get("ANY_TOTAL", 0)), int(f.get("ANY_FREE", 0)), int(f.get("ANY_LARGEST", 0))
    check("MEMORY totals consistent", tot >= free >= larg > 0, "total=%d free=%d largest=%d" % (tot, free, larg))
    check("MEMORY total ~ -m 1024", 900 * 2**20 <= tot <= 1100 * 2**20, str(tot))
    mh = records(r, "MEMHEADER")
    check("MEMORY headers listed", int(f.get("MEMHEADERS", 0)) >= 1 and len(mh) == int(f.get("MEMHEADERS", 0)), repr(mh))
    check("MEMHEADER fields", all(int(m["UPPER"], 16) > int(m["LOWER"], 16) and int(m["FREE"]) <= int(m["SIZE"]) for m in mh), repr(mh))

    # TASKS
    r = c.command("TASKS")
    f = fields(r)
    t = records(r, "TASK")
    check("TASKS block", r[0] == "OK" and r[-1] == "END")
    check("TASKS count matches", len(t) == int(f.get("COUNT", -1)) and int(f.get("TOTAL", 0)) >= len(t) > 5, "count=%s total=%s" % (f.get("COUNT"), f.get("TOTAL")))
    names = [x["NAME"] for x in t]
    check("TASKS lists the agent itself", any("ARDA" in n for n in names), repr(names))
    me = [x for x in t if "ARDA" in x["NAME"]]
    check("agent marked SELF, type cli, state run", me and me[0].get("SELF") == "1" and me[0]["TYPE"] == "cli" and me[0]["STATE"] == "run", repr(me))
    check("TASKS lists the stack", any(n == "AROSTCP" or "AROSTCP" in n for n in names), repr(names))
    states = set(x["STATE"] for x in t)
    check("TASK states valid", states <= {"run", "ready", "wait", "spin", "except", "added", "invalid", "removed", "tombstoned", "unknown"}, repr(states))
    waiting = [x for x in t if x["STATE"] == "wait"]
    check("waiting tasks report a PC", waiting and all(int(x["PC"], 16) != 0 for x in waiting), repr(waiting[:1]))
    nsym = sum(1 for x in waiting if "SYM" in x)
    print("INFO  %d/%d waiting tasks symbolised (hosted: PC is the host signal trampoline, so 0 is expected)" % (nsym, len(waiting)))
    check("stack usage sane", all(int(x["STACKUSED"]) <= int(x["STACK"]) for x in t))
    r2 = c.command("TASKS arostcp")
    t2 = records(r2, "TASK")
    check("TASKS name filter", t2 and all("AROSTCP" in x["NAME"].upper() for x in t2) and len(t2) < len(t), repr([x["NAME"] for x in t2]))
    check("TASKS empty filter result", records(c.command("TASKS zzzznomatch"), "TASK") == [])

    # stability: hammer the three commands
    ok = True
    for i in range(20):
        for cmd in ("SYSINFO", "MEMORY", "TASKS"):
            rr = c.command(cmd)
            if rr[0] != "OK" or rr[-1] != "END":
                ok = False
    check("20x SYSINFO/MEMORY/TASKS stable", ok)
    check("agent still answers PING", c.command("PING") == ["OK PONG"])
    c.command("QUIT")
    c.close()

    failed = [n for n, ok, _ in results if not ok]
    print("\n%d tests, %d failed" % (len(results), len(failed)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
