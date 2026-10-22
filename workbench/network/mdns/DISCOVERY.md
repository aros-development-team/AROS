# Queued DNS-SD discovery

bonami.library and mdnsd implement IPv4 service browse and asynchronous
resolve, without running client hooks in mdnsd or starting dispatch
processes. Each operation has one event queue, read in the
caller's own task. A caller either reads it from a loop it already runs, or
lends mdnsd a message that comes back when there is something to read. The
original fifteen vectors keep their offsets and signatures. Vectors 16 and
17 (LVO 20 and 21 after the four standard entries) append
`ReadMDNSEvent(ref, event)` and `RequestMDNSEvent(ref, request)`. Library
version is 41.2, private IPC version 3; the library and daemon must be
installed together.

## Calls and ownership

```c
#include <proto/bonami.h>
#include <bonami/mdns.h>

MDNSBrowseRef ref = NULL;
struct MDNSDiscoveryEvent event;
struct TagItem tags[] = {
    { MDNS_EventMode, MDNS_EVENT_QUEUE },
    { MDNS_BrowseRef, (IPTR)&ref },
    { TAG_DONE, 0 }
};
LONG rc = BrowseServiceTypeA("_apple-midi._udp", NULL, tags);
/* From the same task's own loop, while rc == MDNS_OK: */
rc = ReadMDNSEvent(ref, &event);
/* MDNS_OK: a complete snapshot. MDNSERR_NOTREADY: none waiting. */
StopBrowse(ref);
```

Reading needs nothing else: a task that already wakes for its own timers can
read in that loop. A task that should sleep until there is something to read
lends a request:

```c
struct MsgPort *port = CreateMsgPort();
struct MDNSEventRequest request;
BOOL lent;

request.er_Msg.mn_ReplyPort = port;
request.er_Msg.mn_Length = sizeof(request);
lent = RequestMDNSEvent(ref, &request) == MDNS_OK;
while (lent) {
    Wait(1UL << port->mp_SigBit | other_signals);
    if (GetMsg(port) == &request.er_Msg) {
        lent = FALSE;
        if (request.er_Result == MDNS_OK) {
            while (ReadMDNSEvent(ref, &event) == MDNS_OK) {
                /* ... */
            }
            lent = RequestMDNSEvent(ref, &request) == MDNS_OK;
        }
    }
    /* ... other signals; to end: */
}
StopBrowse(ref);
if (lent) {
    WaitPort(port);     /* already replied: returns at once */
    GetMsg(port);
}
DeleteMsgPort(port);
```

`ResolveServiceA(name, type, NULL, tags)` uses `MDNS_ResolveRef` instead.
It issues SRV/TXT queries, follows the target with an A query and reports
`MEVENT_RESOLVED` or `MEVENT_RESOLVE_FAILED` after five seconds. It does not
require a prior browse/PTR. Stop the operation with `StopResolve()` after
either result. Browse automatically resolves its instances and only emits
complete snapshots, using `MEVENT_ADD` and `MEVENT_REMOVE`.

The queue-mode tag and an output reference are required. Non-NULL hooks and
calls without the mode return `MDNSERR_NOTREADY`; no hook is dereferenced.
Only `.local` TCP/UDP service types are supported. Nondefault flags and
interface selection are rejected. No generic host lookup, record query,
service-type enumeration or Envoy bridge is introduced in this slice.

Each operation belongs to its creating Exec task ID. Another task cannot
read, request or stop it. Stop removes both queued events and the operation
before replying, so afterwards its reference cannot deliver another event.
Close of the engine/the task's last library close releases all operations;
the daemon's five-second reap timer removes those of a task that ended
without stopping them, found with `FindTaskByPID()`. A client may read
slowly, exit or fail without executing anything in mdnsd.

## Optional wakeup: a lent request

`RequestMDNSEvent()` follows the device I/O model. On `MDNS_OK` the
`MDNSEventRequest` belongs to mdnsd until it is replied to its
`mn_ReplyPort`, which happens exactly once; on any other result it was
never taken. The caller sets `mn_ReplyPort` and `mn_Length`; mdnsd writes
only `er_Result` and the message header, through `ReplyMsg()`. It never
calls client code and never signals a task directly.

- `MDNS_OK`: events are waiting. A reply stands for any number of them, so
  read until `MDNSERR_NOTREADY`, then lend the request again. A request lent
  while events are already waiting is replied at once.
- `MDNSERR_BUSY`: the queue overflowed (see below).
- `MDNSERR_CANCELLED`: `StopBrowse()`/`StopResolve()` stopped the operation.
- `MDNSERR_NOENGINE`: mdnsd ended.

Each operation holds at most one lent request; lending a second one returns
`MDNSERR_BADPARAM`. mdnsd handles a stop and an event in the order they
reach it, so there is no window in which both apply: either the event
replied first and the stop finds nothing lent, or the stop replies with
`MDNSERR_CANCELLED`. In both cases the request is back on the caller's port
before the stop call returns. When mdnsd ends it replies every lent request
with `MDNSERR_NOENGINE` before it removes its public port, so a caller that
finds mdnsd gone never waits for one.

As with a device, a caller must stop each operation and collect its request
before it deletes the port or frees the request. That is the contract, not
something mdnsd can enforce: Exec gives no way to know that a port is still
valid. Two cases are handled on a best-effort basis only. A request lent by
a task that has ended, or that closes the library with it still lent, is
dropped and never replied. A task that ends between mdnsd's check and its
reply is not protected against. A daemon that crashes cannot reply at all.

References are opaque local IDs, not allocation addresses. Each includes a
library-side association with the daemon's Exec task ID. If mdnsd restarts,
a read returns `MDNSERR_NOENGINE` even if the replacement daemon allocated
the same numerical remote handle. Stop remains safe. Discard old discovery
state and references and register/browse again; an existence check alone
cannot detect this situation. Registration handle incarnation remains the
parent's separate limitation.

No API changes a supplied output reference/event on failure. Read returns
`MDNSERR_BADPARAM` for a stopped/foreign reference, `MDNSERR_NOTREADY` when
its queue is empty, and `MDNSERR_BUSY` after queue/cache exhaustion. A busy
subscription is terminal: stop it, discard the client's cached view and
resubscribe. It never silently drops a removal while pretending the event
stream is complete. These are snapshots: an unconsumed update for the same
name/type/interface replaces its earlier queued snapshot. When an instance
changes while its removal is still unread, the caller sees only the newest
state: the queued removal is replaced by the addition that follows it.
Consumers must replace entries by that identity rather than count
ADD/REMOVE pairs.

## Data and wire behavior

`MDNSDiscoveryEvent` has no pointers. It contains the instance's single UTF-8
label (up to 63 bytes), service type, one IPv4 address, interface IPv4 address,
port and counted **wire-format TXT** (up to 512 bytes). Addresses are in network
byte order, port in host order. TXT is length-prefixed strings, may contain
binary NUL, and is not a C string or escaped text. This carries the MIDI 2.0
98-byte endpoint and 42-byte product identifier without escaped-text expansion.
TXT key matching belongs to the consumer and must be ASCII case-insensitive.

DNS compression is expanded with bounded traversal. Label boundaries remain
counted, including literal dots/backslashes in the instance; the registration
API's existing dot restriction does not apply to received discovery labels.
Instances containing embedded NUL cannot be represented and are rejected.

The cache is keyed by interface, owner/type and data, and holds only what an
active operation can reach: the PTRs of a browsed type, the SRV and TXT of an
instance of a browsed type or of an instance being resolved, and the A of a
host such an SRV names. Records of services nobody asked for are not cached,
refreshed or queried for (RFC 6762 section 5.2). An A record that arrives
before any SRV naming its host is held for one second (eight at most) and
cached when that SRV comes; an older one is asked for again by the
missing-record query. When the cache is full, the oldest
record no operation reaches any more gives way; only if every record is
still reached are the operations that want the new record failed with
`MDNSERR_BUSY`. PTR, SRV, TXT and A have independent expirations and refresh
queries. Browsing retries exponentially,
up to one hour, and queries list known answers with enough remaining TTL.
Missing components are queried separately. Goodbye and cache-flush handling
allow a one-second rescue period; recently received members of an RRset are
protected from a flush. A snapshot becomes unavailable if any required record
expires. Apart from an A that comes more than a second before its SRV,
arrival order does not matter. Network changes clear the old views,
report removals and restart queries. Resolve is a one-shot result; browse
tracks subsequent changes.

## Deliberate bounds and open review points

Lazy cache allocation holds 128 records, 32 service views and 16 operations
(at most four per task), with eight pending snapshots per operation. Views
exist only for instances of browsed types; more than 32 of them fail the
browses of the type that overflowed, once, and no other operation. The views
are rebuilt only when the cache or the operations change or a record
expires, not on every pass of mdnsd's loop. TXT is
limited to the parent parser's 512-byte record buffer; larger TXT cannot be
resolved here. Cache lifetimes are conservatively capped at 4500 seconds.
A multihomed/round-robin service exposes one currently selected address per
interface; multiple-address selection and SRV priority/weight policy are not
implemented. The last received usable record wins within that interface.

The existing own-packet filter means local registrations are not reflected
into this discovery cache. This slice discovers remote peers; explicit local
reflection needs additional work. IPv6, passive duplicate-question suppression,
negative caching/NSEC and continuation of oversized known-answer lists are
not implemented. The parent service's other RFC limitations still apply.

Discovery adds LVOs 20 and 21 after the table of upstream Bonami; whether
that numbering may be taken is part of the event-delivery decision in #1559.

Callers should read with a finite per-iteration budget. The original Hook
API is refused rather than served through a dispatcher process. Full RFC,
IPC and SMP safety are not claimed: the lent request has been checked on
uniprocessor hosted AROS only, and the public-port lookup/publication race
of the registration service applies here too. No SMP/hardware/Apple Bonjour
runtime results are claimed.
