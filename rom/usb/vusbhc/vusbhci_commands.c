/*
    Copyright © 2015-2017, The AROS Development Team. All rights reserved.
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
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>

#include "vusbhci_device.h"

/*
    FIXME: cmdAbortIO does not work for the request that is already passed to libusb
*/
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
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(0, ("Entering function\n"));

    /* We should do a proper reset sequence with a real driver */
    unit->state = UHSF_RESET;
    unit->roothub.addr = 0;
    unit->state = UHSF_OPERATIONAL;
    mybug_unit(0, ("Done\n\n"));
    return RC_OK;
}

/* Standard Requests */

/*
    GetStatus:
        bmRequestType (URTF_IN|URTF_STANDARD|URTF_DEVICE) 10000000B
        bRequest USR_GET_STATUS
        wValue Zero
        wIndex Zero
        wLength Two
        Data Device Status

    CHECKME: U_GSB_SELF_POWERED=8 and U_GSB_REMOTE_WAKEUP=9, should they be 0 and 1?

*/
UWORD GetStatus(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    UWORD *devicestatus;
    devicestatus = ioreq->iouh_Data;

    *devicestatus = (U_GSF_SELF_POWERED & (~U_GSF_REMOTE_WAKEUP));

    ioreq->iouh_Actual = wLength;

    mybug_unit(-1, ("GetStatus(%ld) %02x\n", wLength, *devicestatus));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    ClearFeature:
        bmRequestType (URTF_IN|URTF_STANDARD|URTF_DEVICE) 00000000B
        bRequest CLEAR_FEATURE
        wValue Feature Selector
        wIndex Zero
        wLength Zero
        Data None
*/
UWORD ClearFeature(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    SetFeature:
        bmRequestType (URTF_IN|URTF_STANDARD|URTF_DEVICE) 00000000B
        bRequest SET_FEATURE
        wValue Feature Selector
        wIndex Zero
        wLength Zero
        Data None
*/
UWORD SetFeature(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    SetAddress:
        bmRequestType   (URTF_OUT|URTF_STANDARD|URTF_DEVICE) 00000000B
        bRequest        USR_SET_ADDRESS
        wValue          Device Address
        wIndex          Zero
        wLength         Zero
        Data            None
*/
UWORD SetAddress(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("SetAddress (address %d)\n", wValue));

    /* It is a Request Error if wValue, wIndex, or wLength are other than as specified above. */
    if( (wValue) && (!(wIndex)) && (!(wLength)) ) {

        unit->roothub.addr = wValue;
        ioreq->iouh_Actual = wLength;

        mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
        return UHIOERR_NO_ERROR;
    }

    mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
    return UHIOERR_BADPARAMS;
}

/*
    GetDescriptor:
        bmRequestType   (URTF_OUT|URTF_STANDARD|URTF_DEVICE) 10000000B
        bRequest        GET_DESCRIPTOR
        wValue          Descriptor Type & Index
        wIndex          Zero or Language ID
        wLength         Descriptor Length
        Data            Descriptor
*/ 	 	 	 	
UWORD GetDescriptor(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    CONST_STRPTR roothubstring = NULL;
    CONST_STRPTR roothubstrings[] = {"The AROS Development Team.", "VUSBHCI root hub (USB2.00)", "VUSBHCI root hub (USB3.00)", "Standard Config", "Hub interface" };
    UBYTE        index;

    switch((wValue>>8)) {
        case UDT_DEVICE:
            mybug_unit(-1, ("GetDeviceDescriptor UDT_DEVICE (length %ld)\n", wLength));

            ioreq->iouh_Actual = (wLength > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : wLength;
            CopyMem((APTR) &unit->roothub.devdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

            mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
            return UHIOERR_NO_ERROR;
        break;

        case UDT_CONFIGURATION:
            index = (wValue & 0xff);

            mybug_unit(-1, ("GetDeviceDescriptor UDT_CONFIGURATION (configuration %d, length %ld)\n",index, wLength));

            if(index == 0) {

                ioreq->iouh_Actual = (wLength > sizeof(struct RHConfig)) ? sizeof(struct RHConfig) : wLength;
                CopyMem((APTR) &unit->roothub.config, ioreq->iouh_Data, ioreq->iouh_Actual);

                mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
                return UHIOERR_NO_ERROR;
            }

            mybug_unit(-1, ("Our roothub supports only one configuration\n"));
            mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
            return UHIOERR_BADPARAMS;
        break;

        case UDT_STRING:
            index = (wValue & 0xff);

            mybug_unit(-1, ("GetStringDescriptor UDT_STRING (index %d)\n", index));

            struct UsbStdStrDesc *strdesc = (struct UsbStdStrDesc *) ioreq->iouh_Data;

            switch(index) {
                case 0:
                    if(wLength > 1) {
                        strdesc->bLength = sizeof(struct UsbStdStrDesc);
                        strdesc->bDescriptorType = UDT_STRING;
                        ioreq->iouh_Actual = 2;

                        if(wLength > 3) {
                            strdesc->bString[0] = AROS_WORD2LE(0x0409); // English (Yankee)
                            ioreq->iouh_Actual = sizeof(struct UsbStdStrDesc);
                        }

                        mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
                        return UHIOERR_NO_ERROR;
                    }

                    mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
                    return UHIOERR_BADPARAMS; //CHECKME: Should we return stall?
                break;

                case 1:
                    roothubstring = roothubstrings[0];
                break;

                case 2:
                    if(unit->roothub.devdesc.bcdUSB == AROS_WORD2LE(0x0200)) {
                        roothubstring = roothubstrings[1];
                    } else {
                        roothubstring = roothubstrings[2];
                    }
                break;

                case 3:
                    roothubstring = roothubstrings[3];
                break;

                case 4:
                    roothubstring = roothubstrings[4];
                break;

                default:
                    mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
                    return UHIOERR_BADPARAMS; //CHECKME: Should we return stall?
            }

            if(wLength > 1) {
                UBYTE i = strlen(roothubstring);

                strdesc->bLength = (i*sizeof(strdesc->bString))+sizeof(strdesc->bLength)+sizeof(strdesc->bDescriptorType);
                strdesc->bDescriptorType = UDT_STRING;
                ioreq->iouh_Actual = 2;

                if(wLength > 3) {
                    for(i=0; i<wLength; i++) {
                        strdesc->bString[i] = AROS_WORD2LE((UWORD)roothubstring[i]);
                    }
                }

                mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
                return UHIOERR_NO_ERROR;
            }

            mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
            return UHIOERR_BADPARAMS; //CHECKME: Should we return stall?
        break;

    }

    mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
    return UHIOERR_BADPARAMS; //CHECKME: Should we return stall?
}

/*
    SetDescriptor:
        bmRequestType   (URTF_OUT|URTF_STANDARD|URTF_DEVICE) 00000000B
        bRequest        SET_DESCRIPTOR
        wValue          Descriptor Type & Index
        wIndex          Zero or Language ID
        wLength         Descriptor Length
        Data            Descriptor
*/
UWORD SetDescriptor(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    GetConfiguration:
        bmRequestType   (URTF_IN|URTF_STANDARD|URTF_DEVICE) 10000000B
        bRequest        USR_GET_CONFIGURATION
        wValue          Zero
        wIndex          Zero
        wLength         1
        Data            Configuration Value
*/
UWORD GetConfiguration(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    UWORD *configurationvalue;
    configurationvalue = ioreq->iouh_Data;

    *configurationvalue =  unit->roothub.config.cfgdesc.bConfigurationValue;

    ioreq->iouh_Actual = wLength;

    mybug_unit(-1, ("GetConfiguration() %ld\n", configurationvalue));

    return UHIOERR_NO_ERROR;
}

/*
    SetConfiguration:
        bmRequestType   (URTF_OUT|URTF_STANDARD|URTF_DEVICE) 00000000B
        bRequest        USR_SET_CONFIGURATION
        wValue          Configuration Value
        wIndex          Zero
        wLength         Zero
        Data            None

    Note:
        We have only one configuration, but implement some sanity still
        If more than one configuration is specified we ignore the rest in GetDescriptor(UDT_CONFIGURATION)

*/
UWORD SetConfiguration(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("SetConfiguration (configuration %d)\n", wValue));

    /* It is a Request Error if wValue, wIndex, or wLength are other than as specified above. */
    if( (wValue == unit->roothub.config.cfgdesc.bConfigurationValue) && (!(wIndex)) && (!(wLength)) ) {

        ioreq->iouh_Actual = wLength;

        mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
        return UHIOERR_NO_ERROR;
    }

    mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
    return UHIOERR_BADPARAMS;
}

/* Hub Class Requests */

/*
    ClearHubFeature:
        bmRequestType   (URTF_OUT|URTF_CLASS|URTF_DEVICE) 00100000B
        bRequest        USR_CLEAR_FEATURE
        wValue          Feature Selector
        wIndex          Zero
        wLength         Zero
        Data            None
*/
UWORD ClearHubFeature(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    ClearPortFeature:
        bmRequestType (URTF_OUT|URTF_CLASS|URTF_OTHER) 00100011B
        bRequest USR_CLEAR_FEATURE
        wValue Feature Selector
        wIndex (Selector|Port)
        wLength Zero
        Data None

    FIXME: Check what flags to set and clear on this and SetPortFeature

*/
UWORD ClearPortFeature(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("Clearing feature 0x%02x on port %d\n",wValue, (wIndex & 0xff)));

    if( ((!(wIndex & 0xff)) || ((wIndex & 0xff) > unit->roothub.hubdesc.bNbrPorts)) ) {
        mybug_unit(-1, ("Port %ld out of range\n", (wIndex & 0xff)));
        mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
        return UHIOERR_BADPARAMS;
    }

    switch(wValue) {
        case UFS_PORT_ENABLE:
            mybug_unit(-1, ("UFS_PORT_ENABLE\n"));
            unit->roothub.portstatus.wPortStatus &= ~UPSF_PORT_ENABLE;
        break;

        case UFS_PORT_SUSPEND:
            mybug_unit(-1, ("UFS_PORT_SUSPEND\n"));
        break;

        case UFS_PORT_POWER:
            mybug_unit(-1, ("UFS_PORT_POWER\n"));
            unit->roothub.portstatus.wPortStatus &= ~UPSF_PORT_POWER;
        break;

        case UFS_PORT_INDICATOR:
            mybug_unit(-1, ("UFS_PORT_INDICATOR\n"));
            unit->roothub.portstatus.wPortChange &= ~UPSF_PORT_INDICATOR;
        break;

        case UFS_C_PORT_CONNECTION:
            mybug_unit(-1, ("UFS_C_PORT_CONNECTION\n"));
            unit->roothub.portstatus.wPortChange &= ~UPSF_PORT_CONNECTION;
        break;

        case UFS_C_PORT_RESET:
            mybug_unit(-1, ("UFS_C_PORT_RESET\n"));
            unit->roothub.portstatus.wPortChange &= ~UPSF_PORT_RESET;
        break;

        case UFS_C_PORT_ENABLE:
            mybug_unit(-1, ("UFS_C_PORT_ENABLE\n"));
            unit->roothub.portstatus.wPortChange &= ~UPSF_PORT_ENABLE;
        break;

        case UFS_C_PORT_SUSPEND:
            mybug_unit(-1, ("UFS_C_PORT_SUSPEND\n"));
            unit->roothub.portstatus.wPortChange &= ~UPSF_PORT_SUSPEND;
        break;

        case UFS_C_PORT_OVER_CURRENT:
            mybug_unit(-1, ("UFS_C_PORT_OVER_CURRENT\n"));
            unit->roothub.portstatus.wPortChange &= ~UPSF_PORT_OVER_CURRENT;
        break;
    }

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;

}

/*
    ClearTTBuffer:
        bmRequestType (URTF_OUT|URTF_CLASS|URTF_DEVICE) 00100000B
        bRequest USR_CLEAR_TT_BUFFER
        wValue Dev_Addr, EP_Num
        wIndex TT_port
        wLength Zero
        Data None
        
        Not for USB3
*/

UWORD ClearTTBuffer(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    GetHubDescriptor:
        bmRequestType (URTF_IN|URTF_CLASS|URTF_DEVICE) 10100000B
        bRequest USR_GET_DESCRIPTOR
        wValue Descriptor Type and Descriptor Index
        wIndex Zero (or language ID)
        wLength Descriptor Length
        Data Descriptor
*/
UWORD GetHubDescriptor(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    switch((wValue>>8)) {
        case UDT_HUB:
            mybug_unit(-1, ("GetHubDescriptor UDT_HUB (length %ld)\n", wLength));

            ioreq->iouh_Actual = (wLength > sizeof(struct UsbHubDesc)) ? sizeof(struct UsbHubDesc) : wLength;
            CopyMem((APTR) &unit->roothub.hubdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

            mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
            return UHIOERR_NO_ERROR;
        break;

/*
        case UDT_SSHUB:
            mybug_unit(-1, ("GetHubDescriptor UDT_SSHUB (length %ld)\n", wLength));

            ioreq->iouh_Actual = (wLength > sizeof(struct UsbHubSSDesc)) ? sizeof(struct UsbHubSSDesc) : wLength;
            CopyMem((APTR) &unit->roothub.hubdesc, ioreq->iouh_Data, ioreq->iouh_Actual);

            mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
            return UHIOERR_NO_ERROR;
        break;
*/
    }

    return UHIOERR_BADPARAMS;
}

/*
    GetHubStatus:
        bmRequestType (URTF_IN|URTF_CLASS|URTF_DEVICE) 10100000B
        bRequest USR_GET_STATUS
        wValue Zero
        wIndex Zero
        wLength Four
        Data Hub Status and Change Status
*/
UWORD GetHubStatus(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    /* It is a Request Error if wValue, wIndex, or wLength are other than as specified above. */
    if( (!(wValue)) && (!(wIndex)) && (wLength == 4) ) {

        struct UsbHubStatus *usbhubstatus = (struct UsbHubStatus *) ioreq->iouh_Data;

        usbhubstatus->wHubStatus = unit->roothub.hubstatus.wHubStatus;
        usbhubstatus->wHubChange = unit->roothub.hubstatus.wHubChange;

        mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
        return UHIOERR_NO_ERROR;
    }

    mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
    return UHIOERR_BADPARAMS;
}

/*
    GetPortStatus:
        bmRequestType (URTF_IN|URTF_CLASS|URTF_OTHER) 10100011B
        bRequest USR_GET_STATUS
        wValue Zero
        wIndex Port
        wLength Four
        Data Port Status and Change Status
*/
UWORD GetPortStatus(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("Port #%ld\n", wIndex));

    if( (wValue) || (wLength != 4) || (!wIndex) || (wIndex > unit->roothub.hubdesc.bNbrPorts) ) {
        mybug_unit(-1, ("Port %ld out of range\n", wIndex));
        mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
        return UHIOERR_BADPARAMS;
    }

    struct UsbPortStatus *usbportstatus = (struct UsbPortStatus *) ioreq->iouh_Data;

    /* We have only one port per 'controller' */
    usbportstatus->wPortStatus = unit->roothub.portstatus.wPortStatus;
    usbportstatus->wPortChange = unit->roothub.portstatus.wPortChange;

    mybug_unit(-1, ("usbportstatus->wPortStatus %01x\n", usbportstatus->wPortStatus));
    mybug_unit(-1, ("usbportstatus->wPortChange %01x\n", usbportstatus->wPortChange));

    if(usbportstatus->wPortStatus&UPSF_PORT_CONNECTION) {
        mybug_unit(-1, (" - UPSF_PORT_CONNECTION\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_ENABLE) {
        mybug_unit(-1, (" - UPSF_PORT_ENABLE\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_SUSPEND) {
        mybug_unit(-1, (" - UPSF_PORT_SUSPEND\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_OVER_CURRENT) {
        mybug_unit(-1, (" - UPSF_PORT_OVER_CURRENT\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_RESET) {
        mybug_unit(-1, (" - UPSF_PORT_RESET\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_POWER) {
        mybug_unit(-1, (" - UPSF_PORT_POWER\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_LOW_SPEED) {
        mybug_unit(-1, (" - UPSF_PORT_LOW_SPEED\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_HIGH_SPEED) {
        mybug_unit(-1, (" - UPSF_PORT_HIGH_SPEED\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_TEST_MODE) {
        mybug_unit(-1, (" - UPSF_PORT_TEST_MODE\n"));
    }

    if(usbportstatus->wPortStatus&UPSF_PORT_INDICATOR) {
        mybug_unit(-1, (" - UPSF_PORT_INDICATOR\n"));
    }

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    GetPortErrorCount:
        bmRequestType (URTF_IN|URTF_CLASS|URTF_OTHER) 10100011B
        bRequest USR_PORT_ERR_COUNT (Check: Has it been defined?)
        wValue Zero
        wIndex Port
        wLength Two
        Data Number of link errors on this port
*/
UWORD GetPortErrorCount(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    ResetTT:
        bmRequestType (URTF_OUT|URTF_CLASS|URTF_OTHER) 00100011B
        bRequest USR_RESET_TT
        wValue Zero
        wIndex Port
        wLength Zero
        Data None
        
        Not for USB3
*/
UWORD ResetTT(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    SetHubDescriptor:
        bmRequestType (URTF_OUT|URTF_CLASS|URTF_DEVICE) 00100000B
        bRequest USR_SET_DESCRIPTOR
        wValue Descriptor Type and Descriptor Index
        wIndex Zero or Language ID
        wLength Descriptor Length
        Data Descriptor
*/
UWORD SetHubDescriptor(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    SetHubFeature:
        bmRequestType (URTF_OUT|URTF_CLASS|URTF_DEVICE) 00100000B
        bRequest USR_SET_FEATURE
        wValue Feature Selector
        wIndex Zero
        wLength Zero
        Data None
*/
UWORD SetHubFeature(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    SetHubDescriptor:
        bmRequestType (URTF_OUT|URTF_CLASS|URTF_DEVICE) 00100000B
        bRequest USR_SET_HUB_DEPTH (Check: Has it been defined?)
        wValue Hub Depth
        wIndex Zero
        wLength Zero
        Data None
*/
UWORD SetHubDepth(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    SetPortFeature:
        bmRequestType (URTF_OUT|URTF_CLASS|URTF_OTHER) 00100011B
        bRequest USR_SET_FEATURE
        wValue Feature Selector
        wIndex (Selector|Port)
        wLength Zero
        Data None
    FIXME: Check what flags to set and clear on this and ClearPortFeature

*/
UWORD SetPortFeature(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("Setting feature 0x%02x on port %d\n",wValue, (wIndex & 0xff)));

    if( ((!(wIndex & 0xff)) || ((wIndex & 0xff) > unit->roothub.hubdesc.bNbrPorts)) ) {
        mybug_unit(-1, ("Port %ld out of range\n", (wIndex & 0xff)));
        mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
        return UHIOERR_BADPARAMS;
    }

    switch(wValue) {

        /* Features that can be set with this request */
        case UFS_PORT_RESET:
            mybug_unit(-1, ("UFS_PORT_RESET\n"));
            unit->roothub.portstatus.wPortStatus |= (UPSF_PORT_ENABLE|UPSF_PORT_POWER);
        break;

        case UFS_PORT_SUSPEND:
            mybug_unit(-1, ("UFS_PORT_SUSPEND\n"));
        break;

        case UFS_PORT_POWER:
            mybug_unit(-1, ("UFS_PORT_POWER\n"));
            unit->roothub.portstatus.wPortStatus |= UPSF_PORT_POWER;
        break;

        case UFS_PORT_TEST:
            mybug_unit(-1, ("UFS_PORT_TEST\n"));
        break;

        case UFS_PORT_INDICATOR:
            mybug_unit(-1, ("UFS_PORT_INDICATOR\n"));
        break;

        /* Features that can be set with this request but are not required */
        case UFS_C_PORT_CONNECTION:
            mybug_unit(-1, ("UFS_C_PORT_CONNECTION\n"));
        break;

        case UFS_C_PORT_RESET:
            mybug_unit(-1, ("UFS_C_PORT_RESET\n"));
        break;

        case UFS_C_PORT_ENABLE:
            mybug_unit(-1, ("UFS_C_PORT_ENABLE\n"));
        break;

        case UFS_C_PORT_SUSPEND:
            mybug_unit(-1, ("UFS_C_PORT_SUSPEND\n"));
        break;

        case UFS_C_PORT_OVER_CURRENT:
            mybug_unit(-1, ("UFS_C_PORT_OVER_CURRENT\n"));
        break;

        default:
            mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
            return UHIOERR_BADPARAMS;
        break;

    }

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    GetTTState:
        bmRequestType (URTF_IN|URTF_CLASS|URTF_OTHER) 10100011B
        bRequest USR_GET_TT_STATE
        wValue TT_Flags
        wIndex Port
        wLength TT State Length
        Data TT State
        
        Not for USB3
*/
UWORD GetTTState(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

/*
    StopTT:
        bmRequestType (URTF_OUT|URTF_CLASS|URTF_OTHER) 00100011B
        bRequest USR_STOP_TT
        wValue Zero
        wIndex Port
        wLength Zero
        Data None
        
        Not for USB3
*/
UWORD StopTT(struct IOUsbHWReq *ioreq, UWORD wValue, UWORD wIndex, UWORD wLength) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;
    mybug_unit(-1, ("Entering function\n"));

    mybug_unit(-1, ("return UHIOERR_NO_ERROR\n\n"));
    return UHIOERR_NO_ERROR;
}

WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq) {

    UWORD bmRequestType      = (ioreq->iouh_SetupData.bmRequestType) & (URTF_STANDARD | URTF_CLASS | URTF_VENDOR);
    UWORD bmRequestDirection = (ioreq->iouh_SetupData.bmRequestType) & (URTF_IN | URTF_OUT);
    UWORD bmRequestRecipient = (ioreq->iouh_SetupData.bmRequestType) & (URTF_DEVICE | URTF_INTERFACE | URTF_ENDPOINT | URTF_OTHER);

    UWORD bRequest           = (ioreq->iouh_SetupData.bRequest);
    UWORD wValue             = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD wIndex             = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD wLength            = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    /* Endpoint 0 is used for control transfers only and can not be assigned to any other function. */
    if(ioreq->iouh_Endpoint != 0) {
        mybug_unit(-1, ("Wrong endpoint number! %ld\n", ioreq->iouh_Endpoint));
        mybug_unit(-1, ("return UHIOERR_BADPARAMS\n\n"));
        return UHIOERR_BADPARAMS;
    }

    switch(((ULONG)ioreq->iouh_SetupData.bmRequestType<<16)|((ULONG)ioreq->iouh_SetupData.bRequest)) {

/* Standard Requests */
        case (((URTF_IN|URTF_STANDARD|URTF_DEVICE)<<16)|(USR_GET_STATUS)):
            return(GetStatus(ioreq, wValue, wIndex, wLength));

        case (((URTF_IN|URTF_STANDARD|URTF_DEVICE)<<16)|(USR_CLEAR_FEATURE)):
            return(ClearFeature(ioreq, wValue, wIndex, wLength));

        case (((URTF_IN|URTF_STANDARD|URTF_DEVICE)<<16)|(USR_SET_FEATURE)):
            return(SetFeature(ioreq, wValue, wIndex, wLength));

        case ((((URTF_OUT|URTF_STANDARD|URTF_DEVICE))<<16)|(USR_SET_ADDRESS)):
            return(SetAddress(ioreq, wValue, wIndex, wLength));

        case (((URTF_IN|URTF_STANDARD|URTF_DEVICE)<<16)|(USR_GET_DESCRIPTOR)):
            return(GetDescriptor(ioreq, wValue, wIndex, wLength));

        case (((URTF_OUT|URTF_STANDARD|URTF_DEVICE)<<16)|(USR_SET_DESCRIPTOR)):
            return(SetDescriptor(ioreq, wValue, wIndex, wLength));

        case ((((URTF_OUT|URTF_STANDARD|URTF_DEVICE))<<16)|(USR_GET_CONFIGURATION)):
            return(GetConfiguration(ioreq, wValue, wIndex, wLength));

        case ((((URTF_OUT|URTF_STANDARD|URTF_DEVICE))<<16)|(USR_SET_CONFIGURATION)):
            return(SetConfiguration(ioreq, wValue, wIndex, wLength));

/* Hub Class Requests. Check here if the command is for USB2 or USB3 hub...  */
        case (((URTF_OUT|URTF_CLASS|URTF_DEVICE)<<16)|(USR_CLEAR_FEATURE)):
            return(ClearHubFeature(ioreq, wValue, wIndex, wLength));

        case (((URTF_OUT|URTF_CLASS|URTF_OTHER)<<16)|(USR_CLEAR_FEATURE)):
            return(ClearPortFeature(ioreq, wValue, wIndex, wLength));

        case (((URTF_OUT|URTF_CLASS|URTF_DEVICE)<<16)|(USR_SET_FEATURE)):
            return(SetHubFeature(ioreq, wValue, wIndex, wLength));

        case (((URTF_OUT|URTF_CLASS|URTF_OTHER)<<16)|(USR_SET_FEATURE)):
            return(SetPortFeature(ioreq, wValue, wIndex, wLength));

        case (((URTF_IN|URTF_CLASS|URTF_DEVICE)<<16)|(USR_GET_DESCRIPTOR)):
            return(GetHubDescriptor(ioreq, wValue, wIndex, wLength));

        case ((((URTF_IN|URTF_CLASS|URTF_DEVICE))<<16)|(USR_GET_STATUS)):
            return(GetHubStatus(ioreq, wValue, wIndex, wLength));

        case ((((URTF_IN|URTF_CLASS|URTF_OTHER))<<16)|(USR_GET_STATUS)):
            return(GetPortStatus(ioreq, wValue, wIndex, wLength));

//        case (((( X ))<<16)|( X )):
//            return( X (ioreq));

        default:
            break;
    }

    mybug_unit(-1, ("Nothing done!\n\n"));
    return UHIOERR_BADPARAMS;
}

WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(0, ("Entering function\n"));

    if((ioreq->iouh_Endpoint != 1) || (!ioreq->iouh_Length)) {
        mybug_unit(-1, ("UHIOERR_BADPARAMS\n"));
        return(UHIOERR_BADPARAMS); // was UHIOERR_STALL
    }

#if 0
    if(unit->roothub.portstatus.wPortChange) {
        mybug_unit(-1, ("unit->roothub.portstatus.wPortChange = %02x\n", unit->roothub.portstatus.wPortChange));
        *((UBYTE *) ioreq->iouh_Data) = unit->roothub.portstatus.wPortChange;
        ioreq->iouh_Actual = 1;
        unit->roothub.portstatus.wPortChange &= ~UPSF_PORT_CONNECTION;
        mybug_unit(-1, ("unit->roothub.portstatus.wPortChange = %02x\n", unit->roothub.portstatus.wPortChange));
        return(0);
    }
#endif

    mybug_unit(0, ("ioreq added to roothub intrxfer_queue\n"));

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ObtainSemaphore(&unit->roothub.intrxfer_queue_lock);
    AddTail(&unit->roothub.intrxfer_queue, (struct Node *) ioreq);
    ReleaseSemaphore(&unit->roothub.intrxfer_queue_lock);

    return(RC_DONTREPLY);
}


WORD cmdControlXFer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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

    mybug_unit(0, ("Sending transfer request to libusb\n\n"));
    return(do_libusb_ctrl_transfer(ioreq));
}

WORD cmdIntXFer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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
        return(cmdIntXFerRootHub(ioreq));
    }

    mybug_unit(0, ("Adding INTR transfer request to queue\n"));
    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    ObtainSemaphore(&unit->intrxfer_queue_lock);
    AddTail(&unit->intrxfer_queue, (struct Node *) ioreq);
    ReleaseSemaphore(&unit->intrxfer_queue_lock);

    return(RC_DONTREPLY);
}

WORD cmdBulkXFer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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

    mybug_unit(0, ("Adding BULK transfer request to queue\n"));
    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    ObtainSemaphore(&unit->bulkxfer_queue_lock);
    AddTail(&unit->bulkxfer_queue, (struct Node *) ioreq);
    ReleaseSemaphore(&unit->bulkxfer_queue_lock);

    return(RC_DONTREPLY);
}

WORD cmdISOXFer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

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

    mybug_unit(0, ("Adding ISOC transfer request to queue\n"));
    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    ObtainSemaphore(&unit->isocxfer_queue_lock);
    AddTail(&unit->isocxfer_queue, (struct Node *) ioreq);
    ReleaseSemaphore(&unit->isocxfer_queue_lock);

    return(RC_DONTREPLY);
}

void uhwCheckRootHubChanges(struct VUSBHCIUnit *unit) {
    mybug_unit(0, ("Entering function\n"));

    mybug_unit(0, ("usbportstatus->wPortStatus %01x\n", unit->roothub.portstatus.wPortStatus));
    mybug_unit(0, ("usbportstatus->wPortChange %01x\n", unit->roothub.portstatus.wPortChange));

    struct IOUsbHWReq *ioreq;

    ObtainSemaphore(&unit->roothub.intrxfer_queue_lock);
    if(unit->roothub.portstatus.wPortChange && unit->roothub.intrxfer_queue.lh_Head->ln_Succ) {
        ioreq = (struct IOUsbHWReq *) unit->roothub.intrxfer_queue.lh_Head;
        while(((struct Node *) ioreq)->ln_Succ) {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);

            *((UBYTE *) ioreq->iouh_Data) = (1<<1);
            ioreq->iouh_Actual = 1;

            ReplyMsg(&ioreq->iouh_Req.io_Message);
            ioreq = (struct IOUsbHWReq *) unit->roothub.intrxfer_queue.lh_Head;
        }
    }

    ReleaseSemaphore(&unit->roothub.intrxfer_queue_lock);
}

