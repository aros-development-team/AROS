/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver hw command support functions

    NB - do not use in the interrupt handler(s)
*/

#if defined(PCIUSB_ENABLEXHCI)
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
                   ULONG trbflags, ULONG *resflags)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_inctx *inctx;
    volatile struct xhci_slot *slot = NULL;
    WORD queued;

    Disable();
    if (dmaaddr) {
        ULONG portsc = 0;
        UBYTE port = 0;
        UWORD ctxoff = 1;
        if (hc->hc_Flags & HCF_CTX64)
            ctxoff <<= 1;

        UBYTE slotid = (trbflags >> 24) & 0xFF;
        if ((slotid > 0) && (slotid < USB_DEV_MAX)) {
            struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slotid];

            if (devCtx && devCtx->dc_SlotCtx.dmaa_Ptr)
                slot = (volatile struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;
        }

        if (!slot) {
            inctx = (volatile struct xhci_inctx *)dmaaddr;
            slot = xhciInputSlotCtx(inctx, ctxoff);

            pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s: slot input context @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, slot);
        }

        if (slot) {
            ULONG slotctx1 = AROS_LE2LONG(slot->ctx[1]);

            port = (slotctx1 >> 16) & 0xff;

            pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s: port #%u" DEBUGCOLOR_RESET" \n", __func__, port);

            if (port == 0 || port > hc->hc_NumPorts) {
                pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "%s: invalid port in slot context (slot=%u port=%u)" DEBUGCOLOR_RESET" \n",
                            __func__, (trbflags >> 24) & 0xFF, port);
                Enable();
                return -1;
            }

            volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);
            portsc = AROS_LE2LONG(xhciports[port - 1].portsc);

            pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s:     portsc=%08x" DEBUGCOLOR_RESET" \n", __func__, portsc);
        }
        if (!(slot) || !(portsc & XHCIF_PR_PORTSC_CCS)) {
            pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "%s: port disconnected (slot=%u port=%u portsc=%08lx)" DEBUGCOLOR_RESET" \n",
                         __func__, (trbflags >> 24) & 0xFF, (UWORD)port, (unsigned long)portsc);

            Enable();
            return -1;
        }
        queued = xhciQueueTRB(hc, xhcic->xhc_OPRp, (UQUAD)(IPTR)dmaaddr, 0, trbflags);
    } else {
        queued = xhciQueueTRB(hc, xhcic->xhc_OPRp, 0, 0, trbflags);
    }
    if (queued != -1) {
        xhcic->xhc_CmdResults[queued].flags = 0xFFFFFFFF;
        xhcic->xhc_CmdResults[queued].status = 0xFFFFFFFF;
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: Failed to queue command" DEBUGCOLOR_RESET" \n", __func__);
    }
    Enable();

    if (queued != -1) {
        xhciRingDoorbell(hc, 0, 0);

        /* Wait for completion with a bounded timeout to avoid hanging */
        for (ULONG waitms = 0; waitms < 1000; waitms++) {
            if (xhcic->xhc_CmdResults[queued].status != 0xFFFFFFFF) {
                if (resflags)
                    *resflags = xhcic->xhc_CmdResults[queued].flags;

                return xhcic->xhc_CmdResults[queued].status;
            }

            uhwDelayMS(1, hc->hc_Unit);
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

    if (!ioreq)
        return -1;

    Disable();
    if (dmaaddr) {
        ULONG portsc = 0;
        UBYTE port = 0;
        UWORD ctxoff = 1;
        if (hc->hc_Flags & HCF_CTX64)
            ctxoff <<= 1;

        UBYTE slotid = (trbflags >> 24) & 0xFF;
        if ((slotid > 0) && (slotid < USB_DEV_MAX)) {
            struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slotid];

            if (devCtx && devCtx->dc_SlotCtx.dmaa_Ptr)
                slot = (volatile struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;
        }

        if (!slot) {
            inctx = (volatile struct xhci_inctx *)dmaaddr;
            slot = xhciInputSlotCtx(inctx, ctxoff);
        }

        if (slot) {
            volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);

            ULONG slotctx1 = AROS_LE2LONG(slot->ctx[1]);

            port = (slotctx1 >> 16) & 0xff;

            if (port > 0 && port <= hc->hc_NumPorts)
                portsc = AROS_LE2LONG(xhciports[port - 1].portsc);
        }
        if (!(slot) || !(portsc & XHCIF_PR_PORTSC_CCS)) {
            Enable();
            return -1;
        }
        queued = xhciQueueTRB(hc, cmdring, (UQUAD)(IPTR)dmaaddr, 0, trbflags);
    } else
        queued = xhciQueueTRB(hc, cmdring, 0, 0, trbflags);

    if (queued != -1) {
        xhcic->xhc_CmdResults[queued].flags = 0xFFFFFFFF;
        xhcic->xhc_CmdResults[queued].status = 0xFFFFFFFF;
        xhciRingLock();
        cmdring->ringio[queued] = &ioreq->iouh_Req;
        xhciRingUnlock();
    }

    Enable();

    if (queued != -1) {
        xhciRingDoorbell(hc, 0, 0);
        return 1;
    }

    return -1;
}

/*
 * Submit a command to the xHCI command TRB
 * returns -1 on failure, or the slotid
 */
LONG xhciCmdSlotEnable(struct PCIController *hc)
{
    ULONG trbflags = TRBF_FLAG_CRTYPE_ENABLE_SLOT, cmdflags;
    LONG cc;
    UBYTE slotid;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);
    cc = xhciCmdSubmit(hc, NULL, trbflags, &cmdflags);
    if (cc != TRB_CC_SUCCESS)
        return -1;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s: flags = %08x" DEBUGCOLOR_RESET" \n", __func__, cmdflags);

    slotid = (cmdflags >> 24) & 0XFF;

    if (slotid == 0 || slotid >= USB_DEV_MAX)
        return -1;

    return slotid;
}

#if !defined(PCIUSB_INLINEXHCIOPS)
/*
 * Remaining functions return
 * -1 on failure, or the completion code
 */
LONG xhciCmdSlotDisable(struct PCIController *hc, ULONG slot)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_DISABLE_SLOT;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL);
}

LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot,
                          APTR dmaaddr, ULONG bsr, struct IOUsbHWReq *ioreq)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_ADDRESS_DEVICE;
    /* Address Device Command TRB: bit 9 = BSR (Block SetAddress Request) */
    if (bsr)
        flags |= (1UL << 9);

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, slot, dmaaddr);

    if (ioreq)
        return xhciCmdSubmitAsync(hc, dmaaddr, flags, ioreq);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
}

LONG xhciCmdEndpointStop(struct PCIController *hc, ULONG slot, ULONG epid, ULONG suspend)
{
    ULONG flags = (slot << 24) | (suspend << 23) | (epid << 16) | TRBF_FLAG_CRTYPE_STOP_ENDPOINT;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL);
}

LONG xhciCmdEndpointReset(struct PCIController *hc, ULONG slot, ULONG epid, ULONG preserve)
{
    ULONG flags = (slot << 24) | (epid << 16) | TRBF_FLAG_CRTYPE_RESET_ENDPOINT | (preserve << 9);

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL);
}

LONG xhciCmdEndpointConfigure(struct PCIController *hc, ULONG slot, APTR dmaaddr)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_CONFIGURE_ENDPOINT;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
}

LONG xhciCmdContextEvaluate(struct PCIController *hc, ULONG slot, APTR dmaaddr)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_EVALUATE_CONTEXT;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
}

LONG xhciCmdNoOp(struct PCIController *hc, ULONG slot, APTR dmaaddr)
{
    ULONG flags = TRBF_FLAG_TRTYPE_NOOP;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
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
        thistask = FindTask(NULL);
        hc = thistask->tc_UserData;
        xhcic = xhciGetHCPrivate(hc);
        xhcic->xhc_xHCERTask = thistask;
        SetTaskPri(thistask, 100);
    }
    xhcic->xhc_DoWorkSignal = AllocSignal(-1);

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "'%s' @ 0x%p, DoWorkSignal=%d"
                    DEBUGCOLOR_RESET" \n",
                    ((struct Node *)xhcic->xhc_xHCERTask)->ln_Name, xhcic->xhc_xHCERTask, xhcic->xhc_DoWorkSignal);

    if (xhcic->xhc_ReadySigTask)
        Signal(xhcic->xhc_ReadySigTask, 1L << xhcic->xhc_ReadySignal);

    for (;;) {
        ULONG xhcictsigs = Wait(1 << xhcic->xhc_DoWorkSignal);
#if defined(DEBUG) && (DEBUG > 1)
        pciusbXHCIDebugV("xHCI",
                        DEBUGCOLOR_SET "IDnest %d TDNest %d"
                        DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_xHCERTask->tc_IDNestCnt, xhcic->xhc_xHCERTask->tc_TDNestCnt);
#endif

        if (xhcictsigs & (1 << xhcic->xhc_DoWorkSignal)) {
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Processing pending HC work" DEBUGCOLOR_RESET" \n");
            xhciHandleFinishedTDs(hc);

            if (hc->hc_IntXFerQueue.lh_Head->ln_Succ)
                xhciScheduleIntTDs(hc);

            if (hc->hc_IsoXFerQueue.lh_Head->ln_Succ)
                xhciScheduleIsoTDs(hc);

            if (hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
                xhciScheduleAsyncTDs(hc, &hc->hc_CtrlXFerQueue, UHCMD_CONTROLXFER);

            if (hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
                xhciScheduleAsyncTDs(hc, &hc->hc_BulkXFerQueue, UHCMD_BULKXFER);
        }
    }
    AROS_USERFUNC_EXIT
}

#endif /* PCIUSB_ENABLEXHCI */
