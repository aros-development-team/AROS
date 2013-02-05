/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <asm/bcm2835.h>
#include <hardware/usb2otg.h>

#include "usb2otg_intern.h"

struct Unit * FNAME_DEV(OpenUnit)(struct IOUsbHWReq *ioreq,
                        LONG unitnr,
                        LIBBASETYPEPTR USB2OTGBase)
{
    struct USB2OTGUnit *unit = NULL;

    D(bug("[USB2OTG] %s(unit:0x%p, ioreq:0x%p)\n",
                __PRETTY_FUNCTION__, unit, ioreq));

    // We only support the single unit presently
    if ((unitnr == 0) && (USB2OTGBase->hd_Unit))
    {
        unit = USB2OTGBase->hd_Unit;
        if (!(unit->hu_UnitAllocated))
        {
            unit->hu_UnitAllocated = TRUE;
            return (&unit->hu_Unit);
        }
        else
        {
            ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
            D(bug("[USB2OTG] %s: Unit %ld already in use\n",
                        __PRETTY_FUNCTION__, unitnr));
        }
    }

    return(NULL);
}

void FNAME_DEV(CloseUnit)(struct IOUsbHWReq *ioreq, struct USB2OTGUnit *unit, LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] %s(unit:0x%p, ioreq:0x%p)\n",
                __PRETTY_FUNCTION__, unit, ioreq));

    unit->hu_UnitAllocated = FALSE;
}

WORD FNAME_DEV(cmdReset)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] CMD_RESET(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdFlush)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] CMD_FLUSH(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdQueryDevice)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_QUERYDEVICE(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdUsbReset)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_USBRESET(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdUsbResume)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_USBRESUME(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdUsbSuspend)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_USBSUSPEND(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdUsbOper)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_USBOPER(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdControlXFer)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_CONTROLXFER(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdBulkXFer)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_BULKXFER(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdIntXFer)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_INTXFER(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}

WORD FNAME_DEV(cmdIsoXFer)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] UHCMD_ISOXFER(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}
            
WORD FNAME_DEV(cmdNSDeviceQuery)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *unit,
                       LIBBASETYPEPTR base)
{
    D(bug("[USB2OTG] NSCMD_DEVICEQUERY(unit:0x%p, ioreq:0x%p)\n",
                unit, ioreq));

    return (WORD)NULL;
}
