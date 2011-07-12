/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#include LC_LIBDEFS_FILE

#include "pciehci.h"
#include "pciehci_uhw.h"

UWORD cmdQueryDevice(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    KPRINTF(DBL_UHWIO, ("UHCMD_QUERYDEVICE ioreq: 0x%p, taglist: 0x%p\n", ioreq, taglist));

    if((tag = FindTagItem(UHA_State, taglist))) {
        *((ULONG *) tag->ti_Data) = (ULONG) uhwGetUsbState(ioreq, ehu, ehd);
        count++;
    }

    if((tag = FindTagItem(UHA_Manufacturer, taglist))) {
        *((STRPTR *) tag->ti_Data) = "Chris Hodges & AROS Development Team";
        count++;
    }

    if((tag = FindTagItem(UHA_ProductName, taglist))) {
        *((STRPTR *) tag->ti_Data) = "EHCI USB2.0";
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
    return(ioreq->iouh_State = UHSF_OPERATIONAL);
}
/* \\\ */
