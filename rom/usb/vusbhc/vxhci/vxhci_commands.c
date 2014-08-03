/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/macros.h>

#include <proto/exec.h>
#include <proto/arossupport.h>

#include <devices/usb_hub.h>
#include <devices/usbhardware.h>

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

/* we cannot use AROS_WORD2LE in struct initializer */
#if AROS_BIG_ENDIAN
#define WORD2LE(w) (UWORD)(((w) >> 8) & 0x00FF) | (((w) << 8) & 0xFF00)
#else
#define WORD2LE(w) (w)
#endif

const struct UsbStdDevDesc RHDevDesc = { sizeof(struct UsbStdDevDesc), UDT_DEVICE, WORD2LE(0x0110), HUB_CLASSCODE, 0, 0, 8, WORD2LE(0x0000), WORD2LE(0x0000), WORD2LE(0x0100), 1, 2, 0, 1 };

const struct UsbSSHubDesc  RHSSHubDesc = { 12,                                           // 0 Number of bytes in this descriptor, including this byte. (12 bytes)
                                           UDT_SSHUB,                                    // 1 Descriptor Type, value: 2AH for SuperSpeed hub descriptor
                                           0,                                            // 2 Number of downstream facing ports that this hub supports. The maximum number of ports a hub can support is 15
                                           WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP), // 3 wHubCharacteristics
                                           0,                                            // 5 bPwrOn2PwrGood
                                           10,                                           // 6 bHubContrCurrent
                                           0,                                            // 7 bHubHdrDecLat
                                           0,                                            // 8 wHubDelay
                                           0                                             // 10 DeviceRemovable
                                         };

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq) {
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
                *((STRPTR *) tag->ti_Data) = "VXHCI";
                count++;
                break;
            case UHA_Capabilities:
                *((ULONG *) tag->ti_Data) = (UHCF_USB20|UHCF_USB30);
                count++;
                break;
            default:
                break;
        }
    }

    ioreq->iouh_Actual = count;
    return RC_OK;
}

WORD cmdUsbReset(struct IOUsbHWReq *ioreq) {
    bug("[VXHCI] cmdUsbReset: Entering function\n");

    struct VXHCIUnit *unit;

    /* We should do a proper reset sequence with a real driver */
    unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;
    if(unit != NULL) {
        unit->unit_state = UHSF_OPERATIONAL;
    } else {
        /* FIXME: Move unit checks somewhere else... */
        return UHIOERR_BADPARAMS;
    }

    return RC_OK;
}

WORD cmdControlXFer(struct IOUsbHWReq *ioreq) {
    bug("[VXHCI] cmdControlXFer: Entering function\n");

    struct VXHCIUnit *unit;

    bug("[VXHCI] cmdControlXFer: ioreq->iouh_DevAddr %lx\n", ioreq->iouh_DevAddr);

    /* Check the status of the controller */
    unit = (struct VXHCIUnit *) ioreq->iouh_Req.io_Unit;
    if(unit != NULL) {
        /*
            We might encounter these states:
            UHSB_OPERATIONAL USB can be used for transfers
            UHSB_RESUMING    USB is currently resuming
            UHSB_SUSPENDED   USB is in suspended state
            UHSB_RESET       USB is just inside a reset phase
        */
        if(unit->unit_state == UHSF_OPERATIONAL) {
            bug("[VXHCI] cmdControlXFer: Unit state is operational\n");
        } else {
            bug("[VXHCI] cmdControlXFer: Unit state: UHSF_SUSPENDED\n");
            return UHIOERR_USBOFFLINE;
        }

        /* Assuming when iouh_DevAddr is 0 it addresses hub... */
        if(ioreq->iouh_DevAddr == 0) {
            return(cmdControlXFerRootHub(ioreq));
        }

    } else {
        /* CHECKME: Is this correct? */
        return UHIOERR_BADPARAMS;
    }



    return RC_DONTREPLY;
}

WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq) {
    bug("[VXHCI] cmdControlXFerRootHub: Entering function\n");

    UWORD rt = ioreq->iouh_SetupData.bmRequestType;
    UWORD req = ioreq->iouh_SetupData.bRequest;
    UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    bug("[VXHCI] cmdControlXFerRootHub: Endpoint number is %ld and request type is %lx\n", ioreq->iouh_Endpoint, rt);

    /* Endpoint 0 is used for control transfers only and can not be assigned to any other function. */
    if(ioreq->iouh_Endpoint != 0) {
        return UHIOERR_BADPARAMS;
    }

    /* Check the request type */
    if(rt&&URTF_IN) {
        bug("[VXHCI] cmdControlXFerRootHub: Request direction is device to host\n");
    } else {
        bug("[VXHCI] cmdControlXFerRootHub: Request direction is host to device\n");
    }

/*
    switch(rt) {
        case (URTF_STANDARD|URTF_DEVICE):
            bug("[VXHCI] cmdControlXFerRootHub: switch(rt) (URTF_STANDARD|URTF_DEVICE)\n");
            switch(req) {
                case USR_SET_ADDRESS:
                    bug("[VXHCI] cmdControlXFerRootHub: switch(req) (USR_SET_ADDRESS) SetAddress = %ld\n", val);
                    //unit->hu_RootHubAddr = val;
                    ioreq->iouh_Actual = len;
                    return(0);
                case USR_SET_CONFIGURATION:
                    bug("[VXHCI] cmdControlXFerRootHub: switch(req) (USR_SET_CONFIGURATION) SetConfiguration = %ld\n", val);
                    ioreq->iouh_Actual = len;
                    return(0);
            }
            break;
        case (URTF_IN|URTF_STANDARD|URTF_DEVICE):
            bug("[VXHCI] cmdControlXFerRootHub: switch(rt) (URTF_IN|URTF_STANDARD|URTF_DEVICE)\n");
            break;
        case (URTF_CLASS|URTF_OTHER):
            bug("[VXHCI] cmdControlXFerRootHub: switch(rt) (URTF_CLASS|URTF_OTHER)\n");
            break;
        case (URTF_IN|URTF_CLASS|URTF_OTHER):
            bug("[VXHCI] cmdControlXFerRootHub: switch(rt) (URTF_IN|URTF_CLASS|URTF_OTHER)\n");
            break;
        case (URTF_IN|URTF_CLASS|URTF_DEVICE):
            bug("[VXHCI] cmdControlXFerRootHub: switch(rt) (URTF_IN|URTF_CLASS|URTF_DEVICE)\n");
            break;
        default:
            bug("[VXHCI] cmdControlXFerRootHub: switch(rt) DEFAULT\n");
            break;
    } /* switch(rt) */

    return UHIOERR_BADPARAMS;
}


