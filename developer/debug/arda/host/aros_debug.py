#!/usr/bin/env python3
"""aros_debug.py - host-side client for ARDA, the AROS Remote Debug Agent.

Standard library only.  One command per invocation, non-interactive:

    aros_debug.py --host 10.0.0.2 --port 7777 ping
    aros_debug.py --host 10.0.0.2 version
    aros_debug.py help
    aros_debug.py --json status
    aros_debug.py --output tasks.txt tasks
    aros_debug.py raw "LOG READ 50"      # send a line verbatim

Multi-word commands are joined: `aros_debug.py log read 20` sends `LOG READ 20`.
Defaults come from ARDA_HOST / ARDA_PORT when set.

Exit status: 0 success, 1 the agent answered ERR, 2 connection or protocol
failure, 3 usage error.
"""
import argparse
import json
import os
import socket
import sys
import time

PROTOCOL = "1"
EXIT_OK, EXIT_ERR, EXIT_CONN, EXIT_USAGE = 0, 1, 2, 3


class ArdaError(Exception):
    """Transport or protocol failure (not an ERR reply)."""


class Response:
    def __init__(self, first):
        self.first = first
        self.ok = first.startswith("OK")
        self.code = 0
        self.text = ""
        self.lines = []
        self.truncated = False
        if first == "OK":
            self.multi = True
        elif first.startswith("OK "):
            self.multi = False
            self.text = first[3:]
        elif first.startswith("ERR "):
            self.multi = False
            parts = first.split(" ", 2)
            try:
                self.code = int(parts[1])
            except (IndexError, ValueError):
                raise ArdaError("malformed ERR line: %r" % first)
            self.text = parts[2] if len(parts) > 2 else ""
        else:
            raise ArdaError("unexpected response line: %r" % first)

    @property
    def fields(self):
        out = {}
        for line in self.lines:
            if "=" in line and " " not in line.split("=", 1)[0]:
                k, v = line.split("=", 1)
                out[k] = v
        return out

    def to_dict(self):
        d = {"ok": self.ok, "status": "OK" if self.ok else "ERR"}
        if not self.ok:
            d["error"] = {"code": self.code, "message": self.text}
        elif self.multi:
            d["lines"] = self.lines
            f = self.fields
            if f:
                d["fields"] = f
            d["truncated"] = self.truncated
        else:
            d["text"] = self.text
        return d


class ArdaClient:
    def __init__(self, host, port, connect_timeout=5.0, timeout=10.0):
        self.host, self.port = host, port
        self.connect_timeout, self.timeout = connect_timeout, timeout
        self.sock = None
        self.buf = b""

    def connect(self):
        try:
            self.sock = socket.create_connection((self.host, self.port), timeout=self.connect_timeout)
        except OSError as e:
            raise ArdaError("cannot connect to %s:%d: %s" % (self.host, self.port, e))
        self.sock.settimeout(self.timeout)
        return self

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            finally:
                self.sock = None

    def __enter__(self):
        return self.connect()

    def __exit__(self, *exc):
        self.close()

    def _readline(self):
        while b"\n" not in self.buf:
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                raise ArdaError("timeout waiting for response from %s:%d" % (self.host, self.port))
            except OSError as e:
                raise ArdaError("receive failed: %s" % e)
            if not chunk:
                raise ArdaError("connection closed by agent")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return line.rstrip(b"\r").decode("ascii", "replace")

    def send_line(self, line):
        if "\n" in line or "\r" in line:
            raise ArdaError("request must be a single line")
        if len(line) > 256:
            raise ArdaError("request longer than 256 bytes")
        try:
            self.sock.sendall((line + "\n").encode("ascii"))
        except UnicodeEncodeError:
            raise ArdaError("request must be ASCII")
        except OSError as e:
            raise ArdaError("send failed: %s" % e)

    def read_response(self):
        r = Response(self._readline())
        if r.multi:
            while True:
                line = self._readline()
                if line == "END":
                    break
                if line == "END TRUNCATED":
                    r.truncated = True
                    break
                r.lines.append(line)
        return r

    def command(self, line):
        self.send_line(line)
        return self.read_response()

    # ---- control mode: raw bodies ----
    def _read_exact(self, n):
        """n raw bytes after a status line; bytes already buffered come first."""
        out = bytearray()
        if self.buf:
            take = self.buf[:n]
            out += take
            self.buf = self.buf[len(take):]
        while len(out) < n:
            try:
                chunk = self.sock.recv(min(1 << 20, n - len(out)))
            except socket.timeout:
                raise ArdaError("timeout while receiving a file body (%d of %d bytes)" % (len(out), n))
            except OSError as e:
                raise ArdaError("receive failed: %s" % e)
            if not chunk:
                raise ArdaError("connection closed by agent during a file body")
            out += chunk
        return bytes(out)

    def put(self, local, remote):
        """PUT: upload a local file as remote path (control mode). Returns the agent's reply."""
        with open(local, "rb") as f:
            data = f.read()
        r = self.command("PUT %s %d" % (remote, len(data)))
        if not r.ok:
            return r
        if r.text != "READY":
            return r                    # zero-length file: WROTE 0
        try:
            self.sock.sendall(data)
        except OSError as e:
            raise ArdaError("send failed during the body: %s" % e)
        return self.read_response()

    def get(self, remote, local):
        """GET: download remote path into local file (control mode). Returns the reply."""
        self.send_line("GET %s" % remote)
        r = Response(self._readline())
        if not r.ok:
            return r
        words = r.text.split()
        if len(words) != 2 or words[0] != "SIZE":
            raise ArdaError("unexpected GET reply: %s" % r.text)
        n = int(words[1])
        data = self._read_exact(n)
        with open(local, "wb") as f:
            f.write(data)
        r.text = "READ %d" % n
        return r

    def quit(self):
        try:
            self.send_line("QUIT")
            self.read_response()
        except ArdaError:
            pass


def format_human(cmd, r):
    if not r.ok:
        return "ERR %d: %s" % (r.code, r.text)
    if not r.multi:
        return r.text
    out = list(r.lines)
    if r.truncated:
        out.append("[response truncated by agent]")
    return "\n".join(out)


def main(argv=None):
    ap = argparse.ArgumentParser(description="ARDA host client (protocol %s)" % PROTOCOL,
                                 formatter_class=argparse.RawDescriptionHelpFormatter,
                                 epilog=__doc__.split("\n\n", 1)[1])
    ap.add_argument("--host", default=os.environ.get("ARDA_HOST", "127.0.0.1"))
    ap.add_argument("--port", type=int, default=int(os.environ.get("ARDA_PORT", "7777")))
    ap.add_argument("--connect-timeout", type=float, default=5.0, metavar="SECS")
    ap.add_argument("--timeout", type=float, default=60.0, metavar="SECS",
                    help="response timeout (default 60; WAIT and transfers may need more)")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--output", "-o", metavar="FILE", help="also write the output to FILE")
    ap.add_argument("--quiet", "-q", action="store_true", help="no stdout when --output is used")
    ap.add_argument("--no-quit", action="store_true", help="do not send QUIT before closing")
    ap.add_argument("command", nargs="+", help="command word(s), e.g. ping, version, help, 'raw LINE'")
    a = ap.parse_args(argv)

    words = a.command
    verb = words[0].lower()
    transfer = None
    if verb == "raw":
        if len(words) < 2:
            ap.error("raw needs the line to send")
        line = " ".join(words[1:])
    elif verb in ("put", "get"):
        # put LOCAL REMOTE / get REMOTE LOCAL: file transfers (control mode)
        if len(words) != 3:
            ap.error("%s needs two paths" % verb)
        transfer = (verb, words[1], words[2])
        line = verb.upper() + " " + (words[2] if verb == "put" else words[1])
    elif verb in ("run", "setenv", "getenv", "dir", "delete", "mkdir", "job", "wait", "jobs", "reboot"):
        # control commands keep their arguments' case (paths, command lines, values)
        line = verb.upper() + ((" " + " ".join(words[1:])) if len(words) > 1 else "")
    else:
        line = " ".join(words).upper()

    started = time.time()
    try:
        with ArdaClient(a.host, a.port, a.connect_timeout, a.timeout) as c:
            if transfer and transfer[0] == "put":
                r = c.put(transfer[1], transfer[2])
            elif transfer:
                r = c.get(transfer[1], transfer[2])
            else:
                r = c.command(line)
            if not a.no_quit and line.split(" ", 1)[0] not in ("QUIT", "REBOOT"):
                c.quit()
    except ArdaError as e:
        if a.json:
            print(json.dumps({"ok": False, "status": "CONNECTION", "command": line,
                              "host": a.host, "port": a.port, "error": {"code": 0, "message": str(e)}}))
        else:
            print("aros_debug: %s" % e, file=sys.stderr)
        return EXIT_CONN

    elapsed = int((time.time() - started) * 1000)
    if a.json:
        d = r.to_dict()
        d.update({"command": line, "host": a.host, "port": a.port, "elapsed_ms": elapsed})
        text = json.dumps(d, indent=2)
    else:
        text = format_human(line, r)

    if a.output:
        try:
            with open(a.output, "w") as f:
                f.write(text + "\n")
        except OSError as e:
            print("aros_debug: cannot write %s: %s" % (a.output, e), file=sys.stderr)
            return EXIT_USAGE
    if not (a.output and a.quiet):
        stream = sys.stdout if (r.ok or a.json) else sys.stderr
        print(text, file=stream)
    return EXIT_OK if r.ok else EXIT_ERR


if __name__ == "__main__":
    sys.exit(main())
