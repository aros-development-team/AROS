/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI XHCI USB host controller
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/stdc.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>

#include <asm/io.h>
#include <inttypes.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include "pcixhci_device.h"

#include LC_LIBDEFS_FILE

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE) {
    mybug(0,("[PCIXHCI] Init: Entering function\n"));

    /* Try to find a usable host controller, if not found then we are done. */
    return(PCIXHCI_Discover(LIBBASE));
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR LIBBASE, struct IOUsbHWReq *ioreq, ULONG unitnum, ULONG flags) {
    mybug(0, ("[PCIXHCI] Open: Entering function\n"));
    mybug(0, ("[PCIXHCI] Open: Unit %d\n", unitnum));

    struct PCIXHCIUnit *unit;

    /* Default to open failure. */
    ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;
    ioreq->iouh_Req.io_Unit = NULL;

    /*
        Number of units eg. virtual xhci controllers.
        Host controller is divided into individual units if it has both usb2.0 and usb3.0 ports
    */
    if(unitnum<PCIXHCIBase->unit_count) {

        if(ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq)) {
            mybug(-1, ("[PCIXHCI] Open: Invalid MN_LENGTH!\n"));
            ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
        }

        ioreq->iouh_Req.io_Unit = NULL;

        ForeachNode(&PCIXHCIBase->unit_list, unit) {
            mybug(0, ("[PCIXHCI] Open: Opening unit number %d\n", unitnum));
            if(unit->number == unitnum) {
                mybug(0, ("        Found unit from node list %s %p\n\n", unit->name, unit));
                ioreq->iouh_Req.io_Unit = (struct Unit *) unit;
                break;
            }
        }

        if(ioreq->iouh_Req.io_Unit != NULL) {

            /* Opened ok! */
            ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ioreq->iouh_Req.io_Error				   = 0;

            return TRUE;
        } else {
            return FALSE;
        }
    }

    return FALSE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR LIBBASE, struct IOUsbHWReq *ioreq) {
    mybug(0, ("[PCIXHCI] Close: Entering function\n"));

    ioreq->iouh_Req.io_Unit   = (APTR) -1;
    ioreq->iouh_Req.io_Device = (APTR) -1;

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, BeginIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), struct PCIXHCIBase *, PCIXHCIBase, 5, PCIXHCI) {
    AROS_LIBFUNC_INIT
    mybug(0, ("[PCIXHCI] BeginIO: Entering function\n"));

    WORD ret = RC_OK;

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error				   = UHIOERR_NO_ERROR;

    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;

    if(unit != NULL) {

        switch (ioreq->iouh_Req.io_Command) {
            case CMD_RESET:
                mybug_unit(0, ("CMD_RESET\n"));
                break;
            case CMD_FLUSH:
                mybug_unit(0, ("CMD_FLUSH\n"));
                break;
            case UHCMD_QUERYDEVICE:
                mybug_unit(0, ("UHCMD_QUERYDEVICE\n"));
                ret = cmdQueryDevice(ioreq);
                break;
            case UHCMD_USBRESET:
                mybug_unit(0, ("UHCMD_USBRESET\n"));
                ret = cmdUsbReset(ioreq);
                break;
            case UHCMD_USBRESUME:
                mybug_unit(0, ("UHCMD_USBRESUME\n"));
                break;
            case UHCMD_USBSUSPEND:
                mybug_unit(0, ("UHCMD_USBSUSPEND\n"));
                break;
            case UHCMD_USBOPER:
                mybug_unit(0, ("UHCMD_USBOPER\n"));
                //ret = cmdUsbOper(ioreq);
                break;
            case UHCMD_CONTROLXFER:
                mybug_unit(0, ("UHCMD_CONTROLXFER unit %p %s\n", unit, unit->name));
                ret = cmdControlXFer(ioreq);
                break;
            case UHCMD_BULKXFER:
                mybug_unit(0, ("UHCMD_BULKXFER\n"));
                break;
            case UHCMD_INTXFER:
                mybug_unit(0, ("UHCMD_INTXFER unit %p %s\n", unit, unit->name));
                ret = cmdIntXFer(ioreq);
                break;
            case UHCMD_ISOXFER:
                mybug_unit(0, ("UHCMD_ISOXFER\n"));
                break;

            /* Poseidon doesn't actually check this, ever... */
            case NSCMD_DEVICEQUERY:
                mybug_unit(0, ("NSCMD_DEVICEQUERY\n"));

                static const UWORD NSDSupported[] = {
                    CMD_FLUSH, CMD_RESET,
                    UHCMD_QUERYDEVICE,
                    UHCMD_USBRESET,
                    UHCMD_USBRESUME,
                    UHCMD_USBSUSPEND,
                    UHCMD_USBOPER,
                    UHCMD_CONTROLXFER ,
                    UHCMD_ISOXFER,
                    UHCMD_INTXFER,
                    UHCMD_BULKXFER,
                    NSCMD_DEVICEQUERY,
                    0
                };

                struct NSDeviceQueryResult *nsdq = (struct NSDeviceQueryResult *)((struct IOStdReq *)(ioreq))->io_Data;
                nsdq->DevQueryFormat    = 0;
                nsdq->SizeAvailable     = sizeof(struct NSDeviceQueryResult);
                nsdq->DeviceType        = NSDEVTYPE_USBHARDWARE;
                nsdq->DeviceSubType     = 0;
                nsdq->SupportedCommands = (UWORD *)NSDSupported;
                ret = RC_OK;
                break;
            default:
                mybug_unit(-1, ("IOERR_NOCMD\n"));
                ret = IOERR_NOCMD;
                break;
        }
    } else {
        /* We have aborted the request as unit is invalid */
        ret = IOERR_ABORTED;
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

AROS_LH1(LONG, AbortIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), struct PCIXHCIBase *, PCIXHCIBase, 6, PCIXHCI) {
    AROS_LIBFUNC_INIT
    mybug(-1, ("[PCIXHCI] AbortIO: Entering function\n"));

    if(ioreq->iouh_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE) {
        if(cmdAbortIO(ioreq)) {
            return(0);
        }
    }

    return(-1);
    AROS_LIBFUNC_EXIT
}


