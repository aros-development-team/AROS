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

#include <aros/io.h>
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

#include "pcixhci_intern.h"

#include LC_LIBDEFS_FILE

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq) {

    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(0, ("Entering function\n"));

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
                *((STRPTR *) tag->ti_Data) = "PCI XHCI USB 3.0 Host Controller Driver";
                count++;
                break;
            case UHA_Description:
                *((STRPTR *) tag->ti_Data) = "PCI Extensible Host Controller Interface";
                count++;
                break;
            case UHA_Capabilities:
                /*
                    ISOCHRONOUS:
                    - Guarantees access to bandwidth but a packet or frame maybe dropped now and then
                    - Isochronous transfers occur continuously and periodically.
                    - The maximum data payload size is specified in the endpoint descriptor of an Isochronous Endpoint
                    - Check if alternative interfaces with varying isochronous payload sizes exist.
                    - Data being sent on an isochronous endpoint can be less than the pre-negotiated size and may vary in length from transaction to transaction
                */
                *((ULONG *) tag->ti_Data) = (UHCF_USB30|UHCB_ISO);
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


/*
    Perform a hardware reset on the controller.
*/
WORD cmdReset(struct IOUsbHWReq *ioreq) {

    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(-1, ("Entering function\n"));

    if(PCIXHCI_HCReset(unit)) {
        mybug_unit(-1, ("Done with OK\n\n"));
        return RC_OK;
    } else {
        mybug_unit(-1, ("Done with not OK\n\n"));
        return UHIOERR_HOSTERROR;
    }
}

/*
    We get called if the HW reset succeeded
*/
WORD cmdUsbReset(struct IOUsbHWReq *ioreq) {

    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;
    struct PCIXHCIPort *port = NULL;

    mybug_unit(0, ("Entering function\n"));

    /* (Re)build descriptors */
    /* This is our root hub device descriptor */
    unit->roothub.devdesc.bLength                       = sizeof(struct UsbStdDevDesc);
    unit->roothub.devdesc.bDescriptorType               = UDT_DEVICE;
    unit->roothub.devdesc.bcdUSB                        = AROS_WORD2LE(0x0300);
    unit->roothub.devdesc.bDeviceClass                  = HUB_CLASSCODE;
    unit->roothub.devdesc.bDeviceSubClass               = 0;
    unit->roothub.devdesc.bDeviceProtocol               = 0;
    unit->roothub.devdesc.bMaxPacketSize0               = 9; // Valid values are 8, 9(SuperSpeed), 16, 32, 64
    unit->roothub.devdesc.idVendor                      = AROS_WORD2LE(0x0000);
    unit->roothub.devdesc.idProduct                     = AROS_WORD2LE(0x0000);
    unit->roothub.devdesc.bcdDevice                     = AROS_WORD2LE(0x0300);
    unit->roothub.devdesc.iManufacturer                 = 1;
    unit->roothub.devdesc.iProduct                      = 2;
    unit->roothub.devdesc.iSerialNumber                 = 0;
    unit->roothub.devdesc.bNumConfigurations            = 1;

    /* This is our root hub config descriptor */
    unit->roothub.config.cfgdesc.bLength                = sizeof(struct UsbStdCfgDesc);
    unit->roothub.config.cfgdesc.bDescriptorType        = UDT_CONFIGURATION;
    unit->roothub.config.cfgdesc.wTotalLength           = AROS_WORD2LE(sizeof(struct RHConfig));
    unit->roothub.config.cfgdesc.bNumInterfaces         = 1;
    unit->roothub.config.cfgdesc.bConfigurationValue    = 1;
    unit->roothub.config.cfgdesc.iConfiguration         = 3;
    unit->roothub.config.cfgdesc.bmAttributes           = (USCAF_ONE|USCAF_SELF_POWERED);
    unit->roothub.config.cfgdesc.bMaxPower              = 0;

    unit->roothub.config.ifdesc.bLength                 = sizeof(struct UsbStdIfDesc);
    unit->roothub.config.ifdesc.bDescriptorType         = UDT_INTERFACE;
    unit->roothub.config.ifdesc.bInterfaceNumber        = 0;
    unit->roothub.config.ifdesc.bAlternateSetting       = 0;
    unit->roothub.config.ifdesc.bNumEndpoints           = 1;
    unit->roothub.config.ifdesc.bInterfaceClass         = HUB_CLASSCODE;
    unit->roothub.config.ifdesc.bInterfaceSubClass      = 0;
    unit->roothub.config.ifdesc.bInterfaceProtocol      = 0;
    unit->roothub.config.ifdesc.iInterface              = 4;

    unit->roothub.config.epdesc.bLength                 = sizeof(struct UsbStdEPDesc);
    unit->roothub.config.epdesc.bDescriptorType         = UDT_ENDPOINT;
    unit->roothub.config.epdesc.bEndpointAddress        = (URTF_IN|1);
    unit->roothub.config.epdesc.bmAttributes            = USEAF_INTERRUPT;
    unit->roothub.config.epdesc.wMaxPacketSize          = AROS_WORD2LE(8);
    unit->roothub.config.epdesc.bInterval               = 12;

    /* This is our root hub hub descriptor */
    unit->roothub.hubdesc.bLength                       = sizeof(struct UsbSSHubDesc);
    unit->roothub.hubdesc.bDescriptorType               = UDT_SSHUB;
    unit->roothub.hubdesc.bNbrPorts                     = 0;
    ForeachNode(&unit->roothub.port_list, port) {
        unit->roothub.hubdesc.bNbrPorts++;
    }
    unit->roothub.hubdesc.wHubCharacteristics           = AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP);
    unit->roothub.hubdesc.bPwrOn2PwrGood                = 0;
    unit->roothub.hubdesc.bHubContrCurrent              = 10;
    unit->roothub.hubdesc.bHubHdrDecLat                 = 0;
    unit->roothub.hubdesc.wHubDelay                     = 0;
    unit->roothub.hubdesc.DeviceRemovable               = 0;
    unit->roothub.bosdesc.bLength                       = sizeof(struct UsbStdBOSDesc);
    unit->roothub.bosdesc.bDescriptorType               = UDT_BOS;
    /* TODO: Arbitrary, set real values according to the host controller */
    unit->roothub.bosdesc.wTotalLength                  = 16;
    unit->roothub.bosdesc.bNumDeviceCaps                = 2;

    /* Reset the address */
    unit->roothub.addr = 0;

    /* our unit is now in operational state */
    unit->state = UHSF_OPERATIONAL;

    mybug_unit(0, ("Done\n\n"));
    return RC_OK;
}

WORD cmdControlXFer(struct IOUsbHWReq *ioreq) {
    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(0, ("Entering function\n"));

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

    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;
    struct PCIXHCIPort *port = NULL;

    mybug_unit(0, ("Entering function\n"));

    UWORD bmRequestType      = (ioreq->iouh_SetupData.bmRequestType) & (URTF_STANDARD | URTF_CLASS | URTF_VENDOR);
    UWORD bmRequestDirection = (ioreq->iouh_SetupData.bmRequestType) & (URTF_IN | URTF_OUT);
    UWORD bmRequestRecipient = (ioreq->iouh_SetupData.bmRequestType) & (URTF_DEVICE | URTF_INTERFACE | URTF_ENDPOINT | URTF_OTHER);

    UWORD bRequest           = (ioreq->iouh_SetupData.bRequest);
    UWORD wIndex             = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD wValue             = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD wLength            = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

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
                                                    snprintf(roothubname, 99, "XHCI Root Hub Unit %d", unit->number) ;
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
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_INTERFACE\n");
                                        break;

                                    case UDT_ENDPOINT:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_ENDPOINT\n");
                                        break;

                                    case UDT_DEVICE_QUALIFIER:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_DEVICE_QUALIFIER\n");
                                        break;

                                    case UDT_OTHERSPEED_QUALIFIER:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_OTHERSPEED_QUALIFIER\n");
                                        break;

                                    case UDT_INTERFACE_POWER:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_INTERFACE_POWER\n");
                                        break;

                                    case UDT_OTG:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_OTG\n");
                                        break;

                                    case UDT_DEBUG:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_DEBUG\n");
                                        break;

                                    case UDT_INTERFACE_ASSOCIATION:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_INTERFACE_ASSOCIATION\n");
                                        break;

                                    case UDT_SECURITY:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_SECURITY\n");
                                        break;

                                    case UDT_ENCRYPTION_TYPE:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_ENCRYPTION_TYPE\n");
                                        break;

                                    case UDT_BOS:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_BOS\n");

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbStdBOSDesc)) ? sizeof(struct UsbStdBOSDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.bosdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                                        mybug_unit(0, ("Done\n\n"));
                                        return UHIOERR_NO_ERROR;
                                        break;

                                    case UDT_DEVICE_CAPABILITY:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_DEVICE_CAPABILITY\n");
                                        break;

                                    case UDT_WIRELESS_EP_COMP:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: UDT_WIRELESS_EP_COMP\n");
                                        break;

                                    default:
                                        bug("[PCIXHCI] cmdControlXFerRootHub: switch( (wValue>>8) ) %ld\n", (wValue>>8));
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
                                mybug_unit(0, ("[PCIXHCI] cmdControlXFerRootHub: USR_GET_DESCRIPTOR\n"));

                                switch( (wValue>>8) ) {
                                    case UDT_SSHUB:
                                        mybug_unit(0, ("UDT_SSHUB\n"));
                                        mybug_unit(0, ("GetRootHubDescriptor USB3.0 (%ld)\n", wLength));

                                        ioreq->iouh_Actual = (wLength > sizeof(struct UsbSSHubDesc)) ? sizeof(struct UsbSSHubDesc) : wLength;
                                        CopyMem((APTR) &unit->roothub.hubdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

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
                mybug_unit(0, ("[PCIXHCI] cmdControlXFerRootHub: URTF_STANDARD\n"));

                switch(bmRequestRecipient) {
                    case URTF_DEVICE:
                        mybug_unit(0, ("[PCIXHCI] cmdControlXFerRootHub: URTF_DEVICE\n"));

                        switch(bRequest) {
                            case USR_SET_ADDRESS:
                                mybug_unit(0, ("USR_SET_ADDRESS\n"));
                                unit->roothub.addr = wValue;
                                ioreq->iouh_Actual = wLength;
                                mybug_unit(0, ("Done\n\n"));
                                return UHIOERR_NO_ERROR;
                                break;

                            case USR_SET_CONFIGURATION:
                                /* We do not have alternative configuration err... set the device to D0 state = go ?*/
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
                            case USR_CLEAR_FEATURE:
                                mybug_unit(0, ("USR_CLEAR_FEATURE\n"));

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

    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(-1, ("Entering function\n"));

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

    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("Nothing done!\n\n"));
    return RC_DONTREPLY;
}

WORD cmdGetString(struct IOUsbHWReq *ioreq, char *cstring) {

    struct PCIXHCIUnit *unit = (struct PCIXHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(-1, ("Entering function\n"));

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
                mybug_unit(0, ("Done\n\n"));
                return UHIOERR_NO_ERROR;
            }
        }

    } else {
        ioreq->iouh_Actual = wLength;
        mybug_unit(0, ("Done\n\n"));
        return UHIOERR_NO_ERROR;
    }

    return UHIOERR_BADPARAMS;
}
