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

        if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) == NULL) {
            struct pciusbXHCIDevice *devCtx;

            devCtx = xhciFindDeviceCtx(hc, ioreq->iouh_DevAddr);
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Device context for addr %u = 0x%p" DEBUGCOLOR_RESET" \n", ioreq->iouh_DevAddr, devCtx);
            if (devCtx != NULL) {
                ULONG txep = xhciEndpointID(ioreq->iouh_Endpoint, (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0);

                if ((txep == 0) || (txep >= MAX_DEVENDPOINTS) || !devCtx->dc_EPAllocs[txep].dmaa_Ptr) {
                    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                    ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                    return;
                }

                driprivate = AllocMem(sizeof(struct pciusbXHCIIODevPrivate), MEMF_ANY|MEMF_CLEAR);
                if (!driprivate) {
                    pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: Failed to allocate IO Driver Data!" DEBUGCOLOR_RESET" \n", __func__);
                    //TODO :
                    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                    ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                    return;
                }
                driprivate->dpDevice = devCtx;
                driprivate->dpEPID = txep;
            } else {
                pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "xHCI: No device attached" DEBUGCOLOR_RESET" \n");
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                ReplyMsg(&ioreq->iouh_Req.io_Message);
                return;
            }
        } else {
            struct pciusbXHCIDevice *devCtx;
            ULONG txep;

            devCtx = xhciFindDeviceCtx(hc, ioreq->iouh_DevAddr);
            txep = xhciEndpointID(ioreq->iouh_Endpoint, (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0);

            if ((!devCtx) || (txep >= MAX_DEVENDPOINTS) || !devCtx->dc_EPAllocs[txep].dmaa_Ptr) {
                pciusbXHCIDebug("xHCI", DEBUGWARNCOLOR_SET "Reusing driprivate=%p failed: devCtx=%p EPID=%u" DEBUGCOLOR_RESET" \n",
                                driprivate, devCtx, txep);
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                ReplyMsg(&ioreq->iouh_Req.io_Message);
                return;
            }

            driprivate->dpDevice = devCtx;
            driprivate->dpEPID   = txep;
        }

        if (driprivate && driprivate->dpDevice)
            xhciDumpEndpointCtx(hc, driprivate->dpDevice, driprivate->dpEPID, "interrupt schedule");

        {
            /* Interrupt TD flags */
            if (ioreq->iouh_Dir == UHDIR_IN) {
                /* For INT endpoints, DS_DIR is not required; only ISP is meaningful here. */
                trbflags |= TRBF_FLAG_ISP;          /* interrupt on short packet */
            }

            /* All interrupt payload TRBs are NORMAL from xHCI’s POV */
            trbflags |= TRBF_FLAG_TRTYPE_NORMAL;

            /****** SETUP COMPLETE ************/
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%u + %u nak timeout set" DEBUGCOLOR_RESET" \n", hc->hc_FrameCounter, (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT));

            /****** INSERT TRANSACTION ************/
            Disable();
            BOOL txdone = FALSE;
            if ((ioreq->iouh_DriverPrivate1 = driprivate) != NULL) {
                // mark endpoint as busy
                pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "%s: using DevEP %02lx" DEBUGCOLOR_RESET" \n", __func__, devadrep);
                unit->hu_DevBusyReq[devadrep] = ioreq;

                if (!driprivate->dpDevice) {
                    Enable();
                    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                    ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                    return;
                }

                epring = driprivate->dpDevice->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr;

                if (!epring) {
                    Enable();
                    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                    ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                    return;
                }

                pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "%s: EP ring @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, epring);

                // No setup or status TRB's
                driprivate->dpSTRB = (UWORD)-1;
                driprivate->dpSttTRB = (UWORD)-1;

                // queue the transaction
                UQUAD dma_addr;
                ULONG dmalen = ioreq->iouh_Length;
                APTR dmaptr = CachePreDMA(ioreq->iouh_Data, &dmalen,
                                          (ioreq->iouh_Dir == UHDIR_IN) ? DMAFLAGS_PREREAD : DMAFLAGS_PREWRITE);
#if !defined(PCIUSB_NO_CPUTOPCI)
                dma_addr = (IPTR)CPUTOPCI(hc, hc->hc_PCIDriverObject, dmaptr);
#else
                dma_addr = (IPTR)dmaptr;
#endif
                queued = xhciQueueData(
                    hc, epring,
                    dma_addr,
                    dmalen,
                    ioreq->iouh_MaxPktSize,
                    trbflags,
                    TRUE);
                if (queued != -1) {
                    driprivate->dpTxSTRB = queued;
                    {
                        volatile struct xhci_trb *trb = &epring->ring[driprivate->dpTxSTRB];

                        pciusbXHCIDebugTRB("xHCI",
                            DEBUGCOLOR_SET "INT TD TRB: addr_lo=%08lx addr_hi=%08lx, tlen=%lu, flags=%08lx" DEBUGCOLOR_RESET" \n",
                            (unsigned long)trb->dbp.addr_lo,
                            (unsigned long)trb->dbp.addr_hi,
                            (unsigned long)(trb->tparams & 0x00FFFFFF),
                            (unsigned long)trb->flags);

                        pciusbXHCIDebugTRB("xHCI",
                            DEBUGCOLOR_SET "INT TD buffer ptr: trb=%p, ioreq->Data=%p (dma 0x%p)" DEBUGCOLOR_RESET" \n",
                            (APTR)((IPTR)trb->dbp.addr_lo |
                                  ((hc->hc_Flags & HCF_ADDR64)
                                       ? ((UQUAD)trb->dbp.addr_hi << 32)
                                       : 0)),
                            ioreq->iouh_Data, dma_addr);
                    }
                    int cnt;
                    if (epring->next > driprivate->dpTxSTRB + 1)
                        driprivate->dpTxETRB = epring->next - 1;
                    else
                        driprivate->dpTxETRB = driprivate->dpTxSTRB;
                    for (cnt = driprivate->dpTxSTRB; cnt < (driprivate->dpTxETRB + 1); cnt ++)
                        epring->ringio[cnt] = &ioreq->iouh_Req;
                    AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);
                    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Transaction queued in TRB #%u" DEBUGCOLOR_RESET" \n", driprivate->dpTxSTRB);
                    txdone = TRUE;
                }
            }
            Enable();
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

    xhciFreePeriodicContext(hc, hc->hc_Unit, &rtn->rtn_IOReq);

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
