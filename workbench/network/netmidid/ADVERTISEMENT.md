# AppleMIDI DNS-SD advertisement

The service registers its configured session name and AppleMIDI control port
as `_apple-midi._udp` through the system `dnssd.library`. `mdnsd` owns the
multicast socket, hostname, probing, DNS-SD records, conflict resolution and
goodbyes. `netmidid` does not implement another mDNS responder.

The installed service stays disabled by default. Its Order=10 places it after
the system responder (Order=20). Enable the service through the existing
AROSTCP configuration. `ENV:NetMIDI` still configures the session, for example:

```ini
session_name=AROS MIDI
local_port=5004
```

No browse or resolve operation is used. Peers can invite this advertised
session, or the service can invite the IPv4 peer already supported by its
configuration. MIDI traffic continues when the library or daemon is absent.
Network MIDI 2.0 advertisement is a separate integration.

Registration attempts and session checks are limited to once per ten
seconds. The session is checked against mdnsd's incarnation
(`DNSSD_GetStatusA()`): if the daemon disappeared or was restarted, the
session and its registrations are discarded and registered again. A registration that mdnsd refuses for another reason, such as a name
this machine already advertises, is retried as well, and the reason is printed
once. Shutdown closes the session, which withdraws the registration with its
goodbye, before closing the library. The
daemon handles interface changes and DNS conflicts; successful registration is
asynchronous and does not mean probing has already completed.

A change to `ENV:NetMIDI` begins the sessions anew but keeps the advertisement
when the session name and port are unchanged; peers see the service leave
only when what is advertised changes.

Instance names are one label of at most 63 bytes, UTF-8, dots allowed. A
search a transport asks for (`netmidi_discovery_browse()`) is a browse of the
session, and each instance it finds is followed by a resolve with no time
limit, so that a peer whose address, port or endpoint name changes is
reported again; at most 16 are followed at a time.
