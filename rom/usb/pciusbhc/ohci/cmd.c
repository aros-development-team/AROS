/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2009-2012, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <devices/usb_hub.h>

#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <clib/alib_protos.h>

#include "debug.h"

#include "cmd_protos.h"
#include "roothub_protos.h"
#include "chip_protos.h"
#include "pci_protos.h"

#include "pci.h"
#include "chip.h"

struct my_NSDeviceQueryResult
{
    ULONG DevQueryFormat;       /* this is type 0               */
    ULONG SizeAvailable;        /* bytes available              */
    UWORD DeviceType;           /* what the device does         */
    UWORD DeviceSubType;        /* depends on the main type     */
    const UWORD *SupportedCommands;     /* 0 terminated list of cmd's   */
};

static BOOL OpenTimer(struct PCIUnit *unit, struct PCIDevice *base);
static void CloseTimer(struct PCIUnit *unit, struct PCIDevice *base);
static UWORD GetUsbState(struct IOUsbHWReq *ioreq,
    struct PCIUnit *unit, struct PCIDevice *base);

static const struct TagItem query_tags[] = {
    {UHA_Manufacturer, (IPTR) "Chris Hodges, AROS Development Team"},
    {UHA_Description, (IPTR) "Open Host Controller Interface driver"},
    {UHA_Copyright,
        (IPTR) "Â©2007-2012 Chris Hodges,\n AROS Development Team"},
    {UHA_Version, VERSION_NUMBER},
    {UHA_Revision, REVISION_NUMBER},
    {UHA_DriverVersion, 0x220},
    {TAG_END, 0}
};

/* /// "OpenTimer()" */
static BOOL OpenTimer(struct PCIUnit *unit, struct PCIDevice *base)
{
    if ((unit->hu_MsgPort = CreateMsgPort()))
    {
        if ((unit->hu_TimerReq =
                (struct timerequest *)CreateIORequest(unit->hu_MsgPort,
                    sizeof(struct timerequest))))
        {
            if (!OpenDevice("timer.device", UNIT_MICROHZ,
                    (struct IORequest *)unit->hu_TimerReq, 0))
            {
                unit->hu_TimerReq->tr_node.io_Message.mn_Node.ln_Name =
                    "PCI hardware";
                unit->hu_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
                KPRINTF(1, ("opened timer device\n"));
                return TRUE;
            }
            DeleteIORequest((struct IORequest *)unit->hu_TimerReq);
            unit->hu_TimerReq = NULL;
        }
        DeleteMsgPort(unit->hu_MsgPort);
        unit->hu_MsgPort = NULL;
    }
    KPRINTF(5, ("failed to open timer.device\n"));
    return FALSE;
}
/* \\\ */

/* /// "DelayMS()" */
void DelayMS(ULONG milli, struct PCIUnit *unit)
{
    unit->hu_TimerReq->tr_time.tv_secs = 0;
    unit->hu_TimerReq->tr_time.tv_micro = milli * 1000;
    DoIO((struct IORequest *)unit->hu_TimerReq);
}
/* \\\ */

/* /// "CloseTimer()" */
static void CloseTimer(struct PCIUnit *unit, struct PCIDevice *base)
{
    if (unit->hu_MsgPort)
    {
        if (unit->hu_TimerReq)
        {
            KPRINTF(1, ("closing timer.device\n"));
            CloseDevice((APTR) unit->hu_TimerReq);
            DeleteIORequest((struct IORequest *)unit->hu_TimerReq);
            unit->hu_TimerReq = NULL;
        }
        DeleteMsgPort(unit->hu_MsgPort);
        unit->hu_MsgPort = NULL;
    }
}
/* \\\ */

/* /// "Open_Unit()" */
struct Unit *Open_Unit(struct IOUsbHWReq *ioreq,
    LONG unitnr, struct PCIDevice *base)
{
    struct PCIUnit *unit = NULL;

    if (!base->hd_ScanDone)
    {
        base->hd_ScanDone = TRUE;
        if (!pciInit(base))
        {
            return NULL;
        }
    }
    unit = (struct PCIUnit *)base->hd_Units.lh_Head;
    while (((struct Node *)unit)->ln_Succ)
    {
        if (unit->hu_UnitNo == unitnr)
        {
            break;
        }
        unit = (struct PCIUnit *)((struct Node *)unit)->ln_Succ;
    }
    if (!((struct Node *)unit)->ln_Succ)
    {
        KPRINTF(20, ("Unit %ld does not exist!\n", unitnr));
        return NULL;
    }
    if (unit->hu_UnitAllocated)
    {
        ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
        KPRINTF(5, ("Unit %ld already open!\n", unitnr));
        return NULL;
    }

    if (OpenTimer(unit, base))
    {

        if (pciAllocUnit(unit)) // hardware self test
        {
            unit->hu_UnitAllocated = TRUE;
            unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_INTERRUPT;
            unit->hu_NakTimeoutInt.is_Node.ln_Name = "PCI NakTimeout";
            unit->hu_NakTimeoutInt.is_Node.ln_Pri = -16;
            unit->hu_NakTimeoutInt.is_Data = unit;
            unit->hu_NakTimeoutInt.is_Code = (VOID_FUNC) NakTimeoutInt;

            CopyMem(unit->hu_TimerReq, &unit->hu_NakTimeoutReq,
                sizeof(struct timerequest));
            unit->hu_NakTimeoutReq.tr_node.io_Message.mn_ReplyPort =
                &unit->hu_NakTimeoutMsgPort;
            unit->hu_NakTimeoutMsgPort.mp_Node.ln_Type = NT_MSGPORT;
            unit->hu_NakTimeoutMsgPort.mp_Flags = PA_SOFTINT;
            unit->hu_NakTimeoutMsgPort.mp_SigTask = &unit->hu_NakTimeoutInt;
            NewList(&unit->hu_NakTimeoutMsgPort.mp_MsgList);
            Cause(&unit->hu_NakTimeoutInt);
            return &unit->hu_Unit;
        }
        else
        {
            ioreq->iouh_Req.io_Error = IOERR_SELFTEST;
            KPRINTF(20, ("Hardware allocation failure!\n"));
        }
        CloseTimer(unit, base);
    }
    return NULL;
}
/* \\\ */

/* /// "Close_Unit()" */
void Close_Unit(struct PCIDevice *base,
    struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    /* Disable all interrupts */
    unit->hu_NakTimeoutMsgPort.mp_Flags = PA_IGNORE;
    unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_SOFTINT;
    AbortIO((APTR) & unit->hu_NakTimeoutReq);

    pciFreeUnit(unit);

    CloseTimer(unit, base);
    unit->hu_UnitAllocated = FALSE;
}
/* \\\ */

/* /// "GetUsbState()" */
static UWORD GetUsbState(struct IOUsbHWReq *ioreq,
    struct PCIUnit *unit, struct PCIDevice *base)
{
    return ioreq->iouh_State = UHSF_OPERATIONAL;
}
/* \\\ */

/* /// "cmdReset()" */
/*
 *======================================================================
 * cmdReset(ioreq, unit, base)
 *======================================================================
 *
 * This is the device CMD_RESET routine.
 *
 * Resets the whole USB hardware. Goes into USBOperational mode right
 * after. Must NOT be called from an interrupt.
 *
 */

WORD cmdReset(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    KPRINTF(10, ("CMD_RESET ioreq: 0x%p\n", ioreq));

    DelayMS(1, unit);
    GetUsbState(ioreq, unit, base);

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbReset()" */
/*
 *======================================================================
 * cmdUsbReset(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBRESET routine.
 *
 * Resets the USB bus. Goes into USBOperational mode right after. Must
 * NOT be called from an interrupt.
 *
 */

WORD cmdUsbReset(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    KPRINTF(10, ("UHCMD_USBRESET ioreq: 0x%p\n", ioreq));

    /* FIXME */
    GetUsbState(ioreq, unit, base);

    unit->hu_FrameCounter = 1;
    unit->hu_RootHubAddr = 0;

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbResume()" */
/*
 *======================================================================
 * cmdUsbResume(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBRESUME routine.
 *
 * Tries to resume from USBSuspend mode into USBOperational.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbResume(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    KPRINTF(10, ("UHCMD_USBRESUME ioreq: 0x%p\n", ioreq));

    /* FIXME */
    GetUsbState(ioreq, unit, base);
    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbSuspend()" */
/*
 *======================================================================
 * cmdUsbSuspend(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBSUSPEND routine.
 *
 * Sets the USB into USBSuspend mode.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbSuspend(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    KPRINTF(10, ("UHCMD_USBSUSPEND ioreq: 0x%p\n", ioreq));

    /* FIXME */
    GetUsbState(ioreq, unit, base);
    if (ioreq->iouh_State & UHSF_SUSPENDED)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbOper()" */
/*
 *======================================================================
 * cmdUsbOper(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBOPER routine.
 *
 * Sets the USB into USBOperational mode.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbOper(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    KPRINTF(10, ("UHCMD_USBOPER ioreq: 0x%p\n", ioreq));

    /* FIXME */
    GetUsbState(ioreq, unit, base);
    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdQueryDevice()" */
/*
 *======================================================================
 * cmdQueryDevice(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_QUERYDEVICE routine.
 *
 * Returns information about the hardware.
 *
 */

WORD cmdQueryDevice(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    struct TagItem *taglist = (struct TagItem *)ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    KPRINTF(10, ("UHCMD_QUERYDEVICE ioreq: 0x%p, taglist: 0x%p\n", ioreq,
            taglist));

    while ((tag = NextTagItem(&taglist)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case UHA_State:
            *((ULONG *) tag->ti_Data) =
                (ULONG) GetUsbState(ioreq, unit, base);
            break;
        case UHA_ProductName:
            *((STRPTR *) tag->ti_Data) = unit->hu_ProductName;
            break;
        default:
            *((IPTR *) tag->ti_Data) =
                GetTagData(tag->ti_Tag, *((IPTR *) tag->ti_Data),
                query_tags);
        }
        count++;
    }

    ioreq->iouh_Actual = count;
    return RC_OK;
}
/* \\\ */

WORD cmdXFer(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    struct PCIController *hc;
    UWORD xfer_type = 0;

    // get transfer type
    switch (ioreq->iouh_Req.io_Command)
    {
    case UHCMD_CONTROLXFER:
        xfer_type = CTRL_XFER;
        break;
    case UHCMD_ISOXFER:
        xfer_type = ISO_XFER;
        break;
    case UHCMD_INTXFER:
        xfer_type = INT_XFER;
        break;
    case UHCMD_BULKXFER:
        xfer_type = BULK_XFER;
        break;
    }

    KPRINTF(10, ("UHCMD_%sXFER ioreq: 0x%p\n", xfer_names[xfer_type],
            ioreq));
    GetUsbState(ioreq, unit, base);
    if (!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return UHIOERR_USBOFFLINE;
    }

    if (xfer_type == CTRL_XFER)
    {
        /* Root Hub Emulation */
        if (ioreq->iouh_DevAddr == unit->hu_RootHubAddr)
        {
            return cmdControlXFerRootHub(ioreq, unit, base);
        }
    }
    else if (xfer_type == INT_XFER)
    {
        /* Root Hub Emulation */
        if (ioreq->iouh_DevAddr == unit->hu_RootHubAddr)
        {
            return cmdIntXFerRootHub(ioreq, unit, base);
        }
    }
    else if (xfer_type == BULK_XFER || xfer_type == ISO_XFER)
    {
        if (ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            return UHIOERR_BADPARAMS;
        }
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if (!hc)
    {
        return UHIOERR_HOSTERROR;
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;
    ioreq->iouh_DriverPrivate1 = NULL;

    Disable();
    AddTail(&hc->hc_XferQueues[xfer_type], (struct Node *)ioreq);
    Enable();
    Cause(&hc->hc_CompleteInt);

    KPRINTF(10, ("UHCMD_%sXFER processed ioreq: 0x%p\n",
            xfer_names[xfer_type], ioreq));
    return RC_DONTREPLY;
}
/* \\\ */

/* /// "cmdFlush()" */
/*
 *======================================================================
 * cmdFlush(ioreq, base)
 *======================================================================
 *
 * This is the device CMD_FLUSH routine.
 *
 * This routine abort all pending transfer requests.
 *
 */

WORD cmdFlush(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    struct IOUsbHWReq *cmpioreq;
    struct PCIController *hc;
    UWORD i;
    struct List *list;

    KPRINTF(10, ("CMD_FLUSH ioreq: 0x%p\n", ioreq));

    Disable();
    cmpioreq = (struct IOUsbHWReq *)unit->hu_RHIOQueue.lh_Head;
    while (((struct Node *)cmpioreq)->ln_Succ)
    {
        Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
        cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
        ReplyMsg(&cmpioreq->iouh_Req.io_Message);
        cmpioreq = (struct IOUsbHWReq *)unit->hu_RHIOQueue.lh_Head;
    }
    hc = (struct PCIController *)unit->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        for (i = 0; i < XFER_COUNT; i++)
        {
            list = (struct List *)&hc->hc_XferQueues[i];
            while ((cmpioreq = (struct IOUsbHWReq *)RemHead(list)) != NULL)
            {
                cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
                ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            }
        }
        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }
    Enable();
    /* Return success
     */
    return RC_OK;
}
/* \\\ */

/* /// "NSD stuff" */

static const UWORD NSDSupported[] = {
    CMD_FLUSH, CMD_RESET,
    UHCMD_QUERYDEVICE, UHCMD_USBRESET,
    UHCMD_USBRESUME, UHCMD_USBSUSPEND,
    UHCMD_USBOPER, UHCMD_CONTROLXFER,
    UHCMD_ISOXFER, UHCMD_INTXFER,
    UHCMD_BULKXFER,
    NSCMD_DEVICEQUERY, 0
};

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq,
    struct PCIUnit *unit, struct PCIDevice *base)
{
    struct my_NSDeviceQueryResult *query;

    query = (struct my_NSDeviceQueryResult *)ioreq->io_Data;

    KPRINTF(10, ("NSCMD_DEVICEQUERY ioreq: 0x%p query: 0x%p\n", ioreq,
            query));

    /* NULL ptr?
       Enough data?
       Valid request?
     */
    if ((!query) ||
        (ioreq->io_Length < sizeof(struct my_NSDeviceQueryResult)) ||
        (query->DevQueryFormat != 0) || (query->SizeAvailable != 0))
    {
        /* Return error. This is special handling, since iorequest is only
           guaranteed to be sizeof(struct IOStdReq). If we'd let our
           devBeginIO dispatcher return the error, it would trash some
           memory past end of the iorequest (ios2_WireError field).
         */
        ioreq->io_Error = IOERR_NOCMD;
        TermIO((struct IOUsbHWReq *)ioreq, base);

        /* Don't reply, we already did.
         */
        return RC_DONTREPLY;
    }

    ioreq->io_Actual = query->SizeAvailable
        = sizeof(struct my_NSDeviceQueryResult);
    query->DeviceType = NSDEVTYPE_USBHARDWARE;
    query->DeviceSubType = 0;
    query->SupportedCommands = NSDSupported;

    /* Return success (note that this will NOT poke ios2_WireError).
     */
    return RC_OK;
}
/* \\\ */

/* /// "TermIO()" */
/*
 *===========================================================
 * TermIO(ioreq, base)
 *===========================================================
 *
 * Return completed ioreq to sender.
 *
 */

void TermIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base)
{
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message
     */
    if (!(ioreq->iouh_Req.io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
}
/* \\\ */

/* /// "cmdAbortIO()" */
BOOL cmdAbortIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base)
{
    struct PCIUnit *unit = (struct PCIUnit *)ioreq->iouh_Req.io_Unit;
    struct IOUsbHWReq *cmpioreq;
    struct PCIController *hc;
    BOOL foundit = FALSE;
    UWORD i;

    KPRINTF(10, ("cmdAbort(%p)\n", ioreq));

    Disable();
    cmpioreq = (struct IOUsbHWReq *)unit->hu_RHIOQueue.lh_Head;
    while (((struct Node *)cmpioreq)->ln_Succ)
    {
        if (ioreq == cmpioreq)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            Enable();
            ioreq->iouh_Req.io_Error = IOERR_ABORTED;
            TermIO(ioreq, base);
            return TRUE;
        }
        cmpioreq =
            (struct IOUsbHWReq *)cmpioreq->iouh_Req.io_Message.
            mn_Node.ln_Succ;
    }

    hc = (struct PCIController *)unit->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        for (i = 0; i < XFER_COUNT && !foundit; i++)
        {
            for (cmpioreq =
                (struct IOUsbHWReq *)hc->hc_XferQueues[i].lh_Head;
                ((struct Node *)cmpioreq)->ln_Succ != NULL && !foundit;
                cmpioreq =
                (struct IOUsbHWReq *)cmpioreq->iouh_Req.io_Message.
                mn_Node.ln_Succ)
            {
                if (ioreq == cmpioreq)
                    foundit = TRUE;
            }
        }
        if (!foundit)
        {
            // IOReq is probably pending in some transfer structure
            cmpioreq = (struct IOUsbHWReq *)hc->hc_TDQueue.lh_Head;
            while (((struct Node *)cmpioreq)->ln_Succ)
            {
                if (ioreq == cmpioreq)
                {
                    /*
                     * Request's ED is in use by the HC, as well as its TDs
                     * and data buffers.
                     * Schedule abort on the HC driver and reply the request
                     * only when done. However return success.
                     */
                    ioreq->iouh_Req.io_Error = IOERR_ABORTED;
                    AbortRequest(hc, ioreq);
                    Enable();
                    return TRUE;
                }
                cmpioreq =
                    (struct IOUsbHWReq *)cmpioreq->iouh_Req.
                    io_Message.mn_Node.ln_Succ;
            }
            break;

        }
        if (foundit)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            break;
        }
        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }
    Enable();

    if (foundit)
    {
        ioreq->iouh_Req.io_Error = IOERR_ABORTED;
        TermIO(ioreq, base);
    }
    else
    {
        KPRINTF(20, ("WARNING, could not abort unknown IOReq %p\n", ioreq));
    }
    return foundit;
}
/* \\\ */

/* /// "CheckSpecialCtrlTransfers()" */
void CheckSpecialCtrlTransfers(struct PCIController *hc,
    struct IOUsbHWReq *ioreq)
{
    struct PCIUnit *unit = hc->hc_Unit;

    /* Clear Feature(Endpoint halt) */
    if ((ioreq->iouh_SetupData.bmRequestType ==
            (URTF_STANDARD | URTF_ENDPOINT))
        && (ioreq->iouh_SetupData.bRequest == USR_CLEAR_FEATURE)
        && (ioreq->iouh_SetupData.wValue ==
            AROS_WORD2LE(UFS_ENDPOINT_HALT)))
    {
        KPRINTF(10, ("Resetting toggle bit for endpoint %ld\n",
                AROS_WORD2LE(ioreq->iouh_SetupData.wIndex) & 0xf));
        unit->
            hu_DevDataToggle[(ioreq->iouh_DevAddr << 5) |
            (AROS_WORD2LE(ioreq->
                    iouh_SetupData.wIndex) & 0xf) | ((AROS_WORD2LE(ioreq->
                        iouh_SetupData.wIndex) & 0x80) >> 3)] = 0;
    }
    else if ((ioreq->iouh_SetupData.bmRequestType ==
            (URTF_STANDARD | URTF_DEVICE))
        && (ioreq->iouh_SetupData.bRequest == USR_SET_ADDRESS))
    {
        /* Set Address -> clear all endpoints */
        ULONG epnum;
        ULONG adr = AROS_WORD2BE(ioreq->iouh_SetupData.wValue) >> 3;
        KPRINTF(10, ("Resetting toggle bits for device address %ld\n",
                adr >> 5));
        for (epnum = 0; epnum < 31; epnum++)
        {
            unit->hu_DevDataToggle[adr + epnum] = 0;
        }
        // transfer host controller ownership
        unit->hu_DevControllers[ioreq->iouh_DevAddr] = NULL;
        unit->hu_DevControllers[adr >> 5] = hc;
    }
    else if ((ioreq->iouh_SetupData.bmRequestType ==
            (URTF_CLASS | URTF_OTHER))
        && (ioreq->iouh_SetupData.bRequest == USR_SET_FEATURE)
        && (ioreq->iouh_SetupData.wValue == AROS_WORD2LE(UFS_PORT_RESET)))
    {
        // a hub will be enumerating a device on this host controller soon!
        KPRINTF(10, ("Hub RESET caught, assigning Dev0 to %p!\n", hc));
        unit->hu_DevControllers[0] = hc;
    }
}
/* \\\ */

/* /// "NakTimeoutInt()" */
AROS_INTH1(NakTimeoutInt, struct PCIUnit *, unit)
{
    AROS_INTFUNC_INIT struct PCIController *hc;
    struct IOUsbHWReq *ioreq;
    UWORD target;
    ULONG framecnt;

    KPRINTF(1, ("Enter NakTimeoutInt(0x%p)\n", unit));

    // check for NAK Timeouts
    hc = (struct PCIController *)unit->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        if (!(hc->hc_Flags & HCF_ONLINE))
        {
            hc = (struct PCIController *)hc->hc_Node.ln_Succ;
            continue;
        }
        UpdateFrameCounter(hc);
        framecnt = hc->hc_FrameCounter;
        // NakTimeout
        ioreq = (struct IOUsbHWReq *)hc->hc_TDQueue.lh_Head;
        while (((struct Node *)ioreq)->ln_Succ)
        {
            // Remember the successor because AbortRequest() will move the request to another list
            struct IOUsbHWReq *succ =
                (struct IOUsbHWReq *)ioreq->iouh_Req.io_Message.
                mn_Node.ln_Succ;

            if (ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
            {
                KPRINTF(1, ("Examining IOReq=%p with OED=%p\n", ioreq,
                        ioreq->iouh_DriverPrivate1));
                if (ioreq->iouh_DriverPrivate1)
                {
                    KPRINTF(1,
                        ("CTRL=%04lx, CMD=%01lx, F=%ld, hccaDH=%08lx, "
                            "hcDH=%08lx, CH=%08lx, CCH=%08lx, "
                            "IntStatus=%08lx, IntEn=%08lx\n",
                            READREG32_LE(hc->hc_RegBase, OHCI_CONTROL),
                            READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS),
                            READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT),
                            READMEM32_LE(&hc->hc_HCCA->ha_DoneHead),
                            READREG32_LE(hc->hc_RegBase, OHCI_DONEHEAD),
                            READREG32_LE(hc->hc_RegBase, OHCI_CTRL_HEAD_ED),
                            READREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED),
                            READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS),
                            READREG32_LE(hc->hc_RegBase, OHCI_INTEN)));

                    target =
                        (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
                        ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                    if (framecnt > unit->hu_NakTimeoutFrame[target])
                    {
                        // give the thing the chance to exit gracefully
                        KPRINTF(200,
                            ("HC 0x%p NAK timeout %ld > %ld, IOReq=%p\n",
                                hc, framecnt,
                                unit->hu_NakTimeoutFrame[target], ioreq));
                        ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                        AbortRequest(hc, ioreq);
                    }
                }
            }
            ioreq = succ;
        }

        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }

    CheckRootHubChanges(unit);

    unit->hu_NakTimeoutReq.tr_time.tv_micro = 150 * 1000;
    SendIO((APTR) & unit->hu_NakTimeoutReq);

    KPRINTF(1, ("Exit NakTimeoutInt(0x%p)\n", unit));

    return FALSE;

AROS_INTFUNC_EXIT}
/* \\\ */
