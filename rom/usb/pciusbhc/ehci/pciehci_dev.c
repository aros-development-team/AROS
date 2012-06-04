/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#include LC_LIBDEFS_FILE

#include "pciehci_uhw.h"

UWORD cmdQueryDevice(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);
UWORD cmdReset(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);
UWORD cmdUsbReset(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);
UWORD cmdUsbResume(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);
UWORD cmdUsbSuspend(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);
UWORD cmdUsbOper(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);


static int Open(LIBBASETYPEPTR LIBBASE, struct IOUsbHWReq *ioreq,  ULONG unit, ULONG flags) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_DEVIO,("EHC Open: "));

    if(ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq)) {
        KPRINTF2(DBL_DEVIO, ("Invalid MN_LENGTH"));
        ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
    } else {
        ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;

        struct Unitnode *ehu_unitnode;
        ForeachNode(&ehd->ehd_unitnodelist, ehu_unitnode) {

            struct ehu_unit *ehu = (struct ehu_unit *)ehu_unitnode->ehu_unitptr;
            if(ehu->ehu_unitnumber == unit) {
                ioreq->iouh_Req.io_Unit = (struct Unit *)ehu;

                if(ehu->ehu_devunit.unit_OpenCnt) {
                    ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
                    KPRINTF2(DBL_DEVIO, ("Unit %ld already open", unit));
                    break;
                }else{
                    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
                    ioreq->iouh_Req.io_Error = 0;
                    ehu->ehu_devunit.unit_OpenCnt++;
                    KPRINTF2(DBL_DEVIO, ("Unit %ld opened\n", unit));
                    return TRUE;
                }
            }
        }

        KPRINTF2(DBL_DEVIO, ("Unit %ld does not exist", unit));
    }

    KPRINTF2(DBL_DEVIO, (", could not open unit!\n"));
    ioreq->iouh_Req.io_Unit = NULL;
    return FALSE;
}

static int Close(LIBBASETYPEPTR LIBBASE, struct IOUsbHWReq *ioreq) {
//  LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_DEVIO,("EHC Close: \n"));

    struct ehu_unit *ehu = (struct ehu_unit *) ioreq->iouh_Req.io_Unit;
    ehu->ehu_devunit.unit_OpenCnt--;

    return TRUE;
}

AROS_LH1(void, BeginIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), LIBBASETYPEPTR, LIBBASE, 5, pciehci) {
	AROS_LIBFUNC_INIT

    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_DEVIO,("EHC BeginIO: "));

    struct ehu_unit *ehu = (struct ehu_unit *) ioreq->iouh_Req.io_Unit;

    UWORD ret = RC_OK;

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error                   = UHIOERR_NO_ERROR;

    if (ioreq->iouh_Req.io_Command < NSCMD_DEVICEQUERY) {
        switch (ioreq->iouh_Req.io_Command) {
			case CMD_RESET:
                KPRINTF2(DBL_DEVIO,("cmdReset\n"));
                ret = cmdReset(ioreq, ehu, ehd);
                break;

			case CMD_FLUSH:
                KPRINTF2(DBL_DEVIO,("cmdFlush\n"));
//              ret = cmdFlush(ioreq, ehu, ehd);
                break;

            case UHCMD_QUERYDEVICE:
                KPRINTF2(DBL_DEVIO,("cmdQueryDevice\n"));
                ret = cmdQueryDevice(ioreq, ehu, ehd);
                break;

			case UHCMD_USBRESET:
                KPRINTF2(DBL_DEVIO,("cmdUsbReset\n"));
                ret = cmdUsbReset(ioreq, ehu, ehd);
                break;

			case UHCMD_USBRESUME:
                KPRINTF2(DBL_DEVIO,("cmdUsbResume\n"));
                ret = cmdUsbResume(ioreq, ehu, ehd);
                break;

			case UHCMD_USBSUSPEND:
                KPRINTF2(DBL_DEVIO,("cmdUsbSuspend\n"));
                ret = cmdUsbSuspend(ioreq, ehu, ehd);
                break;

			case UHCMD_USBOPER:
                KPRINTF2(DBL_DEVIO,("cmdUsbOper\n"));
                ret = cmdUsbOper(ioreq, ehu, ehd);
                break;

			case UHCMD_CONTROLXFER:
                KPRINTF2(DBL_DEVIO,("cmdControlXFer\n"));
//              ret = cmdControlXFer(ioreq, ehu, ehd);
                break;

			case UHCMD_BULKXFER:
                KPRINTF2(DBL_DEVIO,("cmdBulkXFer\n"));
//              ret = cmdBulkXFer(ioreq, ehu, ehd);
                break;

			case UHCMD_INTXFER:
                KPRINTF2(DBL_DEVIO,("cmdIntXFer\n"));
//              ret = cmdIntXFer(ioreq, ehu, ehd);
                break;

			case UHCMD_ISOXFER:
                KPRINTF2(DBL_DEVIO,("cmdIsoXFer\n"));
//              ret = cmdIsoXFer(ioreq, ehu, ehd);
                break;

            default:
                KPRINTF2(DBL_DEVIO,("IOERR_NOCMD\n"));
                ret = IOERR_NOCMD;
                break;
        }
    } else {
        switch(ioreq->iouh_Req.io_Command) {
            case NSCMD_DEVICEQUERY:
                KPRINTF2(DBL_DEVIO,("cmdNSDeviceQuery\n"));

                struct NSDeviceQueryResult *nsdq = (struct NSDeviceQueryResult *)((struct IOStdReq *)(ioreq))->io_Data;
                nsdq->DevQueryFormat    = 0;
                nsdq->SizeAvailable     = sizeof(struct NSDeviceQueryResult);
                nsdq->DeviceType        = NSDEVTYPE_USBHARDWARE;
                nsdq->DeviceSubType     = 0;
                nsdq->SupportedCommands = (UWORD *)NSDSupported;

                ret = RC_OK;

                break;

            default:
                KPRINTF2(DBL_DEVIO,("IOERR_NOCMD\n"));
                ret = IOERR_NOCMD;
                break;
        }
    }

    if(ret != RC_DONTREPLY) {
        /* Set error codes */
        if (ret != RC_OK) {
            ioreq->iouh_Req.io_Error = ret & 0xff;
        }
        /* Terminate the iorequest */
        ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;
        /* If not quick I/O, reply the message */
        if(!(ioreq->iouh_Req.io_Flags & IOF_QUICK)) {
            ReplyMsg(&ioreq->iouh_Req.io_Message);
        }
    }

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), LIBBASETYPEPTR, LIBBASE, 6, pciehci) {
	AROS_LIBFUNC_INIT

//  LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_DEVIO,("EHC AbortIO: \n"));

    return TRUE;
	AROS_LIBFUNC_EXIT
}

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

UWORD cmdReset(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC CMD_RESET: 0x%p\n", ioreq));

//  uhwDelayMS(1, unit);
    uhwGetUsbState(ioreq, ehu, ehd);    /* FIXME */

    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

UWORD cmdUsbReset(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_USBRESET: 0x%p\n", ioreq));

    uhwGetUsbState(ioreq, ehu, ehd);  /* FIXME */

//  ehu->ehu_FrameCounter = 1;
//  ehu->ehu_RootHubAddr = 0;

    if(ioreq->iouh_State & UHSF_OPERATIONAL){
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

UWORD cmdUsbResume(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_USBRESUME: 0x%p\n", ioreq));

    uhwGetUsbState(ioreq, ehu, ehd);  /* FIXME */
    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

UWORD cmdUsbSuspend(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_USBSUSPEND: 0x%p\n", ioreq));

    uhwGetUsbState(ioreq, ehu, ehd);  /* FIXME */
    if(ioreq->iouh_State & UHSF_SUSPENDED) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

UWORD cmdUsbOper(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_UHWIO, ("EHC UHCMD_USBOPER: 0x%p\n", ioreq));

    uhwGetUsbState(ioreq, ehu, ehd);  /* FIXME */
    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}


ADD2OPENDEV(Open,0)
ADD2CLOSEDEV(Close,0)
