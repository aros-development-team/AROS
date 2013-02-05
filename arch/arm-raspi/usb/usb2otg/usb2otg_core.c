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
    unit->hu_UnitAllocated = FALSE;
}
