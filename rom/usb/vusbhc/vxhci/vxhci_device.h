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
#define VXHCI_NUMUNITS 1

/* Maximum number of ports per protocol (USB2.0/USB3.0) */
/* FIXME: Allow any arbitrary port count per protocol */
#define VXHCI_NUMPORTS 2

#define RC_OK         0
#define RC_DONTREPLY -1

struct VXHCIPort {
    struct Node         port_node;
    char                port_name[256];
    ULONG               port_number;
    ULONG               port_state;
    ULONG               port_type;
};

struct VXHCIRootHub {
    struct List         port_list;
};

struct VXHCIUnit {
    struct Node         unit_node;
    char                unit_name[256];
    ULONG               unit_number;
    ULONG               unit_state;
    struct VXHCIRootHub unit_roothub;
};

struct VXHCIBase {

    struct Device       device;
    /* UNIT refers to one of the virtual xhci controllers. */
    struct List         unit_list;

};

#endif /* VXHCI_DEVICE_H */
