/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#include LC_LIBDEFS_FILE

#include "pciehci_uhw.h"

UWORD cmdQueryDevice(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_QUERYDEVICE: 0x%p, taglist: 0x%p\n", ioreq, taglist));

    if((tag = FindTagItem(UHA_State, taglist))) {
        *((ULONG *) tag->ti_Data) = (ULONG) uhwGetUsbState(ioreq, ehu, ehd);
        count++;
    }

    if((tag = FindTagItem(UHA_Manufacturer, taglist))) {
        *((STRPTR *) tag->ti_Data) = "Chris Hodges & AROS Development Team";
        count++;
    }

    if((tag = FindTagItem(UHA_ProductName, taglist))) {
        *((STRPTR *) tag->ti_Data) = "EHCI USB2.0 PCI driver";
        count++;
    }

    if((tag = FindTagItem(UHA_Description, taglist))) {
        *((STRPTR *) tag->ti_Data) = "EHCI host controller driver for PCI cards";
        count++;
    }

    if((tag = FindTagItem(UHA_Copyright, taglist))) {
        *((STRPTR *) tag->ti_Data) ="©2007-2009 Chris Hodges, ©2009-2011 AROS APL";
        count++;
    }

    if((tag = FindTagItem(UHA_Version, taglist))) {
        *((ULONG *) tag->ti_Data) = VERSION_NUMBER;
        count++;
    }

    if((tag = FindTagItem(UHA_Revision, taglist))) {
        *((ULONG *) tag->ti_Data) = REVISION_NUMBER;
        count++;
    }

    if((tag = FindTagItem(UHA_DriverVersion, taglist))) {
        *((ULONG *) tag->ti_Data) = 0x220;
        count++;
    }

    if((tag = FindTagItem(UHA_Capabilities, taglist))) {
        *((ULONG *) tag->ti_Data) = UHCF_USB20;
        count++;
    }

    ioreq->iouh_Actual = count;

    return RC_OK;
}

UWORD uhwGetUsbState(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
//  LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    return(ioreq->iouh_State = UHSF_OPERATIONAL);
}

WORD cmdReset(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC CMD_RESET: 0x%p\n", ioreq));

//  uhwDelayMS(1, unit);
    uhwGetUsbState(ioreq, ehu, ehd);    /* FIXME */

    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

WORD cmdUsbReset(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_USBRESET: 0x%p\n", ioreq));

    uhwGetUsbState(ioreq, ehu, ehd);  /* FIXME */

    ehu->ehu_FrameCounter = 1;
    ehu->ehu_RootHubAddr = 0;

    if(ioreq->iouh_State & UHSF_OPERATIONAL){
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

WORD cmdUsbResume(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_USBRESUME: 0x%p\n", ioreq));

    uhwGetUsbState(ioreq, ehu, ehd);  /* FIXME */
    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

WORD cmdUsbSuspend(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_USBSUSPEND: 0x%p\n", ioreq));

    uhwGetUsbState(ioreq, ehu, ehd);  /* FIXME */
    if(ioreq->iouh_State & UHSF_SUSPENDED) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

WORD cmdUsbOper(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_USBOPER: 0x%p\n", ioreq));

    uhwGetUsbState(ioreq, ehu, ehd);  /* FIXME */
    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

