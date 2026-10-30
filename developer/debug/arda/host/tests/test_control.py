#!/usr/bin/env python3
"""Control-mode tests: drive the ArdaClient class against a live agent started with CONTROL.

    python3 test_control.py --host 10.0.0.2 [--port 7777]

Needs a writable RAM: on the agent side. Leaves nothing behind.
"""
import argparse
import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from aros_debug import ArdaClient, ArdaError  # noqa: E402

failures = 0


def check(name, cond, detail=""):
    global failures
    print("%s %s%s" % ("ok  " if cond else "FAIL", name, (" (%s)" % detail) if detail and not cond else ""))
    if not cond:
        failures += 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=os.environ.get("ARDA_HOST", "127.0.0.1"))
    ap.add_argument("--port", type=int, default=int(os.environ.get("ARDA_PORT", "7777")))
    a = ap.parse_args()

    with ArdaClient(a.host, a.port, timeout=60.0) as c:
        r = c.command("VERSION")
        check("VERSION reports control", r.ok and r.text.endswith("control"), r.text)
        r = c.command("STATUS")
        check("STATUS CONTROL=1", r.ok and "CONTROL=1" in r.lines)

        # a binary file with every byte value, larger than one recv chunk and not a multiple of it
        payload = bytes(range(256)) * 1300 + b"\x00\n\r\xff"
        tmp = tempfile.NamedTemporaryFile(delete=False)
        tmp.write(payload)
        tmp.close()
        r = c.put(tmp.name, "RAM:arda-test.bin")
        check("PUT binary", r.ok and r.text == "WROTE %d" % len(payload), r.text)

        r = c.command("DIR RAM:")
        names = [l for l in r.lines if 'NAME="arda-test.bin"' in l]
        check("DIR lists the file with its size", r.ok and names and "SIZE=%d" % len(payload) in names[0],
              names[0] if names else "missing")

        back = tmp.name + ".back"
        r = c.get("RAM:arda-test.bin", back)
        with open(back, "rb") as f:
            got = f.read()
        check("GET returns identical bytes", r.ok and got == payload, "%d bytes" % len(got))

        # the connection is still in line mode afterwards
        r = c.command("PING")
        check("line mode resumes after bodies", r.ok and r.text == "PONG", r.text)

        r = c.command("SETENV ARDATEST hello world")
        check("SETENV", r.ok, r.text)
        r = c.command("GETENV ARDATEST")
        check("GETENV round trip", r.ok and r.text == "hello world", r.text)
        r = c.command("SETENV ARDATEST")
        check("SETENV unset", r.ok and r.text == "UNSET", r.text)
        r = c.command("GETENV ARDATEST")
        check("GETENV after unset is 404", (not r.ok) and r.code == 404, r.text)

        r = c.command("RUN RAM:arda-job.out Echo \"job output\"")
        check("RUN starts a job", r.ok and r.text.startswith("JOB "), r.text)
        jobid = r.text.split()[1] if r.ok else "0"
        r = c.command("WAIT %s 30" % jobid)
        check("WAIT returns the rc", r.ok and r.text.startswith("RC "), r.text)
        r = c.get("RAM:arda-job.out", back)
        with open(back, "rb") as f:
            out = f.read()
        check("job output captured", r.ok and b"job output" in out, out[:40])
        r = c.command("JOBS")
        check("JOBS lists it as done", r.ok and any(("ID=%s " % jobid) in l and "STATE=done" in l for l in r.lines))

        r = c.command("RUN RAM:arda-job2.out Wait 2")
        jobid = r.text.split()[1] if r.ok else "0"
        r = c.command("JOB %s" % jobid)
        check("JOB shows running", r.ok and any("STATE=running" in l for l in r.lines))
        r = c.command("WAIT %s 1" % jobid)
        check("WAIT times out with 408", (not r.ok) and r.code == 408, r.text)
        time.sleep(2.5)
        r = c.command("WAIT %s 5" % jobid)
        check("WAIT after it finished", r.ok, r.text)

        for p in ("RAM:arda-test.bin", "RAM:arda-job.out", "RAM:arda-job2.out"):
            r = c.command("DELETE %s" % p)
            check("DELETE %s" % p, r.ok, r.text)
        r = c.command("MKDIR RAM:arda-dir")
        check("MKDIR", r.ok, r.text)
        r = c.command("DELETE RAM:arda-dir")
        check("DELETE directory", r.ok, r.text)
        r = c.command("GET RAM:arda-test.bin")
        check("GET of a missing file is 404", (not r.ok) and r.code == 404, r.text)
        c.quit()
    os.unlink(tmp.name)
    if os.path.exists(back):
        os.unlink(back)
    print("%d failure(s)" % failures)
    return 1 if failures else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ArdaError as e:
        print("test_control: %s" % e, file=sys.stderr)
        sys.exit(2)
