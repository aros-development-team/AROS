# mdnsd and bonami.library

`mdnsd` is a multicast DNS responder (RFC 6762) for AROSTCP. It answers for
the host's name in the mDNS domain, `<hostname>.local`, on every interface
that can multicast, so that other machines on the link find this one by name
without a DNS server: `ping amiga.local`, `ssh amiga.local`.

It also answers for the services that programs on this host offer (DNS-SD,
RFC 6763), which they register through `bonami.library`, so that other
machines find a printer, a file share or a MIDI session without being told
an address or a port.

It also browses for services and resolves them asynchronously, through a
queue per operation with an optional wakeup by a lent request message. See
[DISCOVERY.md](DISCOVERY.md) for the API, lifetime contract and limits.
It can also list the service types present on the link (RFC 6763 section
9). Generic host lookup and record queries remain separate.

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
- **Answers.** An `A` or `ANY` question for the name is answered with the
  address of the interface it came in on. A question from another port than
  5353, from a resolver that knows plain DNS, is answered to that port alone
  with its ID and question, a time to live of ten seconds and no cache-flush
  bit (section 6.7). A question with the unicast-response bit is answered to
  the asker alone when the group had the record within the last 30 seconds,
  and to the group otherwise (section 5.4).
- **Interfaces.** One UDP 5353 socket for each interface that is up, can
  multicast and has an IPv4 address, bound to it with `SO_BINDTODEVICE`. The
  interfaces are looked at every three seconds; one that came, went or
  changed its address makes mdnsd probe again.
- **Goodbye.** When it stops, or lets go of the stack, it withdraws the
  record with a time to live of zero.

## Services

- **The records.** A registered service is an instance name under a type,
  `Printer._ipp._tcp.local`, with a `PTR` record from the type to the
  instance, the instance's `SRV` (this host's name and the port) and `TXT`,
  and the host's address. Its own records live 75 minutes and the `SRV` two,
  or what its client asked for.
- **The name** is asked for and announced as the host's is, and waits for
  the host's name to be settled. When another host has it the service
  becomes `Printer (2)`, then `Printer (3)`; when two hosts ask at the same
  moment the `SRV` records are compared (section 8.2).
- **Answers.** A question for the type is answered after 20 to 120 ms,
  picked at random, with every instance of the type in one response and the
  host's address once (section 6); an instance the asker lists as known is
  left out (section 7.1). A question for one instance is answered at once.
  `_services._dns-sd._udp.local` is answered with the types there are.
  Plain DNS resolvers and the unicast-response bit are treated as for the
  host's name.
- **Another host's service.** A client may register a service with a host
  name and an address of its own, for a machine that does not speak mDNS:
  mdnsd then answers for that name too.
- **Clients that go away.** With a task's last `CloseLibrary()` what it
  registered is withdrawn. A task that ends without closing is noticed
  within a few seconds through its stable Exec task ID. `FindTaskByPID()`
  checks running/spinning tasks on SMP as well as ready/waiting tasks;
  a new task reusing an old task's address does not inherit its services.

## bonami.library

The library holds no state: each call is a message to mdnsd's public port,
and the reply is waited for. Without mdnsd the calls return
`MDNSERR_NOENGINE`.

| Function | What it does |
|---|---|
| `OpenMDNSEngine()` | says whether mdnsd is running |
| `CloseMDNSEngine()` | withdraws everything the calling task registered |
| `RegisterServiceA(tags)` | offers a service; returns at once |
| `UpdateServiceA(ref, tags)` | changes its text or port |
| `DeregisterService(ref)` | withdraws it |

Some upstream ABI slots remain compatibility stubs.
`MakeTXTRecordA`, `GetMDNSStatusA`,
`InquiryFromNIPCTagsA` and `PublishNIPCEntitiesA` return `MDNSERR_NOTREADY`.

Version 41.3 also provides `LookupHostA()` and queued generic record queries:
`QueryRecordA()` / `StopQuery()`, `ReadMDNSRecordEvent()` and the existing
`RequestMDNSEvent()` wakeup. See `DISCOVERY.md` for ownership, record format,
bounds and restart behavior. Hook delivery remains deliberately unavailable;
queries do not execute client code in mdnsd.
`FreeTXTRecord` and `UnpublishNIPCEntitiesA`
do nothing. They invoke no callbacks, change no output buffers and take
ownership of no memory. Public headers include the upstream handles,
events, flags, service snapshots, tags and Envoy definitions. These
definitions preserve source compatibility; only the registration tags
documented by the implemented functions are acted on in this release.

### Task lifetime and compatibility

The first fifteen slots, their order/registers and existing tag/error values
follow upstream Bonami. The AROS implementation is version 41.2. Its daemon
and library must be installed together: their private IPC is now version 3.

Registration lifetime deliberately differs from upstream Bonami:

- `CloseMDNSEngine()` withdraws the calling task's registrations; it does
  not merely drop an engine reference and does not stop the system daemon.
- Only the registering task can update or deregister its service. Another
  task's update returns `MDNSERR_BADPARAM`; its deregister call does nothing.
  Passing a reference to a main task does not transfer ownership.
- A duplicate local instance name/type returns `MDNSERR_DUPLICATE`.
- The registering task's final library close or termination withdraws its
  services, even while other tasks of the program still have the library
  open. Keep the registering task alive for as long as publication is wanted.

Instance names are one DNS label (at most 63 bytes). Literal dots are
rejected with `MDNSERR_BADPARAM` until presentation escaping is supported.
Host names supplied for proxies must be a single label or a .local name,
and must differ from this host's current and base names. Proxy host names
are not independently probed; the client supplies a name/address it may
advertise. A proxy name and address must be supplied together.

`MDNS_TimeToLive` is accepted on registration and update in the range
1..4500 seconds. An invalid TTL rejects the update before applying changes.

```c
#include <proto/bonami.h>

MDNSRegisterRef ref = NULL;
struct TagItem tags[] = {
    { MDNS_ServiceName, (IPTR)"Living Room" },
    { MDNS_ServiceType, (IPTR)"_http._tcp" },
    { MDNS_Port, 80 },
    { MDNS_TXTRecord, (IPTR)"path=/ note=two\\ words" },
    { MDNS_RegisterRef, (IPTR)&ref },
    { TAG_END, 0 }
};

if (RegisterServiceA(tags) == MDNS_OK) {
    /* ... */
    DeregisterService(ref);
}
```

The text is `key=value` items with a space between them; a space inside an
item is written with a backslash before it.

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

- IPv4 only: no `AAAA` record, and nothing is sent to `ff02::fb`.
- The number after a name that was lost is not remembered when mdnsd
  restarts: it starts from the system's name again.
- At most eight interfaces.
- A service's text is at most 256 characters, its instance name 63 bytes.
- A question for the service types is not delayed, known answers that
  come in a second packet are not waited for, and a record that went to
  the group less than a second ago is sent again all the same.
- It needs a stack that delivers IPv4 multicast to a socket that joined a
  group, and a SANA-II driver that accepts `S2_ADDMULTICASTADDRESS` and
  `S2_MULTICAST`.

## Files

| File | What it holds |
|---|---|
| `mdnsd.c` | the main loop: signals, sockets, time |
| `mdnsd_service.c` | registration with `netservices.library` |
| `mdns_net.c` | the interfaces and their sockets, the name in use |
| `mdns_host.c` | the host's name: probing, announcing, answering, conflicts |
| `mdns_reg.c` | registered services: probing, announcing, answering |
| `mdns_client.c` | the library's messages; clients that went away |
| `mdns_packet.c` | takes a received packet apart |
| `mdns_dns.c` | the DNS wire format |
| `mdns_ipc.h` | the messages between the library and mdnsd |
| `library/` | `bonami.library` |
| `include/` | `<bonami/mdns.h>`, `<libraries/bonamibase.h>` |
| `services.d/mdns` | the service manager's entry |

Derived from [amigazen Bonami](https://github.com/amigazen/Bonami), under
its BSD 2-Clause license; see `LICENSE`.
