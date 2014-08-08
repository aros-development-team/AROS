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
                if(unit->roothub.devdesc.bcdUSB == 0x200) {
                    *((STRPTR *) tag->ti_Data) = "VXHCI (USB2.0 ports)";
                } else {
                    *((STRPTR *) tag->ti_Data) = "VXHCI (USB3.0 ports)";
                }
                count++;
                break;
            case UHA_Description:
                if(unit->roothub.devdesc.bcdUSB == 0x200) {
                    *((STRPTR *) tag->ti_Data) = "Virtual XHCI (USB2.0 ports)";
                } else {
                    *((STRPTR *) tag->ti_Data) = "Virtual XHCI (USB3.0 ports)";
                }
                count++;
                break;
            case UHA_Capabilities:
#if(1)
                if(unit->roothub.devdesc.bcdUSB == 0x200) {
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

WORD cmdUsbReset(struct IOUsbHWReq *ioreq) {
    mybug(0, ("[VXHCI] cmdUsbReset: Entering function\n"));

    struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;

    /* We should do a proper reset sequence with a real driver */
    unit->state = UHSF_RESET;
    unit->roothub.addr = 0;
    unit->state = UHSF_RESUMING;
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
    D(UWORD wIndex             = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex));
    UWORD wValue             = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD wLength            = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    struct VXHCIUnit *unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;

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

                                        /*
                                            Poseidon first does a dummy psdPipeSetup(URTF_IN|URTF_STANDARD|URTF_DEVICE, USR_GET_DESCRIPTOR, UDT_DEVICE)
                                            with 8 byte transfer size. It will then set the address with psdPipeSetup(URTF_STANDARD|URTF_DEVICE, USR_SET_ADDRESS)
                                            After that Poseidon does again psdPipeSetup(URTF_IN|URTF_STANDARD|URTF_DEVICE, USR_GET_DESCRIPTOR, UDT_DEVICE) with
                                            8 byte transfer size to get the bMaxPacketSize0 for transfer sizes.
                                            Only after that will it read the whole descriptor.
                                        */
                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.devdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(0, ("Done\n\n"));
                                        return(0);
                                        break;

                                    case UDT_CONFIGURATION:
                                        mybug_unit(0, ("UDT_CONFIGURATION\n"));
                                        mybug_unit(0, ("GetConfigDescriptor (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct RHConfig)) ? sizeof(struct RHConfig) : wLength;
                                        CopyMem((APTR) &unit->roothub.config, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        //bug("sizeof(struct RHConfig) = %ld (should be 25)\n", sizeof(struct RHConfig));
                                        mybug_unit(0, ("Done\n\n"));
                                        return(0);

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
                                                        strdesc->bString[1] = AROS_WORD2LE(0x0409); // English (Yankee)
                                                        ioreq->iouh_Actual = sizeof(struct UsbStdStrDesc);
                                                        mybug_unit(0, ("Done\n\n"));
                                                        return(0);
                                                    } else {
                                                        ioreq->iouh_Actual = wLength;
                                                        mybug_unit(0, ("Done\n\n"));
                                                        return(0);
                                                    }

                                                    break;

                                                case 1:
                                                    return cmdGetString(ioreq, "The AROS Development Team.");
                                                    break;

                                                case 2: {
                                                    char roothubname[100];
                                                    sprintf(roothubname, "VXHCI USB%d.0 Root Hub", (unit->roothub.devdesc.bcdUSB == 0x200) ? 2 : 3);
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
                                return(0);
                                break;

                        } /* switch(bRequest) */
                        break; /* case URTF_DEVICE: */

                    case URTF_INTERFACE:
                        mybug_unit(0, ("URTF_INTERFACE\n"));
                        break;

                    case URTF_ENDPOINT:
                        mybug_unit(0, ("URTF_ENDPOINT\n"));
                        break;

                    case URTF_OTHER:
                        mybug_unit(0, ("URTF_OTHER\n"));
                        break;

                    default:
                        mybug_unit(0, ("Request defaulting %ld\n", bRequest));
                        break;

                } /* switch(bmRequestRecipient) */
                break;

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
                                    return(UHIOERR_STALL);
                                }
                                *mptr++ = 0;
                                *mptr++ = 0;
                                ioreq->iouh_Actual = 4;
                                mybug_unit(-1, ("Something done, check me...\n\n"));
                                return(0);
                                break;


                            case USR_GET_DESCRIPTOR:
                                mybug_unit(0, ("[VXHCI] cmdControlXFerRootHub: USR_GET_DESCRIPTOR\n"));

                                switch( (wValue>>8) ) {
                                    case UDT_HUB:
                                        mybug_unit(0, ("UDT_HUB\n"));
                                        mybug_unit(0, ("GetRootHubDescriptor USB2.0 (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbHubDesc)) ? sizeof(struct UsbHubDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.hubdesc.usb20, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(0, ("Done\n\n"));
                                        return(0);
                                        break;

                                    case UDT_SSHUB:
                                        mybug_unit(0, ("UDT_SSHUB\n"));
                                        mybug_unit(0, ("GetRootHubDescriptor USB3.0 (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbSSHubDesc)) ? sizeof(struct UsbSSHubDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.hubdesc.usb30, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(0, ("Done\n\n"));
                                        return(0);
                                        break;
                                }

                                mybug_unit(0, ("Done\n\n"));
                                return(0);
                                break;
                        }
                        break;

                } /* case URTF_CLASS */
                break;

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
                                return(0);
                                break;

                            case USR_SET_CONFIGURATION:
                                /* We do not have alternative configuration */
                                mybug_unit(0, ("USR_SET_CONFIGURATION\n"));
                                ioreq->iouh_Actual = wLength;
                                mybug_unit(0, ("Done\n\n"));
                                return(0);
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
                return(0);
            }
        }

    } else {
        ioreq->iouh_Actual = wLength;
        mybug(0, ("[VXHCI] cmdGetString: Done\n\n"));
        return(0);
    }

    return UHIOERR_BADPARAMS;
}
