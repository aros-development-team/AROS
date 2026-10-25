#!/usr/bin/env python3
"""ARDA Phase 1 protocol tests: raw-socket checks against a running agent.

Standard library only.  Usage:
    test_phase1.py --host 10.0.0.2 --port 7777 [--wait 60]
Exit status 0 when every test passes, 1 otherwise.  --wait retries the first
connection for that many seconds so the test can start before the agent does.
"""
import argparse
import socket
import sys
import time

TIMEOUT = 5.0


class Conn:
    def __init__(self, host, port, timeout=TIMEOUT):
        self.s = socket.create_connection((host, port), timeout=timeout)
        self.buf = b""

    def send(self, data):
        if isinstance(data, str):
            data = data.encode()
        self.s.sendall(data)

    def readline(self):
        while b"\n" not in self.buf:
            chunk = self.s.recv(4096)
            if not chunk:
                raise EOFError("peer closed")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return line.decode("ascii", "replace")

    def response(self):
        """Read one complete response: single line, or OK ... END block."""
        first = self.readline()
        if first != "OK":
            return [first]
        lines = [first]
        while True:
            line = self.readline()
            lines.append(line)
            if line == "END" or line.startswith("END "):
                return lines

    def command(self, cmd):
        self.send(cmd + "\n")
        return self.response()

    def closed_by_peer(self):
        try:
            data = self.s.recv(1)
        except socket.timeout:
            return False
        return data == b""

    def close(self):
        self.s.close()


results = []


def check(name, cond, detail=""):
    results.append((name, bool(cond), detail))
    print("%s  %s%s" % ("PASS" if cond else "FAIL", name, (" - " + detail) if detail else ""))


def wait_for_agent(host, port, secs):
    deadline = time.time() + secs
    last = None
    while time.time() < deadline:
        try:
            c = Conn(host, port)
            c.close()
            return True
        except OSError as e:
            last = e
            time.sleep(1)
    print("agent not reachable at %s:%d after %ds: %s" % (host, port, secs, last))
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    ap.add_argument("--wait", type=int, default=0)
    a = ap.parse_args()

    if a.wait and not wait_for_agent(a.host, a.port, a.wait):
        return 1

    # 1. basic commands
    c = Conn(a.host, a.port)
    check("PING", c.command("PING") == ["OK PONG"])
    r = c.command("VERSION")
    check("VERSION", len(r) == 1 and r[0].startswith("OK ARDA/1 "), repr(r))
    r = c.command("HELP")
    check("HELP multi-line", r[0] == "OK" and r[-1] == "END" and any(l.startswith("PING") for l in r[1:-1]), repr(r))
    r = c.command("STATUS")
    kv = dict(l.split("=", 1) for l in r[1:-1] if "=" in l)
    check("STATUS block", r[0] == "OK" and r[-1] == "END" and kv.get("PROTOCOL") == "1", repr(r))
    check("STATUS counts this connection", kv.get("CONNECTIONS") not in (None, "0"), repr(kv.get("CONNECTIONS")))

    # 2. case-insensitive, surrounding blanks, CRLF, empty lines ignored
    check("lower case + CRLF", c.command("  ping  \r") == ["OK PONG"])
    c.send("\n\n\n")
    check("empty lines ignored", c.command("PING") == ["OK PONG"])

    # 3. errors
    r = c.command("NOSUCH thing")
    check("unknown command -> ERR 404", len(r) == 1 and r[0].startswith("ERR 404 "), repr(r))
    r = c.command("PING\x01")
    check("non-printable -> ERR 400", len(r) == 1 and r[0].startswith("ERR 400 "), repr(r))
    c.send("X" * 1000 + "\n")
    r = c.response()
    check("overlong line -> ERR 400", len(r) == 1 and r[0].startswith("ERR 400 "), repr(r))
    check("agent resyncs after overlong line", c.command("PING") == ["OK PONG"])

    # 4. partial writes and pipelining
    for ch in "PING\n":
        c.send(ch)
        time.sleep(0.05)
    check("byte-at-a-time request", c.response() == ["OK PONG"])
    c.send("PING\nVERSION\nPING\n")
    r1, r2, r3 = c.response(), c.response(), c.response()
    check("pipelined requests", r1 == ["OK PONG"] and r2[0].startswith("OK ARDA/") and r3 == ["OK PONG"])

    # 5. second client while busy
    try:
        c2 = Conn(a.host, a.port)
        r = c2.response()
        check("second client refused with ERR 503", r[0].startswith("ERR 503 "), repr(r))
        check("second client closed", c2.closed_by_peer())
        c2.close()
    except OSError as e:
        check("second client refused with ERR 503", False, str(e))
    check("first client still served", c.command("PING") == ["OK PONG"])

    # 6. QUIT closes, reconnection works
    check("QUIT", c.command("QUIT") == ["OK BYE"])
    check("connection closed after QUIT", c.closed_by_peer())
    c.close()
    c = Conn(a.host, a.port)
    check("reconnect after QUIT", c.command("PING") == ["OK PONG"])

    # 7. abrupt disconnect, then reconnect
    c.send("PI")
    c.close()
    time.sleep(0.5)
    c = Conn(a.host, a.port)
    check("reconnect after abrupt close", c.command("PING") == ["OK PONG"])
    r = c.command("STATUS")
    kv = dict(l.split("=", 1) for l in r[1:-1] if "=" in l)
    check("counters advanced", int(kv.get("CONNECTIONS", 0)) >= 4 and int(kv.get("ERRORS", 0)) >= 3, repr(kv))
    c.command("QUIT")
    c.close()

    failed = [n for n, ok, _ in results if not ok]
    print("\n%d tests, %d failed" % (len(results), len(failed)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
