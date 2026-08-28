/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Raspberry Pi I2S Driver Initialization
*/

#include <config.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include "library.h"
#include "DriverData.h"

APTR KernelBase = NULL;

BOOL DriverInit(struct DriverBase *AHIsubBase) {
    struct RPiI2SBase *RPiI2SBase = (struct RPiI2SBase *)AHIsubBase;
    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 37);
    if (!DOSBase) return FALSE;
    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase) return FALSE;
    RPiI2SBase->periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);
    if (!RPiI2SBase->periiobase) return FALSE;
    return TRUE;
}

VOID DriverCleanup(struct DriverBase *AHIsubBase) {
    struct RPiI2SBase *RPiI2SBase = (struct RPiI2SBase *)AHIsubBase;
    CloseLibrary((struct Library *)DOSBase);
}
