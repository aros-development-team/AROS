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

void xhciFreeAsyncContext(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* Deactivate the endpoint */
    xhciFinishRequest(hc, unit, ioreq);
}

static ULONG xhciTDSetupFlags(ULONG tdflags, ULONG txtype, ULONG has_data)
{
    ULONG setupflags;

    setupflags = tdflags & ~(TRB_FLAG_TYPE_MASK);
    setupflags |= (TRBF_FLAG_TRTYPE_SETUP | TRBF_FLAG_IDT | TRBF_FLAG_CH);

    /*
     * TRB Transfer Type (TRT) bits:
     *
     * - No DATA stage: TRT = 0
     * - DATA OUT stage: TRT = 2
     * - DATA IN stage:  TRT = 3
     *
     * We derive the DATA direction from TRBF_FLAG_DS_DIR (set for IN).
     */
    if (!has_data) {
        /* No data stage. Make sure TRT is "no data". */
        setupflags &= ~(3UL << 16);
    } else if (tdflags & TRBF_FLAG_DS_DIR) {
        /* DATA IN */
        setupflags |= (3UL << 16);
    } else {
        /* DATA OUT */
        setupflags |= (2UL << 16);
    }

    return setupflags;
}

static ULONG xhciTDStatusFlags(ULONG tdflags)
{
    ULONG statusflags = 0;

    /* Status stage only needs the direction bit (IN=1, OUT=0) */
    if (tdflags & TRBF_FLAG_DS_DIR)
        statusflags |= TRBF_FLAG_DS_DIR;

    statusflags |= TRBF_FLAG_TRTYPE_STATUS;
    statusflags |= TRBF_FLAG_IOC;

    return statusflags;
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

static BOOL isStandardTRBTransfer(struct IOUsbHWReq *ioreq, ULONG txtype)
{
    return (txtype == UHCMD_BULKXFER) ||
        !((txtype == UHCMD_CONTROLXFER) &&
          (((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_DEVICE)) &&
            (ioreq->iouh_SetupData.bRequest == USR_SET_ADDRESS)) ||
           ((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_ENDPOINT)) &&
            (ioreq->iouh_SetupData.bRequest == USR_CLEAR_FEATURE) &&
            (ioreq->iouh_SetupData.wValue == AROS_WORD2LE(UFS_ENDPOINT_HALT)))));
}

static BOOL xhciQueueControlStages(struct PCIController *hc, struct IOUsbHWReq *ioreq,
    struct pciusbXHCIIODevPrivate *driprivate, volatile struct pcisusbXHCIRing *epring,
    ULONG trbflags)
{
    UQUAD setupdata_inline;
    ULONG has_data = (ioreq->iouh_Data && ioreq->iouh_Length) ? 1 : 0;
    WORD queued;

    xhciTDSetupInlinedata(&setupdata_inline, ioreq, UHCMD_CONTROLXFER);
    ULONG sf = xhciTDSetupFlags(trbflags, UHCMD_CONTROLXFER, has_data);
    sf |= TRBF_FLAG_IDT;  /* Force IDT for Setup Stage TRB */
    pciusbXHCIDebug("xHCI",
                    "Queueing SETUP TRB: inline=0x%08lx%08lx, len=%u\n",
                    (ULONG)((setupdata_inline >> 32) & 0xffffffffUL),
                    (ULONG)( setupdata_inline        & 0xffffffffUL),
                    (unsigned)sizeof(ioreq->iouh_SetupData));
    /* SETUP stage */
    queued = xhciQueueTRB(hc, epring, setupdata_inline,
                          sizeof(ioreq->iouh_SetupData),
                          sf);
    pciusbXHCIDebug("xHCI",
                    "xhciQueueTRB (SETUP) -> queued=%d\n",
                    (int)queued);

    if (queued == -1)
        return FALSE;

    driprivate->dpSTRB = queued;
    epring->ringio[queued] = &ioreq->iouh_Req;

    /* DATA stage (if any) */
    if (has_data) {
        /*
         * Control TD must be chained: Setup -> Data -> Status.
         * Therefore Data Stage TRB(s) must have CH=1 when Status follows.
         */
        queued = xhciQueuePayloadTRBs(hc, ioreq, driprivate, epring, trbflags | TRBF_FLAG_CH, FALSE);
        pciusbXHCIDebug("xHCI",
                        "xhciQueuePayloadTRBs (DATA) -> queued=%d\n",
                        (int)queued);
    } else {
        driprivate->dpTxSTRB = driprivate->dpSTRB;
        driprivate->dpTxETRB = (epring->next > 0) ? (epring->next - 1) : (XHCI_EVENT_RING_TRBS - 1);
        queued = driprivate->dpSTRB;
        pciusbXHCIDebug("xHCI",
                        "No DATA stage, using dpSTRB=%d\n",
                        (int)queued);
    }

    if (queued == -1)
        return FALSE;

    /*
     * Status stage direction rules:
     *
     * - If there is a DATA stage, Status direction is the opposite of the
     *   DATA stage direction (TRBF_FLAG_DS_DIR).
     * - If there is no DATA stage, Status direction is always IN.
     *
     * TRBF_FLAG_DS_DIR is already used to describe DATA direction in trbflags.
     */
    {
        ULONG status_tdflags = trbflags;

        /* Status Stage must terminate the TD: never chained. */
        status_tdflags &= ~TRBF_FLAG_CH;

        if (has_data) {
            /* Opposite of DATA direction. */
            status_tdflags ^= TRBF_FLAG_DS_DIR;
        } else {
            /* No DATA stage: Status is always IN. */
            status_tdflags |= TRBF_FLAG_DS_DIR;
        }

        pciusbXHCIDebug("xHCI",
                        "Queueing STATUS TRB\n");
        ULONG stf = xhciTDStatusFlags(status_tdflags);
        queued = xhciQueueTRB(hc, epring, 0, 0,
                              stf);
        pciusbXHCIDebug("xHCI",
                        "xhciQueueTRB (STATUS) -> queued=%d\n",
                        (int)queued);
    }

    if (queued != -1) {
        driprivate->dpSttTRB = queued;
        epring->ringio[queued] = &ioreq->iouh_Req;
    }

    return queued != -1;
}

void xhciScheduleAsyncTDs(struct PCIController *hc, struct List *txlist, ULONG txtype)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq, *ionext;
    UWORD devadrep;

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

        UWORD effdir = xhciEffectiveDataDir(ioreq);
        devadrep = xhciDevEPKey(ioreq);

        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "New Async transfer to dev=%u ep=%u (DevEP=%02lx): len=%lu, dir=%s, cmd=%u" DEBUGCOLOR_RESET" \n",
                        ioreq->iouh_DevAddr,
                        ioreq->iouh_Endpoint,
                        devadrep,
                        (ULONG)ioreq->iouh_Length,
                        (effdir == UHDIR_IN) ? "IN" : "OUT",
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

        volatile struct pcisusbXHCIRing *epring = NULL;
        struct pciusbXHCIIODevPrivate *driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        struct pciusbXHCIDevice *devCtx = NULL;
        ULONG trbflags = 0;
        WORD queued = -1;
        BOOL txdone = FALSE;

        if (!driprivate || !driprivate->dpDevice) {
            pciusbError("xHCI",
                        DEBUGWARNCOLOR_SET "xHCI: Missing prepared transfer context for Dev=%u EP=%u" DEBUGCOLOR_RESET" \n",
                        ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            continue;
        }

        devCtx = driprivate->dpDevice;

        if (isStandardTRBTransfer(ioreq, txtype)) {
            pciusbXHCIDebug("xHCI",
                            "Normal transfer path: txtype=%lx, DevAddr=%u, EP=%u\n",
                            txtype, ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);

            trbflags |= xhciBuildDataTRBFlags(ioreq, txtype);

            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "Frame=%u: Nak timeout for DevEP=%02lx set to %u" DEBUGCOLOR_RESET" \n",
                            hc->hc_FrameCounter, devadrep,
                            unit->hu_NakTimeoutFrame[devadrep]);

            if (txtype != UHCMD_CONTROLXFER) {
                driprivate->dpSTRB = (UWORD)-1;
                driprivate->dpSttTRB = (UWORD)-1;
            }

            if (xhciActivateEndpointTransfer(hc, unit, ioreq, driprivate, devadrep, &epring)) {
                if (txtype == UHCMD_CONTROLXFER) {
                    txdone = xhciQueueControlStages(hc, ioreq, driprivate, epring, trbflags);
                } else {
                    queued = xhciQueuePayloadTRBs(hc, ioreq, driprivate, epring, trbflags, TRUE);
                    if (queued != -1)
                        txdone = TRUE;
                }

                if (txdone) {
                    AddTail(&hc->hc_TDQueue,
                            (struct Node *) ioreq);
                    pciusbXHCIDebug("xHCI",
                                    DEBUGCOLOR_SET "Transaction queued in TRB #%u (Dev=%u EP=%u)" DEBUGCOLOR_RESET" \n",
                                    driprivate->dpTxSTRB,
                                    ioreq->iouh_DevAddr,
                                    ioreq->iouh_Endpoint);
                }
            }

            if (!txdone) {
                pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: txdone=FALSE, failed to submit transaction (Dev=%u EP=%u, txtype=%lx)" DEBUGCOLOR_RESET" \n",
                    ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, txtype);

                driprivate->dpSTRB   = (UWORD)-1;
                driprivate->dpTxSTRB = (UWORD)-1;
                driprivate->dpTxETRB = (UWORD)-1;
                driprivate->dpSttTRB = (UWORD)-1;

                if (ioreq->iouh_Req.io_Error) {
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                } else {
                    Disable();
                    AddHead(txlist, (struct Node *) ioreq);
                    Enable();
                }
            } else {
                pciusbXHCIDebug("xHCI",
                                "Ringing doorbell: slot=%u epid=%u\n",
                                driprivate->dpDevice->dc_SlotID,
                                driprivate->dpEPID);
                xhciRingDoorbell(hc, driprivate->dpDevice->dc_SlotID, driprivate->dpEPID);
            }
        }

        pciusbXHCIDebug("xHCI",
                        "---- IOReq done: %p ----\n", ioreq);
    }

    pciusbXHCIDebug("xHCI", "%s: exit\n", __func__);
}
#endif /* PCIUSB_ENABLEXHCI */
