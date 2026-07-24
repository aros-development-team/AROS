# AROSTCP `bsdsocket` Implementation Notes

Technical reference for the AROSTCP network stack — the `bsdsocket.library` implementation under
`AROS/workbench/network/stacks/AROSTCP/bsdsocket`. It describes the architecture, the protocol
engines, and how to build and test the stack.

- **Component:** `bsdsocket.library` (with a compatibility `miami.library`)
- **Version:** 5.5 — kernel v0.40
- **Lineage:** AmiTCP/IP → AROSTCP, derived from 4.3BSD-Net/2 with a KAME-derived IPv6 stack;
  presents the Amiga `bsdsocket.library`/Roadshow API surface.
- **IPv6:** compiled in unconditionally (`conf/conf.h` defines `INET6 1`).

---

## 1. Overview

AROSTCP is a monolithic BSD TCP/IP stack ported to AROS. The protocol machinery (sockets, mbufs,
the routing radix tree, the TCP/UDP/IP engines) follows its Net/2 and KAME origins; the
AROS-specific layer presents the stack as an Amiga shared library, maps the BSD kernel primitives
(`spl`, `tsleep`/`wakeup`, `malloc`, timeouts) onto Exec facilities, and drives network hardware
through SANA-II device drivers.

The stack runs as a single AROS process (the AROSTCP daemon) that owns the protocol state and the
periodic timers, plus one `bsdsocket.library` base per opening task. Applications link against the
library and use the Berkeley socket API (`socket`/`bind`/`connect`/`send`/`recv`/…).

---

## 2. Source layout

| Directory   | Contents |
|-------------|----------|
| `api/`      | The `bsdsocket.library` API surface: syscall layer (`amiga_syscalls.c`, `amiga_sendrecv.c`), library open/close/dispatch (`amiga_api.c`, `amiga_libcalls.c`, `amiga_generic*.c`), the resolver (`res_*.c`, `gethostnamadr.c`, `getxbyy.c`), address/interface helpers (`getifaddrs.c`, `if_name*`, `inet_*`), and the Roadshow/Miami compatibility APIs (`amiga_roadshow.c`, `miami_api.c`). |
| `kern/`     | Kernel glue and the socket layer: `uipc_socket*.c` (socket layer), `uipc_mbuf.c` (mbufs), `kern_malloc.c` (pooled allocator), `kern_synch.c` (`spl`/`tsleep`/`wakeup`), `subr_prf.c` (kernel `printf`/`log`), `amiga_main.c` (daemon init), configuration/ARexx/GUI/DHCP integration (`amiga_config.c`, `amiga_rexx.c`, `amiga_gui.c`, `amiga_dhcp.c`), name database (`amiga_netdb.c`), access control (`accesscontrol.c`). |
| `net/`      | Link and routing layer: routing (`route.c`, `radix.c`, `rtsock.c`), interfaces (`if.c`, `if_loop.c`), the SANA-II driver interface (`if_sana.c`, `sana2arp.c`, `sana2*.c`), BPF (`bpf*.c`), packet filter hooks (`pfil.c`, `ipfilter.c`), and the software-interrupt queue (`netisr.c`). |
| `netinet/`  | IPv4 and the (dual-stack) TCP engine: `ip_input.c`/`ip_output.c`/`ip_icmp.c`, `in.c`/`in_pcb.c`/`in_proto.c`, the TCP engine (`tcp_input.c`, `tcp_output.c`, `tcp_subr.c`, `tcp_usrreq.c`, `tcp_timer.c`), the SYN cache (`tcp_syncache.c`), congestion control (`tcp_cc_newreno.c`, `tcp_cc_cubic.c`, `tcp_cc.h`), UDP (`udp_usrreq.c`), raw IP (`raw_ip.c`), and the Internet checksum (`in_cksum.c` / `in_cksum_sse2.c`). |
| `netinet6/` | IPv6 (KAME-derived): `ip6_input.c`/`ip6_output.c`, `icmp6.c`, neighbour discovery (`nd6.c`, `nd6_nbr.c`, `nd6_rtr.c`), MLD (`mld6.c`), `in6.c`/`in6_pcb.c`/`in6_proto.c`, UDP6 (`udp6_usrreq.c`), and the IPv6 checksum (`in6_cksum.c` / `in6_cksum_sse2.c`). |
| `sys/`, `conf/` | Local system headers and the `conf.h` build configuration (`INET6`, debug gates, compiler macros). Shared protocol headers live in `AROS/workbench/network/common/include`. |

---

## 3. Runtime architecture

### 3.1 Library and API model

Each task that opens `bsdsocket.library` receives its own library base (`struct SocketBase`), a
per-opener structure holding that task's descriptor table (`dTable`), signal masks, `errno`
pointer, resolver state, and a private timer. A single `MasterSocketBase` tracks the aggregate
open count for expunge. Closing a base closes all sockets it still owns (`__CloseSocket`), which
may block for `SO_LINGER`.

The API layer (`amiga_syscalls.c`) implements the Berkeley calls: each builds the necessary
`mbuf`s (for example, an address `mbuf` with `m_len = addrlen` and `sa_len` set) and invokes the
socket-layer entry points (`sobind`, `soconnect`, `sosend`, `soreceive`, …), which in turn dispatch
to the protocol via `so->so_proto->pr_usrreq(so, PRU_*, …)`.

### 3.2 Concurrency and synchronization

The stack uses a **single global lock** (`syscall_semaphore`, a `SignalSemaphore`). Every entry
through the library API acquires it, so all protocol processing is serialized — there is no
parallelism across sockets or tasks. This is the AmiTCP model; it is simple and correct, and much
of the code's safety rests on this single-threaded assumption.

BSD interrupt-priority levels are emulated in `kern_synch.c`: `splnet`/`splimp`/`splx` manipulate a
single `spl_semaphore` mutex and a `spl_level` counter, so raising to any level effectively waits
for the current holder. There is exactly one held mutex whenever `spl_level > 0`, regardless of
nesting depth.

`tsleep`/`wakeup` implement blocking waits on a channel:

- Sleepers are enqueued on a hashed `sleep_queue` (keyed by wait channel) **before** the `spl`
  mutex is dropped, so there is no lost-wakeup window.
- `tsleep(p, chan, …)` takes the caller's `SocketBase p`, whose private `timerPort`/`tsleep_timer`
  provide the timeout. It must be called holding `syscall_semaphore`; it releases both the `spl`
  level and `syscall_semaphore` around the actual wait and reacquires them on return.
- The daemon task itself never `tsleep`s (a `DIAGNOSTIC` guard enforces this).

### 3.3 Memory management

**mbufs** (`uipc_mbuf.c`) back all packet and address storage. Allocation is bounded by a
compile-time configuration (`mbconf`): a memory cap of **4 MB** (1 MB on m68k) and a **2 KB**
cluster size, with mbufs and clusters allocated in chunks. Free lists are `splimp`-protected.

> **`M_WAIT` semantics.** The port does not honour the blocking `M_WAIT`/`M_WAITOK` flag —
> allocation returns `NULL` once the mbuf cap or the general pool is exhausted, unlike stock BSD
> where `M_WAIT` never fails. Socket-layer and protocol call sites therefore NULL-check every
> allocation and fail the operation with `ENOBUFS` rather than dereferencing `NULL` under memory
> pressure. This invariant must be maintained by new code.

**General allocations** (`kern_malloc.c`, the `MALLOC`/`bsd_malloc` family) come from a single Exec
memory pool (`CreatePool(MEMF_PUBLIC, …)`) serialized by `malloc_semaphore`; these can also return
`NULL` and are checked accordingly.

---

## 4. Protocol control blocks (PCB model)

A single `struct inpcb` (`common/include/netinet/in_pcb.h`) carries **both** address families:

- **IPv4:** `inp_laddr`/`inp_faddr` (`in_addr`), `inp_lport`/`inp_fport`, `inp_route`, `inp_ip`.
- **IPv6** (`#if INET6`): `inp_laddr6`/`inp_faddr6` (`in6_addr`), `in6p_hops`, `in6p_v6only`,
  `in6p_tclass`, `in6p_flags`, `in6p_moptions`. The IPv6 fields are placed at the **end** of the
  structure.

There is no `inp_vflag`; the family is derived from context (the socket's protocol domain, or the
IP version nibble of an inbound packet). `inp_pcbinfo` references the per-protocol table
`{ listhead, hashbase, hashsize, lastport }`. **TCP4 and TCP6 share** the `tcbinfo`/`tcb` table;
UDP shares `udb`.

### protosw wiring

- **IPv4** (`netinet/in_proto.c`, `inetsw`): TCP (`SOCK_STREAM`) → `tcp_input`/`tcp_usrreq`;
  UDP → `udp_input`/`udp_usrreq`.
- **IPv6** (`netinet6/in6_proto.c`, `inet6sw`): TCP6 (`SOCK_STREAM`) → the shared dual-stack
  `tcp_input`/`tcp_usrreq`; UDP6 → `udp6_input`/`udp6_usrreq` (a separate path).
- `ip6_input` demuxes with `pr = &inet6sw[ip6_protox[nxt]]; pr->pr_input(m, off = 40, nxt)` — the
  same vararg convention as IPv4, where the header offset is the first vararg.

---

## 5. TCP engine (dual-stack)

`tcp_input`/`tcp_output`/`tcp_subr`/`tcp_syncache` are built around `struct tcpiphdr`
(`ipovly[20]` + `tcphdr[20]` = 40 bytes). The engine is address-family aware: all header building
and the TCP state machine are shared, and only the family-specific edges branch on `isipv6`
(see §6).

- **`tcp_input(void *arg, …)`** detects the family from the IP version nibble. IPv4 performs the
  `ipovly` pseudo-header `in_cksum`; IPv6 verifies `in6_cksum` and normalises the packet to the
  `tcpiphdr` layout. The shared path then computes `off = ti_off << 2`, parses options, advances
  `m_data` past IP+TCP+options, and locates the PCB.
- **`tcp_output`** copies `tp->t_template` into the mbuf, fills the TCP fields/options, then calls
  IPv4 `ip_output` or IPv6 `tcp_v6output`. It panics if `tp->t_template == 0`, so `tcp_connect` and
  `syncache_socket` build the template before the first send.
- **`tcp_subr`** provides `tcp_template` (per-connection `tcpiphdr` template), `tcp_respond`
  (RST/ACK/keep-alive; dual-stack), `tcp_v6output` (shared IPv6 sender), `tcp_newtcpcb`/`tcp_close`,
  and the SYN-cookie generate/validate helpers.
- **`tcp_usrreq`** dispatches PRU_BIND/LISTEN/CONNECT/SOCKADDR/PEERADDR to the `in_*` (IPv4) or
  `in6_*` (IPv6) PCB helpers based on `tcp_isipv6(inp)`; `tcp_connect` has an IPv6 branch.

### 5.1 Congestion control

`tcp_cc.h` defines a pluggable congestion-control framework: a `struct tcp_cc_algo` vtable
(`on_ack`, `on_loss`, `on_rto`, `on_ecn`, …), a registry `tcp_cc_algos[TCP_CC_MAX]`, and a default
selector `tcp_cc_default`. Two algorithms are built in:

- **NewReno** (`tcp_cc_newreno.c`) — the RFC 5681/6582 baseline.
- **CUBIC** (`tcp_cc_cubic.c`) — RFC 8312, the **default** (`tcp_cc_default = TCP_CC_CUBIC`).

Each `tcpcb` selects an algorithm via `t_cc_algo`; `CC_ALGO(tp)` resolves it and falls back to
NewReno if unset. The initial congestion window follows **RFC 6928 IW10**
(`snd_cwnd = MIN(10 · mss, MAX(2 · mss, 14600))`).

### 5.2 SYN cache and SYN cookies

`tcp_syncache.c` holds half-open connections in a hash table rather than as real sockets on the
listen queue, with **stateless SYN cookies** as the overflow fallback, so a SYN flood cannot
exhaust socket/accept-queue resources.

- `syncache_add` (inbound SYN) → `syncache_respond` (SYN-ACK).
- `syncache_expand` (final ACK) → `syncache_socket` builds the `ESTABLISHED` socket.
- `syncache_chkrst` (RST); entries time out via `tcp_slowtimo`; the table is purged in `tcp_close`.

The SYN cache is fully dual-stack (see §6).

### 5.3 Timers

`tcp_timer.c` runs the fast (200 ms) and slow (500 ms) timers driven from the daemon: retransmit
(with exponential backoff), persist, keepalive, 2MSL/TIME-WAIT, and delayed-ACK. `tcp_slowtimo`
also advances the ISS clock and expires SYN-cache entries.

---

## 6. Dual-stack IPv6 (TCP6 / UDP6)

TCP6 and UDP6 operate end-to-end over `::1` and over any interface. TCP6 performs active and passive
open (handshake via the SYN cache), data transfer (including large transfers with window scaling),
orderly teardown, and connect-refused RST; UDP6 round-trips datagrams.

The TCP send/receive buffer is always assembled around `struct tcpiphdr` (the IPv4 `ipovly`+`tcphdr`
overlay), so header building and the state machine are shared; only the family edges branch on
`isipv6`.

- **Family detection.** For PCB operations, `tcp_isipv6(inp)` tests whether the socket's protocol
  domain is `AF_INET6` (correct for `sonewconn` children and syncache sockets). For an inbound
  segment, `tcp_input` reads the IP version nibble of the first header byte.
- **Shared v6 sender** `tcp_v6output(m, laddr6, faddr6, hlim, tlen, inp)` (`tcp_subr.c`): the mbuf
  must start at the TCP header. It `M_PREPEND`s and fills the `ip6_hdr` (writing `ip6_flow = 0`
  **before** `ip6_vfc`, which overlap in a union, so the version nibble survives), `m_pullup`s
  `ip6_hdr`+`tcphdr` contiguous so the checksum store lands in the real TCP header, computes
  `in6_cksum`, and calls `ip6_output`. Shared by `tcp_output`, `tcp_respond`, and
  `syncache_respond`.
- **`tcp_output` (v6)** slides the mbuf forward over the 20-byte `ipovly` so it starts at the TCP
  header, skips the IPv4 `in_cksum` (`ti_sum = 0`), then calls `tcp_v6output`.
- **`tcp_input` (v6)** pulls up `ip6_hdr`+`tcphdr`, saves `src6`/`dst6`, verifies `in6_cksum`, then
  normalises to the `tcpiphdr` layout (strips the 40-byte `ip6_hdr`, leaving a 20-byte `ipovly`
  overlay → net `m_data += 20`) so the rest of the routine runs unchanged. Family edges: PCB lookup
  (`in6_pcblookup(&tcb, …)`, one pass handles exact + wildcard), the access-control hook (IPv4
  only), the multicast guard, and `dropwithreset`.
- **`tcp_respond`** takes extra `(isipv6, laddr6, faddr6)` args and auto-detects v6 from `tp` when
  the caller passes `isipv6 = 0` (the keep-alive timer). `laddr6`/`faddr6` are in *our* send
  orientation (`dropwithreset` passes `&dst6, &src6`; the shared code still swaps the ports).
- **SYN cache (dual-stack).** `struct syncache` carries `sc_isipv6` + `sc_faddr6`/`sc_laddr6`, and a
  `struct sc_key` (via `syncache_key`) abstracts the family for hash/lookup/fill.
  `syncache_respond` builds the v6 SYN-ACK via `tcp_v6output`. `syncache_socket` sets the v6
  four-tuple directly (`inp_laddr6`/`faddr6`/`lport`/`fport`) — v6 PCBs are found by a linear scan
  of `tcb`, so there is no `in_pcbrehash`/`in_pcbconnect`. SYN cookies fold the 128-bit addresses to
  32 bits (`syncache_v6fold`) for the v4-shaped cookie hash (consistent because those addresses are
  also present on the return ACK). `syncache_add`/`expand`/`chkrst` take `(isipv6, src6, dst6)`.
- **Source address selection.** `in6_pcbconnect` selects a source via `rtalloc()` +
  `in6_ifawithifp()` (mirroring `udp6_output`); an unbound socket must not connect from the
  unspecified address (`::`), which `ip6_input` discards on receipt (RFC 4291 scope).
- **Port allocation.** `in6_pcballoclport` (IPPORT range + `in6_pcblookup` collision check) is used
  for both wildcard and explicit-address binds. An explicit `{addr, port}` bind that collides with
  an existing PCB is rejected with `EADDRINUSE` unless `SO_REUSEADDR`/`SO_REUSEPORT` is set.
- **UDP6** is a separate path (`udp6_usrreq`/`udp6_output`/`udp6_input`) reusing the shared `inpcb`.
  `if_loop.c loconfig()` configures `lo0` with `::1` (and `127.0.0.1`); `::1` is in `in6_ifaddr` so
  `ip6_input` treats it as "ours". `IPV6_V6ONLY` and `IPV6_UNICAST_HOPS` round-trip.

### 6.1 Neighbour Discovery

The IPv6 ND engine (RFC 4861: NS/NA resolution, RS/RA, DAD, NUD) is multicast-based and runs over a
real link layer; it is not exercised by loopback. `nd6_nbr.c` handles NS/NA/DAD, `icmp6.c`
dispatches ICMPv6, and `in6.c` performs link-local autoconfiguration and joins the solicited-node
group via `in6_addmulti` → `S2_ADDMULTICASTADDRESS`, mapping `ff02::X` → `33:33:XX:XX:XX:XX`.
`nd6_lookup` returns a neighbour route with its reference **held** (KAME semantics); every caller
`rtfree()`s it when done.

---

## 7. IPv4, ICMP, and UDP

- **IPv4** (`ip_input.c`/`ip_output.c`) provides input demux, forwarding, fragmentation and
  reassembly, and options handling.
- **ICMP** (`ip_icmp.c`) generates and processes error and informational messages. Error generation
  is rate-limited by a token bucket (`icmp_ratelimit`): a burst of 10 tokens refills at 5 tokens per
  500 ms slow-tick, keyed off the `tcp_now` clock; suppressed errors are counted in
  `icmpstat.icps_ratelimit`, bounding ICMP error amplification.
- **UDP** (`udp_usrreq.c`) is the standard datagram path over the shared `udb` table.

---

## 8. Checksums

The Internet checksum has two implementations selected by CPU at build time (mmakefile `ifeq` on
`AROS_TARGET_CPU`):

- **x86_64:** SSE2-optimised (`in_cksum_sse2.c`, `in6_cksum_sse2.c`) — a vectorised ones-complement
  sum with explicit inter-mbuf odd-byte handling. On x86_64 this is the only implementation built.
- **All other architectures** (including m68k): the portable `in_cksum.c` / `in6_cksum.c`.

Both variants produce identical results across even, odd-length, odd-start-address, and multi-mbuf
inputs. Because `struct ip6_hdr` is packed, `in6_cksum` copies `ip6_src`/`ip6_dst` into aligned
locals before summing rather than reading them through a `u_int16_t *` taken from the header.

---

## 9. Routing and link layer

- **Routing** uses the classic BSD radix tree (`radix.c`) with `route.c` (`rtalloc`/`rtrequest`/
  `rtfree` reference counting) and the `PF_ROUTE` socket interface (`rtsock.c`).
- **Interfaces** are managed in `if.c`; `if_loop.c` provides the loopback `lo0`.
- **Link layer** is SANA-II. `if_sana.c` binds each configured interface to a SANA-II device
  (`OpenDevice`, `S2_DEVICEQUERY`/`S2_GETSTATIONADDRESS`, buffer-management callbacks), maps
  SANA-II wire types to RFC 1573 interface types, and drives TX/RX. `sana2arp.c` implements IPv4
  ARP with a **per-interface** ARP table (each `sana_softc` has its own `ss_arp.table` under
  `ARPTAB_LOCK`).

---

## 10. Robustness and security properties

Properties the implementation provides:

- **SYN-flood resistance** via the SYN cache plus stateless cookies (§5.2).
- **ICMP error rate limiting** via a token bucket (§7).
- **Memory-pressure safety.** Because `M_WAIT` can fail (§3.3), socket-layer and protocol
  allocation sites NULL-check and return `ENOBUFS` rather than dereferencing `NULL`.
- **Input bounds validation.** Header and length fields are validated against the actual packet
  bounds before use: the IPv6 extension-header walk bounds the running offset and caps the header
  count (`ip6_input.c`), UDP6 validates `uh_ulen` before using it as the checksum span
  (`udp6_usrreq.c`), TCP option parsing bounds each option length against the remaining bytes
  (`tcp_dooptions`), and the resolver bounds DNS answer parsing (`getanswer`: `MAXADDRS`/`eom`
  guards) and query construction (`res_mkquery`).
- **IPv6 fragmentation** allocates each output fragment from an mbuf **cluster** (`MCLGET`) sized
  for the header plus payload; payload is never copied into a bare `MHLEN` mbuf.

> **Single-lock dependency.** Several data structures are safe only under the global
> `syscall_semaphore` (§3.2) — for example `radix.c`'s shared `maskedKey` scratch and the cluster
> reference count. Finer-grained locking would have to revisit these.

---

## 11. Current limitations

- **No concurrency.** All API calls serialize on the global `syscall_semaphore` (§3.2); there is no
  parallelism across sockets or CPUs and no SMP support. Throughput is bounded by single-threaded
  protocol processing.
- **Non-blocking allocation.** `M_WAIT`/`M_WAITOK` do not block (§3.3); allocations fail with
  `ENOBUFS` once the mbuf cap (4 MB; 1 MB on m68k) or the general pool is exhausted rather than
  waiting for memory to be freed.
- **IPv6 output fragmentation is bounded by the cluster size.** Each fragment is built in a single
  ~2 KB mbuf cluster, so a fragment's payload cannot exceed `MCLBYTES`. This covers standard link
  MTUs (Ethernet/SANA-II) but not jumbo-frame IPv6.
- **x86_64 checksum requires SSE2.** Only the SSE2 checksum is compiled on x86_64, with no scalar
  fallback. SSE2 is baseline on all x86_64 CPUs, so this is not a practical restriction.
- **Unimplemented API surface.** Several Roadshow interface-management entrypoints
  (`AddInterfaceTagList`, `ConfigureInterfaceTagList`, `ObtainInterfaceList`, …) and Miami calls
  (`MiamiSysCtl`, `MiamiGetHardwareLen`, `sockatmark`, …) are stubs that log "not implemented".

---

## 12. Include-ordering constraints

`struct inpcb`'s IPv6 members are under `#if INET6`, defined only via `conf/conf.h` (pulled in by
the `<conf.h>` wrapper, which then includes `in_pcb.h` with `INET6` set). Therefore any TCP file
that touches the v6 `inpcb` fields must include `<conf.h>` **before** `<netinet/in_pcb.h>`.

In `tcp_output.c`, `<conf.h>` also transitively includes `<netinet/tcp_fsm.h>`, so
`#define TCPOUTFLAGS` must appear **before** `#include <conf.h>` or `tcp_outflags[]` is left
undeclared. `tcp_input.c` and `tcp_subr.c` do not use `tcp_outflags`, so they simply include
`conf.h` first.

`tcp_iss` is typed `tcp_seq` (unsigned); its definition and the `extern` in `tcp_compat.h` are kept
in lockstep.

---

## 13. Building and testing

### Build

- Stack only, from the build directory (e.g. `build-linux-x86_64-gcc16`):
  `make workbench-network-stacks-arostcp-bsdsocket-quick`.
- After editing shared headers (`common/include/…`) run `make network-includes-copy` first.
- Debug tracing: `D(bug(...))` is gated by a per-file `DEBUG` that `conf.h` forces to 0. For
  ad-hoc tracing use an unconditional `bug("...")`, which reaches the hosted `AROSBootstrap`
  stderr.

### Test

- **cunit socket tests:** `developer/debug/test/net/socket/` (`socktest.h` helpers;
  `cunit-net-{socket,tcp,udp,inet6}.c`; `tapecho.c` echo server; `v6diag.c` diagnostic). Build with
  `make test-socket-cunit` (the non-cunit `test-socket` builds only `v6diag`/`tapecho`).
- **Hosted run:** the AROSTCP daemon must be running (it provides `bsdsocket.library` and
  auto-configures `lo0`/`127.0.0.1`/`::1`). Install a focused `S/User-Startup` that `Run`s
  `SYS:System/Network/AROSTCP/C/AROSTCP`, `WaitForPort AROSTCP`, runs the test, then `Shutdown`;
  launch with `timeout N ./boot/linux/AROSBootstrap` from `bin/linux-x86_64/AROS/` (with `DISPLAY`
  set). Restore the original `User-Startup` afterwards.
- **External networking / ND:** requires a pre-created persistent user-owned TAP (`sudo`); the tap
  device's `TUNSETIFF` needs `CAP_NET_ADMIN`. ND can be exercised by running AROS inside an
  unprivileged user+net namespace (`unshare -Urn`) attached to a TAP, with an `AF_PACKET` sniffer.
