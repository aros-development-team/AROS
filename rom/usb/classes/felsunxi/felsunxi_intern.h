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

    struct PsdDevice       *pd;

    struct Library         *ps;
    struct Library         *MUIMasterBase;

    LONG                    readysignal;
    struct Task            *readysigtask;

    struct Task            *felsunxitask;
    struct MsgPort         *felsunxitask_msgport;
    struct Message         *felsunxitask_msg;

};

struct FELSunxiBase {
    struct Library          library;
};

struct FELSunxiDevice * AttemptDeviceBinding(LIBBASETYPEPTR LIBBASE, struct PsdDevice *pd);
struct FELSunxiDevice * ForceDeviceBinding(LIBBASETYPEPTR LIBBASE, struct PsdDevice *pd);
void ReleaseDeviceBinding(LIBBASETYPEPTR LIBBASE, struct FELSunxiDevice *FELSunxiDevice);

AROS_UFP0(void, FELSunxiTask);

#endif /* FELSUNXI_INTERN_H */

