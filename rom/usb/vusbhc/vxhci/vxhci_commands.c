/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <proto/arossupport.h>

#include <devices/usbhardware.h>

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq) {
    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    while((tag = LibNextTagItem(&taglist)) != NULL) {
        switch (tag->ti_Tag) {
            case UHA_Manufacturer:
                *((STRPTR *) tag->ti_Data) = "The AROS Development Team";
                count++;
                break;
            case UHA_Version:
                *((ULONG *) tag->ti_Data) = VERSION_NUMBER;
                count++;
                break;
            case UHA_Revision:
                *((ULONG *) tag->ti_Data) = REVISION_NUMBER;
                count++;
                break;
            case UHA_Copyright:
                *((STRPTR *) tag->ti_Data) ="©2014 The AROS Development Team";
                count++;
                break;
            case UHA_Capabilities:
                *((ULONG *) tag->ti_Data) = (UHCF_USB20|UHCF_USB30);
                count++;
                break;
            default:
                break;
        }
    }

    ioreq->iouh_Actual = count;
    return RC_OK;
}

WORD cmdControlXFer(struct IOUsbHWReq *ioreq) {
    bug("[VXHCI] cmdControlXFer: Entering function\n");

    struct VXHCIUnit *unit;

    bug("[VXHCI] cmdControlXFer: ioreq->iouh_DevAddr %lx\n", ioreq->iouh_DevAddr);

    /* Check the status of the controller */
    unit = ioreq->iouh_Req.io_Unit;
    if(unit != NULL) {
        switch(unit->unit_state) {
            default:
            case UHSF_SUSPENDED:
                bug("[VXHCI] cmdControlXFer: Unit state: UHSF_SUSPENDED\n");
                return UHIOERR_USBOFFLINE;
                break;
            case UHSF_OPERATIONAL:
                bug("[VXHCI] cmdControlXFer: Unit state: UHSF_OPERATIONAL\n");
                break;
        }
    } else {
        /* CHECKME: Is this correct? */
        return UHIOERR_BADPARAMS;
    }

    /* Assuming when iouh_DevAddr is 0 it addresses hub... */

    return RC_DONTREPLY;
}



