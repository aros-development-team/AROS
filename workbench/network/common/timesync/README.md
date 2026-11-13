# TimeSync

`timesyncd` keeps the AROS system clock in step with an NTP server using
SNTP (RFC 4330).  It is an AROSTCP network service: the stack's service
manager starts it when the network comes up and stops it when the network
goes down, and it pauses across a network reconfigure and syncs again as
soon as the new configuration is in place.

## Files

| File | Purpose |
|------|---------|
| `SYS:System/Network/Services/timesyncd` | the daemon (and one-shot command) |
| `SYS:System/Network/AROSTCP/db/services.d/timesync` | service manager script (`Enabled=yes` by default) |
| `ENVARC:AROSTCP/timesync.conf` | configuration, copied to `ENV:` at boot |
| `T:timesync.log` | one line per sync and per error (configurable) |

## Configuration

`ENV:AROSTCP/timesync.conf` is `Key=Value` text; the daemon re-reads it when
it changes.

| Key | Default | Meaning |
|-----|---------|---------|
| `Server` | `pool.ntp.org` | servers tried in order; repeat the key or separate with spaces/commas |
| `Interval` | `3600` | seconds between syncs |
| `Timeout` | `5` | seconds to wait for a reply |
| `Retry` | `60` | first retry delay after a failure, doubling up to `Interval` |
| `SetClock` | `yes` | `no` only reports the offset |
| `SetBattClock` | `yes` | also write the battery-backed clock (as `SetClock SAVE`) |
| `UTCOffset` | `auto` | minutes east of UTC, or `auto` for the Locale preferences |
| `LogFile` | `T:timesync.log` | `none` disables the log |
| `Verbose` | `no` | also print to standard output |

To turn the service off, set `Enabled=no` in the services.d script (or use
the Services page of the Network preferences) and reload the stack.

## As a command

    timesyncd ONCE [SERVER host ...] [VERBOSE] [NOSET] [NOBATTCLOCK]
                   [TIMEOUT secs] [UTCOFFSET minutes] [CONFIG file]

`ONCE` performs a single synchronisation and exits with return code 0 on
success, 10 on failure and 5 if interrupted.  Servers given on the command
line replace those of the configuration file.  `NOSET` shows what the
correction would be without touching the clock.

## How the time is set

The system clock runs in local time, so the daemon converts it to UTC with
the locale's GMT offset (or `UTCOffset`) before comparing it with the
server's, and converts back when it sets it.  The battery-backed clock is
written in UTC, matching `SetClock`.  The clock is stepped, not slewed.

A reply is accepted only from the server that was asked (connected UDP
socket), only when it echoes the request's transmit timestamp, which
carries a random nonce, and only when it is a real server reply: a
kiss-o'-death, an unsynchronised server, a missing transmit timestamp or
inconsistent timestamps are rejected and logged.
