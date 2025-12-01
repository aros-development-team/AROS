/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver async transfer support functions
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>
#include <aros/cpu.h>

#include <devices/usb_hub.h>

#include <string.h>

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

void xhciFreeAsyncContext(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* Deactivate the endpoint */
    xhciFinishRequest(hc, unit, ioreq);
}

static ULONG xhciTDSetupFlags(ULONG tdflags, ULONG txtype)
{
    ULONG setupflags = tdflags & ~(TRB_FLAG_TYPE_MASK);

    setupflags |= (TRBF_FLAG_TRTYPE_SETUP|TRBF_FLAG_IDT|TRBF_FLAG_CH);
    if (tdflags & TRBF_FLAG_DS_DIR)
        setupflags |= (3 << 16);
    else
        setupflags |= (2 << 16);

    return setupflags;
}

static ULONG xhciTDStatusFlags(ULONG tdflags)
{
    ULONG statusflags = tdflags & ~(TRB_FLAG_TYPE_MASK);

    statusflags |= TRBF_FLAG_TRTYPE_STATUS;
    statusflags |= TRBF_FLAG_IOC;

    return statusflags;
}

static UBYTE xhciEndpointIDFromIndex(UWORD wIndex)
{
    UBYTE epid;
    UBYTE endpoint = (wIndex & 0x0f);

    if (endpoint == 0)
        return 0;

    epid = endpoint << 1;
    if (wIndex & 0x80)
        epid |= 0x01;

    return epid;
}

static void xhciTDSetupInlinedata(UQUAD *inlinedata, struct IOUsbHWReq *ioreq, ULONG txtype)
{
#if AROS_BIG_ENDIAN
    UBYTE *outdatab = (UBYTE *)inlinedata;
    outdatab[0] = ioreq->iouh_SetupData.bmRequestType;
    outdatab[1] = ioreq->iouh_SetupData.bRequest;
    UWORD *outdataw = (UWORD *)&outdatab[2];
    outdataw[0]= AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    outdataw[1]= AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    outdataw[2]= AROS_WORD2LE(ioreq->iouh_SetupData.wLength);
#else
    CopyMem(&ioreq->iouh_SetupData, inlinedata, sizeof(UQUAD));
#endif
}

void xhciScheduleAsyncTDs(struct PCIController *hc, struct List *txlist, ULONG txtype)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq, *ionext;
    UWORD devadrep;
    BOOL doCompletion = FALSE;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" txtype=%lx, txlist=%p\n",
                    __func__, txtype, txlist);

#if defined(DEBUG) && (DEBUG > 1)
    {
        struct Task * thisTask = FindTask(NULL);
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "Task @ 0x%p, IDnest %d TDNest %d" DEBUGCOLOR_RESET" \n",
                        thisTask, thisTask->tc_IDNestCnt, thisTask->tc_TDNestCnt);
    }
#endif

    /* *** Schedule Transfers *** */
    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "Scheduling new Async transfers (Type %lx)..." DEBUGCOLOR_RESET" \n",
                    txtype);

    ForeachNodeSafe(txlist, ioreq, ionext) {
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "---- IOReq start: %p ----" DEBUGCOLOR_RESET" \n",
                        ioreq);

        devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint;
        if ((txtype == UHCMD_BULKXFER) && (ioreq->iouh_Dir == UHDIR_IN))
            devadrep += 0x10;

        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "New Async transfer to dev=%u ep=%u (DevEP=%02lx): len=%lu, dir=%s, cmd=%u" DEBUGCOLOR_RESET" \n",
                        ioreq->iouh_DevAddr,
                        ioreq->iouh_Endpoint,
                        devadrep,
                        (ULONG)ioreq->iouh_Length,
                        (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT",
                        ioreq->iouh_Req.io_Command);

        pciusbXHCIDebug("xHCI",
                        "    Flags=0x%08lx, NakTO=%u, MaxPkt=%u, Interval=%u\n",
                        ioreq->iouh_Flags,
                        ioreq->iouh_NakTimeout,
                        ioreq->iouh_MaxPktSize,
                        ioreq->iouh_Interval);

        if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER) {
            pciusbXHCIDebug("xHCI",
                            "    SETUP: bmReqType=0x%02x bReq=0x%02x wValue=0x%04x wIndex=0x%04x wLength=%u\n",
                            ioreq->iouh_SetupData.bmRequestType,
                            ioreq->iouh_SetupData.bRequest,
                            (unsigned)AROS_LE2WORD(ioreq->iouh_SetupData.wValue),
                            (unsigned)AROS_LE2WORD(ioreq->iouh_SetupData.wIndex),
                            (unsigned)AROS_LE2WORD(ioreq->iouh_SetupData.wLength));
        }

        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbXHCIDebug("xHCI",
                            DEBUGWARNCOLOR_SET "DevEP %02lx in use, IOReq=%p requeued/left in list" DEBUGCOLOR_RESET" \n",
                            devadrep, ioreq);
            continue;
        }

        /****** SETUP TRANSACTION ************/
        volatile struct pcisusbXHCIRing *epring = NULL;
        struct pciusbXHCIIODevPrivate *driprivate;
        struct pcisusbXHCIDevice *devCtx = NULL;
        ULONG trbflags = 0;
        WORD queued = -1;

        if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) == NULL) {
            pciusbXHCIDebug("xHCI",
                            "No existing driver private for IOReq=%p, obtaining device ctx...\n",
                            ioreq);

            devCtx = xhciObtainDeviceCtx(hc, ioreq);

            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "Device context for addr %u = 0x%p" DEBUGCOLOR_RESET" \n",
                            ioreq->iouh_DevAddr, devCtx);

            if (devCtx != NULL) {
                ULONG txep;

                if (ioreq->iouh_Endpoint == 0) {
                    txep = 1;
                    pciusbXHCIDebug("xHCI",
                                    "Using EP0 -> EPID %lu (default control)\n",
                                    txep);
                } else {
                    volatile struct xhci_inctx *in =
                        (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
                    struct xhci_ep *ep;
                    in->acf = 0;

                    UWORD ctxoff = 1;
                    if (hc->hc_Flags & HCF_CTX64)
                        ctxoff <<= 1;

                    pciusbXHCIDebug("xHCI",
                                    "Initializing non-EP0 endpoint: dev=%u, ep=%u, dir=%s, txtype=%lx\n",
                                    ioreq->iouh_DevAddr,
                                    ioreq->iouh_Endpoint,
                                    (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT",
                                    txtype);

                    /* initialize the endpoint for use .. */
                    txep = xhciInitEP(hc, devCtx,
                                      ioreq,
                                      ioreq->iouh_Endpoint,
                                      (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0,
                                      txtype,
                                      ioreq->iouh_MaxPktSize,
                                      ioreq->iouh_Interval,
                                      ioreq->iouh_Flags);

                    pciusbXHCIDebug("xHCI",
                                    "xhciInitEP returned EPID=%lu\n",
                                    txep);

                    if (txep > 0) {
                        volatile struct xhci_slot *islot =
                            (volatile struct xhci_slot *)&in[ctxoff];
                        ep = (struct xhci_ep *)&in[ctxoff * (txep + 1)];

                        pciusbXHCIDebug("xHCI",
                                        DEBUGCOLOR_SET "EPID %u initialized, sending Endpoint Configure" DEBUGCOLOR_RESET" \n",
                                        (unsigned)txep);

                        /* Send configure command. */
                        LONG cc = xhciCmdEndpointConfigure(hc, devCtx->dc_SlotID, devCtx->dc_IN.dmaa_DMA);
                        pciusbXHCIDebug("xHCI",
                                        "xHCI: EndpointConfigure slot=%u epid=%lu -> cc=%ld\n",
                                        devCtx->dc_SlotID, txep, cc);

                        if (cc != 1) {
                            pciusbError("xHCI",
                                DEBUGWARNCOLOR_SET "xHCI: Failed to configure Endpoint (%u)" DEBUGCOLOR_RESET" \n",
                                (unsigned)txep);
                            pciusbError("xHCI",
                                DEBUGWARNCOLOR_SET "xHCI: Endpoint %u, Dir %s, MaxSize %u" DEBUGCOLOR_RESET" \n",
                                ioreq->iouh_Endpoint,
                                (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT",
                                ioreq->iouh_MaxPktSize);
                            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            ReplyMsg(&ioreq->iouh_Req.io_Message);
                            pciusbXHCIDebug("xHCI",
                                            "Leaving %s early: endpoint configure failed\n",
                                            __func__);
                            return;
                        }

                        pciusbXHCIDebug("xHCI",
                                        DEBUGCOLOR_SET "%s: Endpoint configured" DEBUGCOLOR_RESET" \n",
                                        __func__);

                        ep->ctx[1] = ep->ctx[0] = 0;
                        ep->deq.addr_hi = ep->deq.addr_lo = 0;
                        ep->length = 0;
                    }
                }

                pciusbXHCIDebug("xHCI",
                                "Allocating driver private for IOReq=%p, EPID=%lu\n",
                                ioreq, (ULONG)txep);

                driprivate = AllocMem(sizeof(struct pciusbXHCIIODevPrivate),
                                      MEMF_ANY|MEMF_CLEAR);
                if (!driprivate) {
                    pciusbError("xHCI",
                        DEBUGWARNCOLOR_SET "%s: Failed to allocate IO Driver Data!" DEBUGCOLOR_RESET" \n",
                        __func__);
                    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                    ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                    pciusbXHCIDebug("xHCI",
                                    "Leaving %s early: driprivate allocation failed\n",
                                    __func__);
                    return;
                }
                driprivate->dpDevice = devCtx;
                driprivate->dpEPID  = txep;
            } else {
                pciusbWarn("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: No device attached for DevAddr=%u, EP=%u" DEBUGCOLOR_RESET" \n",
                    ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                ReplyMsg(&ioreq->iouh_Req.io_Message);
                pciusbXHCIDebug("xHCI",
                                "Leaving %s early: no device context\n",
                                __func__);
                return;
            }
        } else {
            devCtx = driprivate->dpDevice;
            pciusbXHCIDebug("xHCI",
                            "Reusing existing driver private=%p, devCtx=%p, EPID=%u\n",
                            driprivate, devCtx, driprivate->dpEPID);
        }

        /*
         * Normal transfer path vs special-case control handling
         */
        if ((txtype == UHCMD_BULKXFER) ||
                !((txtype == UHCMD_CONTROLXFER) &&
                  (((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_DEVICE)) &&
                    (ioreq->iouh_SetupData.bRequest == USR_SET_ADDRESS)) ||
                   ((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_ENDPOINT)) &&
                    (ioreq->iouh_SetupData.bRequest == USR_CLEAR_FEATURE) &&
                    (ioreq->iouh_SetupData.wValue == AROS_WORD2LE(UFS_ENDPOINT_HALT)))))) {


            pciusbXHCIDebug("xHCI",
                            "Normal transfer path: txtype=%lx, DevAddr=%u, EP=%u\n",
                            txtype, ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);

            /* Decide direction and TRB type based on transfer type */
            if (txtype == UHCMD_CONTROLXFER) {
                /* Control transfers: if we have a data stage, use DATA TRB + DS_DIR */
                if (ioreq->iouh_Data && ioreq->iouh_Length) {
                    if (ioreq->iouh_Dir == UHDIR_IN)
                        trbflags |= TRBF_FLAG_DS_DIR;     /* IN data stage */

                    trbflags |= TRBF_FLAG_TRTYPE_DATA;    /* Data Stage TRB */
                } else {
                    /* No data stage (wLength == 0) – nothing to mark specially here;
                       the SETUP/STATUS TRBs get their type from xhciTDSetupFlags/StatusFlags.
                       For any fallback direct payload, keep it NORMAL. */
                    trbflags |= TRBF_FLAG_TRTYPE_NORMAL;
                }
            } else {
                /* Bulk / other non-control – payload TRBs are just NORMAL */
                trbflags |= TRBF_FLAG_TRTYPE_NORMAL;
            }

            /****** SETUP COMPLETE ************/
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "Frame=%u: Nak timeout for DevEP=%02lx set to %u" DEBUGCOLOR_RESET" \n",
                            hc->hc_FrameCounter, devadrep,
                            unit->hu_NakTimeoutFrame[devadrep]);

            /****** INSERT TRANSACTION ************/
            Disable();
            BOOL txdone = FALSE;
            if ((ioreq->iouh_DriverPrivate1 = driprivate) != NULL) {
                /* mark endpoint as busy */
                pciusbXHCIDebugEP("xHCI",
                                  DEBUGCOLOR_SET "%s: using DevEP %02lx" DEBUGCOLOR_RESET" \n",
                                  __func__, devadrep);
                unit->hu_DevBusyReq[devadrep] = ioreq;
                epring = driprivate->dpDevice->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr;

                pciusbXHCIDebugEP("xHCI",
                                  DEBUGCOLOR_SET "%s: EP ring @ 0x%p (EPID=%u)" DEBUGCOLOR_RESET" \n",
                                  __func__, epring, driprivate->dpEPID);

                if (txtype == UHCMD_CONTROLXFER) {
                    /* queue the setup */
                    UQUAD setupdata_inline;
                    xhciTDSetupInlinedata(&setupdata_inline, ioreq, txtype);
                    pciusbXHCIDebug("xHCI",
                                    "Queueing SETUP TRB: inline=0x%llx, len=%u\n",
                                    (unsigned long long)setupdata_inline,
                                    (unsigned)sizeof(ioreq->iouh_SetupData));

                    queued = xhciQueueTRB(hc, epring, setupdata_inline,
                                          sizeof(ioreq->iouh_SetupData),
                                          xhciTDSetupFlags(trbflags, txtype));
                    pciusbXHCIDebug("xHCI",
                                    "xhciQueueTRB (SETUP) -> queued=%d\n",
                                    (int)queued);

                    if (queued != -1) {
                        driprivate->dpSTRB = queued;
                        epring->ringio[queued] = &ioreq->iouh_Req;
                    }
                } else {
                    driprivate->dpSTRB = (UWORD)-1;
                    queued = 0;
                }

                if (queued != -1) {
                    /* queue the transaction data (if any) */
                    if (ioreq->iouh_Data && ioreq->iouh_Length) {
                        pciusbXHCIDebug("xHCI",
                                        "Queueing DATA: buf=%p, len=%lu, mps=%u\n",
                                        ioreq->iouh_Data,
                                        (ULONG)ioreq->iouh_Length,
                                        ioreq->iouh_MaxPktSize);
                        UQUAD dma_addr;
#if !defined(PCIUSB_NO_CPUTOPCI)
                        dma_addr = (IPTR)CPUTOPCI(hc, hc->hc_PCIDriverObject, ioreq->iouh_Data);
#else
                        dma_addr = (IPTR)ioreq->iouh_Data;
#endif
                        queued = xhciQueueData(hc, epring,
                                               dma_addr,
                                               ioreq->iouh_Length,
                                               ioreq->iouh_MaxPktSize,
                                               trbflags,
                                               (txtype == UHCMD_CONTROLXFER) ? FALSE : TRUE);
                        pciusbXHCIDebug("xHCI",
                                        "xhciQueueData -> queued=%d\n",
                                        (int)queued);
                    } else {
                        queued = driprivate->dpSTRB;
                        pciusbXHCIDebug("xHCI",
                                        "No DATA stage, using dpSTRB=%d\n",
                                        (int)queued);
                    }

                    if (queued != -1) {
                        int cnt;
                        driprivate->dpTxSTRB = queued;
                        if (epring->next > driprivate->dpTxSTRB + 1)
                            driprivate->dpTxETRB = epring->next - 1;
                        else
                            driprivate->dpTxETRB = driprivate->dpTxSTRB;

                        pciusbXHCIDebug("xHCI",
                                        "TX TRB range: STRB=%u ETRB=%u, epring->next=%u\n",
                                        driprivate->dpTxSTRB,
                                        driprivate->dpTxETRB,
                                        epring->next);

                        for (cnt = driprivate->dpTxSTRB;
                             cnt < (driprivate->dpTxETRB + 1);
                             cnt ++) {
                            epring->ringio[cnt] = &ioreq->iouh_Req;
                        }

                        if (txtype == UHCMD_CONTROLXFER) {
                            /* finally queue the status */
                            pciusbXHCIDebug("xHCI",
                                            "Queueing STATUS TRB\n");
                            queued = xhciQueueTRB(hc, epring, 0, 0,
                                                  xhciTDStatusFlags(trbflags));
                            pciusbXHCIDebug("xHCI",
                                            "xhciQueueTRB (STATUS) -> queued=%d\n",
                                            (int)queued);

                            if (queued != -1) {
                                driprivate->dpSttTRB = queued;
                                epring->ringio[queued] = &ioreq->iouh_Req;
                            }
                        } else {
                            driprivate->dpSttTRB = (UWORD)-1;
                            queued = 0;
                        }

                        if (queued != -1) {
                            AddTail(&hc->hc_TDQueue,
                                    (struct Node *) ioreq);
                            pciusbXHCIDebug("xHCI",
                                            DEBUGCOLOR_SET "Transaction queued in TRB #%u (Dev=%u EP=%u)" DEBUGCOLOR_RESET" \n",
                                            driprivate->dpTxSTRB,
                                            ioreq->iouh_DevAddr,
                                            ioreq->iouh_Endpoint);
                            txdone = TRUE;
                        }
                    }
                }
            }
            Enable();

            /*
             * If we failed to get an endpoint,
             * or queue the transaction, requeue it
             */
            if (!txdone) {
                pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: txdone=FALSE, failed to submit transaction (Dev=%u EP=%u, txtype=%lx)" DEBUGCOLOR_RESET" \n",
                    ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, txtype);

                driprivate->dpSTRB   = (UWORD)-1;
                driprivate->dpTxSTRB = (UWORD)-1;
                driprivate->dpTxETRB = (UWORD)-1;
                driprivate->dpSttTRB = (UWORD)-1;

                Disable();
                AddHead(txlist, (struct Node *) ioreq);
                Enable();
            } else {
                pciusbXHCIDebug("xHCI",
                                "Ringing doorbell: slot=%u epid=%u\n",
                                driprivate->dpDevice->dc_SlotID,
                                driprivate->dpEPID);
                xhciRingDoorbell(hc, driprivate->dpDevice->dc_SlotID, driprivate->dpEPID);
            }
        } else if (ioreq->iouh_SetupData.bRequest == USR_SET_ADDRESS) {
            /* Handle SET_ADDRESS by short-circuiting in software for xHCI */

            /* newaddr = wValue from setup packet */
            UWORD newaddr = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);

            pciusbXHCIDebug("xHCI",
                "SET_ADDRESS short-circuit: slot=%u new=%u devctx=%p, DevAddr(before)=%u\n",
                devCtx ? devCtx->dc_SlotID : 0,
                newaddr,
                devCtx,
                ioreq->iouh_DevAddr);

            /* Record the new USB address in software only */
            if (devCtx) {
                devCtx->dc_DevAddr = newaddr;
                pciusbXHCIDebug("xHCI",
                                "SET_ADDRESS: devCtx->dc_DevAddr now %u\n",
                                devCtx->dc_DevAddr);
            } else {
                pciusbXHCIDebug("xHCI",
                                "SET_ADDRESS: WARNING: devCtx is NULL\n");
            }

            Disable();
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
            if ((ioreq->iouh_DriverPrivate1 = driprivate) != NULL) {
                driprivate->dpCC = TRB_CC_SUCCESS;
                AddTail(&hc->hc_TDQueue, &ioreq->iouh_Req.io_Message.mn_Node);
                pciusbXHCIDebug("xHCI",
                                "SET_ADDRESS: queued SW completion on TDQueue, dpCC=SUCCESS\n");
            } else {
                pciusbXHCIDebug("xHCI",
                                "SET_ADDRESS: no driprivate, requeuing on txlist\n");
                AddHead(txlist, &ioreq->iouh_Req.io_Message.mn_Node);
            }
            Enable();
            doCompletion = TRUE;
        } else {
            /* Handle CLEAR_FEATURE(ENDPOINT_HALT) without passing to the device */
            Disable();
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;

            if (devCtx) {
                UWORD epid = xhciEndpointIDFromIndex(
                    AROS_WORD2LE(ioreq->iouh_SetupData.wIndex));

                pciusbXHCIDebug("xHCI",
                                "CLEAR_FEATURE: mapped index to EPID=%u\n",
                                epid);

                if (epid > 0) {
                    LONG cc = xhciCmdEndpointStop(hc, devCtx->dc_SlotID, epid, FALSE);

                    pciusbXHCIDebug("xHCI",
                                    "CLEAR_FEATURE: EndpointStop slot=%u epid=%u -> cc=%ld\n",
                                    devCtx->dc_SlotID, epid, cc);

                    if (cc == TRB_CC_SUCCESS) {
                        cc = xhciCmdEndpointReset(hc, devCtx->dc_SlotID, epid, 0);
                    }

                    pciusbXHCIDebug("xHCI",
                                    "CLEAR_FEATURE: EndpointReset slot=%u epid=%u -> cc=%ld\n",
                                    devCtx->dc_SlotID, epid, cc);

                    if (cc == TRB_CC_SUCCESS) {
                        if (!driprivate) {
                            driprivate = AllocMem(sizeof(struct pciusbXHCIIODevPrivate),
                                                  MEMF_ANY|MEMF_CLEAR);
                            if (driprivate) {
                                driprivate->dpDevice = devCtx;
                                driprivate->dpEPID   = epid;
                                ioreq->iouh_DriverPrivate1 = driprivate;
                            }
                        }

                        if (driprivate) {
                            driprivate->dpCC = TRB_CC_SUCCESS;
                            AddTail(&hc->hc_TDQueue, &ioreq->iouh_Req.io_Message.mn_Node);
                            pciusbXHCIDebug("xHCI",
                                            "CLEAR_FEATURE: queued SW completion on TDQueue\n");
                            doCompletion = TRUE;
                        } else {
                            pciusbError("xHCI",
                                DEBUGWARNCOLOR_SET "CLEAR_FEATURE: no driprivate, reporting HOSTERROR" DEBUGCOLOR_RESET"\n");
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            AddHead(txlist, &ioreq->iouh_Req.io_Message.mn_Node);
                        }
                    } else {
                        pciusbError("xHCI",
                            DEBUGWARNCOLOR_SET "CLEAR_FEATURE: EndpointReset failed cc=%ld, HOSTERROR" DEBUGCOLOR_RESET"\n",
                            cc);
                        ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                        AddHead(txlist, &ioreq->iouh_Req.io_Message.mn_Node);
                    }
                } else {
                    pciusbXHCIDebug("xHCI",
                                    "CLEAR_FEATURE: epid==0, requeuing\n");
                    AddHead(txlist, &ioreq->iouh_Req.io_Message.mn_Node);
                }
            } else {
                pciusbXHCIDebug("xHCI",
                                "CLEAR_FEATURE: no devCtx, requeuing\n");
                AddHead(txlist, &ioreq->iouh_Req.io_Message.mn_Node);
            }
            Enable();
        }

        pciusbXHCIDebug("xHCI",
                        "---- IOReq done: %p ----\n", ioreq);
    }

    if (doCompletion) {
        pciusbXHCIDebug("xHCI",
                        "doCompletion=TRUE -> SureCause(hc_CompleteInt)\n");
        SureCause(base, &hc->hc_CompleteInt);
    }

    pciusbXHCIDebug("xHCI", "%s: exit\n", __func__);
}

#endif /* PCIUSB_ENABLEXHCI */
