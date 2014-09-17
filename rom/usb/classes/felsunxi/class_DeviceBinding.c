/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include "felsunxi_intern.h"

struct FELSunxiDevice * AttemptDeviceBinding(LIBBASETYPEPTR LIBBASE, struct PsdDevice *pd) {
    mybug(-1,("FELSunxi AttemptDeviceBinding\n"));

    struct Library *ps;

    IPTR prodid;
    IPTR vendid;

    if((ps = OpenLibrary("poseidon.library", 4))) {
        psdGetAttrs(PGA_DEVICE, pd, DA_VendorID, &vendid, DA_ProductID, &prodid, TAG_END);
        CloseLibrary(ps);
        if((vendid == 0x1f3a) && (prodid == 0xefe8)) {
            return(ForceDeviceBinding(LIBBASE, pd));
        }
    }

    return NULL;
}

struct FELSunxiDevice * ForceDeviceBinding(LIBBASETYPEPTR LIBBASE, struct PsdDevice *pd) {
    mybug(-1,("FELSunxi ForceDeviceBinding\n"));

    struct FELSunxiDevice *FELSunxiDevice;
    struct Library *ps;

    if((ps = OpenLibrary("poseidon.library", 4))) {
        if((FELSunxiDevice = psdAllocVec(sizeof(struct FELSunxiDevice)))) {

            ObtainSemaphore(&LIBBASE->device_listlock);
                AddTail(&LIBBASE->device_list, &FELSunxiDevice->node);
            ReleaseSemaphore(&LIBBASE->device_listlock);

            CloseLibrary(ps);
            return FELSunxiDevice;
        }
        CloseLibrary(ps);
    }

    return NULL;
}

void ReleaseDeviceBinding(LIBBASETYPEPTR LIBBASE, struct FELSunxiDevice *FELSunxiDevice) {
    mybug(-1,("FELSunxi ReleaseDeviceBinding\n"));

    struct Library *ps;

    if((ps = OpenLibrary("poseidon.library", 4))) {

        ObtainSemaphore(&LIBBASE->device_listlock);
            Remove(&FELSunxiDevice->node);
        ReleaseSemaphore(&LIBBASE->device_listlock);

        psdFreeVec(FELSunxiDevice);
        CloseLibrary(ps);
    }
}

