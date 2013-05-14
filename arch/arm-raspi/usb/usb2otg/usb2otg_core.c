/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <asm/bcm2835.h>
#include <hardware/usb2otg.h>

#include "usb2otg_intern.h"

static const UWORD __suported_cmds[] =
{
    CMD_FLUSH,
    CMD_RESET,
    UHCMD_QUERYDEVICE,
    UHCMD_USBRESET,
    UHCMD_USBRESUME,
    UHCMD_USBSUSPEND,
    UHCMD_USBOPER,
    UHCMD_CONTROLXFER,
    UHCMD_ISOXFER,
    UHCMD_INTXFER,
    UHCMD_BULKXFER,
    NSCMD_DEVICEQUERY,
    0
};

struct Unit * FNAME_DEV(OpenUnit)(struct IOUsbHWReq *ioreq,
                        LONG unitnr,
                        LIBBASETYPEPTR USB2OTGBase)
{
    struct USB2OTGUnit *otg_Unit = NULL;
    unsigned int        otg_RegVal, chan;

    D(bug("[USB2OTG] %s(unit:0x%p, ioreq:0x%p)\n",
                __PRETTY_FUNCTION__, otg_Unit, ioreq));

    // We only support a single unit presently
    if ((unitnr == 0) && (USB2OTGBase->hd_Unit))
    {
        otg_Unit = USB2OTGBase->hd_Unit;
        if (!(otg_Unit->hu_UnitAllocated))
        {
            otg_Unit->hu_UnitAllocated = TRUE;

            otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE2);

            if (!(otg_Unit->hu_OperatingMode) || (otg_Unit->hu_OperatingMode == USB2OTG_USBHOSTMODE))
            {
                if ((otg_Unit->hu_HostChans = ((otg_RegVal & (0xF << 14)) >> 14) + 1) > EPSCHANS_MAX)
                    otg_Unit->hu_HostChans = EPSCHANS_MAX;
                
                D(bug("[USB2OTG] %s: USB Core HOST mode -:\n",
                            __PRETTY_FUNCTION__));
                D(bug("[USB2OTG] %s:      Channels: %d\n",
                            __PRETTY_FUNCTION__, otg_Unit->hu_HostChans));

                for (chan = 0; chan < otg_Unit->hu_HostChans; chan++) {
                    *((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_INTRMASK)) = 
                        (USB2OTG_INTRCHAN_STALL|USB2OTG_INTRCHAN_BABBLEERROR|USB2OTG_INTRCHAN_TRANSACTIONERROR) |
                        (USB2OTG_INTRCHAN_NEGATIVEACKNOWLEDGE|USB2OTG_INTRCHAN_ACKNOWLEDGE|USB2OTG_INTRCHAN_NOTREADY) |
                        (USB2OTG_INTRCHAN_HALT|USB2OTG_INTRCHAN_FRAMEOVERRUN|USB2OTG_INTRCHAN_DATATOGGLEERROR);

                    D(bug("[USB2OTG] %s:      Chan #%d FIFO @ 0x%p, Characteristics: %08x\n",
                                __PRETTY_FUNCTION__,
                                chan, USB2OTG_FIFOBASE + (chan * USB2OTG_FIFOSIZE),
                                *((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_CHARBASE))));
                }

                D(bug("[USB2OTG] %s: Enabling HOST Channel Interrupts ...\n",
                            __PRETTY_FUNCTION__));
                *((volatile unsigned int *)USB2OTG_HOSTINTRMASK) = (1 << otg_Unit->hu_HostChans) - 1;
            }

            if (!(otg_Unit->hu_OperatingMode) || (otg_Unit->hu_OperatingMode == USB2OTG_USBDEVICEMODE))
            {
                if ((otg_Unit->hu_DevEPs = ((otg_RegVal & (0xF << 10)) >> 10) + 1) > EPSCHANS_MAX)
                    otg_Unit->hu_DevEPs = EPSCHANS_MAX;

                otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE4);
                otg_Unit->hu_DevInEPs = ((otg_RegVal & (0xF << 26)) >> 26) + 1;

                D(bug("[USB2OTG] %s: USB Core DEVICE mode -:\n",
                            __PRETTY_FUNCTION__));
                D(bug("[USB2OTG] %s:      Endpoints: %d/%d\n",
                            __PRETTY_FUNCTION__, otg_Unit->hu_DevInEPs, otg_Unit->hu_DevEPs));

                for (chan = 0; chan < otg_Unit->hu_DevEPs; chan++) {
                    D(bug("[USB2OTG] %s:      Endpoint #%d ", __PRETTY_FUNCTION__, chan));
                    if (chan < otg_Unit->hu_DevInEPs)
                    {
                        D(bug("IN_CTL: %08x\n", *((volatile unsigned int *)(USB2OTG_DEV_INEP_BASE + (chan * USB2OTG_DEV_EPSIZE) + USB2OTG_DEV_INEP_DIEPCTL))));
                    }
                    else
                    {
                        D(bug("OUT_CTL: %08x\n", *((volatile unsigned int *)(USB2OTG_DEV_OUTEP_BASE + (chan * USB2OTG_DEV_EPSIZE) + USB2OTG_DEV_OUTEP_DOEPCTL))));
                    }
                }
            }

            otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE3);
            D(bug("[USB2OTG] %s: Operating Mode: %0x\n",
                        __PRETTY_FUNCTION__, (otg_RegVal & 0x7)));
            D(bug("[USB2OTG] %s: Queue Depths:\n",
                        __PRETTY_FUNCTION__));
            D(bug("[USB2OTG] %s:      Periodic Transmit: 0x%0x\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0x3 << 24)) >> 24)));
            D(bug("[USB2OTG] %s:      Non-Periodic Transmit: 0x%0x\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0x3 << 22)) >> 22)));
            D(bug("[USB2OTG] %s:      Device Tokens: 0x%0x\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0x1F << 26)) >> 26)));
            otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE3);
            D(bug("[USB2OTG] %s:      FIFO: %d bytes\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0xFFFF << 16)) >> 16) << 2));

            D(bug("[USB2OTG] %s: Xfer Size: %0x\n",
                        __PRETTY_FUNCTION__, (otg_RegVal & 0xF)));

            return (&otg_Unit->hu_Unit);
        }
        else
        {
            ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
            D(bug("[USB2OTG] %s: Unit %ld already in use\n",
                        __PRETTY_FUNCTION__, unitnr));
        }
    }

    return(NULL);
}

void FNAME_DEV(CloseUnit)(struct IOUsbHWReq *ioreq, struct USB2OTGUnit *otg_Unit, LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] %s(unit:0x%p, ioreq:0x%p)\n",
                __PRETTY_FUNCTION__, otg_Unit, ioreq));

    otg_Unit->hu_UnitAllocated = FALSE;
}

void FNAME_DEV(TermIO)(struct IOUsbHWReq *ioreq,
            LIBBASETYPEPTR USB2OTGBase)
{
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    if (!(ioreq->iouh_Req.io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
}

WORD FNAME_DEV(cmdNSDeviceQuery)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    struct USBNSDeviceQueryResult *query = (struct USBNSDeviceQueryResult *)ioreq->io_Data;

    D(bug("[USB2OTG] NSCMD_DEVICEQUERY(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if ((!query) ||
       (ioreq->io_Length < sizeof(struct USBNSDeviceQueryResult)) ||
       (query->DevQueryFormat != 0) ||
       (query->SizeAvailable != 0))
    {
        ioreq->io_Error = IOERR_NOCMD;
        FNAME_DEV(TermIO)((struct IOUsbHWReq *) ioreq, USB2OTGBase);

        return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct USBNSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_USBHARDWARE;
    query->DeviceSubType     = 0;
    query->SupportedCommands = __suported_cmds;

    return RC_OK;
}

WORD FNAME_DEV(cmdQueryDevice)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    D(bug("[USB2OTG] UHCMD_QUERYDEVICE(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (((tag = FindTagItem(UHA_State, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = (IPTR)UHSF_OPERATIONAL;
        count++;
    }

    if (((tag = FindTagItem(UHA_Manufacturer, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "The AROS Dev Team";
        count++;
    }

    if (((tag = FindTagItem(UHA_ProductName, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "HS USB 2.0 OTG Controller";
        count++;
    }

    if (((tag = FindTagItem(UHA_Description, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "Synopsys/DesignWare USB 2.0 OTG Controller for Raspberry Pi";
        count++;
    }

    if (((tag = FindTagItem(UHA_Copyright, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) ="Â©2013 The AROS Dev Team";
        count++;
    }

    if (((tag = FindTagItem(UHA_Version, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = VERSION_NUMBER;
        count++;
    }

    if (((tag = FindTagItem(UHA_Revision, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = REVISION_NUMBER;
        count++;
    }

    if (((tag = FindTagItem(UHA_DriverVersion, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = 0x100;
        count++;
    }

    if (((tag = FindTagItem(UHA_Capabilities, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = UHCF_USB20;
        count++;
    }
    ioreq->iouh_Actual = count;
    return RC_OK;
}

WORD FNAME_DEV(cmdReset)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] CMD_RESET(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdFlush)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] CMD_FLUSH(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return RC_OK;
}

WORD FNAME_DEV(cmdUsbReset)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBRESET(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbResume)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBRESUME(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbSuspend)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBSUSPEND(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbOper)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBOPER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdControlXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_CONTROLXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdBulkXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_BULKXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdIntXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_INTXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdIsoXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_ISOXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return (WORD)UHIOERR_USBOFFLINE;
}
