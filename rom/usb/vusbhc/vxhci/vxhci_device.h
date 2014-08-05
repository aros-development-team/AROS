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

#include <devices/usb.h>
#include <devices/usbhardware.h>
#include <devices/newstyle.h>

/* Maximum number of units */
#define VXHCI_NUMCONTROLLERS 2

/* Maximum number of ports per protocol (USB2.0/USB3.0) */
#define VXHCI_NUMPORTS20 2
#define VXHCI_NUMPORTS30 2

#define RC_OK         0
#define RC_DONTREPLY -1

WORD cmdUsbReset(struct IOUsbHWReq *ioreq);
WORD cmdNSDeviceQuery(struct IOStdReq *ioreq);
WORD cmdQueryDevice(struct IOUsbHWReq *ioreq);
WORD cmdControlXFer(struct IOUsbHWReq *ioreq);
WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq);
WORD cmdIntXFer(struct IOUsbHWReq *ioreq);
WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq);

struct VXHCIPort {
    struct Node             node;
    char                    name[256];
    ULONG                   number;
    ULONG                   state;
};

struct VXHCIRootHub {
    struct List              port_list;
    ULONG                    port_count;

    UWORD                    addr;
    struct UsbStdDevDesc     devdesc;
    struct RHConfig {
        struct UsbStdCfgDesc cfgdesc;
        struct UsbStdIfDesc  ifdesc;
        struct UsbStdEPDesc  epdesc;
    }                        config;
};

struct VXHCIUnit {
    struct Node             node;
    char                    name[256];
    ULONG                   number;
    ULONG                   state;
    struct VXHCIRootHub     roothub;
};

struct VXHCIBase {

    struct Device           device;
    /* UNIT refers to one of the virtual xhci controllers per port protocol. */
    struct List             unit_list;
    ULONG                   unit_count;

};

#endif /* VXHCI_DEVICE_H */
