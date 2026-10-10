# btmidi.class

MIDI over Bluetooth LE, as the MIDI Association's BLE MIDI 1.0
specification describes it, offered to CAMD applications.

**AROS as the peripheral.** The class registers the BLE MIDI service
(`03B80E5A-EDE8-4B33-A751-6CE34EC4C700`, I/O characteristic
`7772E5DB-3868-4112-A1A9-F2669D106BF3`) with the stack's GATT server. A
phone or computer that connects plays through the CAMD clusters
`BLE MIDI In` (what it sends) and `BLE MIDI Out` (what it receives).
`btgatt.class` decides whether the service is offered and whether the radios
advertise. Each central has its own reassembly state, so several can send at
once. The class asks centrals for a 15 ms connection interval
while its service is enabled (`BSRA_LEConnInterval`) and sizes its packets
to the smallest payload among subscribed links (`BSA_LENotifyPayload`).

**AROS as the central.** bluetooth.library offers the services of every
registered device to the classes; the class binds to each BLE MIDI service.
A bound device, such as a keyboard or a controller, gets a CAMD node named
after it, `<name> In` and `<name> Out`. Like `btbattery.class`, the binding
never connects the device itself: it follows the stack's connect and
disconnect events, reads the characteristic once after a link comes up, as
the specification asks of a central, and then subscribes to notifications.

The settings window, opened from Bluetooth Preferences, renames the
peripheral role's CAMD ports and shows its activity.

| file | what it holds |
|---|---|
| `btmidi.c` | the class: peripheral role, configuration, entry points |
| `btmidi_central.c` | the bindings to BLE MIDI peripherals |
| `btmidi_gui.c` | the settings window |
| `blemidi.c` | the BLE MIDI packet format and message reassembly, portable C |
| `camdbridge.c` | a CAMD node with its two clusters |
