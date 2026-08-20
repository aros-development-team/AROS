/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Per-device-class emulation for the virtual Bluetooth controller.

    The core controller emulation (HCI/L2CAP/SDP/GATT plumbing) lives in
    vbthci_peer.c; the behaviour that is specific to a class of peer device
    (its HID report map, the input reports it streams, ...) lives in a source
    file of its own so the core does not grow into one monolithic blob. Add a
    new device class by dropping in a vbthci_<class>.c that implements the hooks
    it needs and wiring it into the switch points in vbthci_peer.c.
*/

#ifndef VBTHCI_DEVCLASS_H
#define VBTHCI_DEVCLASS_H

#include "vbthci_intern.h"

/* *** HID over GATT (HOGP) - vbthci_hid.c *** */

/* TRUE if this fake device is one we emulate as an HID (HOGP) peer. */
BOOL vbtp_IsHid(const struct VBTFakeDevice *fd);

/* The device's HID Report Map (report descriptor). *len receives its length. */
const UBYTE *vbtp_HidReportDesc(const struct VBTFakeDevice *fd, ULONG *len);

/* Fill buf with the device's next simulated input report and return its
   length. `step` is a monotonically increasing counter that drives the
   simulated motion / keystrokes. buf must have room for at least 8 bytes. */
ULONG vbtp_HidInputReport(const struct VBTFakeDevice *fd, ULONG step, UBYTE *buf);

#endif /* VBTHCI_DEVCLASS_H */
