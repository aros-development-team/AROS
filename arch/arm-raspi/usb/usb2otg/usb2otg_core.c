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
    unsigned int        otg_RegVal;

    D(bug("[USB2OTG] %s(unit:0x%p, ioreq:0x%p)\n",
                __PRETTY_FUNCTION__, otg_Unit, ioreq));

    // We only support a single unit presently
    if ((unitnr == 0) && (USB2OTGBase->hd_Unit))
    {
        otg_Unit = USB2OTGBase->hd_Unit;
        if (!(otg_Unit->hu_UnitAllocated))
        {
            otg_Unit->hu_UnitAllocated = TRUE;

            otg_RegVal = *((volatile unsigned int *)USB2OTG_INTR);

            otg_Unit->hu_OperatingMode = (BOOL)(otg_RegVal & USB2OTG_INTRCORE_CURRENTMODE);

            otg_RegVal = *((volatile unsigned int *)USB2OTG_LPMCONFIG);            

            if (otg_Unit->hu_OperatingMode == USB2OTG_USBHOSTMODE)
            {
                D(bug("[USB2OTG] %s: Core running in HOST mode\n",
                            __PRETTY_FUNCTION__));
                D(bug("[USB2OTG] %s: Host Channels: %d\n",
                            __PRETTY_FUNCTION__, ((otg_RegVal & (0xF << 14)) >> 14)));
            }
            else
            {
                D(bug("[USB2OTG] %s: Core running in DEVICE mode\n",
                            __PRETTY_FUNCTION__));
                D(bug("[USB2OTG] %s: Device Endpoints: %d\n",
                            __PRETTY_FUNCTION__, ((otg_RegVal & (0xF << 10)) >> 10)));
            }

            otg_RegVal = *((volatile unsigned int *)(USB2OTG_HARDWARE + 4));
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
            otg_RegVal = *((volatile unsigned int *)(USB2OTG_HARDWARE + 8));
            D(bug("[USB2OTG] %s:      FIFO: %d\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0xFFFF << 16)) >> 16)));

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
