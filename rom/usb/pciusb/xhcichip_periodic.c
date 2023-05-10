/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved

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

#undef base
#define base (hc->hc_Device)

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
    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* Deactivate the endpoint */
    xhciFinishRequest(hc, unit, ioreq);
}

void xhciScheduleIntTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq, *ionext;
    UWORD devadrep;
#if (0)
    BOOL doCompletion = FALSE;
#endif

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* *** Schedule Transfers *** */
    pciusbDebug("xHCI", DEBUGCOLOR_SET "Scheduling new INT transfers ..." DEBUGCOLOR_RESET" \n");
    ForeachNodeSafe(&hc->hc_IntXFerQueue, ioreq, ionext)
    {
        devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbDebug("xHCI", DEBUGCOLOR_SET "New INT transfer to %ld.%ld: %ld bytes" DEBUGCOLOR_RESET" \n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        pciusbDebug("xHCI", DEBUGCOLOR_SET "    IOReq @ 0x%p" DEBUGCOLOR_RESET" \n", ioreq);

        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            pciusbDebug("xHCI", "DevEP %02lx in use!\n", devadrep);
            continue;
        }

        /****** SETUP TRANSACTION ************/
        volatile struct pcisusbXHCIRing *epring = NULL;
        struct pciusbXHCIIODevPrivate *driprivate;
        ULONG trbflags = 0;
        WORD queued = -1;

        if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) == NULL)
        {
            struct pcisusbXHCIDevice *devCtx;
            UWORD hciport;
#if (1)
            hciport = ioreq->iouh_HubPort - 1;
#else
            psdGetAttrs(PGA_DEVICE, pd, DA_AtHubPortNumber, &hciport, TAG_END); 
#endif
            pciusbDebug("xHCI", DEBUGCOLOR_SET "Device context for port #%u = 0x%p" DEBUGCOLOR_RESET" \n", hciport + 1, hc->hc_Devices[hciport]);
            if ((devCtx = hc->hc_Devices[hciport]) != NULL)
            {
                ULONG txep;

                if (ioreq->iouh_Endpoint == 0)
                {
                        txep = 1;
                }
                else
                {
                    volatile struct xhci_inctx *in = (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
                    struct xhci_ep *ep;
                    in->acf = 0;

                    UWORD ctxoff = 1;
                    if (hc->hc_Flags & HCF_CTX64)
                        ctxoff <<= 1;

                    /* initialize the endpoint for use .. */
                    txep = xhciInitEP(hc, devCtx,
                                ioreq->iouh_Endpoint,
                                (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0,
                                UHCMD_INTXFER,
                                ioreq->iouh_MaxPktSize);

                    if (txep > 0)
                    {
                        volatile struct xhci_slot *islot = (volatile struct xhci_slot *)&in[ctxoff];
                        ep = (struct xhci_ep *)&in[ctxoff * (txep + 1)];

                        pciusbDebug("xHCI", DEBUGCOLOR_SET "EPID %u initialized" DEBUGCOLOR_RESET" \n", txep);

                        if(ioreq->iouh_Interval >= 255)
                        {
                            ep->ctx[0] |= (10 << 16);
                        }
#if (0)
                        else
                        {
                            int val, last = 3;
                            for (val = 3; val <= 10; val++)
                            {
                                if (pow(2, val) * 125 < ioreq->iouh_Interval)
                                    last = val;
                            }
                        }
#endif
                        pciusbDebug("xHCI", DEBUGCOLOR_SET "Interval %u = %u" DEBUGCOLOR_RESET" \n", ioreq->iouh_Interval, (ep->ctx[0] >> 16) & 0xFF);
#if (0)
                        if (ioreq->iouh_Flags & UHFF_SUPERSPEED)
                        {
                            islot->ctx[0] |= SLOT_CTX_SUPERSPEED;
                        } else
#endif
                        if (ioreq->iouh_Flags & UHFF_HIGHSPEED)
                        {
                            islot->ctx[0] |= SLOTF_CTX_HIGHSPEED;
                        }
                        else if(ioreq->iouh_Flags & UHFF_LOWSPEED)
                        {
                            islot->ctx[0] |= SLOTF_CTX_LOWSPEED;
                        }
                        else
                        {
                            islot->ctx[0] |= SLOTF_CTX_FULLSPEED;
                        }

                        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                        {
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "*** SPLIT TRANSACTION to HubPort %ld at Addr %ld" DEBUGCOLOR_RESET" \n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr);
                            islot->ctx[0]    |= ioreq->iouh_SplitHubAddr;
                            islot->ctx[1]    |= (ioreq->iouh_SplitHubPort) << 16;
                        } else {

                        }

                        /* Send configure command. */
                        LONG cc = xhciCmdEndpointConfigure(hc, devCtx->dc_SlotID, devCtx->dc_IN.dmaa_DMA);
                        if (cc != 1)
                        {
                            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Failed to configure Endpoint (%u)" DEBUGCOLOR_RESET" \n", txep);
                            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Endpoint %u, Dir %s, MaxSize %u" DEBUGCOLOR_RESET" \n", ioreq->iouh_Endpoint,
                                                                                                                                                                                            (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT",
                                                                                                                                                                                            ioreq->iouh_MaxPktSize);
                            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            ReplyMsg(&ioreq->iouh_Req.io_Message);
                            return;
                        }

                        pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: Endpoint configured" DEBUGCOLOR_RESET" \n", __func__);

                        ep->ctx[1] = ep->ctx[0] = 0;
                        ep->deq.addr_hi = ep->deq.addr_lo = 0;
                        ep->length = 0;
                    }
                }

                driprivate = AllocMem(sizeof(struct pciusbXHCIIODevPrivate), MEMF_ANY|MEMF_CLEAR);
                if (!driprivate)
                {
                    pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: Failed to allocate IO Driver Data!" DEBUGCOLOR_RESET" \n", __func__);
                    //TODO : 
                    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                    ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                    return;
                }
                driprivate->dpDevice = devCtx;
                driprivate->dpEPID = txep;
            }
            else
            {
                pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "xHCI: No device attached" DEBUGCOLOR_RESET" \n");
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                ReplyMsg(&ioreq->iouh_Req.io_Message);
                return;
            }
        }

        {
            APTR txdata = ioreq->iouh_Data;

            if ((ioreq->iouh_Dir == UHDIR_IN) != 0)
                trbflags |= (TRBF_FLAG_DS_DIR|TRBF_FLAG_ISP);
            if ((ioreq->iouh_Data) &&  (ioreq->iouh_Length))
            {
                trbflags |= TRBF_FLAG_TRTYPE_NORMAL;
            }
            else if ((ioreq->iouh_Length))
            {
                trbflags |= TRBF_FLAG_TRTYPE_DATA;
            }

            /****** SETUP COMPLETE ************/
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                    (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
            pciusbDebug("xHCI", DEBUGCOLOR_SET "%u + %u nak timeout set" DEBUGCOLOR_RESET" \n", hc->hc_FrameCounter, (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT));

            /****** INSERT TRANSACTION ************/
            Disable();
            BOOL txdone = FALSE;
            if ((ioreq->iouh_DriverPrivate1 = driprivate) != NULL)
            {
                // mark endpoint as busy
                pciusbDebugEP("xHCI", DEBUGCOLOR_SET "%s: using DevEP %02lx" DEBUGCOLOR_RESET" \n", __func__, devadrep);
                unit->hu_DevBusyReq[devadrep] = ioreq;
                epring = driprivate->dpDevice->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr;

                pciusbDebugEP("xHCI", DEBUGCOLOR_SET "%s: EP ring @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, epring);

                // No setup or status TRB's
                driprivate->dpSTRB = (UWORD)-1;
                driprivate->dpSttTRB = (UWORD)-1;

                // queue the transaction
                queued = xhciQueueData(hc, epring, (UQUAD)(IPTR)ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_MaxPktSize, trbflags, TRUE);
                if (queued != -1)
                {
                    int cnt;
                    driprivate->dpTxSTRB = queued;
                    if (epring->next > driprivate->dpTxSTRB + 1)
                        driprivate->dpTxETRB = epring->next - 1;
                    else
                        driprivate->dpTxETRB = driprivate->dpTxSTRB;
                    for (cnt = driprivate->dpTxSTRB; cnt < (driprivate->dpTxETRB + 1); cnt ++)
                        epring->ringio[cnt] = &ioreq->iouh_Req;
                    AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Transaction queued in TRB #%u" DEBUGCOLOR_RESET" \n", driprivate->dpTxSTRB);
                    txdone = TRUE;
                }
            }
            Enable();
            /*
             * If we failed to get an endpoint,
             * or queue the transaction, requeue it
             */
            if (!txdone)
            {
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
#if (0)
    if (doCompletion)
        SureCause(base, &hc->hc_CompleteInt);
#endif
}
#if (0)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* *** INT Transfers *** */
    pciusbDebug("xHCI", DEBUGCOLOR_SET "Scheduling new INT transfers..." DEBUGCOLOR_RESET" \n");
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbDebug("xHCI", DEBUGCOLOR_SET "New INT transfer to %ld.%ld: %ld bytes" DEBUGCOLOR_RESET" \n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);

        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            pciusbWarn("xHCI", DEBUGCOLOR_SET "Endpoint %02lx in use!" DEBUGCOLOR_RESET" \n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            pciusbDebug("xHCI", DEBUGCOLOR_SET "*** SPLIT TRANSACTION to HubPort %ld at Addr %ld" DEBUGCOLOR_RESET" \n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr);

            // full speed and low speed handling
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "*** LOW SPEED ***" DEBUGCOLOR_RESET" \n");
            }

            if(ioreq->iouh_Interval >= 255)
            {

            } else {
#if (0)
                cnt = 0;
                do
                {

                } while(ioreq->iouh_Interval >= (1<<cnt));
#endif
            }
        } else {

        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
//        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: using DevEP %02lx" DEBUGCOLOR_RESET" \n", __func__, devadrep);
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] =
            (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
        pciusbDebug("xHCI", DEBUGCOLOR_SET "%u + %u nak timeout set" DEBUGCOLOR_RESET" \n", hc->hc_FrameCounter, (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT));

        Disable();
        AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);

        /****** Insert the transfer ************/

        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    }
}
#endif

WORD xhciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    return RC_OK;
}

WORD xhciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    return RC_OK;
}

void xhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    return;
}

void xhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    return;
}

void xhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    return;
}

#endif /* PCIUSB_ENABLEXHCI */
