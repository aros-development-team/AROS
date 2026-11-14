# mdnsd and dnssd.library

`mdnsd` is a dual-stack multicast DNS responder (RFC 6762) for AROSTCP. It answers for
the host's name in the mDNS domain, `<hostname>.local`, on every interface
that can multicast, so that other machines on the link find this one by name
without a DNS server: `ping amiga.local`, `ssh amiga.local`.

It also answers for the services that programs on this host offer (DNS-SD,
RFC 6763), which they register through `dnssd.library`, so that other
machines find a printer, a file share or a MIDI session without being told
an address or a port; and it finds the services of other machines for
them: browse, resolve, address lookup and record questions, answered from
one cache of what it hears on the links.

## What it does

- **The name.** The first label of what the system calls itself
  (`gethostname()`), in `.local`. Before using it mdnsd asks the link for it
  three times, 250 ms apart, with the address record it would publish in the
  authority section, and announces it twice when nobody objects.
- **Conflicts.** If another host answers a probe, or later sends an address
  record for the name with an address that is not this host's, mdnsd takes
  `name-2`, then `name-3`, and probes for that. When two hosts probe for the
  same name at the same moment, the one whose address sorts earlier waits a
  second and probes again (section 8.2).
- **Answers.** An `A`, `AAAA` or `ANY` question for the name is answered with
  the addresses of the interface it came in on. A question from another port than
  5353, from a resolver that knows plain DNS, is answered to that port alone
  with its ID and question, a time to live of ten seconds and no cache-flush
  bit (section 6.7). A question with the unicast-response bit is answered to
  the asker alone when the group had the record within the last 30 seconds,
  and to the group otherwise (section 5.4).
- **Interfaces.** One IPv4 and one IPv6 UDP 5353 socket for every usable
  link. `IP_PKTINFO` and `IPV6_PKTINFO` name the link a datagram came by,
  and each answer leaves by the link that was asked, from that link's
  address. Tentative, duplicate, detached and deprecated IPv6 addresses are
  not used. AROSTCP signals mdnsd when an address changes, and a
  three-second scan remains as a fallback; a link that came, went or changed
  its usable address makes mdnsd probe again.
- **Goodbye.** When it stops, or lets go of the stack, it withdraws the
  record with a time to live of zero.

## Services

- **The records.** A registered service is an instance name under a type,
  `Printer._ipp._tcp.local`, with a `PTR` record from the type to the
  instance, the instance's `SRV` (this host's name and the port) and `TXT`,
  and the host's address. Its own records live 75 minutes and the `SRV` two,
  or what its client asked for (`DNSSD_TimeToLive`).
- **The registration** is kept as DNS has it (`dnssd_reg.h`): the
  instance's own label (UTF-8, dots allowed), type and domain as wire names,
  subtypes as labels, and the names its records carry built once, compared
  with what comes in as wire names.
- **Subtypes.** A registration may have up to eight subtype labels. Each
  publishes a shared PTR such as `_printer._sub._ipp._tcp.local` to the
  ordinary instance; SRV, TXT and addresses remain the base registration's.
- **The name** is asked for and announced as the host's is, and waits for
  the host's name to be settled. When another host has it the service
  becomes `Printer (2)`, then `Printer (3)`; when two hosts ask at the same
  moment their records are compared (section 8.2).
- **Answers.** A question for the type is answered after 20 to 120 ms,
  picked at random, with every instance of the type in one response and the
  host's address once (section 6); an instance the asker lists as known is
  left out (section 7.1). A question for one instance is answered at once.
  `_services._dns-sd._udp.local` is answered with the types there are.
  Plain DNS resolvers and the unicast-response bit are treated as for the
  host's name.
- **Another host's service.** A client may register a service for a machine
  that does not speak mDNS, with that host's name and addresses, IPv4 and
  IPv6: their A and AAAA records are probed, announced, answered and
  defended with the service's.
- **Clients that go away.** A session's services are withdrawn, with their
  goodbyes, when it is closed or its task ends: `task.resource` tells mdnsd
  at once (`TNA_REMOVED`), and a look by stable Exec task ID every few
  seconds remains as recovery.

## The cache

Every answer mdnsd hears goes into one cache (`mdns_cache.c`), names in it
as uncompressed wire labels, within a budget (24 KB on the 68k, 128 KB
elsewhere). What an operation wants is a watch on it: a question asked on the
links as RFC 6762 says - first within 20-120 ms, then after 1, 2, 4 s... up
to an hour - with the answers already known, and refreshed at 80-95 % of
each watched record's life. The questions due on a link at one time go in
one query. A record a cache-flush answer replaces, or a goodbye, goes a
second later. When the budget is full, a record nobody watches goes first.

## dnssd.library

A program opens a session with a message port of its own; each request is
an operation of the session. mdnsd keeps the results, puts a message on the
port when there are some, and the program reads them one event at a time
into a buffer of its own. No pointer of the daemon's is left with the
program, nothing arrives for an operation once it is cancelled, and every
request is checked, over the bytes it has, before mdnsd reads it.

| Function | What it does |
|---|---|
| `DNSSD_OpenSessionA()`, `DNSSD_CloseSession()` | a session, and its end (its operations with it) |
| `DNSSD_ReadEvent()` | the session's next event |
| `DNSSD_BrowseA()` | the instances of a type, or the types on the link |
| `DNSSD_ResolveA()` | an instance's host, port, TXT and addresses; with `DNSSD_Timeout` 0 followed until cancelled |
| `DNSSD_LookupAddressA()`, `DNSSD_LookupAddressSyncA()` | a host's addresses |
| `DNSSD_QueryRecordA()` | any record |
| `DNSSD_RegisterA()`, `DNSSD_UpdateA()` | publishes a service, and changes it |
| `DNSSD_Cancel()` | ends an operation |
| `DNSSD_NameToText()`, `DNSSD_OperationID()`, `DNSSD_GetStatusA()` | helpers |

Each function's AutoDoc is in `dnssd/dnssd_api.c`; the events and tags in
`<dnssd/dnssd.h>`.

```c
#include <proto/dnssd.h>

static const UBYTE txt[] = { 6, 'p','a','t','h','=','/' };
DNSSDSessionRef session;
DNSSDOperationRef op;

if (DNSSD_OpenSessionA(&session, port, NULL) == DNSSD_OK &&
    DNSSD_Register(session, "Living Room", "_http._tcp", 80, &op,
        DNSSD_TXT, (IPTR)txt, DNSSD_TXTLength, sizeof(txt), TAG_DONE) == DNSSD_OK) {
    /* DNSSD_EVENT_REGISTERED says the name it has; until ... */
    DNSSD_CloseSession(session);
}
```

## The tools

In `C:`, on `dnssd.library` (`tools/`, each with its AutoDoc):

- `dnssd` in the manner of Apple's `dns-sd` and Avahi's `avahi-browse`:
  browse (`-B`, `-Z`), resolve (`-L`), register (`-R`, `-P` for another
  host), record questions (`-q`) and lookups (`-G`).
- `Nearby`, `Share` and `Handshake`, which say the same in plain words:
  what is on the network, advertise a service, reach one.

A browse in `dnssd` or `Nearby` follows what it finds: up to 16 instances
are resolved until cancelled, and a change of their port, TXT or addresses
is shown as it happens; others are resolved four at a time and followed
when there is room.

## How it runs

mdnsd is a managed service of AROSTCP. The service manager starts it from
`db/services.d/mdns` and stops it with the stack. It registers with
`netservices.library` and is signalled around a reconfiguration: at the
start it says goodbye and closes `bsdsocket.library`, at the end it opens it
again and probes on the interfaces it finds. The stack usually finishes a
reload before mdnsd runs, so both signals arrive together and it closes and
opens in one go.

Started by hand it runs until Ctrl-C.

## Limits

- One usable IPv6 link-local address is selected per interface.
- The number after a name that was lost is not remembered when mdnsd
  restarts: it starts from the system's name again.
- At most 32 interfaces.
- A service's TXT data is at most 8900 bytes (RFC 6763 section 6.2 asks for
  1300 where it can), its instance name 63 bytes.
- Known answers that come in a second packet are not waited for.
- It needs a stack that delivers IPv4/IPv6 multicast to sockets that joined
  their groups, and a SANA-II driver that accepts the corresponding multicast
  MAC registrations.

## Files

| File | What it holds |
|---|---|
| `mdnsd.c` | the main loop: signals, sockets, time |
| `mdnsd_service.c` | registration with `netservices.library` |
| `mdns_net.c`, `dnssd_link*.c` | the links, their addresses and sockets, the name in use |
| `mdns_host.c` | the host's name: probing, announcing, answering, conflicts |
| `mdns_reg.c`, `dnssd_reg.c` | registered services: probing, announcing, answering |
| `mdns_cache.c` | the cache and the questions that keep it |
| `mdns_packet.c`, `mdns_dns.c` | received packets; the DNS wire format |
| `dnssd_server.c` | `dnssd.library`'s sessions and operations |
| `dnssd_ipc.c`, `dnssd_ipc.h`, `dnssd_ipc_port.h` | the requests between the library and mdnsd |
| `dnssd_model.c` | the events, as they are copied to a program |
| `dnssd/` | `dnssd.library` |
| `include/` | `<dnssd/dnssd.h>` |
| `tools/` | `dnssd`, `Nearby`, `Share`, `Handshake` |
| `services.d/mdns` | the service manager's entry |

Derived from [amigazen Bonami](https://github.com/amigazen/Bonami), under
its BSD 2-Clause license; see `LICENSE`.
