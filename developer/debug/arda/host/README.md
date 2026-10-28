# ARDA - AROS Remote Debug Agent

A small, read-only diagnostic agent that runs inside AROS and answers a
line-oriented protocol over TCP, plus a standard-library-only Python client
for the development host.  It exists so that a running AROS instance can be
inspected remotely, from scripts or automated tooling, without touching its
GUI or shell.

```
developer/debug/arda/
  arda.c arda_net.c arda_cmd.c     agent core: args, socket loop, protocol
  arda_sys.c                       SYSINFO MEMORY TASKS
  arda_log.c                       LOG STATUS/READ/CLEAR, log.resource listener
  arda_netdiag.c                   NET STATUS/INTERFACES/ROUTES/ARP/STATS
  ardatrace.h ardatrace.c          instrumentation API + link library (Phase 6)
  tracetest.c                      emits trace events (test helper)
  host/aros_debug.py               the client
  host/PROTOCOL.md                 wire protocol
  host/tests/                      protocol tests (raw socket + client)
```

## Building

The agent is part of the normal AROS build (mmake target `debug-tools-arda`,
pulled in by `debug-tools` / `workbench-tools`).  From a configured build
directory:

```
make -j16 debug-tools-arda          # first time: also copies <arda/ardatrace.h>, builds libardatrace.a and tracetest
make -j16 debug-tools-arda-quick    # iterate on the agent sources only
```

Outputs: `SYS:Developer/Debug/ARDA`, `SYS:Developer/Debug/Tests/arda/tracetest`,
`Developer/lib/libardatrace.a`, `Developer/include/arda/ardatrace.h`.
Needs the network headers staged (`network-includes-copy`, done by the full
target) and a stack that provides `bsdsocket.library` v4 (AROSTCP).

## Running the agent on AROS

```
ARDA [PORT=<n>] [ADDRESS=<dotted quad>] [IDLE=<secs>] [QUIET] [CONTROL]
```
* `CONTROL` enables the control mode (PROTOCOL.md): file transfer (`PUT`/`GET`),
  `DIR`/`DELETE`/`MKDIR`, `SETENV`/`GETENV`, `RUN`/`JOBS`/`JOB`/`WAIT` and
  `REBOOT`. Off by default; the agent is then read-only as documented.

* Default: port 7777 on **127.0.0.1**.  Remote access requires an explicit
  `ADDRESS=` (the interface address or `0.0.0.0`) and should only be used on
  a trusted development network: the protocol has no authentication.
* `IDLE` (default 60, 0 = never) drops a client that sends nothing.
* One client at a time; a second connection gets `ERR 503` and is closed.
* Stop it with Ctrl-C / `Break <n> C`; it closes the client, the listening
  socket and every library it opened.
* Typical: `Run >T:arda.log SYS:Developer/Debug/ARDA ADDRESS=10.0.0.2`
  after `WaitForPort AROSTCP`.

Hosted (linux-x86_64) deployment: the build tree *is* SYS:, so `make`
installs straight into the booted system.  To reach it from Linux boot
inside a user+net namespace with a TAP named `aros0` (10.0.0.1) and give the
stack `eth0 DEV=DEVS:Networks/tap.device UNIT=0 IP=10.0.0.2 NETMASK=255.255.255.0 UP`
in `Prefs/Env-Archive/AROSTCP/db/interfaces` (ENV:AROSTCP/db is seeded from
there; the copy under System/Network is not read).

## Using the client

```
python3 aros_debug.py --host 10.0.0.2 --port 7777 ping
python3 aros_debug.py --host 10.0.0.2 version
python3 aros_debug.py --host 10.0.0.2 help
python3 aros_debug.py --host 10.0.0.2 --json sysinfo
python3 aros_debug.py --host 10.0.0.2 tasks arostcp          # name filter
python3 aros_debug.py --host 10.0.0.2 --output mem.txt memory
python3 aros_debug.py --host 10.0.0.2 log read 50
python3 aros_debug.py --host 10.0.0.2 net interfaces
python3 aros_debug.py --host 10.0.0.2 raw "LOG READ SINCE=120 SRC=app"

# control mode (agent started with CONTROL)
python3 aros_debug.py --host 10.0.0.2 put build/nouveau.hidd SYS:Devs/Drivers/nouveau.hidd
python3 aros_debug.py --host 10.0.0.2 get SYS:nouveau.log nouveau.log
python3 aros_debug.py --host 10.0.0.2 setenv NOUVEAU_NVDEC_EXP 6
python3 aros_debug.py --host 10.0.0.2 run RAM:job.out SYS:Extras/MultiMedia/Video/vadecode -n 2 clip.mp4
python3 aros_debug.py --host 10.0.0.2 --timeout 300 wait 1 240
python3 aros_debug.py --host 10.0.0.2 dir SYS:nouveau-dump
python3 aros_debug.py --host 10.0.0.2 reboot
```

* `ARDA_HOST` / `ARDA_PORT` in the environment set the defaults.
* `--connect-timeout`, `--timeout` (response), `--json`, `--output FILE`
  (`--quiet` to suppress stdout), `--no-quit`.
* Exit status: 0 OK, 1 the agent answered `ERR` (message on stderr, or in
  the JSON `error` object), 2 connection/protocol failure, 3 usage.
* JSON shape: `{"ok", "status", "command", "host", "port", "elapsed_ms",
  "text" | "lines" + "fields" + "truncated", "error": {"code", "message"}}`.
  `fields` is the `KEY=VALUE` body parsed into a dict; record lines
  (`TASK ...`, `IFACE ...`, `ROUTE ...`, `ARP ...`, `LOG ...`, `MEMHEADER ...`)
  are left in `lines` for the caller to parse.

### Scripted remote debugging

Every command is non-interactive and returns within the configured
timeouts, so the client can be driven from shell scripts or any automation
on the development host:

1. `aros_debug.py --host H ping` - is the instance alive?  Exit 2 means the
   agent/stack is not reachable; exit 0 means it is.
2. `aros_debug.py --host H --json sysinfo` / `memory` / `tasks` - snapshot
   the system; `tasks <name>` to focus on one component; `TASK` records carry
   state, priority, stack use, pending signals and CPU time.
3. `aros_debug.py --host H log read` - read the trace ring: the agent's own
   events, `ArdaTraceEvent()` calls from instrumented code and (where the
   resource exists) system log entries.  `log read SINCE=<seq>` to poll
   incrementally, `log clear` to start fresh before reproducing a bug.
4. `aros_debug.py --host H net status|interfaces|routes|arp|stats` - stack
   state when investigating networking.
5. `--output FILE` saves any of the above for later comparison.

## Instrumenting AROS code (Phase 6)

```c
#include <arda/ardatrace.h>          /* link with -lardatrace (uselibs="ardatrace") */

ArdaTraceEvent("fs.mount", unit, "mounting");          /* name, integer value, text */
ArdaTraceEventF("fs.mount", rc, "mount failed err=%ld", (long)err);
```

Events land in one system-wide ring (1024 entries of name[32] + text[96] +
value + timer.device timestamp) found through the public semaphore
`ARDA.trace`.  The ring is created by the first user and never freed, so
instrumentation works without the agent and with no client connected;
`LOG READ` later shows what accumulated, `LOG STATUS` shows overwrites.
Calls are safe from any task (they take a semaphore), not from interrupts,
and never touch the network.

## What LOG does and does not capture

| Source | Captured |
|---|---|
| Agent events (connects, errors) | yes |
| `ArdaTraceEvent()` from programs linked with libardatrace | yes |
| `log.resource` entries (`logAddEntry` producers) | yes when the resource exists; it is configure-gated (`--with-logresource`) and absent on hosted builds, `LOG STATUS` reports `SYSLOG=unavailable` |
| Kernel / library `bug()`, `kprintf()` output | **no**. There is no supported hook; on hosted builds it goes to the bootstrap's stderr (the test harness captures that file), on native to the serial/memory sink.  `Developer/Debug/sashimi` can capture it by patching exec, which this agent deliberately does not do. |

## Known limitations (verified on linux-x86_64 hosted)

* `TASKS`: on hosted builds the saved PC of every waiting task is the host
  signal trampoline, so the `SYM=` field never appears there; it is produced
  via debug.library `DecodeLocation()` where the PC is meaningful.
* `NET SOCKETS` is `ERR 501`: AROSTCP's `GetNetworkStatistics()` and
  `ObtainInterfaceList()` are stubs, and C:netstat reads stack memory via
  libkvm.  Nothing read-only and supported exists, so it is not implemented.
* `NET INTERFACES` `RXPKT` stays 0 on SANA-II interfaces: `if_sana.c`
  counts received bytes but never `if_ipackets` (stack accounting gap, not an
  agent issue).
* `CPU_MHZ` and cache sizes are 0 where processor.resource does not report them.
* The generated `defines/miami.h` macro for the 7-argument `MiamiSysCtl()`
  is malformed and the miami/bsdsocket headers both define
  `QueryInterfaceTagList`; `arda_netdiag.c` therefore calls `MiamiSysCtl`
  through its LVO directly and does not include `proto/miami.h`.

## Testing

Host-side tests (`host/tests/`), each exits non-zero on failure and prints
`PASS`/`FAIL` per check:

| Script | Covers |
|---|---|
| `test_phase1.py` | PING/VERSION/HELP/STATUS/QUIT, case/CRLF/blank lines, unknown command, non-printable bytes, 1000-byte line + resync, byte-at-a-time and pipelined requests, second-client refusal, QUIT and abrupt-close reconnects |
| `test_client.py` | `aros_debug.py` as a subprocess: human/JSON output, `--output`, env defaults, exit codes for ERR / closed port / unreachable host / busy agent / usage |
| `test_phase3.py` | SYSINFO keys and ISO time, MEMORY consistency and headers, TASKS records, filter, stability (20 rounds) |
| `test_phase4.py` | LOG STATUS/READ/CLEAR, events emitted before the agent started, live events via tracetest, SINCE/SRC/count filters, ring wrap at 1024, READ cap |
| `test_phase5.py` | NET STATUS/INTERFACES/ROUTES/ARP/STATS, counters advancing, subsets, error codes |
| `test_idle.py` | idle timeout (`ERR 408`) with `IDLE=5`, and that a default-bound agent is not reachable from the network |
| `run_all.py` | runs the suites above in order |

Executed 2026-10-25 on the linux-x86_64-gcc16 hosted build booted in a
network namespace with a TAP interface (see "Running the agent on AROS"): all
suites pass (117 checks) and the agent shuts down cleanly on Break with no
leaked sockets or library bases reported.  Not tested: native (pc-x86_64)
hardware, the `log.resource` listener path (resource absent on hosted; it
compiles and degrades to `SYSLOG=unavailable`), IPv6 routes beyond the
link-local entries the stack creates by itself.
