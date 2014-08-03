/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Virtual XHCI USB host controller
    Lang: English
*/


#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>
#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/alib_protos.h>
#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include <oop/oop.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/utility.h>
#include <proto/bootloader.h>
#include <proto/dos.h>
#include <asm/io.h>

#include <devices/usbhardware.h>

#include <stdio.h>

#include "vxhci_device.h"

#include LC_LIBDEFS_FILE

#define DEBUG 1
#include <aros/debug.h>

struct VXHCIUnit *VXHCI_AddNewUnit(ULONG unitnum);
struct VXHCIPort *VXHCI_AddNewPort(ULONG unitnum, ULONG portnum);

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR VXHCIBase) {
    bug("[VXHCI] Init: Entering function\n");

    struct VXHCIUnit *unit;
    ULONG i;

    NEWLIST(&VXHCIBase->unit_list);

    for (i=0; i<VXHCI_NUMUNITS; i++) {
        unit = VXHCI_AddNewUnit(i);
        if(unit == NULL) {
            /*
                Free previous units if any exists
            */
            ForeachNode(&VXHCIBase->unit_list, unit) {
                bug("[VXHCI] Init: Removing unit structure %s at %p\n", unit->unit_node.ln_Name, unit);
                REMOVE(unit);
                FreeVec(unit);
            }
            return FALSE;
        } else {
            AddTail(&VXHCIBase->unit_list,(struct Node *)unit);
        }
    }

    return TRUE;
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR VXHCIBase, struct IOUsbHWReq *ioreq, ULONG unitnum, ULONG flags) {
    bug("[VXHCI] Open: Entering function\n");

    struct VXHCIUnit *unit;

    /* Default to open failure. */
    ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;

    /*
        Number of units eg. virtual xhci controllers. Fail when unit number exceeds the limit.
    */
    if(unitnum<VXHCI_NUMUNITS) {

        if(ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq)) {
            bug("[VXHCI] Open: Invalid MN_LENGTH!\n");
            ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
        }

        ioreq->iouh_Req.io_Unit = NULL;

        ForeachNode(&VXHCIBase->unit_list, unit) {
            bug("[VXHCI] Open: Opening unit number %d\n", unitnum);
            if(unit->unit_number == unitnum) {
                bug("        Found unit from node list\n");
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

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR VXHCIBase, struct IOUsbHWReq *ioreq) {
    bug("[VXHCI] Close: Entering function\n");

    ioreq->iouh_Req.io_Unit   = (APTR) -1;
    ioreq->iouh_Req.io_Device = (APTR) -1;

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, BeginIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), struct VXHCIBase *, VXHCIBase, 5, VXHCI) {
    AROS_LIBFUNC_INIT
    bug("[VXHCI] BeginIO: Entering function\n");

    WORD ret = RC_OK;

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error				   = UHIOERR_NO_ERROR;

    switch (ioreq->iouh_Req.io_Command) {
        case CMD_RESET:
            bug("[VXHCI] BeginIO: CMD_RESET\n");
            break;
        case CMD_FLUSH:
            bug("[VXHCI] BeginIO: CMD_FLUSH\n");
            break;
        case UHCMD_QUERYDEVICE:
            bug("[VXHCI] BeginIO: UHCMD_QUERYDEVICE\n");
            ret = cmdQueryDevice(ioreq);
            break;
        case UHCMD_USBRESET:
            bug("[VXHCI] BeginIO: UHCMD_USBRESET\n");
            ret = cmdUsbReset(ioreq);
            break;
        case UHCMD_USBRESUME:
            bug("[VXHCI] BeginIO: UHCMD_USBRESUME\n");
            break;
        case UHCMD_USBSUSPEND:
            bug("[VXHCI] BeginIO: UHCMD_USBSUSPEND\n");
            break;
        case UHCMD_USBOPER:
            bug("[VXHCI] BeginIO: UHCMD_USBOPER\n");
            //ret = cmdUsbOper(ioreq);
            break;
        case UHCMD_CONTROLXFER:
            bug("[VXHCI] BeginIO: UHCMD_CONTROLXFER\n");
            ret = cmdControlXFer(ioreq);
            break;
        case UHCMD_BULKXFER:
            bug("[VXHCI] BeginIO: UHCMD_BULKXFER\n");
            break;
        case UHCMD_INTXFER:
            bug("[VXHCI] BeginIO: UHCMD_INTXFER\n");
            break;
        case UHCMD_ISOXFER:
            bug("[VXHCI] BeginIO: UHCMD_ISOXFER\n");
            break;

        /* Poseidon doesn't actually check this, ever... */
        case NSCMD_DEVICEQUERY:
            bug("[VXHCI] BeginIO: NSCMD_DEVICEQUERY\n");

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
            bug("[VXHCI] BeginIO: IOERR_NOCMD\n");
            ret = IOERR_NOCMD;
            break;
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

AROS_LH1(LONG, AbortIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), struct VXHCIBase *, VXHCIBase, 6, VXHCI) {
    AROS_LIBFUNC_INIT
    bug("[VXHCI] AbortIO: Entering function\n");

    return TRUE;
    AROS_LIBFUNC_EXIT
}

struct VXHCIUnit *VXHCI_AddNewUnit(ULONG unitnum) {

    struct VXHCIUnit *unit;
    struct VXHCIPort *port;

    ULONG i;

    unit = AllocVec(sizeof(struct VXHCIUnit), MEMF_ANY|MEMF_CLEAR);
    if(unit == NULL) {
        bug("[VXHCI] VXHCI_AddNewUnit: Failed to create new unit structure\n");
        return NULL;
    } else {
        unit->unit_node.ln_Type = NT_USER;
        unit->unit_number = unitnum;
        sprintf(unit->unit_name, "VXHCI%x", unit->unit_number);
        unit->unit_node.ln_Name = (STRPTR)&unit->unit_name;

        unit->unit_state = UHSF_SUSPENDED;

        NEWLIST(&unit->unit_roothub.port_list);
        for (i=0; i<VXHCI_NUMPORTS*2; i++) {

            port = VXHCI_AddNewPort(unitnum, i);
            if(port == NULL) {
                /*
                    Free previous ports if any exists and delete this unit
                */
                bug("[VXHCI] VXHCI_AddNewUnit: Failed to create new port structure\n");
                ForeachNode(&unit->unit_roothub.port_list, port) {
                    bug("[VXHCI] VXHCI_AddNewUnit: Removing port structure %s at %p\n", port->port_node.ln_Name, port);
                    REMOVE(port);
                    FreeVec(port);
                }
                FreeVec(unit);
                return FALSE;
            } else {
                AddTail(&unit->unit_roothub.port_list,(struct Node *)port);
            }
        }

        bug("[VXHCI] VXHCI_AddNewUnit:\n");
        bug("        Created new unit numbered %d at %p\n",unit->unit_number, unit);
        bug("        Unit node name %s\n", unit->unit_node.ln_Name);
        D(switch(unit->unit_state) {
            case UHSF_SUSPENDED:
                bug("        Unit state: UHSF_SUSPENDED\n");
                break;
            case UHSF_OPERATIONAL:
                bug("        Unit state: UHSF_OPERATIONAL\n");
                break;
            default:
                bug("        Unit state: %lx (Error?)\n", unit->unit_state);
                break;
        });

        return unit;
    }
}

struct VXHCIPort *VXHCI_AddNewPort(ULONG unitnum, ULONG portnum) {
    struct VXHCIPort *port;

    port = AllocVec(sizeof(struct VXHCIPort), MEMF_ANY|MEMF_CLEAR);
    if(port == NULL) {
        bug("[VXHCI] VXHCI_AddNewPort: Failed to create new port structure\n");
        return NULL;
    } else {
        port->port_node.ln_Type = NT_USER;
        /* Poseidon treats port number 0 as roothub */
        port->port_number = portnum+1;
        sprintf(port->port_name, "VXHCI%x:%x", unitnum, port->port_number);
        port->port_node.ln_Name = (STRPTR)&port->port_name;

        if(portnum<VXHCI_NUMPORTS) {
            port->port_type = 2;
        } else {
            port->port_type = 3;
        }
    }

    bug("[VXHCI] VXHCI_AddNewPort:\n");
    bug("        Created new port numbered %d at %p\n",port->port_number, port);
    bug("        Port node name %s\n", port->port_node.ln_Name);
    D(switch(port->port_type) {
        case 2:
            bug("        Port type: USB2.0\n");
            break;
        case 3:
            bug("        Port type: USB3.0\n");
            break;
        default:
            bug("        Port type: %lx (Error?)\n", port->port_type);
            break;
    });

    return port;
}

