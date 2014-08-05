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

#include <devices/usb.h>
#include <devices/usbhardware.h>

#include <stdio.h>

#include "vxhci_device.h"

#include LC_LIBDEFS_FILE

#define DEBUG 1
#include <aros/debug.h>

struct VXHCIUnit *VXHCI_AddNewUnit(ULONG unitnum, UWORD bcdusb);
struct VXHCIPort *VXHCI_AddNewPort(struct VXHCIUnit *unit, ULONG portnum);

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR VXHCIBase) {
    bug("[VXHCI] Init: Entering function\n");

    struct VXHCIUnit *unit;
    ULONG i;

    NEWLIST(&VXHCIBase->unit_list);
    VXHCIBase->unit_count = 0;

    for (i=0; i<VXHCI_NUMCONTROLLERS; i++) {

        #ifdef VXHCI_NUMPORTS20
        unit = VXHCI_AddNewUnit(VXHCIBase->unit_count, 0x200);
        if(unit == NULL) {
            /*
                Free previous units if any exists
            */
            ForeachNode(&VXHCIBase->unit_list, unit) {
                bug("[VXHCI] Init: Removing unit structure %s at %p\n", unit->node.ln_Name, unit);
                REMOVE(unit);
                FreeVec(unit);
            }
            return FALSE;
        } else {
            AddTail(&VXHCIBase->unit_list,(struct Node *)unit);
            VXHCIBase->unit_count++;
        }
        #endif

        unit = VXHCI_AddNewUnit(VXHCIBase->unit_count, 0x300);
        if(unit == NULL) {
            /*
                Free previous units if any exists
            */
            ForeachNode(&VXHCIBase->unit_list, unit) {
                bug("[VXHCI] Init: Removing unit structure %s at %p\n", unit->node.ln_Name, unit);
                REMOVE(unit);
                FreeVec(unit);
            }
            return FALSE;
        } else {
            AddTail(&VXHCIBase->unit_list,(struct Node *)unit);
            VXHCIBase->unit_count++;
        }

    }

    D(ForeachNode(&VXHCIBase->unit_list, unit) {
        bug("[VXHCI] Init: Created unit %d at %p %s\n", unit->number, unit, unit->name);
        struct VXHCIPort *port;
        ForeachNode(&unit->roothub.port_list, port) {
        bug("                      port %d at %p %s\n", port->number, port, port->name);
        }
    });

    return TRUE;
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR VXHCIBase, struct IOUsbHWReq *ioreq, ULONG unitnum, ULONG flags) {
    bug("[VXHCI] Open: Entering function\n");

    struct VXHCIUnit *unit;

    /* Default to open failure. */
    ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;
    ioreq->iouh_Req.io_Unit = NULL;

    /*
        Number of units eg. virtual xhci controllers.
        Host controller is divided into individual units if it has both usb2.0 and usb3.0 ports
    */
    if(unitnum<VXHCIBase->unit_count) {

        if(ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq)) {
            bug("[VXHCI] Open: Invalid MN_LENGTH!\n");
            ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
        }

        ioreq->iouh_Req.io_Unit = NULL;

        ForeachNode(&VXHCIBase->unit_list, unit) {
            bug("[VXHCI] Open: Opening unit number %d\n", unitnum);
            if(unit->number == unitnum) {
                bug("        Found unit from node list %p %s\n", unit, unit->name);
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
    //bug("[VXHCI] BeginIO: Entering function\n");

    WORD ret = RC_OK;

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error				   = UHIOERR_NO_ERROR;

    struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;

    if(unit != NULL) {

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
                bug("[VXHCI] BeginIO: UHCMD_CONTROLXFER unit %p %s\n", unit, unit->name);
                ret = cmdControlXFer(ioreq);
                break;
            case UHCMD_BULKXFER:
                bug("[VXHCI] BeginIO: UHCMD_BULKXFER\n");
                break;
            case UHCMD_INTXFER:
                bug("[VXHCI] BeginIO: UHCMD_INTXFER unit %p %s\n", unit, unit->name);
                ret = cmdIntXFer(ioreq);
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

AROS_LH1(LONG, AbortIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), struct VXHCIBase *, VXHCIBase, 6, VXHCI) {
    AROS_LIBFUNC_INIT
    bug("[VXHCI] AbortIO: Entering function\n");

    return TRUE;
    AROS_LIBFUNC_EXIT
}

struct VXHCIUnit *VXHCI_AddNewUnit(ULONG unitnum, UWORD bcdusb) {

    struct VXHCIUnit *unit;
    struct VXHCIPort *port;

    ULONG i, imax;

    unit = AllocVec(sizeof(struct VXHCIUnit), MEMF_ANY|MEMF_CLEAR);
    if(unit == NULL) {
        bug("[VXHCI] VXHCI_AddNewUnit: Failed to create new unit structure\n");
        return NULL;
    } else {
        unit->node.ln_Type = NT_USER;
        unit->number = unitnum;
        unit->node.ln_Name = (STRPTR)&unit->name;
        unit->state = UHSF_SUSPENDED;

        NEWLIST(&unit->roothub.port_list);

        /* Set the correct bcdUSB for the hub device descriptor */
        unit->roothub.devdesc.bcdUSB = AROS_WORD2LE(bcdusb);

        #ifdef VXHCI_NUMPORTS20
        if(bcdusb == 0x200) {
            sprintf(unit->name, "VXHCI_USB20[%x]", unit->number);
            imax = VXHCI_NUMPORTS20;
        } else {
            sprintf(unit->name, "VXHCI_USB30[%x]", unit->number);
            imax = VXHCI_NUMPORTS30;
        }
        #else
        sprintf(unit->name, "VXHCI_USB30[%x]", unit->number);
        imax = VXHCI_NUMPORTS30;
        #endif

        for (i=0; i<imax; i++) {

            port = VXHCI_AddNewPort(unit, i);
            if(port == NULL) {
                /*
                    Free previous ports if any exists and delete this unit
                */
                bug("[VXHCI] VXHCI_AddNewUnit: Failed to create new port structure\n");
                ForeachNode(&unit->roothub.port_list, port) {
                    bug("[VXHCI] VXHCI_AddNewUnit: Removing port structure %s at %p\n", port->node.ln_Name, port);
                    REMOVE(port);
                    FreeVec(port);
                }
                FreeVec(unit);
                return FALSE;
            } else {
                AddTail(&unit->roothub.port_list,(struct Node *)port);
                unit->roothub.port_count++;
            }
        }

        /* This is our root hub device descriptor */
        unit->roothub.devdesc.bLength            = sizeof(struct UsbStdDevDesc);
        unit->roothub.devdesc.bDescriptorType    = UDT_DEVICE;
        unit->roothub.devdesc.bDeviceClass       = HUB_CLASSCODE;
        unit->roothub.devdesc.bDeviceSubClass    = 0;
        unit->roothub.devdesc.bDeviceProtocol    = 0;
        unit->roothub.devdesc.bMaxPacketSize0    = 8; // Valid values are 8, 16, 32, 64
        unit->roothub.devdesc.idVendor           = AROS_WORD2LE(0x0000);
        unit->roothub.devdesc.idProduct          = AROS_WORD2LE(0x0000);
        unit->roothub.devdesc.bcdDevice          = AROS_WORD2LE(0x0100);
        unit->roothub.devdesc.iManufacturer      = 0; //1 strings not yeat implemented
        unit->roothub.devdesc.iProduct           = 0; //2 strings not yeat implemented
        unit->roothub.devdesc.iSerialNumber      = 0;
        unit->roothub.devdesc.bNumConfigurations = 1;

        /* This is our root hub config descriptor */
        unit->roothub.config.cfgdesc.bLength      = sizeof(struct UsbStdCfgDesc);
        unit->roothub.config.cfgdesc.bLength             = sizeof(struct UsbStdCfgDesc);
        unit->roothub.config.cfgdesc.bDescriptorType     = UDT_CONFIGURATION;
        unit->roothub.config.cfgdesc.wTotalLength        = AROS_WORD2LE(sizeof(struct RHConfig));
        unit->roothub.config.cfgdesc.bNumInterfaces      = 1;
        unit->roothub.config.cfgdesc.bConfigurationValue = 1;
        unit->roothub.config.cfgdesc.iConfiguration      = 0; // 3 strings not yeat implemented
        unit->roothub.config.cfgdesc.bmAttributes        = (USCAF_ONE|USCAF_SELF_POWERED);
        unit->roothub.config.cfgdesc.bMaxPower           = 0;

        unit->roothub.config.ifdesc.bLength              = sizeof(struct UsbStdIfDesc);
        unit->roothub.config.ifdesc.bDescriptorType      = UDT_INTERFACE;
        unit->roothub.config.ifdesc.bInterfaceNumber     = 0;
        unit->roothub.config.ifdesc.bAlternateSetting    = 0;
        unit->roothub.config.ifdesc.bNumEndpoints        = 1;
        unit->roothub.config.ifdesc.bInterfaceClass      = HUB_CLASSCODE;
        unit->roothub.config.ifdesc.bInterfaceSubClass   = 0;
        unit->roothub.config.ifdesc.bInterfaceProtocol   = 0;
        unit->roothub.config.ifdesc.iInterface           = 0; //4 strings not yeat implemented

        unit->roothub.config.epdesc.bLength              = sizeof(struct UsbStdEPDesc);
        unit->roothub.config.epdesc.bDescriptorType      = UDT_ENDPOINT;
        unit->roothub.config.epdesc.bEndpointAddress     = (URTF_IN|1);
        unit->roothub.config.epdesc.bmAttributes         = USEAF_INTERRUPT;
        unit->roothub.config.epdesc.wMaxPacketSize       = AROS_WORD2LE(8);
        unit->roothub.config.epdesc.bInterval            = 12;

        /* This is our root hub hub descriptor */
        if(bcdusb == 0x200) {
            unit->roothub.hubdesc.usb20.bLength             = sizeof(struct UsbHubDesc);
            unit->roothub.hubdesc.usb20.bDescriptorType     = UDT_HUB;
            unit->roothub.hubdesc.usb20.bNbrPorts           = (UBYTE) unit->roothub.port_count;
            unit->roothub.hubdesc.usb20.wHubCharacteristics = AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP);
            unit->roothub.hubdesc.usb20.bPwrOn2PwrGood      = 0;
            unit->roothub.hubdesc.usb20.bHubContrCurrent    = 1;
            unit->roothub.hubdesc.usb20.DeviceRemovable     = 1;
            unit->roothub.hubdesc.usb20.PortPwrCtrlMask     = 0;
        } else {
            unit->roothub.hubdesc.usb30.bLength             = sizeof(struct UsbSSHubDesc);
            unit->roothub.hubdesc.usb30.bDescriptorType     = UDT_SSHUB;
            unit->roothub.hubdesc.usb30.bNbrPorts           = (UBYTE) unit->roothub.port_count;;
            unit->roothub.hubdesc.usb30.wHubCharacteristics = AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP);
            unit->roothub.hubdesc.usb30.bPwrOn2PwrGood      = 0;
            unit->roothub.hubdesc.usb30.bHubContrCurrent    = 10;
            unit->roothub.hubdesc.usb30.bHubHdrDecLat       = 0;
            unit->roothub.hubdesc.usb30.wHubDelay           = 0;
            unit->roothub.hubdesc.usb30.DeviceRemovable     = 0;
        }

        D(bug("[VXHCI] VXHCI_AddNewUnit:\n");
        bug("        Created new unit numbered %d at %p\n",unit->number, unit);
        bug("        Unit node name %s\n", unit->node.ln_Name));

        D(switch(bcdusb) {
            case 0x200:
                bug("        Unit type: USB2.0\n");
                break;
            case 0x300:
                bug("        Unit type: USB3.0\n");
                break;
            default:
                bug("        Unit type: %lx (Error?)\n", bcdusb);
                break;
        });

        D(switch(unit->state) {
            case UHSF_SUSPENDED:
                bug("        Unit state: UHSF_SUSPENDED\n");
                break;
            case UHSF_OPERATIONAL:
                bug("        Unit state: UHSF_OPERATIONAL\n");
                break;
            default:
                bug("        Unit state: %lx (Error?)\n", unit->state);
                break;
        });


        return unit;
    }
}

struct VXHCIPort *VXHCI_AddNewPort(struct VXHCIUnit *unit, ULONG portnum) {
    struct VXHCIPort *port;

    port = AllocVec(sizeof(struct VXHCIPort), MEMF_ANY|MEMF_CLEAR);
    if(port == NULL) {
        bug("[VXHCI] VXHCI_AddNewPort: Failed to create new port structure\n");
        return NULL;
    } else {
        port->node.ln_Type = NT_USER;
        /* Poseidon treats port number 0 as roothub */
        port->number = portnum+1;
        if(unit->roothub.devdesc.bcdUSB == 0x200) {
            sprintf(port->name, "VXHCI_USB20[%d:%d]", unit->number, port->number);
        } else {
            sprintf(port->name, "VXHCI_USB30[%d:%d]", unit->number, port->number);
        }
        port->node.ln_Name = (STRPTR)&port->name;
    }


    D(bug("[VXHCI] VXHCI_AddNewPort:\n");
    bug("        Created new port numbered %d at %p\n",port->number, port);
    bug("        Port node name %s\n", port->node.ln_Name));


    return port;
}

