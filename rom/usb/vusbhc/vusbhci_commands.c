/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VUSBHCI USB host controller
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

#include <proto/exec.h>
#include <proto/stdc.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>

#include "vusbhci_device.h"

#include LC_LIBDEFS_FILE

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq) {
    mybug(0, ("[VUSBHCI] cmdQueryDevice: Entering function\n"));

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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
                *((STRPTR *) tag->ti_Data) ="©2015 The AROS Development Team";
                count++;
                break;
            case UHA_ProductName:
                *((STRPTR *) tag->ti_Data) ="VUSBHCI Host Controller";
                count++;
                break;
            case UHA_Description:
                *((STRPTR *) tag->ti_Data) ="Hosted Host Controller Interface (libusb)";
                count++;
                break;
            case UHA_Capabilities:
                *((ULONG *) tag->ti_Data) = (UHCF_USB20|UHCF_ISO);
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
    mybug(0, ("[VUSBHCI] cmdUsbReset: Entering function\n"));

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    /* We should do a proper reset sequence with a real driver */
    unit->state = UHSF_RESET;
    unit->roothub.addr = 0;
    unit->state = UHSF_OPERATIONAL;
    mybug_unit(0, ("Done\n\n"));
    return RC_OK;
}

WORD cmdControlXFer(struct IOUsbHWReq *ioreq) {
    mybug(0, ("[VUSBHCI] cmdControlXFer: Entering function\n"));

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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
    mybug(-1, ("[VUSBHCI] cmdControlXFerRootHub: Entering function\n"));

    UWORD bmRequestType      = (ioreq->iouh_SetupData.bmRequestType) & (URTF_STANDARD | URTF_CLASS | URTF_VENDOR);
    UWORD bmRequestDirection = (ioreq->iouh_SetupData.bmRequestType) & (URTF_IN | URTF_OUT);
    UWORD bmRequestRecipient = (ioreq->iouh_SetupData.bmRequestType) & (URTF_DEVICE | URTF_INTERFACE | URTF_ENDPOINT | URTF_OTHER);

    UWORD bRequest           = (ioreq->iouh_SetupData.bRequest);
    UWORD wIndex             = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD wValue             = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD wLength            = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    /* Endpoint 0 is used for control transfers only and can not be assigned to any other function. */
    if(ioreq->iouh_Endpoint != 0) {
        mybug_unit(-1, ("Wrong endpoint number! %ld\n", ioreq->iouh_Endpoint));
        return UHIOERR_BADPARAMS;
    }

    /*
        Check the request
            - In USB terminology, the direction of an endpoint (and transfers to or from them) is based on the host.
            - Thus, IN always refers to transfers to the host from a device and OUT always refers to transfers from the host to a device.
    */
    if(bmRequestDirection) {
        mybug_unit(-1, ("Request direction is device to host\n"));

        switch(bmRequestType) {
            case URTF_STANDARD:
                mybug_unit(-1, ("bmRequestType URTF_STANDARD\n"));

                switch(bmRequestRecipient) {
                    case URTF_DEVICE:
                        mybug_unit(-1, ("bmRequestRecipient URTF_DEVICE\n"));

                        switch(bRequest) {
                            case USR_GET_DESCRIPTOR:
                                mybug_unit(-1, ("bRequest USR_GET_DESCRIPTOR\n"));

                                switch( (wValue>>8) ) {
                                    case UDT_DEVICE:
                                        mybug_unit(-1, ("wValue>>8 UDT_DEVICE\n"));
                                        mybug_unit(-1, ("GetDeviceDescriptor (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.devdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(-1, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;
                                        break;

                                    case UDT_CONFIGURATION:
                                        mybug_unit(-1, ("wValue>>8 UDT_CONFIGURATION\n"));
                                        mybug_unit(-1, ("GetConfigDescriptor (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct RHConfig)) ? sizeof(struct RHConfig) : wLength;
                                        CopyMem((APTR) &unit->roothub.config, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        //bug("sizeof(struct RHConfig) = %ld (should be 25)\n", sizeof(struct RHConfig));
                                        mybug_unit(-1, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;

                                        break;

                                    case UDT_STRING:
                                        mybug_unit(-1, ("wValue>>8 UDT_STRING id %d wLength %d\n", (wValue & 0xff), wLength));

                                        if(wLength > 1) {
                                            switch( (wValue & 0xff) ) {
                                                case 0:
                                                    mybug_unit(-1, ("GetStringDescriptor (%ld)\n", wLength));

                                                    /* This is our root hub string descriptor */
                                                    struct UsbStdStrDesc *strdesc = (struct UsbStdStrDesc *) ioreq->iouh_Data;

                                                    strdesc->bLength         = sizeof(struct UsbStdStrDesc);
                                                    strdesc->bDescriptorType = UDT_STRING;

                                                    if(wLength > 3) {
                                                        strdesc->bString[0] = AROS_WORD2LE(0x0409); // English (Yankee)
                                                        ioreq->iouh_Actual = sizeof(struct UsbStdStrDesc);
                                                        mybug_unit(-1, ("Done\n\n"));
                                                        return UHIOERR_NO_ERROR;
                                                    } else {
                                                        ioreq->iouh_Actual = wLength;
                                                        mybug_unit(-1, ("Done\n\n"));
                                                        return UHIOERR_NO_ERROR;
                                                    }
                                                    break;

                                                case 1:
                                                    return cmdGetString(ioreq, "The AROS Development Team.");
                                                    break;

                                                case 2:
                                                    return cmdGetString(ioreq, "VUSBHCI root hub");
                                                    break;
                                                case 3:
                                                    return cmdGetString(ioreq, "Standard Config");
                                                       break;

                                                case 4:
                                                    return cmdGetString(ioreq, "Hub interface");
                                                    break;

                                                default:
                                                    mybug_unit(-1, ("Nothing done\n\n"));
                                                    break;
                                            }

                                            mybug_unit(-1, ("Nothing done\n\n"));
                                        }

                                        mybug_unit(-1, ("Nothing done\n\n"));
                                        break;

                                    case UDT_INTERFACE:
                                        mybug_unit(-1, ("wValue>>8 UDT_INTERFACE\n"));
                                        break;

                                    case UDT_ENDPOINT:
                                        mybug_unit(-1, ("wValue>>8 UDT_ENDPOINT\n"));
                                        break;

                                    case UDT_DEVICE_QUALIFIER:
                                        mybug_unit(-1, ("wValue>>8 UDT_DEVICE_QUALIFIER\n"));
                                        break;

                                    case UDT_OTHERSPEED_QUALIFIER:
                                        mybug_unit(-1, ("wValue>>8 UDT_OTHERSPEED_QUALIFIER\n"));
                                        break;

                                    case UDT_INTERFACE_POWER:
                                        mybug_unit(-1, ("wValue>>8 UDT_INTERFACE_POWER\n"));
                                        break;

                                    case UDT_OTG:
                                        mybug_unit(-1, ("wValue>>8 UDT_OTG\n"));
                                        break;

                                    case UDT_DEBUG:
                                        mybug_unit(-1, ("wValue>>8 UDT_DEBUG\n"));
                                        break;

                                    case UDT_INTERFACE_ASSOCIATION:
                                        mybug_unit(-1, ("wValue>>8 UDT_INTERFACE_ASSOCIATION\n"));
                                        break;

                                    case UDT_SECURITY:
                                        mybug_unit(-1, ("wValue>>8 UDT_SECURITY\n"));
                                        break;

                                    case UDT_ENCRYPTION_TYPE:
                                        mybug_unit(-1, ("wValue>>8 UDT_ENCRYPTION_TYPE\n"));
                                        break;

                                    case UDT_BOS:
                                        mybug_unit(-1, ("wValue>>8 UDT_BOS\n"));
                                        break;

                                    case UDT_DEVICE_CAPABILITY:
                                        mybug_unit(-1, ("wValue>>8 UDT_DEVICE_CAPABILITY\n"));
                                        break;

                                    case UDT_WIRELESS_EP_COMP:
                                        mybug_unit(-1, ("wValue>>8 UDT_WIRELESS_EP_COMP\n"));
                                        break;

                                    default:
                                        mybug_unit(-1, ("wValue>>8 %ld\n", (wValue>>8)));
                                        mybug_unit(-1, ("Nothing done\n\n"));
                                        break;

                                } /* switch( (wValue>>8) ) */

                                mybug_unit(-1, ("Nothing done\n\n"));
                                break; /* case USR_GET_DESCRIPTOR */

                            case USR_GET_STATUS:
                                mybug_unit(-1, ("bRequest USR_GET_STATUS\n"));
                                ((UWORD *) ioreq->iouh_Data)[0] = AROS_WORD2LE(U_GSF_SELF_POWERED);
                                ioreq->iouh_Actual = wLength;
                                mybug_unit(-1, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                            //case USR_GET_INTERFACE: //Undefined. Hubs are allowed to support only one interface.

                        } /* switch(bRequest) */

                        mybug_unit(-1, ("Nothing done\n\n"));
                        break; /* case URTF_DEVICE: */

                    /* switch(bmRequestRecipient) */
                    case URTF_INTERFACE:
                        mybug_unit(-1, ("bmRequestRecipient URTF_INTERFACE\n"));
                        break;

                    /* switch(bmRequestRecipient) */
                    case URTF_ENDPOINT:
                        mybug_unit(-1, ("bmRequestRecipient URTF_ENDPOINT\n"));
                        break;

                    /* switch(bmRequestRecipient) */
                    case URTF_OTHER:
                        mybug_unit(-1, ("bmRequestRecipient URTF_OTHER\n"));
                        break;

                    /* switch(bmRequestRecipient) */
                    default:
                        mybug_unit(-1, ("bmRequestRecipient %ld (unhandled)\n", bRequest));
                        break;

                } /* switch(bmRequestRecipient) */
                break;

            /* switch(bmRequestType) */
            case URTF_CLASS:
                mybug_unit(-1, ("bmRequestType URTF_CLASS\n"));

                switch(bmRequestRecipient) {
                    case URTF_DEVICE:
                        mybug_unit(-1, ("bmRequestRecipient URTF_DEVICE\n"));

                        switch(bRequest) {
                            case USR_GET_STATUS:
                                mybug_unit(-1, ("bRequest USR_GET_STATUS\n"));
                                UWORD *mptr = ioreq->iouh_Data;
                                if(wLength < sizeof(struct UsbHubStatus)) {
                                    break;
                                }
                                *mptr++ = 0;
                                *mptr++ = 0;
                                ioreq->iouh_Actual = 4;
                                //mybug_unit(-1, ("Something done, check me...\n\n"));
                                mybug_unit(-1, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                            /* switch(bRequest) */
                            case USR_GET_DESCRIPTOR:
                                mybug_unit(-1, ("bRequest USR_GET_DESCRIPTOR\n"));

                                switch( (wValue>>8) ) {
                                    case UDT_HUB:
                                        mybug_unit(-1, ("wValue>>8 UDT_HUB\n"));
                                        mybug_unit(-1, ("GetRootHubDescriptor USB2.0 (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbHubDesc)) ? sizeof(struct UsbHubDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.hubdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(-1, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;
                                        break;

                                } /* switch( (wValue>>8) ) */

                                mybug_unit(-1, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                        } /* switch(bRequest) */
                        break;

                    /* switch(bmRequestRecipient) */
                    case URTF_OTHER:
                        mybug_unit(-1, ("bmRequestRecipient URTF_OTHER\n"));

                        switch(bRequest) {
                            case USR_GET_STATUS:
                                mybug_unit(-1, ("bRequest USR_GET_STATUS\n"));

                                if(wLength != sizeof(struct UsbPortStatus)) {
                                    mybug_unit(-1, ("Invalid port status structure!\n\n"));
                                    return(UHIOERR_BADPARAMS);
                                    break;
                                }

                                if((!wIndex) || (wIndex > unit->roothub.hubdesc.bNbrPorts)) {
                                    mybug_unit(-1, ("Port %ld out of range\n\n", wIndex));
                                    return(UHIOERR_STALL);
                                }

                                struct UsbPortStatus *usbportstatus = (struct UsbPortStatus *) ioreq->iouh_Data;

                                usbportstatus->wPortStatus = 0;
                                usbportstatus->wPortChange = 0;

                                mybug_unit(-1, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;

                                mybug_unit(-1, ("Port not found!\n\n"));
                                break;

                        } /* switch(bRequest) */
                        break;

                } /* case URTF_CLASS */
                break;

            /* switch(bmRequestType) */
            case URTF_VENDOR:
                mybug_unit(0, ("bmRequestType URTF_VENDOR\n"));
                break;

        } /* switch(bmRequestType) */

    } else { /* if(bmRequestDirection) */
        mybug_unit(-1, ("Request direction is host to device\n"));

        switch(bmRequestType) {
            case URTF_STANDARD:
                mybug_unit(-1, ("bmRequestType URTF_STANDARD\n"));

                switch(bmRequestRecipient) {
                    case URTF_DEVICE:
                        mybug_unit(-1, ("bmRequestRecipient URTF_DEVICE\n"));

                        switch(bRequest) {
                            case USR_SET_ADDRESS:
                                mybug_unit(-1, ("bRequest USR_SET_ADDRESS\n"));
                                unit->roothub.addr = wValue;
                                ioreq->iouh_Actual = wLength;
                                mybug_unit(-1, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                            case USR_SET_CONFIGURATION:
                                /* We do not have alternative configuration */
                                mybug_unit(-1, ("bRequest USR_SET_CONFIGURATION\n"));
                                ioreq->iouh_Actual = wLength;
                                mybug_unit(-1, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                            //case USR_SET_INTERFACE: //Undefined. Hubs are allowed to support only one interface.

                        } /* switch(bRequest) */
                        break;

                    case URTF_INTERFACE:
                        mybug_unit(-1, ("bmRequestRecipient URTF_INTERFACE\n"));
                        break;

                    case URTF_ENDPOINT:
                        mybug_unit(-1, ("bmRequestRecipient URTF_ENDPOINT\n"));
                        break;

                    case URTF_OTHER:
                        mybug_unit(-1, ("bmRequestRecipient URTF_OTHER\n"));
                        break;

                } /* switch(bmRequestRecipient) */
                break;

            case URTF_CLASS:
                mybug_unit(-1, ("bmRequestType URTF_CLASS\n"));
                switch(bmRequestRecipient) {
                    case URTF_OTHER:
                        mybug_unit(-1, ("bmRequestRecipient URTF_OTHER\n"));

                        switch(bRequest) {
                            case USR_SET_FEATURE:
                                mybug_unit(-1, ("bRequest USR_SET_FEATURE\n"));

                                switch(wValue) {
                                    case UFS_PORT_POWER:
                                        mybug_unit(-1, ("wValue UFS_PORT_POWER\n"));

                                        if((!wIndex) || (wIndex > unit->roothub.hubdesc.bNbrPorts)) {
                                            mybug_unit(-1, ("Port %ld out of range\n\n", wIndex));
                                            return(UHIOERR_STALL);
                                        }

                                        mybug_unit(-1, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;
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
                mybug_unit(-1, ("bmRequestType URTF_VENDOR\n"));
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
    mybug(0, ("[VUSBHCI] cmdIntXFer: Entering function\n"));

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(-1, ("Entering function\n"));

    if((ioreq->iouh_Endpoint != 1) || (!ioreq->iouh_Length)) {
        mybug_unit(-1, ("UHIOERR_BADPARAMS\n"));
        return(UHIOERR_BADPARAMS); // was UHIOERR_STALL
    }

    if(unit->roothub.portchange) {
        mybug_unit(-1, ("Port change\n"));
        *((UBYTE *) ioreq->iouh_Data) = 1;
        ioreq->iouh_Actual = 1;
        unit->roothub.portchange = FALSE;
        return(0);
    }

    mybug_unit(-1, ("ioreq added to roothub io_queue\n"));

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    Disable();
    AddTail(&unit->roothub.io_queue, (struct Node *) ioreq);
    Enable();
    return(RC_DONTREPLY);
}


WORD cmdISOXFer(struct IOUsbHWReq *ioreq) {
    mybug(-1, ("[VUSBHCI] cmdISOXFer: Entering function\n"));

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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

    mybug_unit(-1, ("Nothing done!\n\n"));
    return RC_DONTREPLY;
}

void uhwCheckRootHubChanges(struct VUSBHCIUnit *unit) {
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("unit->roothub.portchange = %d\n", unit->roothub.portchange));

    struct IOUsbHWReq *ioreq;

    if(unit->roothub.portchange && unit->roothub.io_queue.lh_Head->ln_Succ) {
        mybug_unit(-1, ("Port change\n"));

        Disable();
        ioreq = (struct IOUsbHWReq *) unit->roothub.io_queue.lh_Head;
        while(((struct Node *) ioreq)->ln_Succ) {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);

            *((UBYTE *) ioreq->iouh_Data) = 1;
            ioreq->iouh_Actual = 1;

            ReplyMsg(&ioreq->iouh_Req.io_Message);
            ioreq = (struct IOUsbHWReq *) unit->roothub.io_queue.lh_Head;
        }
        unit->roothub.portchange = FALSE;
        Enable();
    }
}

WORD cmdGetString(struct IOUsbHWReq *ioreq, char *cstring) {
    mybug(0, ("[VUSBHCI] cmdGetString: Entering function\n"));

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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
                mybug_unit(-1, ("cmdGetString: Done\n\n"));
                return UHIOERR_NO_ERROR;
            }
        }

    } else {
        ioreq->iouh_Actual = wLength;
        mybug_unit(-1, ("cmdGetString: Done\n\n"));
        return UHIOERR_NO_ERROR;
    }

    mybug_unit(-1, ("UHIOERR_BADPARAMS = cmdGetString()\n\n"));
    return UHIOERR_BADPARAMS;
}
