#!/usr/bin/env python3
"""Phase 2 tests: drive aros_debug.py as a subprocess against a live agent.

    test_client.py --host H --port P [--wait SECS]
"""
import argparse
import json
import os
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
CLIENT = os.path.join(HERE, "..", "aros_debug.py")
results = []


def check(name, cond, detail=""):
    results.append((name, bool(cond)))
    print("%s  %s%s" % ("PASS" if cond else "FAIL", name, (" - " + detail) if detail else ""))


def run(args, env=None, timeout=30):
    e = dict(os.environ)
    e.pop("ARDA_HOST", None)
    e.pop("ARDA_PORT", None)
    if env:
        e.update(env)
    p = subprocess.run([sys.executable, CLIENT] + args, capture_output=True, text=True, timeout=timeout, env=e)
    return p.returncode, p.stdout, p.stderr


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    ap.add_argument("--wait", type=int, default=0)
    a = ap.parse_args()
    base = ["--host", a.host, "--port", str(a.port)]

    deadline = time.time() + a.wait
    while True:
        rc, out, err = run(base + ["ping"])
        if rc == 0 or time.time() >= deadline:
            break
        time.sleep(1)

    check("ping exit 0 + PONG", rc == 0 and out.strip() == "PONG", repr((rc, out, err)))
    rc, out, err = run(base + ["version"])
    check("version", rc == 0 and out.startswith("ARDA/1 "), repr(out))
    rc, out, err = run(base + ["HELP"])
    check("help multi-line", rc == 0 and "PING" in out and "END" not in out.split("\n"), repr(out))
    rc, out, err = run(base + ["--json", "status"])
    try:
        d = json.loads(out)
    except ValueError:
        d = {}
    check("json status", rc == 0 and d.get("ok") is True and d.get("fields", {}).get("PROTOCOL") == "1", out[:200])
    check("json carries command/host/elapsed", d.get("command") == "STATUS" and d.get("host") == a.host and "elapsed_ms" in d)

    rc, out, err = run(base + ["nosuch"])
    check("unknown command -> exit 1, message on stderr", rc == 1 and out == "" and "ERR 404" in err, repr((rc, out, err)))
    rc, out, err = run(base + ["--json", "nosuch"])
    d = json.loads(out) if out else {}
    check("unknown command json", rc == 1 and d.get("ok") is False and d.get("error", {}).get("code") == 404, out[:200])

    rc, out, err = run(base + ["raw", "ping"])
    check("raw sends verbatim (lower case still accepted)", rc == 0 and out.strip() == "PONG", repr(out))

    with tempfile.TemporaryDirectory() as td:
        f = os.path.join(td, "out.txt")
        rc, out, err = run(base + ["--output", f, "version"])
        saved = open(f).read() if os.path.exists(f) else None
        check("--output writes file and prints", rc == 0 and saved and saved.strip() == out.strip(), repr(saved))
        f2 = os.path.join(td, "out.json")
        rc, out, err = run(base + ["--output", f2, "--quiet", "--json", "ping"])
        saved = json.loads(open(f2).read()) if os.path.exists(f2) else None
        check("--output --quiet json", rc == 0 and out == "" and saved and saved.get("text") == "PONG")

    # environment defaults
    rc, out, err = run(["ping"], env={"ARDA_HOST": a.host, "ARDA_PORT": str(a.port)})
    check("ARDA_HOST/ARDA_PORT defaults", rc == 0 and out.strip() == "PONG", repr((rc, err)))

    # connection failures: closed port, unreachable host, short timeouts
    s = socket.socket(); s.bind((a.host if a.host == "127.0.0.1" else "0.0.0.0", 0)); free_port = s.getsockname()[1]; s.close()
    rc, out, err = run(["--host", a.host, "--port", str(free_port), "--connect-timeout", "3", "ping"])
    check("closed port -> exit 2", rc == 2 and "cannot connect" in err, repr((rc, err)))
    rc, out, err = run(["--host", "10.255.255.1", "--port", "7777", "--connect-timeout", "1", "--json", "ping"], timeout=20)
    d = json.loads(out) if out else {}
    check("unreachable host json -> exit 2", rc == 2 and d.get("status") == "CONNECTION", repr((rc, out[:120], err[:120])))

    # response timeout: hold the single client slot open so the next client gets 503 promptly
    holder = socket.create_connection((a.host, a.port), timeout=5)
    rc, out, err = run(base + ["ping"])
    check("busy agent -> ERR 503 exit 1", rc == 1 and "503" in err, repr((rc, err)))
    holder.close()
    time.sleep(0.5)
    rc, out, err = run(base + ["ping"])
    check("served again after holder closes", rc == 0 and out.strip() == "PONG", repr((rc, out, err)))

    # usage error
    rc, out, err = run(base)
    check("no command -> usage error", rc == 2 or rc == 3)

    failed = [n for n, ok in results if not ok]
    print("\n%d tests, %d failed" % (len(results), len(failed)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
