/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <aros/macros.h>

#include <proto/exec.h>
#include <proto/stdc.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>

#include "vxhci_device.h"

#include LC_LIBDEFS_FILE

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq) {
    mybug(0, ("[VXHCI] cmdQueryDevice: Entering function\n"));

    struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;

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
            case UHA_ProductName:
                {
                    static char productname[100];
                    sprintf(productname, "VXHCI (USB%x.%x ports)", AROS_LE2WORD(unit->roothub.devdesc.bcdUSB>>8)&0xf, AROS_LE2WORD(unit->roothub.devdesc.bcdUSB>>4)&0xf);
                    *((STRPTR *) tag->ti_Data) = productname;
                    count++;
                }
                break;
            case UHA_Description:
                {
                    static char description[100];
                    sprintf(description, "Virtual XHCI (USB%x.%x ports)", AROS_LE2WORD(unit->roothub.devdesc.bcdUSB>>8)&0xf, AROS_LE2WORD(unit->roothub.devdesc.bcdUSB>>4)&0xf);
                    *((STRPTR *) tag->ti_Data) = description;
                }
                count++;
                break;
            case UHA_Capabilities:
#if(1)
                if( (AROS_LE2WORD(unit->roothub.devdesc.bcdUSB) >= 0x200) && (AROS_LE2WORD(unit->roothub.devdesc.bcdUSB) < 0x300)) {
                    *((ULONG *) tag->ti_Data) = (UHCF_USB20);
                } else {
                    *((ULONG *) tag->ti_Data) = (UHCF_USB30);
                }
#else
                *((ULONG *) tag->ti_Data) = (UHCF_USB20|UHCF_USB30);
#endif
                count++;
                break;
            default:
                break;
        }
    }

    mybug_unit(0, ("Done\n\n"));

    ioreq->iouh_Actual = count;
    return RC_OK;
}

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq) {
    ioreq->iouh_Req.io_Error = IOERR_ABORTED;
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message */
    if (!(ioreq->iouh_Req.io_Flags & IOF_QUICK)) {
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }

    return TRUE;
}

WORD cmdUsbReset(struct IOUsbHWReq *ioreq) {
    mybug(0, ("[VXHCI] cmdUsbReset: Entering function\n"));

    struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;

    /* We should do a proper reset sequence with a real driver */
    unit->state = UHSF_RESET;
    unit->roothub.addr = 0;
    unit->state = UHSF_OPERATIONAL;
    mybug_unit(0, ("Done\n\n"));
    return RC_OK;
}

WORD cmdControlXFer(struct IOUsbHWReq *ioreq) {
    mybug(0, ("[VXHCI] cmdControlXFer: Entering function\n"));

    struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(0, ("ioreq->iouh_DevAddr %lx\n", ioreq->iouh_DevAddr));
    mybug_unit(0, ("unit->roothub.addr %lx\n", unit->roothub.addr));

    /*
        Check the status of the controller
        We might encounter these states:
        UHSB_OPERATIONAL USB can be used for transfers
        UHSB_RESUMING    USB is currently resuming
        UHSB_SUSPENDED   USB is in suspended state
        UHSB_RESET       USB is just inside a reset phase
    */

    if(unit->state == UHSF_OPERATIONAL) {
        mybug_unit(0, ("Unit state is operational\n"));
    } else {
        mybug_unit(-1, ("Unit state is not operational!\n"));
        return UHIOERR_USBOFFLINE;
    }

    if(ioreq->iouh_DevAddr == unit->roothub.addr) {
        return(cmdControlXFerRootHub(ioreq));
    }

    return RC_DONTREPLY;
}

WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq) {
    mybug(0, ("[VXHCI] cmdControlXFerRootHub: Entering function\n"));

    UWORD bmRequestType      = (ioreq->iouh_SetupData.bmRequestType) & (URTF_STANDARD | URTF_CLASS | URTF_VENDOR);
    UWORD bmRequestDirection = (ioreq->iouh_SetupData.bmRequestType) & (URTF_IN | URTF_OUT);
    UWORD bmRequestRecipient = (ioreq->iouh_SetupData.bmRequestType) & (URTF_DEVICE | URTF_INTERFACE | URTF_ENDPOINT | URTF_OTHER);

    UWORD bRequest           = (ioreq->iouh_SetupData.bRequest);
    D(UWORD wIndex           = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex));
    UWORD wValue             = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD wLength            = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;
    struct VXHCIPort *port = NULL;

    /* Endpoint 0 is used for control transfers only and can not be assigned to any other function. */
    if(ioreq->iouh_Endpoint != 0) {
        mybug_unit(-1, ("Wrong endpoint number! %ld\n", ioreq->iouh_Endpoint));
        return UHIOERR_BADPARAMS;
    }

    /* Check the request */
    if(bmRequestDirection) {
        mybug_unit(0, ("Request direction is device to host\n"));

        switch(bmRequestType) {
            case URTF_STANDARD:
                mybug_unit(0, ("URTF_STANDARD\n"));

                switch(bmRequestRecipient) {
                    case URTF_DEVICE:
                        mybug_unit(0, ("URTF_DEVICE\n"));

                        switch(bRequest) {
                            case USR_GET_DESCRIPTOR:
                                mybug_unit(0, ("USR_GET_DESCRIPTOR\n"));

                                switch( (wValue>>8) ) {
                                    case UDT_DEVICE:
                                        mybug_unit(0, ("UDT_DEVICE\n"));
                                        mybug_unit(0, ("GetDeviceDescriptor (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.devdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(0, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;
                                        break;

                                    case UDT_CONFIGURATION:
                                        mybug_unit(0, ("UDT_CONFIGURATION\n"));
                                        mybug_unit(0, ("GetConfigDescriptor (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct RHConfig)) ? sizeof(struct RHConfig) : wLength;
                                        CopyMem((APTR) &unit->roothub.config, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        //bug("sizeof(struct RHConfig) = %ld (should be 25)\n", sizeof(struct RHConfig));
                                        mybug_unit(0, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;

                                        break;

                                    case UDT_STRING:
                                        mybug_unit(0, ("UDT_STRING id %d\n", (wValue & 0xff)));

                                        if(wLength > 1) {
                                            switch( (wValue & 0xff) ) {
                                                case 0:
                                                    mybug_unit(0, ("GetStringDescriptor (%ld)\n", wLength));

                                                    /* This is our root hub string descriptor */
                                                    struct UsbStdStrDesc *strdesc = (struct UsbStdStrDesc *) ioreq->iouh_Data;

                                                    strdesc->bLength         = sizeof(struct UsbStdStrDesc);
                                                    strdesc->bDescriptorType = UDT_STRING;

                                                    if(wLength > 3) {
                                                        strdesc->bString[0] = AROS_WORD2LE(0x0409); // English (Yankee)
                                                        ioreq->iouh_Actual = sizeof(struct UsbStdStrDesc);
                                                        mybug_unit(0, ("Done\n\n"));
                                                        return UHIOERR_NO_ERROR;
                                                    } else {
                                                        ioreq->iouh_Actual = wLength;
                                                        mybug_unit(0, ("Done\n\n"));
                                                        return UHIOERR_NO_ERROR;
                                                    }

                                                    break;

                                                case 1:
                                                    return cmdGetString(ioreq, "The AROS Development Team.");
                                                    break;

                                                case 2: {
                                                    char roothubname[100];
                                                    sprintf(roothubname, "VXHCI root hub (USB%x.%x)", AROS_LE2WORD(unit->roothub.devdesc.bcdUSB>>8)&0xf, AROS_LE2WORD(unit->roothub.devdesc.bcdUSB>>4)&0xf);
                                                    return cmdGetString(ioreq, roothubname);
                                                    break;
                                                    }

                                                case 3:
                                                    return cmdGetString(ioreq, "Standard Config");
                                                    break;

                                                case 4:
                                                    return cmdGetString(ioreq, "Hub interface");
                                                    break;

                                                default:
                                                    break;
                                            }
                                        }

                                        break;

                                    case UDT_INTERFACE:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_INTERFACE\n");
                                        break;

                                    case UDT_ENDPOINT:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_ENDPOINT\n");
                                        break;

                                    case UDT_DEVICE_QUALIFIER:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_DEVICE_QUALIFIER\n");
                                        break;

                                    case UDT_OTHERSPEED_QUALIFIER:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_OTHERSPEED_QUALIFIER\n");
                                        break;

                                    case UDT_INTERFACE_POWER:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_INTERFACE_POWER\n");
                                        break;

                                    case UDT_OTG:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_OTG\n");
                                        break;

                                    case UDT_DEBUG:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_DEBUG\n");
                                        break;

                                    case UDT_INTERFACE_ASSOCIATION:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_INTERFACE_ASSOCIATION\n");
                                        break;

                                    case UDT_SECURITY:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_SECURITY\n");
                                        break;

                                    case UDT_ENCRYPTION_TYPE:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_ENCRYPTION_TYPE\n");
                                        break;

                                    case UDT_BOS:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_BOS\n");
                                        break;

                                    case UDT_DEVICE_CAPABILITY:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_DEVICE_CAPABILITY\n");
                                        break;

                                    case UDT_WIRELESS_EP_COMP:
                                        bug("[VXHCI] cmdControlXFerRootHub: UDT_WIRELESS_EP_COMP\n");
                                        break;

                                    default:
                                        bug("[VXHCI] cmdControlXFerRootHub: switch( (wValue>>8) ) %ld\n", (wValue>>8));
                                        break;

                                } /* switch( (wValue>>8) ) */
                                break; /* case USR_GET_DESCRIPTOR */

                            case USR_GET_STATUS:
                                mybug_unit(0, ("USR_GET_STATUS\n"));
                                ((UWORD *) ioreq->iouh_Data)[0] = AROS_WORD2LE(U_GSF_SELF_POWERED);
                                ioreq->iouh_Actual = wLength;
                                mybug_unit(0, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                        } /* switch(bRequest) */
                        break; /* case URTF_DEVICE: */

                    /* switch(bmRequestRecipient) */
                    case URTF_INTERFACE:
                        mybug_unit(0, ("URTF_INTERFACE\n"));
                        break;

                    /* switch(bmRequestRecipient) */
                    case URTF_ENDPOINT:
                        mybug_unit(0, ("URTF_ENDPOINT\n"));
                        break;

                    /* switch(bmRequestRecipient) */
                    case URTF_OTHER:
                        mybug_unit(0, ("URTF_OTHER\n"));
                        break;

                    /* switch(bmRequestRecipient) */
                    default:
                        mybug_unit(0, ("Request defaulting %ld\n", bRequest));
                        break;

                } /* switch(bmRequestRecipient) */
                break;

            /* switch(bmRequestType) */
            case URTF_CLASS:
                mybug_unit(0, ("URTF_CLASS\n"));

                switch(bmRequestRecipient) {
                    case URTF_DEVICE:
                        mybug_unit(0, ("URTF_DEVICE\n"));

                        switch(bRequest) {
                            case USR_GET_STATUS:
                                mybug_unit(-1, ("USR_GET_STATUS\n"));
                                UWORD *mptr = ioreq->iouh_Data;
                                if(wLength < sizeof(struct UsbHubStatus)) {
                                    break;
                                }
                                *mptr++ = 0;
                                *mptr++ = 0;
                                ioreq->iouh_Actual = 4;
                                mybug_unit(-1, ("Something done, check me...\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                            /* switch(bRequest) */
                            case USR_GET_DESCRIPTOR:
                                mybug_unit(0, ("[VXHCI] cmdControlXFerRootHub: USR_GET_DESCRIPTOR\n"));

                                switch( (wValue>>8) ) {
                                    case UDT_HUB:
                                        mybug_unit(0, ("UDT_HUB\n"));
                                        mybug_unit(0, ("GetRootHubDescriptor USB2.0 (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbHubDesc)) ? sizeof(struct UsbHubDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.hubdesc.usb20, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(0, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;
                                        break;

                                    /* switch( (wValue>>8) ) */
                                    case UDT_SSHUB:
                                        mybug_unit(0, ("UDT_SSHUB\n"));
                                        mybug_unit(0, ("GetRootHubDescriptor USB3.0 (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbSSHubDesc)) ? sizeof(struct UsbSSHubDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.hubdesc.usb30, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(0, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;
                                        break;

                                } /* switch( (wValue>>8) ) */

                                mybug_unit(0, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                        } /* switch(bRequest) */
                        break;

                    /* switch(bmRequestRecipient) */
                    case URTF_OTHER:
                        mybug_unit(0, ("URTF_OTHER\n"));

                        switch(bRequest) {
                            case USR_GET_STATUS:
                                mybug_unit(0, ("USR_GET_STATUS\n"));
                                if(wLength != sizeof(struct UsbPortStatus)) {
                                    mybug_unit(-1, ("Invalid port status structure!\n\n"));
                                    break;
                                }

                                ForeachNode(&unit->roothub.port_list, port) {
                                    if(port->number == wIndex) {
                                        mybug_unit(0, ("Found port %d named %s\n", port->number, port->name));

                                        struct UsbPortStatus *usbportstatus = (struct UsbPortStatus *) ioreq->iouh_Data;

                                        usbportstatus->wPortStatus = 0;
                                        usbportstatus->wPortChange = 0;

                                        mybug_unit(0, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;
                                    }
                                }

                                mybug_unit(-1, ("Port not found!\n\n"));
                                break;

                        } /* switch(bRequest) */
                        break;

                } /* case URTF_CLASS */
                break;

            /* switch(bmRequestType) */
            case URTF_VENDOR:
                mybug_unit(0, ("URTF_VENDOR\n"));
                break;

        } /* switch(bmRequestType) */

    } else { /* if(bmRequestDirection) */
        mybug_unit(0, ("Request direction is host to device\n"));

        switch(bmRequestType) {
            case URTF_STANDARD:
                mybug_unit(0, ("[VXHCI] cmdControlXFerRootHub: URTF_STANDARD\n"));

                switch(bmRequestRecipient) {
                    case URTF_DEVICE:
                        mybug_unit(0, ("[VXHCI] cmdControlXFerRootHub: URTF_DEVICE\n"));
                        switch(bRequest) {
                            case USR_SET_ADDRESS:
                                mybug_unit(0, ("USR_SET_ADDRESS\n"));
                                unit->roothub.addr = wValue;
                                ioreq->iouh_Actual = wLength;
                                mybug_unit(0, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                            case USR_SET_CONFIGURATION:
                                /* We do not have alternative configuration */
                                mybug_unit(0, ("USR_SET_CONFIGURATION\n"));
                                ioreq->iouh_Actual = wLength;
                                mybug_unit(0, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                        } /* switch(bRequest) */
                        break;

                    case URTF_INTERFACE:
                        mybug_unit(0, ("URTF_INTERFACE\n"));
                        break;

                    case URTF_ENDPOINT:
                        mybug_unit(0, ("URTF_ENDPOINT\n"));
                        break;

                    case URTF_OTHER:
                        mybug_unit(0, ("URTF_OTHER\n"));
                        break;

                } /* switch(bmRequestRecipient) */
                break;

            case URTF_CLASS:
                mybug_unit(0, ("URTF_CLASS\n"));
                switch(bmRequestRecipient) {
                    case URTF_OTHER:
                        mybug_unit(0, ("URTF_OTHER\n"));

                        switch(bRequest) {
                            case USR_SET_FEATURE:
                                mybug_unit(0, ("USR_SET_FEATURE\n"));

                                switch(wValue) {
                                    case UFS_PORT_POWER:
                                        mybug_unit(0, ("UFS_PORT_POWER\n"));

                                        ForeachNode(&unit->roothub.port_list, port) {
                                            if(port->number == wIndex) {
                                                mybug_unit(0, ("Found port %d named %s\n", port->number, port->name));
                                                mybug_unit(0, ("Done\n\n"));
                                                return UHIOERR_NO_ERROR;
                                            }
                                        }

                                        mybug_unit(-1, ("Port not found!\n\n"));
                                        break;

                                    case UFS_PORT_CONNECTION:
                                    case UFS_PORT_ENABLE:
                                    case UFS_PORT_SUSPEND:
                                    case UFS_PORT_OVER_CURRENT:
                                    case UFS_PORT_RESET:
                                    case UFS_PORT_LOW_SPEED:
                                    case UFS_C_PORT_CONNECTION:
                                    case UFS_C_PORT_ENABLE:
                                    case UFS_C_PORT_SUSPEND:
                                    case UFS_C_PORT_OVER_CURRENT:
                                    case UFS_C_PORT_RESET:
                                        break;
                                } /* switch(wValue) */
                                break;
                        } /* switch(bRequest) */
                        break;
                } /* switch(bmRequestRecipient) */
                break;

            case URTF_VENDOR:
                mybug_unit(0, ("URTF_VENDOR\n"));
                break;

        } /* switch(bmRequestType) */

    } /* if(bmRequestDirection) */

    D( mybug_unit(-1, ("bmRequestDirection "));
    switch (bmRequestDirection) {
        case URTF_IN:
            mybug(-1, ("URTF_IN\n"));
            break;
        case URTF_OUT:
            mybug(-1, ("URTF_OUT\n"));
            break;
    }

    mybug_unit(-1, ("bmRequestType "));
    switch(bmRequestType) {
        case URTF_STANDARD:
            mybug(-1, ("URTF_STANDARD\n"));
            break;
        case URTF_CLASS:
            mybug(-1, ("URTF_CLASS\n"));
            break;
        case URTF_VENDOR:
            mybug(-1, ("URTF_VENDOR\n"));
            break;
    }

    mybug_unit(-1, ("bmRequestRecipient "));
    switch (bmRequestRecipient) {
        case URTF_DEVICE:
            mybug(-1, ("URTF_DEVICE\n"));
            break;
        case URTF_INTERFACE:
            mybug(-1, ("URTF_INTERFACE\n"));
            break;
        case URTF_ENDPOINT:
            mybug(-1, ("URTF_ENDPOINT\n"));
            break;
        case URTF_OTHER:
            mybug(-1, ("URTF_OTHER\n"));
            break;
    }

    mybug_unit(-1, ("bRequest "));
    switch(bRequest) {
        case USR_GET_STATUS:
            bug("USR_GET_STATUS\n");
            break;
        case USR_CLEAR_FEATURE:
            mybug(-1, ("USR_CLEAR_FEATURE\n"));
            break;
        case USR_SET_FEATURE:
            mybug(-1, ("USR_SET_FEATURE\n"));
            break;
        case USR_SET_ADDRESS:
            mybug(-1, ("USR_SET_ADDRESS\n"));
            break;
        case USR_GET_DESCRIPTOR:
            mybug(-1, ("USR_GET_DESCRIPTOR\n"));
            break;
        case USR_SET_DESCRIPTOR:
            mybug(-1, ("USR_SET_DESCRIPTOR\n"));
            break;
        case USR_GET_CONFIGURATION:
            mybug(-1, ("USR_GET_CONFIGURATION\n"););
            break;
        case USR_SET_CONFIGURATION:
            mybug(-1, ("USR_SET_CONFIGURATION\n"));
            break;
        case USR_GET_INTERFACE:
            mybug(-1, ("USR_GET_INTERFACE\n"));
            break;
        case USR_SET_INTERFACE:
            mybug(-1, ("USR_SET_INTERFACE\n"));
            break;
        case USR_SYNCH_FRAME:
            mybug(-1, ("USR_SYNCH_FRAME\n"));
            break;
    }

    mybug_unit(-1, ("wIndex %x\n", wIndex));
    mybug_unit(-1, ("wValue %x\n", wValue));
    mybug_unit(-1, ("wLength %d\n", wLength));

    mybug_unit(-1, ("Nothing done!\n\n")) );


    return UHIOERR_BADPARAMS;
}

WORD cmdIntXFer(struct IOUsbHWReq *ioreq) {
    mybug(-1, ("[VXHCI] cmdIntXFer: Entering function\n"));

    struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(-1, ("ioreq->iouh_DevAddr %lx\n", ioreq->iouh_DevAddr));
    mybug_unit(-1, ("unit->roothub.addr %lx\n", unit->roothub.addr));

    /*
        Check the status of the controller
        We might encounter these states:
        UHSB_OPERATIONAL USB can be used for transfers
        UHSB_RESUMING    USB is currently resuming
        UHSB_SUSPENDED   USB is in suspended state
        UHSB_RESET       USB is just inside a reset phase
    */

    if(unit->state == UHSF_OPERATIONAL) {
        mybug_unit(0, ("Unit state is operational\n"));
    } else {
        mybug_unit(-1, ("Unit state is not operational!\n"));
        return UHIOERR_USBOFFLINE;
    }

    if(ioreq->iouh_DevAddr == unit->roothub.addr) {
        mybug_unit(-1, ("Entering cmdIntXFerRootHub\n"));
        return(cmdIntXFerRootHub(ioreq));
    }

    mybug_unit(-1, ("Nothing done!\n\n"));
    return RC_DONTREPLY;
}

WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq) {
    mybug(-1, ("[VXHCI] cmdIntXFerRootHub: Entering function\n"));
    D(struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit);

    mybug_unit(-1, ("Nothing done!\n\n"));
    return RC_DONTREPLY;
}

WORD cmdGetString(struct IOUsbHWReq *ioreq, char *cstring) {
    mybug(0, ("[VXHCI] cmdGetString: Entering function\n"));

    UWORD wLength = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    struct UsbStdStrDesc *strdesc = (struct UsbStdStrDesc *) ioreq->iouh_Data;
    strdesc->bDescriptorType = UDT_STRING;
    strdesc->bLength = (strlen(cstring)*sizeof(strdesc->bString))+sizeof(strdesc->bLength) + sizeof(strdesc->bDescriptorType);

    if(wLength > 2) {
        ioreq->iouh_Actual = 2;
        while(ioreq->iouh_Actual<wLength) {
            strdesc->bString[(ioreq->iouh_Actual-2)/sizeof(strdesc->bString)] = AROS_WORD2LE(*cstring);
            ioreq->iouh_Actual += sizeof(strdesc->bString);
            cstring++;
            if(*cstring == 0) {
                mybug(0, ("[VXHCI] cmdGetString: Done\n\n"));
                return UHIOERR_NO_ERROR;
            }
        }

    } else {
        ioreq->iouh_Actual = wLength;
        mybug(0, ("[VXHCI] cmdGetString: Done\n\n"));
        return UHIOERR_NO_ERROR;
    }

    return UHIOERR_BADPARAMS;
}
