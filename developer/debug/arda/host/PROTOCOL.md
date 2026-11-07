# ARDA wire protocol, version 1

ARDA (AROS Remote Debug Agent) speaks a line-oriented, text-only protocol
over TCP connections.  The agent serves up to four clients at the same time
where the TCP/IP stack allows it (see Transport), and is strictly read-only: no command changes system state, runs programs, writes
memory or files, or stops tasks.

## Transport

* TCP, default port 7777, default bind address 127.0.0.1.  Remote access
  needs an explicit `ADDRESS=` on the agent command line.
* Up to four clients at the same time, each served by a thread of its own,
  where the stack lets the tasks of a program share a library base
  (`SBTC_CAN_SHARE_LIBRARY_BASES`): a command that blocks (`WAIT`, a `GET`
  body) holds up only its own client. Commands themselves run one at a
  time. A further connection is answered with `ERR 503 busy, too many
  clients` and closed immediately.
* On a stack that cannot share a base: one client at a time, and a second
  connection is answered with `ERR 503 busy, one client at a time`.
* The agent drops a client that sends nothing for `IDLE` seconds (default
  60, `IDLE=0` disables) after sending `ERR 408 idle timeout, closing`.
* Lines end with `\n`; a trailing `\r` is stripped.  Empty lines are ignored.

## Requests

```
<COMMAND> [<arguments>]\n
```

* The command word is matched case-insensitively.  Leading/trailing blanks
  are ignored.  Arguments are passed to the command verbatim (trailing
  blanks removed).
* Maximum request line: 256 bytes, excluding the newline.  Longer lines get
  `ERR 400 request line too long (max 256 bytes)` and the rest of the line is
  discarded; the next line is processed normally.
* Only printable ASCII (0x20..0x7E) and TAB are accepted; anything else gets
  `ERR 400 request contains non-printable characters`.
* Requests may be pipelined; responses come back in order.

## Responses

Three shapes, distinguished by the first line:

| First line        | Meaning                                                         |
|-------------------|-----------------------------------------------------------------|
| `OK <text>`       | Success, single line, complete.  `<text>` is never empty.       |
| `OK`              | Success, multi-line.  Body lines follow until a terminator.      |
| `ERR <code> <text>` | Failure, single line, complete.                               |

Multi-line responses are terminated by a line that is exactly `END`, or
`END TRUNCATED` when the response did not fit the agent's response buffer
(64 KiB).  In the truncated case every body line before the terminator is
complete; nothing after it was sent.  Body lines never begin with `END`.

Body lines of structured commands are `KEY=VALUE` pairs (no spaces around
`=`, keys upper case).  Tabular commands use `KEY=VALUE` fields separated by
single spaces on one line per record.  Clients must ignore keys they do not
know; new keys and new commands may be added without a protocol bump.

Error codes (HTTP-flavoured for familiarity):

| Code | Meaning                                    |
|------|--------------------------------------------|
| 400  | malformed request (too long, bad bytes, bad argument) |
| 404  | unknown command                            |
| 408  | idle timeout, connection is being closed   |
| 500  | internal error in the agent                |
| 501  | command known but not available on this system (e.g. resource missing) |
| 503  | agent busy: too many clients               |

## Commands (protocol 1)

| Command   | Response                                             |
|-----------|------------------------------------------------------|
| `PING`    | `OK PONG`                                            |
| `VERSION` | `OK ARDA/1 agent <x.y>`                              |
| `HELP`    | multi-line: `<NAME> <description>` per command       |
| `STATUS`  | multi-line `KEY=VALUE`: PROTOCOL, AGENT, UPTIME, PORT, IDLE_LIMIT, PEER, CONNECTIONS, REJECTED, COMMANDS, ERRORS, MAX_LINE, MAX_RESPONSE |
| `QUIT`    | `OK BYE`, then the agent closes the connection       |
| `SYSINFO` | multi-line `KEY=VALUE`, see below                    |
| `MEMORY`  | multi-line `KEY=VALUE` plus one `MEMHEADER ...` record per exec memory header |
| `TASKS [filter]` | multi-line: `TOTAL=`, `COUNT=`, then one `TASK ...` record per task; `filter` is a case-insensitive substring of the task name |
| `LOG STATUS` | multi-line `KEY=VALUE` describing the trace ring and log sources |
| `LOG READ [n] [SINCE=seq] [SRC=agent\|app\|syslog]` | multi-line: `MATCHED=`, `COUNT=`, `NEXT_SEQ=`, then the newest `n` (default 100, max 300) matching `LOG ...` records, oldest first |
| `LOG CLEAR` | `OK CLEARED <n>`; empties the ring (the only state the agent ever changes, and it is its own) |
| `NET STATUS` | multi-line `KEY=VALUE`: BSDSOCKET_VERSION, STACK_RELEASE, HOSTNAME, [DOMAIN], INTERFACES, ROUTES_INET, ROUTES_INET6, `DNS=<addr> USECOUNT=<n>` lines, DNS_COUNT, STATS, SOCKETS |
| `NET INTERFACES` | multi-line: `COUNT=`, then one `IFACE ...` record per interface |
| `NET ROUTES` | multi-line: `COUNT=`, `COUNT_INET=`, `COUNT_INET6=`, then one `ROUTE ...` record per routing-table entry |
| `NET ARP` | multi-line: `COUNT=`, then one `ARP ...` record per ARP table entry |
| `NET STATS [IP\|TCP\|UDP\|ALL]` | multi-line `KEY=VALUE` counters copied from the stack's `struct ipstat`, `tcpstat`, `udpstat` |
| `NET SOCKETS` | always `ERR 501`: AROSTCP has no supported socket enumeration API |

### SYSINFO keys

`AROS_VERSION`, `AROS_RELEASE`, `AROS_ABI`, `AROS_BUILD_DATE`, `AROS_VARIANT`,
`AROS_ARCHITECTURE` (aros.library ArosInquire); `EXEC_VERSION`, `ATTNFLAGS`,
`VBLANK_HZ`, `ECLOCK_HZ`, `DISPATCH_COUNT`, `IDLE_COUNT` (ExecBase);
`KERNEL_ARCHITECTURE`, `CPU_COUNT` (kernel.resource); `CPU_PROCESSORS`,
`CPU_ARCH`, `CPU_ENDIAN`, `CPU_FAMILY`, `CPU_MODEL`, `CPU_ISA`, `CPU_MHZ`,
`CPU_VECTOR_UNIT`, `CPU_L1_KB`, `CPU_L2_KB`, `CPU_L3_KB`, `CPU_LOAD`
(processor.resource; zero when the platform does not report a value);
`SYSTEM_TIME` (ISO, local clock), `DOS_TIME`, `DATESTAMP`, `AGENT_UPTIME`.
Keys from a missing resource are omitted.

### MEMORY keys and records

`ANY_/CHIP_/FAST_` x `TOTAL/FREE/LARGEST` from `AvailMem()`, `MEMHEADERS=<n>`
and records `MEMHEADER NAME="..." LOWER=0x.. UPPER=0x.. SIZE= FREE= ATTR=0x.. PRI=`
snapshotted from `ExecBase->MemList` under `Forbid()`.

### TASK records

```
TASK ADDR=0x.. NAME="..." TYPE=task|process|cli STATE=run|ready|wait|spin|... PRI= STACK= STACKUSED=
     SIGWAIT=0x.. SIGRECVD=0x.. CPUTIME=<secs.micro> CPUUSAGE= CPU= PROC= PC=0x.. [SYM=module:symbol+0x..] [SELF=1]
```
Taken through task.resource (`LockTaskList`/`NextTaskEntry`/`QueryTaskTagList`),
never by walking exec lists.  `PROC` is the CLI process number (-1 for
tasks, 0 for non-CLI processes).  `SYM` is present when debug.library can
resolve the saved PC; on Linux-hosted builds the saved PC of every waiting
task is the host's signal trampoline, so `SYM` never appears there.  Names
are sanitised: anything outside printable ASCII and `"` becomes `?`.

### LOG records

```
LOG SEQ=<n> TIME=<secs.micro> SRC=agent|app|syslog LEVEL=<n> NAME="..." VALUE=<n> TEXT="..."
```
Entries come from one system-wide ring (see `ardatrace.h`): the agent's own
events, `ArdaTraceEvent()` calls from instrumented programs, and (when
`log.resource` exists) forwarded system log entries.  Raw kernel `bug()`
output is not captured; `LOG STATUS` says so (`KERNEL_DEBUG=not captured`).
`TIME` is `timer.device` system time; 0 when unavailable.

### IFACE, ROUTE and ARP records

```
IFACE NAME=eth0 FLAGS=0x8843 FLAGNAMES=UP,BROADCAST,RUNNING,SIMPLEX,MULTICAST MTU=1500 METRIC=0
      ADDR=10.0.0.2 NETMASK=255.255.255.0 BROADCAST=10.0.0.255 ADDR6=fe80:0:0:0:... HWADDR=3e:5c:25:b6:0b:6d
      STATE=up RXPKT=<n> TXPKT=<n> RXBYTES=<n> TXBYTES=<n> NOPROTO=<n>
ROUTE AF=inet|inet6 DST=<addr> MASK=<mask>|host GW=<addr>|link#<n> FLAGS=0x.. FLAGNAMES=U,G,H,... IFINDEX= USE= MTU= EXPIRE=
ARP   ADDR=<ipv4> HWADDR=<mac>|incomplete FLAGS=0x.. FLAGNAMES=COMPLETE,PERMANENT,PUBLISHED
```
Interfaces come from `SIOCGIFCONF` plus the `SIOCGIF*` ioctls (as C:ifconfig)
and `QueryInterfaceTagList()`; routes from `GetRouteInfo()`; the ARP table
from `SIOCGARPT` (as C:arp); statistics from `MiamiSysCtl()` over
`net.inet.{ip,tcp,udp}` (`*CTL_STATS`).  `BROADCAST` carries the destination
address for point-to-point links and `-` otherwise.  `ADDR6` is `-` when the
stack has no IPv6 socket support.

## Control mode (agent 0.2, protocol 1)

Everything above is read-only. An agent started with the `CONTROL` switch
additionally accepts the commands below, which change the system: write
files, set environment variables, run programs, reboot. Without the switch
each of them answers `ERR 501 control mode off (start ARDA with CONTROL)`.
`VERSION` ends in ` control` and `STATUS` has `CONTROL=1` when it is on. This
is for a trusted development network only; there is still no authentication.

| Command | Response |
|---------|----------|
| `PUT <path> <size>` | `OK READY`, after which the client sends exactly `size` raw bytes, written to `path` (created or truncated) and the volume flushed (ACTION_FLUSH). Then `OK WROTE <size>`, or `ERR 500 write failed, file incomplete`. A zero `size` answers `OK WROTE 0` with no body. Up to 512 MiB. |
| `GET <path>` | `OK SIZE <n>` followed immediately by `n` raw bytes of the file, or `ERR 404`. |
| `DIR [path]` | multi-line `ENTRY NAME="..." TYPE=file\|dir SIZE= DATE="..."` records, then `COUNT=`. Without a path: the agent's current directory. |
| `DELETE <path>` | `OK DELETED` |
| `MKDIR <path>` | `OK CREATED` |
| `SETENV <name> [value]` | `OK SET`; without a value `OK UNSET` (global environment, `GVF_GLOBAL_ONLY`) |
| `GETENV <name>` | `OK <value>` or `ERR 404` |
| `RUN <output-file> <command line>` | `OK JOB <n>`. The command runs in its own process (user shell, 256 KB stack) with `NIL:` as input and `output-file` as output and error; the agent does not wait. Up to 8 job records are kept; a finished one is reused. |
| `JOBS` | multi-line `JOB ID= STATE=running\|done RC= OUTPUT="..." STARTED= ELAPSED= COMMAND="..."`, then `COUNT=` |
| `JOB <n>` | one `JOB` record |
| `WAIT <n> [secs]` | blocks up to `secs` (default 60, max 600) for job `n`; `OK RC <rc>` or `ERR 408 job still running`. The agent serves nothing else meanwhile. |
| `REBOOT` | `OK REBOOTING`; the agent flushes `SYS:`, closes the connection and calls `ColdReboot()` half a second later. |

Raw bodies are the only place bytes outside printable ASCII travel; the line
parser resumes right after them. Paths are AROS paths (`SYS:`, `RAM:`, ...)
relative to the agent's current directory when not absolute.

## Memory inspection (agent 0.3, protocol 1)

`SYMBOL` is always available. `PEEK` and `STACK` read arbitrary memory, so
they are control-mode commands, although they change nothing. A range is
read only when all of it lies in a MemHeader (`TypeOfMem()`) or in a segment
debug.library knows. Anything else gets `ERR 403` and is not touched, so a
bad address cannot fault the agent. Numbers may be decimal or `0x` hex.

| Command | Response |
|---------|----------|
| `SYMBOL <addr>...` | multi-line, one `ADDR=0x... SYM=<module>:<segment>:<symbol>+0x<off>` (or `SYM=none`) per address, up to 32. |
| `PEEK <addr> [<len>] [Q]` | multi-line hex dump of `len` bytes (default 256, max 4096): `ADDR=0x... HEX="xx xx ..." ASCII="..."`, 16 bytes per line. With `Q`: 64-bit words, `ADDR=0x... VALUE=0x... [SYM=...]`, the address rounded down to 8. |
| `STACK <task\|0xaddr> [FROM=<addr>] [MAX=<bytes>] [ALL]` | the task (exact name, or its address) and `SP= LOWER= UPPER= FROM= BYTES=`, then one `ADDR= OFF= VALUE= SYM=` line per stack word that points into a code (`text`) segment; `ALL` lists every word that resolves to any segment. The scan runs upwards from the saved SP, or from `FROM=` (e.g. the SP of a crash report), for `MAX` bytes (default 16 KiB, max 256 KiB), ending at the stack's upper bound. Then `SHOWN=`. |

## Debugging (agent 0.4, protocol 1)

`ADDRESS`, `MODULES`, `LIBS`, `DEVICES`, `RESOURCES`, `PORTS` and
`SEMAPHORES` only read and are always available. `BREAK` sends a signal and
`BACKTRACE` reads a task's stack, so they are control-mode commands. A task
is named exactly as in `TASKS` (its node name, or a CLI process's command
name) or given by address. Lists are copied under `Forbid()`; a pointer
found in them is followed only when it lies in RAM.

| Command | Response |
|---------|----------|
| `BREAK <task\|0xaddr\|JOB=n> [C\|D\|E\|F]` | `TASK=0x... NAME="..." SIGNAL=CTRL_<x> MASK=0x...`: the break signal (default Ctrl-C) sent to the task, or to a `RUN` job's process. `ERR 404` no such task or job, `409` the job is not running. |
| `BACKTRACE <task\|0xaddr> [MAX=<bytes>]` | `TASK NAME= PC= LR= SP= FP= LOWER= UPPER= REGS=saved\|none` from the task's saved context, then `FRAME=<n> KIND=pc\|lr\|stack [AT=<stack addr> OFF=<from SP>] VALUE=0x... SYM=...` per frame, and `FRAMES= SCANNED=`. Stack frames are words, from the saved SP upwards (`MAX` bytes, default 64 KiB, max 256 KiB), that point into a code segment just after a call instruction (riscv `jal`/`jalr`/`c.jalr` to `ra`, aarch64 `bl`/`blr`, x86 `call`), so stale pointers to code are left out. `LR` is listed when it is such an address. A running task (or the agent itself) has no saved context: `ERR 409`. |
| `ADDRESS <symbol\|prefix*> [module]` | `SYMBOL="..." MODULE="..." START=0x... END=0x...` per match (up to 64), then `MATCHES= SHOWN=`. A trailing `*` matches a prefix; `module` restricts the search (case-insensitive). |
| `MODULES [filter]` | `MODULE NAME="..." SYMBOLS=<n> LOW=0x... HIGH=0x...` per module debug.library has symbols for (the range spanned by its symbols), optionally only names containing `filter`; then `COUNT=` (`MORE=1` past 512). |
| `LIBS`, `DEVICES` | `LIB`/`DEVICE NAME="..." BASE=0x... VERSION=<v>.<r> OPENCNT=<n> FLAGS=0x.. [DELEXP=1]` per node, then `COUNT= TOTAL=`. A name is read only from RAM or a segment debug.library knows; one it cannot read is given as `NAME="" NAMEPTR=0x...`. |
| `RESOURCES` | `RESOURCE NAME="..." BASE=0x...` per node (with the library fields when the node is a library), then `COUNT= TOTAL=`. |
| `PORTS` | `PORT NAME="..." ADDR=0x... ACTION=SIGNAL\|SOFTINT\|IGNORE\|ACTION SIGBIT= TASK=0x... TASKNAME="..." MSGS=<queued>` per public port, then `COUNT= TOTAL=`. |
| `SEMAPHORES` | `SEMAPHORE NAME="..." ADDR=0x... NEST= QUEUE= OWNER=0x... OWNERNAME="..." WAITERS=<n> WAIT0="<task>[(shared)]" ...` per public semaphore (first 8 waiters named), then `COUNT= TOTAL=`. |

## Versioning

`VERSION` reports `ARDA/<protocol>`.  The protocol number only changes when
an existing response shape changes.  Adding commands or `KEY=VALUE` fields
is backward compatible; clients should treat unknown commands as `ERR 404`
and unknown keys as informational.
