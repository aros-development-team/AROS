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
            return ForceDeviceBinding(LIBBASE, pd);
        }
    }

    return NULL;
}

struct FELSunxiDevice * ForceDeviceBinding(LIBBASETYPEPTR LIBBASE, struct PsdDevice *pd) {
    mybug(-1,("FELSunxi ForceDeviceBinding\n"));

    struct FELSunxiDevice *FELSunxiDevice;
    struct Library *ps;

    if((ps = OpenLibrary("poseidon.library", 4))) {
        FELSunxiDevice = psdAllocVec(sizeof(struct FELSunxiDevice));
        if(FELSunxiDevice) {
            FELSunxiDevice->ps = ps;
            FELSunxiDevice->pd = pd;
            /* Open MUI for FELSunxiTask, don't bother to continue if it fails */
            FELSunxiDevice->MUIMasterBase = OpenLibrary("muimaster.library", 0);
            if(FELSunxiDevice->MUIMasterBase) {
                mybug(-1,("Creating FELSunxiTask\n"));
                FELSunxiDevice->readysignal = SIGB_SINGLE;
                FELSunxiDevice->readysigtask = FindTask(NULL);
                SetSignal(0, SIGF_SINGLE);

                FELSunxiDevice->felsunxitask = psdSpawnSubTask("FELSunxi task", FELSunxiTask, FELSunxiDevice);
                if(FELSunxiDevice->felsunxitask) {
                /* Wait for FELSunxiTask to be ready */
                    psdBorrowLocksWait(FELSunxiDevice->felsunxitask, 1UL<<FELSunxiDevice->readysignal);
                    return FELSunxiDevice;
                } else {
                    mybug(-1,("Failed to spawn the task\n"));
                }
                CloseLibrary(FELSunxiDevice->MUIMasterBase);
            }
            psdFreeVec(FELSunxiDevice);
        }
        CloseLibrary(ps);
    }

    return NULL;
}

void ReleaseDeviceBinding(LIBBASETYPEPTR LIBBASE, struct FELSunxiDevice *FELSunxiDevice) {
    mybug(-1,("FELSunxi ReleaseDeviceBinding(%p)\n",FELSunxiDevice->pd));

    /*
        We need to support user closing the app window and user pulling the plug
    */
    Signal(FELSunxiDevice->felsunxitask, SIGBREAKF_CTRL_C);
}

