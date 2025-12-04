/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver hw command support functions

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
    volatile struct xhci_dbr *xhcidb = (volatile struct xhci_dbr *)((IPTR)hc->hc_XHCIDB);

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u, %08x)" DEBUGCOLOR_RESET" \n", __func__, slot, value);
    xhcidb[slot].db = AROS_LONG2LE(value);
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
            struct pciusbXHCIDevice *devCtx = hc->hc_Devices[slotid];

            if (devCtx && devCtx->dc_SlotCtx.dmaa_Ptr)
                slot = (volatile struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;
        }

        if (!slot) {
            inctx = (volatile struct xhci_inctx *)dmaaddr;
            slot = xhciInputSlotCtx(inctx, ctxoff);

            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s: slot input context @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, slot);
        }

        if (slot) {
            ULONG slotctx1 = AROS_LE2LONG(slot->ctx[1]);

            port = (slotctx1 >> 16) & 0xff;

            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s: port #%u" DEBUGCOLOR_RESET" \n", __func__, port);

            if (port == 0 || port > hc->hc_NumPorts) {
                pciusbDebug("xHCI", DEBUGWARNCOLOR_SET "%s: invalid port in slot context (slot=%u port=%u)" DEBUGCOLOR_RESET" \n",
                            __func__, (trbflags >> 24) & 0xFF, port);
                Enable();
                return -1;
            }

            volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
            portsc = AROS_LE2LONG(xhciports[port - 1].portsc);

            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s:     portsc=%08x" DEBUGCOLOR_RESET" \n", __func__, portsc);
        }
        if (!(slot) || !(portsc & XHCIF_PR_PORTSC_CCS)) {
            pciusbDebug("xHCI", DEBUGWARNCOLOR_SET "%s: port disconnected (slot=%u port=%u portsc=%08lx)" DEBUGCOLOR_RESET" \n",
                         __func__, (trbflags >> 24) & 0xFF, (UWORD)port, (unsigned long)portsc);

            Enable();
            return -1;
        }
        queued = xhciQueueTRB(hc, hc->hc_OPRp, (UQUAD)(IPTR)dmaaddr, 0, trbflags);
    } else {
        queued = xhciQueueTRB(hc, hc->hc_OPRp, 0, 0, trbflags);
    }
    if (queued != -1) {
        hc->hc_CmdResults[queued].flags = 0xFFFFFFFF;
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: Failed to queue command" DEBUGCOLOR_RESET" \n", __func__);
    }
    Enable();

    if (queued != -1) {
        xhciRingDoorbell(hc, 0, 0);

        /* Wait for completion with a bounded timeout to avoid hanging */
        for (ULONG waitms = 0; waitms < 1000; waitms++) {
            if (hc->hc_CmdResults[queued].flags != 0xFFFFFFFF) {
                if (resflags)
                    *resflags = hc->hc_CmdResults[queued].flags;

                return (hc->hc_CmdResults[queued].tparams >> 24) & 0xFF;
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
    volatile struct xhci_inctx *inctx;
    volatile struct xhci_slot *slot = NULL;
    WORD queued;
    volatile struct pcisusbXHCIRing *cmdring = (volatile struct pcisusbXHCIRing *)hc->hc_OPRp;

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
            struct pciusbXHCIDevice *devCtx = hc->hc_Devices[slotid];

            if (devCtx && devCtx->dc_SlotCtx.dmaa_Ptr)
                slot = (volatile struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;
        }

        if (!slot) {
            inctx = (volatile struct xhci_inctx *)dmaaddr;
            slot = xhciInputSlotCtx(inctx, ctxoff);
        }

        if (slot) {
            volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);

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
        hc->hc_CmdResults[queued].flags = 0xFFFFFFFF;
        cmdring->ringio[queued] = &ioreq->iouh_Req;
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
    volatile struct pcisusbXHCIRing *xring = (volatile struct pcisusbXHCIRing *)hc->hc_OPRp;
    ULONG trbflags = TRBF_FLAG_CRTYPE_ENABLE_SLOT, cmdflags;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (1 != xhciCmdSubmit(hc, NULL, trbflags, &cmdflags))
        return -1;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s: flags = %08x" DEBUGCOLOR_RESET" \n", __func__, cmdflags);

    return (cmdflags >> 24) & 0XFF;
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
                          APTR dmaaddr, struct IOUsbHWReq *ioreq)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_ADDRESS_DEVICE;

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
#endif /* PCIUSB_ENABLEXHCI */
