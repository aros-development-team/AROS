# NetMIDIStatus

Run `C:NetMIDIStatus` to inspect the running network MIDI service. It prints
the local session name and refused invitations, then each active driver port:
peer name, received/sent messages and packets, loss, timing and SysEx/journal
counters. Inactive ports are omitted. The service refreshes its snapshot
about once per second.

The first sixteen slots are `applemidi.0` through `applemidi.15`; the remaining
sixteen are `networkmidi2.0` through `networkmidi2.15` when that transport is
provided by the service. The command itself does not start a session or open
a network socket.

Exit status is zero for a compatible snapshot and RETURN_WARN (5) when the
service is absent or publishes an unsupported status version. The status
structure and its version belong to the parent service change.
