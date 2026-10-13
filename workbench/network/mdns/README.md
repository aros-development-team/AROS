# mdnsd

A multicast DNS responder (RFC 6762) for AROSTCP. It answers for the host's
name in the mDNS domain, `<hostname>.local`, on every interface that can
multicast, so that other machines on the link find this one by name without
a DNS server: `ping amiga.local`, `ssh amiga.local`.

This is the first part of a system mDNS/DNS-SD service. It only speaks for
the host's own name and address. Registering and browsing services, and a
client library, are to follow.

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
| `mdns_packet.c` | takes a received packet apart |
| `mdns_dns.c` | the DNS wire format |
| `services.d/mdns` | the service manager's entry |

Derived from [amigazen Bonami](https://github.com/amigazen/Bonami), under
its BSD 2-Clause license; see `LICENSE`.
