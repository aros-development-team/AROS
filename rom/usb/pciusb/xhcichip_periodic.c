/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver async transfer support functions
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"
#include "xhcichip_schedule.h"

#if defined(DEBUG) && defined(XHCI_LONGDEBUGNAK)
#define XHCI_NAKTOSHIFT         (8)
#else
#define XHCI_NAKTOSHIFT         (3)
#endif

#ifdef base
#undef base
#endif
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

void xhciFreePeriodicContext(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* Deactivate the endpoint */
    xhciFinishRequest(hc, unit, ioreq);
}

void xhciScheduleIntTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq, *ionext;
    UWORD devadrep;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* *** Schedule Transfers *** */
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Scheduling new INT transfers ..." DEBUGCOLOR_RESET" \n");
    ForeachNodeSafe(&hc->hc_IntXFerQueue, ioreq, ionext) {
        devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "New INT transfer to dev=%u ep=%u (DevEP=%02x): len=%lu dir=%s, cmd=%u" DEBUGCOLOR_RESET"\n",
            ioreq->iouh_DevAddr,
            ioreq->iouh_Endpoint,
            devadrep,
            (unsigned long)ioreq->iouh_Length,
            (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT",
            (unsigned)ioreq->iouh_Req.io_Command);
        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "    IOReq=%p Flags=0x%08lx NakTO=%lu MaxPkt=%u Interval=%u" DEBUGCOLOR_RESET"\n",
            ioreq,
            (unsigned long)ioreq->iouh_Flags,
            (unsigned long)ioreq->iouh_NakTimeout,
            (unsigned)ioreq->iouh_MaxPktSize,
            (unsigned)ioreq->iouh_Interval);

        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbXHCIDebug("xHCI", "DevEP %02lx in use!\n", devadrep);
            continue;
        }

        /****** SETUP TRANSACTION ************/
        volatile struct pcisusbXHCIRing *epring = NULL;
        struct pciusbXHCIIODevPrivate *driprivate;
        ULONG trbflags = 0;
        WORD queued = -1;

        if (!xhciInitIOTRBTransfer(hc, ioreq, &hc->hc_IntXFerQueue, ioreq->iouh_Req.io_Command, FALSE, &driprivate))
            continue;

        {
            /* Interrupt TD flags */
            trbflags |= xhciBuildDataTRBFlags(ioreq, ioreq->iouh_Req.io_Command);
            if (ioreq->iouh_Dir == UHDIR_IN) {
                /* For INT endpoints, DS_DIR is not required; only ISP is meaningful here. */
                trbflags |= TRBF_FLAG_ISP;          /* interrupt on short packet */
            }

            /****** SETUP COMPLETE ************/
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%u + %u nak timeout set" DEBUGCOLOR_RESET" \n", hc->hc_FrameCounter, (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT));

            /****** INSERT TRANSACTION ************/
            BOOL txdone = FALSE;
            driprivate->dpSTRB = (UWORD)-1;
            driprivate->dpSttTRB = (UWORD)-1;

            if (xhciActivateEndpointTransfer(hc, unit, ioreq, driprivate, devadrep, &epring)) {
                queued = xhciQueuePayloadTRBs(hc, ioreq, driprivate, epring, trbflags, TRUE);

                if (queued != -1) {
                    AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);
                    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Transaction queued in TRB #%u" DEBUGCOLOR_RESET" \n", driprivate->dpTxSTRB);
                    txdone = TRUE;
                }
            }

            /*
             * If we failed to get an endpoint,
             * or queue the transaction, requeue it
             */
            if (!txdone) {
                driprivate->dpSTRB = (UWORD)-1;
                driprivate->dpTxSTRB = (UWORD)-1;
                driprivate->dpTxETRB = (UWORD)-1;
                driprivate->dpSttTRB = (UWORD)-1;

                Disable();
                pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Failed to submit transaction" DEBUGCOLOR_RESET" \n");
                AddHead(&hc->hc_IntXFerQueue, (struct Node *) ioreq);
                Enable();
            } else {
                xhciRingDoorbell(hc, driprivate->dpDevice->dc_SlotID, driprivate->dpEPID);
            }
        }
    }
}

WORD xhciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return UHIOERR_BADPARAMS;

    for (UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];

        if (ptd) {
            if (ptd->ptd_Descriptor)
                FreeMem(ptd->ptd_Descriptor, sizeof(struct pciusbXHCITRBParams));

            FreeMem(ptd, sizeof(struct PTDNode));
            rtn->rtn_PTDs[idx] = NULL;
        }

        ptd = AllocMem(sizeof(*ptd), MEMF_CLEAR);
        if (!ptd)
            goto fail;

        ptd->ptd_Descriptor = AllocMem(sizeof(struct pciusbXHCITRBParams), MEMF_CLEAR);
        if (!ptd->ptd_Descriptor) {
            FreeMem(ptd, sizeof(*ptd));
            rtn->rtn_PTDs[idx] = NULL;
            goto fail;
        }

        rtn->rtn_PTDs[idx] = ptd;
    }

    rtn->rtn_NextPTD = 0;
    rtn->rtn_BounceBuffer = NULL;

    return RC_OK;

fail:
    xhciFreeIsochIO(hc, rtn);
    return UHIOERR_OUTOFMEMORY;
}

WORD xhciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    rtn->rtn_BufferReq.ubr_Length = rtn->rtn_IOReq.iouh_Length;
    rtn->rtn_BufferReq.ubr_Frame = hc->hc_FrameCounter + 1;
    rtn->rtn_BufferReq.ubr_Flags = 0;

    rtn->rtn_NextPTD = 0;

    return RC_OK;
}

void xhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    /* Use the IOReq stored in the RTIso node for the actual transfer. */
    struct IOUsbHWReq *ioreq = &rtn->rtn_IOReq;

    if (!rtn->rtn_BufferReq.ubr_Buffer || !rtn->rtn_BufferReq.ubr_Length)
        return;

    ioreq->iouh_Data = rtn->rtn_BufferReq.ubr_Buffer;
    ioreq->iouh_Length = rtn->rtn_BufferReq.ubr_Length;
    ioreq->iouh_Req.io_Command = UHCMD_ISOXFER;
    ioreq->iouh_Req.io_Error = 0;

    /* Queue it like any other periodic transfer. */
    AddTail(&hc->hc_IntXFerQueue, (struct Node *)&ioreq->iouh_Req.io_Message.mn_Node);
    xhciScheduleIntTDs(hc);
}

void xhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    Disable();
    xhciFreePeriodicContext(hc, hc->hc_Unit, &rtn->rtn_IOReq);
    Enable();

    if (rtn->rtn_BounceBuffer && rtn->rtn_BounceBuffer != rtn->rtn_BufferReq.ubr_Buffer) {
        usbReleaseBuffer(rtn->rtn_BounceBuffer, rtn->rtn_BufferReq.ubr_Buffer,
                         rtn->rtn_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
        rtn->rtn_BounceBuffer = NULL;
    }
}

void xhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    xhciStopIsochIO(hc, rtn);

    if (!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    for (UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if (ptd) {
            if (ptd->ptd_Descriptor)
                FreeMem(ptd->ptd_Descriptor, sizeof(struct pciusbXHCITRBParams));

            FreeMem(ptd, sizeof(*ptd));
            rtn->rtn_PTDs[idx] = NULL;
        }
    }
}

#endif /* PCIUSB_ENABLEXHCI */
