/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi HDMI Audio AHI Driver Initialization
*/

#include <config.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "library.h"
#include "DriverData.h"

APTR KernelBase = NULL;

BOOL DriverInit(struct DriverBase *AHIsubBase)
{
    struct RPiHDMIBase *RPiHDMIBase = (struct RPiHDMIBase *)AHIsubBase;

    RPiHDMIBase->dosbase = (struct DosLibrary *)OpenLibrary(DOSNAME, 37);
    if (!RPiHDMIBase->dosbase) {
        Req("Unable to open 'dos.library' version 37.\n");
        return FALSE;
    }

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase) {
        Req("Unable to open 'kernel.resource'.\n");
        return FALSE;
    }

    RPiHDMIBase->periiobase = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);
    if (!RPiHDMIBase->periiobase) {
        RPiHDMIBase->periiobase = 0xFE000000UL; /* BCM2711 low-peri */
    }

    return TRUE;
}

VOID DriverCleanup(struct DriverBase *AHIsubBase)
{
    struct RPiHDMIBase *RPiHDMIBase = (struct RPiHDMIBase *)AHIsubBase;

    if (RPiHDMIBase->dosbase) {
        CloseLibrary((struct Library *)RPiHDMIBase->dosbase);
        RPiHDMIBase->dosbase = NULL;
    }
}
