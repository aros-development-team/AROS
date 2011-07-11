/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#include LC_LIBDEFS_FILE

static int Open(LIBBASETYPEPTR LIBBASE, struct IOUsbHWReq *ioreq,  ULONG unit, ULONG flags) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_DEVIO,("EHC Open: "));

    if(ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq)) {
        KPRINTF2(DBL_DEVIO, ("Invalid MN_LENGTH"));
        ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
    } else {
        ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;

        struct ehu_unit *ehu;
        ForeachNode(&ehd->ehd_unitlist, ehu) {
            if(ehu->ehu_unitnumber == unit) {
                ioreq->iouh_Req.io_Unit = ehu;
                if(ehu->ehu_unitallocated) {
                    ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
                    KPRINTF2(DBL_DEVIO, ("Unit %ld already open", unit));
                    break;
                }else{
                    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
                    ioreq->iouh_Req.io_Error = 0;
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
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_DEVIO,("EHC Close: \n"));
    return TRUE;
}

AROS_LH1(void, BeginIO,
    AROS_LHA(struct IOUsbHWReq *, ioreq, A1),
    LIBBASETYPEPTR, LIBBASE, 5, pciehci)
{
	AROS_LIBFUNC_INIT
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_DEVIO,("EHC BeginIO: "));

    struct ehu_unit *ehu = (struct ehu_unit *) ioreq->iouh_Req.io_Unit;

    ULONG ret = RC_OK;

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error                   = UHIOERR_NO_ERROR;

    if (ioreq->iouh_Req.io_Command < NSCMD_DEVICEQUERY) {
        switch (ioreq->iouh_Req.io_Command) {
			case CMD_RESET:
                KPRINTF2(DBL_DEVIO,("cmdReset\n"));
//              ret = cmdReset(ioreq, unit, base);
                break;

			case CMD_FLUSH:
                KPRINTF2(DBL_DEVIO,("cmdFlush\n"));
//              ret = cmdFlush(ioreq, unit, base);
                break;

            case UHCMD_QUERYDEVICE:
                KPRINTF2(DBL_DEVIO,("cmdQueryDevice\n"));
//              ret = cmdQueryDevice(ioreq, unit, base);
                break;

			case UHCMD_USBRESET:
                KPRINTF2(DBL_DEVIO,("cmdUsbReset\n"));
//              ret = cmdUsbReset(ioreq, unit, base);
                break;

			case UHCMD_USBRESUME:
                KPRINTF2(DBL_DEVIO,("cmdUsbResume\n"));
//              ret = cmdUsbResume(ioreq, unit, base);
                break;

			case UHCMD_USBSUSPEND:
                KPRINTF2(DBL_DEVIO,("cmdUsbSuspend\n"));
//              ret = cmdUsbSuspend(ioreq, unit, base);
                break;

			case UHCMD_USBOPER:
                KPRINTF2(DBL_DEVIO,("cmdUsbOper\n"));
//              ret = cmdUsbOper(ioreq, unit, base);
                break;

			case UHCMD_CONTROLXFER:
                KPRINTF2(DBL_DEVIO,("cmdControlXFer\n"));
//              ret = cmdControlXFer(ioreq, unit, base);
                break;

			case UHCMD_BULKXFER:
                KPRINTF2(DBL_DEVIO,("cmdBulkXFer\n"));
//              ret = cmdBulkXFer(ioreq, unit, base);
                break;

			case UHCMD_INTXFER:
                KPRINTF2(DBL_DEVIO,("cmdIntXFer\n"));
//              ret = cmdIntXFer(ioreq, unit, base);
                break;

			case UHCMD_ISOXFER:
                KPRINTF2(DBL_DEVIO,("cmdIsoXFer\n"));
//              ret = cmdIsoXFer(ioreq, unit, base);
                break;

            default:
                KPRINTF2(DBL_DEVIO,("IOERR_NOCMD\n"));
//              ret = IOERR_NOCMD;
                break;
        }
    } else {
        switch(ioreq->iouh_Req.io_Command) {
            case NSCMD_DEVICEQUERY:
                KPRINTF2(DBL_DEVIO,("cmdNSDeviceQuery\n"));
//              ret = cmdNSDeviceQuery((struct IOStdReq *) ioreq, unit, base);
                break;

            default:
                KPRINTF2(DBL_DEVIO,("IOERR_NOCMD\n"));
//              ret = IOERR_NOCMD;
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

AROS_LH1(LONG, AbortIO,
	AROS_LHA(struct IOUsbHWReq *, ioreq, A1),
	LIBBASETYPEPTR, LIBBASE, 6, pciehci)
{
	AROS_LIBFUNC_INIT
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    KPRINTF2(DBL_DEVIO,("EHC AbortIO: \n"));

    return TRUE;
	AROS_LIBFUNC_EXIT
}

ADD2OPENDEV(Open,0)
ADD2CLOSEDEV(Close,0)
