/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver hw command support functions

    NB - do not use in the interrupt handler(s)
*/

#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"

#undef base
#define base (hc->hc_Device)
#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (hc->hc_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif

static const char strXhciEventTaskName[] = "xHCI event task";

#if !defined(PCIUSB_INLINEXHCIOPS)
void xhciRingDoorbell(struct PCIController *hc, ULONG slot, ULONG value)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_dbr *xhcidb = (volatile struct xhci_dbr *)((IPTR)xhcic->xhc_XHCIDB);

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u, %08x)" DEBUGCOLOR_RESET" \n", __func__, slot, value);
    XHCI_MMIO_BARRIER();
    xhcidb[slot].db = AROS_LONG2LE(value);
    /* Read back to flush posted MMIO write (helps some hypervisors). */
    (void)xhcidb[slot].db;
}
#endif

/*
 * Submit a command to the xHCI command TRB
 * returns -1 on failure, or the completion code
 */
LONG xhciCmdSubmit(struct PCIController *hc,
                   APTR dmaaddr,
                   ULONG trbflags, ULONG *resflags,
                   struct timerequest *timerreq)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_inctx *inctx;
    volatile struct xhci_slot *slot = NULL;
    WORD queued;
    ULONG cmd_type = (trbflags >> TRBS_FLAG_TYPE) & TRB_FLAG_TYPE_SMASK;
    BOOL needs_context =
        (cmd_type == TRBB_FLAG_CRTYPE_ADDRESS_DEVICE) ||
        (cmd_type == TRBB_FLAG_CRTYPE_CONFIGURE_ENDPOINT) ||
        (cmd_type == TRBB_FLAG_CRTYPE_EVALUATE_CONTEXT);

    Disable();
    if(dmaaddr && needs_context) {
        ULONG portsc = 0;
        UBYTE port = 0;
        UWORD ctxoff = 1;
        if(hc->hc_Flags & HCF_CTX64)
            ctxoff <<= 1;

        UBYTE slotid = (trbflags >> 24) & 0xFF;
        if((slotid > 0) && (slotid < USB_DEV_MAX)) {
            struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slotid];

            if(devCtx && devCtx->dc_SlotCtx.dmaa_Ptr)
                slot = (volatile struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;
        }

        if(!slot) {
            inctx = (volatile struct xhci_inctx *)dmaaddr;
            slot = xhciInputSlotCtx(inctx, ctxoff);

            pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s: slot input context @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, slot);
        }

        if(slot) {
            ULONG slotctx1 = AROS_LE2LONG(slot->ctx[1]);

            port = (slotctx1 >> 16) & 0xff;

            pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s: port #%u" DEBUGCOLOR_RESET" \n", __func__, port);

            if(port == 0 || port > hc->hc_NumPorts) {
                pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "%s: invalid port in slot context (slot=%u port=%u)" DEBUGCOLOR_RESET" \n",
                           __func__, (trbflags >> 24) & 0xFF, port);
                Enable();
                return -1;
            }

            volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);
            portsc = AROS_LE2LONG(xhciports[port - 1].portsc);

            pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s:     portsc=%08x" DEBUGCOLOR_RESET" \n", __func__, portsc);
        }
        if(!(slot) || !(portsc & XHCIF_PR_PORTSC_CCS)) {
            pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "%s: port disconnected (slot=%u port=%u portsc=%08lx)" DEBUGCOLOR_RESET" \n",
                       __func__, (trbflags >> 24) & 0xFF, (UWORD)port, (unsigned long)portsc);

            Enable();
            return -1;
        }
        /* Flush the input context to memory before writing to command ring */
        if(dmaaddr) {
            /* Assume typical input context size; adjust if needed */
            CacheClearE(dmaaddr, 2048, CACRF_ClearD);
        }
        queued = xhciQueueTRB(hc, xhcic->xhc_OPRp, (UQUAD)(IPTR)dmaaddr, 0, trbflags);
    } else if(dmaaddr) {
        queued = xhciQueueTRB(hc, xhcic->xhc_OPRp, (UQUAD)(IPTR)dmaaddr, 0, trbflags);
    } else {
        queued = xhciQueueTRB(hc, xhcic->xhc_OPRp, 0, 0, trbflags);
    }
    if(queued != -1) {
        xhcic->xhc_CmdResults[queued].flags = 0xFFFFFFFF;
        xhcic->xhc_CmdResults[queued].status = 0xFFFFFFFF;
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: Failed to queue command" DEBUGCOLOR_RESET" \n", __func__);
    }
    Enable();

    if(queued != -1) {
        xhciRingDoorbell(hc, 0, 0);

        /* Wait for completion with a bounded timeout to avoid hanging */
        for(ULONG waitms = 0; waitms < 1000; waitms++) {
            if(xhcic->xhc_CmdResults[queued].status != 0xFFFFFFFF) {
                /* Invalidate any output contexts that may have been updated */
                if(dmaaddr) {
                    /* For commands that update contexts, invalidate cache */
                    CacheClearE(dmaaddr, 2048, CACRF_InvalidateD);
                }

                if(resflags)
                    *resflags = xhcic->xhc_CmdResults[queued].flags;

                return xhcic->xhc_CmdResults[queued].status;
            }

            uhwDelayMS(1, timerreq);
        }

        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "%s: command timed out waiting for completion" DEBUGCOLOR_RESET" \n",
                    __func__);
    }
    return -1;
}

LONG xhciCmdSubmitAsync(struct PCIController *hc,
                        APTR dmaaddr,
                        ULONG trbflags,
                        struct IOUsbHWReq *ioreq)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_inctx *inctx;
    volatile struct xhci_slot *slot = NULL;
    WORD queued;
    volatile struct pcisusbXHCIRing *cmdring = (volatile struct pcisusbXHCIRing *)xhcic->xhc_OPRp;

    if(!ioreq)
        return -1;

    Disable();
    if(dmaaddr) {
        ULONG portsc = 0;
        UBYTE port = 0;
        UWORD ctxoff = 1;
        if(hc->hc_Flags & HCF_CTX64)
            ctxoff <<= 1;

        UBYTE slotid = (trbflags >> 24) & 0xFF;
        if((slotid > 0) && (slotid < USB_DEV_MAX)) {
            struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slotid];

            if(devCtx && devCtx->dc_SlotCtx.dmaa_Ptr)
                slot = (volatile struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;
        }

        if(!slot) {
            inctx = (volatile struct xhci_inctx *)dmaaddr;
            slot = xhciInputSlotCtx(inctx, ctxoff);
        }

        if(slot) {
            volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);

            ULONG slotctx1 = AROS_LE2LONG(slot->ctx[1]);

            port = (slotctx1 >> 16) & 0xff;

            if(port > 0 && port <= hc->hc_NumPorts)
                portsc = AROS_LE2LONG(xhciports[port - 1].portsc);
        }
        if(!(slot) || !(portsc & XHCIF_PR_PORTSC_CCS)) {
            Enable();
            return -1;
        }
        queued = xhciQueueTRB_IO(hc, cmdring, (UQUAD)(IPTR)dmaaddr, 0, trbflags,
                                 &ioreq->iouh_Req);
    } else
        queued = xhciQueueTRB_IO(hc, cmdring, 0, 0, trbflags, &ioreq->iouh_Req);

    if(queued != -1) {
        xhcic->xhc_CmdResults[queued].flags = 0xFFFFFFFF;
        xhcic->xhc_CmdResults[queued].status = 0xFFFFFFFF;
    }
    Enable();

    if(queued != -1) {
        xhciRingDoorbell(hc, 0, 0);
        return 1;
    }

    return -1;
}

/*
 * Submit a command to the xHCI command TRB
 * returns -1 on failure, or the slotid
 */
LONG xhciCmdSlotEnable(struct PCIController *hc, struct timerequest *timerreq)
{
    ULONG trbflags = TRBF_FLAG_CRTYPE_ENABLE_SLOT, cmdflags;
    LONG cc;
    UBYTE slotid;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);
    cc = xhciCmdSubmit(hc, NULL, trbflags, &cmdflags, timerreq);
    if(cc != TRB_CC_SUCCESS)
        return -1;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s: flags = %08x" DEBUGCOLOR_RESET" \n", __func__, cmdflags);

    slotid = (cmdflags >> 24) & 0XFF;

    if(slotid == 0 || slotid >= USB_DEV_MAX)
        return -1;

    return slotid;
}

#if !defined(PCIUSB_INLINEXHCIOPS)
/*
 * Remaining functions return
 * -1 on failure, or the completion code
 */
LONG xhciCmdSlotDisable(struct PCIController *hc, ULONG slot, struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_DISABLE_SLOT;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL, timerreq);
}

LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot,
                          APTR dmaaddr, ULONG bsr, struct IOUsbHWReq *ioreq,
                          struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_ADDRESS_DEVICE;
    /* Address Device Command TRB: bit 9 = BSR (Block SetAddress Request) */
    if(bsr)
        flags |= (1UL << 9);

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, slot, dmaaddr);

    if(ioreq)
        return xhciCmdSubmitAsync(hc, dmaaddr, flags, ioreq);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL, timerreq);
}

LONG xhciCmdEndpointStop(struct PCIController *hc, ULONG slot, ULONG epid, ULONG suspend,
                         struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | (suspend << 23) | (epid << 16) | TRBF_FLAG_CRTYPE_STOP_ENDPOINT;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL, timerreq);
}

LONG xhciCmdEndpointReset(struct PCIController *hc, ULONG slot, ULONG epid, ULONG preserve,
                          struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | (epid << 16) | TRBF_FLAG_CRTYPE_RESET_ENDPOINT | (preserve << 9);

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL, timerreq);
}

LONG xhciCmdSetTRDequeuePtr(struct PCIController *hc, ULONG slot, ULONG epid, APTR dequeue_ptr,
                            BOOL dcs, struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | (epid << 16) | TRBF_FLAG_CRTYPE_SET_TR_DEQUEUE_PTR;
    UQUAD dma = 0;
    /*
     * The Set TR Dequeue Pointer command parameter is a DMA address with bit 0
     * carrying the Dequeue Cycle State (DCS). Do NOT OR flag bits into a CPU
     * pointer and then run it through CPU->PCI translation; translate first,
     * then apply DCS.
     */
    if(dequeue_ptr) {
#if !defined(PCIUSB_NO_CPUTOPCI)
        dma = (UQUAD)(IPTR)CPUTOPCI(hc, hc->hc_PCIDriverObject, dequeue_ptr);
#else
        dma = (UQUAD)(IPTR)dequeue_ptr;
#endif
    }

    if(dcs)
        dma |= 0x1ULL;

    pciusbXHCIDebug("xHCI",
                    DEBUGFUNCCOLOR_SET "%s(slot=%u, epid=%u, ptr=0x%p, dma=%08lx:%08lx, dcs=%u)" DEBUGCOLOR_RESET" \n",
                    __func__, slot, epid, dequeue_ptr,
                    (ULONG)((dma >> 32) & 0xffffffffUL),
                    (ULONG)(dma        & 0xffffffffUL),
                    dcs ? 1U : 0U);

    /* Pass the DMA value (with DCS encoded in bit 0) through to the command TRB. */
    return xhciCmdSubmit(hc, (APTR)(IPTR)dma, flags, NULL, timerreq);
}

LONG xhciCmdEndpointConfigure(struct PCIController *hc, ULONG slot, APTR dmaaddr,
                              struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_CONFIGURE_ENDPOINT;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL, timerreq);
}

LONG xhciCmdContextEvaluate(struct PCIController *hc, ULONG slot, APTR dmaaddr,
                            struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_EVALUATE_CONTEXT;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL, timerreq);
}

LONG xhciCmdNoOp(struct PCIController *hc, ULONG slot, APTR dmaaddr,
                 struct timerequest *timerreq)
{
    ULONG flags = TRBF_FLAG_TRTYPE_NOOP;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL, timerreq);
}
#endif /* !PCIUSB_INLINEXHCIOPS */

AROS_UFH0(void, xhciEventRingTask)
{
    AROS_USERFUNC_INIT

    struct PCIController *hc;
    struct XhciHCPrivate *xhcic;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    {
        struct Task *thistask;
        BOOL timer_ok;

        thistask = FindTask(NULL);
        hc = thistask->tc_UserData;
        xhcic = xhciGetHCPrivate(hc);
        xhcic->xhc_EventTask.xet_Task = thistask;
        SetTaskPri(thistask, 100);

        timer_ok = xhciOpenTaskTimer(&xhcic->xhc_EventTask.xet_TimerPort,
                                     &xhcic->xhc_EventTask.xet_TimerReq,
                                     strXhciEventTaskName);
        if(!timer_ok) {
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: unable to open timer.device" DEBUGCOLOR_RESET" \n", __func__);
        }
    }
    xhcic->xhc_EventTask.xet_ProcessEventsSignal = AllocSignal(-1);

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "'%s' @ 0x%p, ProcessEventsSignal=%d"
                    DEBUGCOLOR_RESET" \n",
                    ((struct Node *)xhcic->xhc_EventTask.xet_Task)->ln_Name,
                    xhcic->xhc_EventTask.xet_Task,
                    xhcic->xhc_EventTask.xet_ProcessEventsSignal);

    if(xhcic->xhc_ReadySigTask)
        Signal(xhcic->xhc_ReadySigTask, 1L << xhcic->xhc_ReadySignal);

    if(!xhcic->xhc_EventTask.xet_TimerReq || xhcic->xhc_EventTask.xet_ProcessEventsSignal == -1)
        goto task_cleanup;

    for(;;) {
        ULONG xhcictsigs = Wait(1 << xhcic->xhc_EventTask.xet_ProcessEventsSignal);
#if defined(DEBUG) && (DEBUG > 1)
        pciusbXHCIDebugV("xHCI",
                         DEBUGCOLOR_SET "IDnest %d TDNest %d"
                         DEBUGCOLOR_RESET" \n",
                         xhcic->xhc_EventTask.xet_Task->tc_IDNestCnt,
                         xhcic->xhc_EventTask.xet_Task->tc_TDNestCnt);
#endif

        if(xhcictsigs & (1 << xhcic->xhc_EventTask.xet_ProcessEventsSignal)) {
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Processing pending HC work" DEBUGCOLOR_RESET" \n");
            xhciHandleFinishedTDs(hc, xhcic->xhc_EventTask.xet_TimerReq);

            if(hc->hc_IntXFerQueue.lh_Head->ln_Succ)
                xhciScheduleIntTDs(hc);

            if(hc->hc_IsoXFerQueue.lh_Head->ln_Succ)
                xhciScheduleIsoTDs(hc);

            if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
                xhciScheduleAsyncTDs(hc, &hc->hc_CtrlXFerQueue, UHCMD_CONTROLXFER);

            if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
                xhciScheduleAsyncTDs(hc, &hc->hc_BulkXFerQueue, UHCMD_BULKXFER);
        }
    }

task_cleanup:
    if(xhcic->xhc_EventTask.xet_ProcessEventsSignal != -1) {
        FreeSignal(xhcic->xhc_EventTask.xet_ProcessEventsSignal);
        xhcic->xhc_EventTask.xet_ProcessEventsSignal = -1;
    }
    xhciCloseTaskTimer(&xhcic->xhc_EventTask.xet_TimerPort,
                       &xhcic->xhc_EventTask.xet_TimerReq);
    AROS_USERFUNC_EXIT
}
