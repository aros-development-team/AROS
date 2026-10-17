# btmidi.class

MIDI over Bluetooth LE, as the MIDI Association's BLE MIDI 1.0
specification describes it, as a CAMD MIDI device.

**The CAMD driver.** Like `camdusbmidi.class`, the class carries a small
CAMD driver (`camd/btmididriver.c`, a `struct MidiDeviceData` with 16
ports) and writes it to `DEVS:Midi/btmidi` when it starts. `camd.library`
loads it from there, and the driver opens the class. CAMD programs see the
ports as `btmidi.in.<n>` and `btmidi.out.<n>`. The class never opens
`camd.library`. For each port CAMD opens, the driver gets an adapter from
the class (`btmidiCAMDOpenPort()`): MIDI bytes from Bluetooth go to CAMD's
receive function one at a time, and CAMD's transmit function is drained
into the adapter's ring when CAMD calls `ActivateXmit`. CAMD does the MIDI
parsing: running status, message lengths and SysEx.

**Port 0: AROS as the peripheral.** The class registers the BLE MIDI
service (`03B80E5A-EDE8-4B33-A751-6CE34EC4C700`, I/O characteristic
`7772E5DB-3868-4112-A1A9-F2669D106BF3`) with the stack's GATT server. What
a connected phone or computer sends arrives on port 0, and what CAMD
programs send to port 0 goes to every subscribed central. `btgatt.class`
decides whether the service is offered and whether the radios advertise.
Several centrals can send at once; only one SysEx at a time reaches CAMD.
The class asks centrals for a 15 ms connection interval while its service
is enabled (`BSRA_LEConnInterval`) and sizes its packets to the smallest
payload among subscribed links (`BSA_LENotifyPayload`).

**Ports 1 to 15: AROS as the central.** bluetooth.library offers the
services of every registered device to the classes; the class binds to each
BLE MIDI service. A bound device, such as a keyboard or a controller, gets
the first free port, and keeps it while the binding exists, connected or
not. A port no device is bound to does not open. Like `btbattery.class`,
the binding never connects the device itself: it follows the stack's
connect and disconnect events, reads the characteristic once after a link
comes up, as the specification asks of a central, and then subscribes to
notifications. CAMD may keep a port open after its device is gone; what is
sent to it is then dropped.

The status window, opened from Bluetooth Preferences, shows which device
has which port and the peripheral role's activity.

| file | what it holds |
|---|---|
| `btmidi.c` | the class: peripheral role, entry points |
| `btmidi_central.c` | the bindings to BLE MIDI peripherals |
| `btmidi_camd.c` | the port adapters and the installation of the driver |
| `btmidi_camd.h` | the adapter, shared with the driver |
| `camd/btmididriver.c` | the CAMD driver embedded in the class |
| `btmidi_gui.c` | the status window |
| `blemidi.c` | the BLE MIDI packet format, portable C |
