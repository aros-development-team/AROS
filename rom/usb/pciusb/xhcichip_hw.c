/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved

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

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u, %08x)" DEBUGCOLOR_RESET" \n", __func__, slot, value);
    xhcidb[slot].db = value;
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
    WORD queued;

    Disable();
    if (dmaaddr)
    {
        ULONG portsc = 0;
        UWORD ctxoff = 1;
        if (hc->hc_Flags & HCF_CTX64)
            ctxoff <<= 1;

        inctx = (volatile struct xhci_inctx *)dmaaddr;
        volatile struct xhci_slot *slot = (void*)&inctx[ctxoff];

        if (slot)
        {
            volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
            UBYTE port;

            pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: slot input context @ 0x%p, ports @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, slot, xhciports);

            port = (slot->ctx[1] >> 16) & 0xff;
            pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: port #%u" DEBUGCOLOR_RESET" \n", __func__, port);

            portsc = xhciports[port - 1].portsc;
            pciusbDebug("xHCI", DEBUGCOLOR_SET "%s:     portsc=%08x" DEBUGCOLOR_RESET" \n", __func__, portsc);
        }
        if (!(slot) || !(portsc & XHCIF_PR_PORTSC_CCS))
        {
            pciusbDebug("xHCI", DEBUGWARNCOLOR_SET "%s: port disconnected" DEBUGCOLOR_RESET" \n", __func__);

            Enable();
            return -1;
        }
        queued = xhciQueueTRB(hc, hc->hc_OPRp, (UQUAD)(IPTR)dmaaddr, 0, trbflags);
    }
    else
        queued = xhciQueueTRB(hc, hc->hc_OPRp, 0, 0, trbflags);
    if (queued != -1)
        hc->hc_CmdResults[queued].flags = 0xFFFFFFFF;
    Enable();

    if (queued != -1)
    {
        xhciRingDoorbell(hc, 0, 0);
        do {
            if (hc->hc_CmdResults[queued].flags != 0xFFFFFFFF)
            {
                if (resflags)
                    *resflags = hc->hc_CmdResults[queued].flags;

                return (hc->hc_CmdResults[queued].tparams >> 24) & 0xFF;
            }
        } while (1);
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

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (1 != xhciCmdSubmit(hc, NULL, trbflags, &cmdflags))
      return -1;

    pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: flags = %08x" DEBUGCOLOR_RESET" \n", __func__, cmdflags);

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

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL);
}

LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot,
                                  APTR dmaaddr)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_ADDRESS_DEVICE;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, slot, dmaaddr);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
}

LONG xhciCmdEndpointStop(struct PCIController *hc, ULONG slot, ULONG epid, ULONG suspend)
{
    ULONG flags = (slot << 24) | (suspend << 23) | (epid << 16) | TRBF_FLAG_CRTYPE_STOP_ENDPOINT;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL);
}

LONG xhciCmdEndpointReset(struct PCIController *hc, ULONG slot, ULONG epid , ULONG preserve)
{
    ULONG flags = (slot << 24) | (epid << 16) | TRBF_FLAG_CRTYPE_RESET_ENDPOINT | (preserve << 9);

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, NULL, flags, NULL);
}

LONG xhciCmdEndpointConfigure(struct PCIController *hc, ULONG slot, APTR dmaaddr)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_CONFIGURE_ENDPOINT;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
}

LONG xhciCmdContextEvaluate(struct PCIController *hc, ULONG slot, APTR dmaaddr)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_EVALUATE_CONTEXT;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
}

LONG xhciCmdNoOp(struct PCIController *hc, ULONG slot, APTR dmaaddr)
{
    ULONG flags = TRBF_FLAG_TRTYPE_NOOP;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(%u)" DEBUGCOLOR_RESET" \n", __func__, slot);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
}
#endif /* !PCIUSB_INLINEXHCIOPS */
#endif /* PCIUSB_ENABLEXHCI */
