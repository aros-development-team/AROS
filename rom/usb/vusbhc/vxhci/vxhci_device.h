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
#define VXHCI_NUMUNITS 2

#define RC_OK		   0
#define RC_DONTREPLY  -1

struct VXHCIUnit {
	struct Node		unit_node;
	char			unit_name[256];
	ULONG			unit_number;
	ULONG			unit_state;
};

struct VXHCIBase {

	struct Device				device;
	/* UNIT refers to one of the virtual xhci controllers. */
	struct List					units;

};

#endif /* VXHCI_DEVICE_H */
