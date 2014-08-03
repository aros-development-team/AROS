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

WORD cmdUsbReset(struct IOUsbHWReq *ioreq) {
    bug("[VXHCI] cmdUsbReset: Entering function\n");

    struct VXHCIUnit *unit;

    /* We should do a proper reset sequence with a real driver */
    unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;
    if(unit != NULL) {
        unit->unit_state = UHSF_OPERATIONAL;
    } else {
        /* FIXME: Move unit checks somewhere else... */
        return UHIOERR_BADPARAMS;
    }

    return RC_OK;
}

WORD cmdControlXFer(struct IOUsbHWReq *ioreq) {
    bug("[VXHCI] cmdControlXFer: Entering function\n");

    struct VXHCIUnit *unit;

    bug("[VXHCI] cmdControlXFer: ioreq->iouh_DevAddr %lx\n", ioreq->iouh_DevAddr);

    /* Check the status of the controller */
    unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;
    if(unit != NULL) {
        /*
            We might encounter these states:
            UHSB_OPERATIONAL USB can be used for transfers
            UHSB_RESUMING    USB is currently resuming
            UHSB_SUSPENDED   USB is in suspended state
            UHSB_RESET       USB is just inside a reset phase
        */
        if(unit->unit_state == UHSF_OPERATIONAL) {
            bug("[VXHCI] cmdControlXFer: Unit state is operational\n");
        } else {
            bug("[VXHCI] cmdControlXFer: Unit state: UHSF_SUSPENDED\n");
            return UHIOERR_USBOFFLINE;
        }

        /* Assuming when iouh_DevAddr is 0 it addresses hub... */
        if(ioreq->iouh_DevAddr == 0) {
            return(cmdControlXFerRootHub(ioreq));
        }

    } else {
        /* CHECKME: Is this correct? */
        return UHIOERR_BADPARAMS;
    }



    return RC_DONTREPLY;
}

WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq) {
    bug("[VXHCI] cmdControlXFerRootHub: Entering function\n");

    UWORD rt = ioreq->iouh_SetupData.bmRequestType;
    UWORD req = ioreq->iouh_SetupData.bRequest;
    UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    bug("[VXHCI] cmdControlXFerRootHub: Endpoint number is %ld and request type is %lx\n", ioreq->iouh_Endpoint, rt);

    /* Endpoint 0 is used for control transfers only and can not be assigned to any other function. */
    if(ioreq->iouh_Endpoint != 0) {
        return(UHIOERR_STALL);
    }

    if(len != ioreq->iouh_Length) {
        return(UHIOERR_STALL);
    }

    /* Check the request type */
    switch(rt) {
    }

}


