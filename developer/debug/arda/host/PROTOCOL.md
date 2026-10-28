# ARDA wire protocol, version 1

ARDA (AROS Remote Debug Agent) speaks a line-oriented, text-only protocol
over a single TCP connection.  The agent serves one client at a time and is
strictly read-only: no command changes system state, runs programs, writes
memory or files, or stops tasks.

## Transport

* TCP, default port 7777, default bind address 127.0.0.1.  Remote access
  needs an explicit `ADDRESS=` on the agent command line.
* One client at a time.  A second connection is answered with
  `ERR 503 busy, one client at a time` and closed immediately.
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
| 503  | agent busy with another client             |

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

## Versioning

`VERSION` reports `ARDA/<protocol>`.  The protocol number only changes when
an existing response shape changes.  Adding commands or `KEY=VALUE` fields
is backward compatible; clients should treat unknown commands as `ERR 404`
and unknown keys as informational.
