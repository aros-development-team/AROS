/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#ifndef FELSUNXI_INTERN_H
#define FELSUNXI_INTERN_H

#include <aros/debug.h>

#include <exec/types.h>
#include <devices/usb.h>
#include <devices/usbhardware.h>
#include <libraries/usbclass.h>
#include <libraries/poseidon.h>
#include <proto/poseidon.h>

#include LC_LIBDEFS_FILE

#define MYBUG_LEVEL 1
#define mybug(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug x; } } while (0); } )
#define mybug_unit(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug("%s %s: ", unit->name, __FUNCTION__); bug x; } } while (0); } )

struct FELSunxiDevice {
    struct Node             node; 
};

struct FELSunxiBase {
    struct Library          library;

    struct List             device_list;
    struct SignalSemaphore  device_listlock;
};

struct FELSunxiDevice * AttemptDeviceBinding(LIBBASETYPEPTR LIBBASE, struct PsdDevice *pd);
struct FELSunxiDevice * ForceDeviceBinding(LIBBASETYPEPTR LIBBASE, struct PsdDevice *pd);
void ReleaseDeviceBinding(LIBBASETYPEPTR LIBBASE, struct FELSunxiDevice *FELSunxiDevice);

#endif /* FELSUNXI_INTERN_H */

