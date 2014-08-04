/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#ifndef VXHCI_DEVICE_H
#define VXHCI_DEVICE_H

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/initializers.h>

#include <devices/usbhardware.h>
#include <devices/newstyle.h>

/* Maximum number of units */
#define VXHCI_NUMCONTROLLERS 1

/* Maximum number of ports per protocol (USB2.0/USB3.0) */
#define VXHCI_NUMPORTS20 1
#define VXHCI_NUMPORTS30 1

#define RC_OK         0
#define RC_DONTREPLY -1

WORD cmdUsbReset(struct IOUsbHWReq *ioreq);
WORD cmdNSDeviceQuery(struct IOStdReq *ioreq);
WORD cmdQueryDevice(struct IOUsbHWReq *ioreq);
WORD cmdControlXFer(struct IOUsbHWReq *ioreq);
WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq);

struct VXHCIPort {
    struct Node         port_node;
    char                port_name[256];
    ULONG               port_number;
    ULONG               port_state;
};

struct VXHCIRootHub {
    struct List         port_list;
    ULONG               port_count;
};

struct VXHCIUnit {
    struct Node         unit_node;
    char                unit_name[256];
    ULONG               unit_number;
    ULONG               unit_type;
    ULONG               unit_state;
    struct VXHCIRootHub unit_roothub;
    UWORD               unit_roothubaddr;
};

struct VXHCIBase {

    struct Device       device;
    /* UNIT refers to one of the virtual xhci controllers per port protocol. */
    struct List         unit_list;
    ULONG               unit_count;

};

#endif /* VXHCI_DEVICE_H */
