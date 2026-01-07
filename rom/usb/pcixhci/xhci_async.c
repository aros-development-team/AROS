/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver async transfer support functions
*/

#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>
#include <aros/cpu.h>

#include <devices/usb_hub.h>

#include <string.h>

#include "uhwcmd.h"
#include "xhciproto.h"
#include "xhci_schedule.h"

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

/* Setup Stage TRB - TRT (Transfer Type) field (bits 17:16 in dword3). */
#define TRBS_FLAG_SETUP_TRT                     16
#define TRB_FLAG_SETUP_TRT_MASK                 (3UL << TRBS_FLAG_SETUP_TRT)
#define TRBF_FLAG_SETUP_TRT_NONE                (0UL << TRBS_FLAG_SETUP_TRT)
#define TRBF_FLAG_SETUP_TRT_OUT                 (2UL << TRBS_FLAG_SETUP_TRT)
#define TRBF_FLAG_SETUP_TRT_IN                  (3UL << TRBS_FLAG_SETUP_TRT)

void xhciFreeAsyncContext(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* Deactivate the endpoint */
    xhciFinishRequest(hc, unit, ioreq);
}

static ULONG xhciTDSetupFlags(ULONG tdflags, ULONG txtype, ULONG has_data)
{
    ULONG setupflags = 0;

    /*
    * A Setup Stage TD consists of a single Setup Stage TRB.  The Setup Stage
    * TRB does not define ENT/ISP/NS/CH fields, so those bit positions are
    * reserved and must be kept clear.
    *
    * Keep only bits that are valid for Setup Stage TRBs (e.g. BEI),
    * then add TYPE/IDT/TRT.
    */
    if(tdflags & TRBF_FLAG_BEI)
        setupflags |= TRBF_FLAG_BEI;

    setupflags |= (TRBF_FLAG_TRTYPE_SETUP | TRBF_FLAG_IDT);

    /*
     * TRT (Transfer Type):
     *  - No DATA stage: TRT = 0
     *  - DATA OUT stage: TRT = 2
     *  - DATA IN stage:  TRT = 3
     *
     * We derive the DATA direction from TRBF_FLAG_DS_DIR (set for IN).
     */
    setupflags &= ~TRB_FLAG_SETUP_TRT_MASK;

    if(!has_data) {
        setupflags |= TRBF_FLAG_SETUP_TRT_NONE;
    } else if(tdflags & TRBF_FLAG_DS_DIR) {
        setupflags |= TRBF_FLAG_SETUP_TRT_IN;
    } else {
        setupflags |= TRBF_FLAG_SETUP_TRT_OUT;
    }

    return setupflags;
}

static ULONG xhciTDStatusFlags(ULONG tdflags)
{
    ULONG statusflags = 0;

    /* Status stage only needs the direction bit (IN=1, OUT=0) */
    if(tdflags & TRBF_FLAG_DS_DIR)
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
    outdataw[0] = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    outdataw[1] = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    outdataw[2] = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);
#else
    CopyMem(&ioreq->iouh_SetupData, inlinedata, sizeof(UQUAD));
#endif
}

static BOOL isStandardTRBTransfer(struct IOUsbHWReq *ioreq, ULONG txtype)
{
    const UBYTE bm = ioreq->iouh_SetupData.bmRequestType;
    const UBYTE br = ioreq->iouh_SetupData.bRequest;

    if(txtype != UHCMD_CONTROLXFER)
        return TRUE;

    /* Special-case: Standard Device request: SET_ADDRESS */
    if(bm == (URTF_STANDARD | URTF_DEVICE) &&
            br == USR_SET_ADDRESS) {
        return FALSE;
    }

    return TRUE;
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
    pciusbXHCIDebugTRBV("xHCI",
                        "Queueing SETUP TRB: inline=0x%08lx%08lx, len=%u\n",
                        (ULONG)((setupdata_inline >> 32) & 0xffffffffUL),
                        (ULONG)(setupdata_inline        & 0xffffffffUL),
                        (unsigned)sizeof(ioreq->iouh_SetupData));
    /* SETUP stage */
    queued = xhciQueueTRB_IO(hc, epring, setupdata_inline,
                             sizeof(ioreq->iouh_SetupData),
                             sf, &ioreq->iouh_Req);
    pciusbXHCIDebugV("xHCI",
                     "xhciQueueTRB (SETUP) -> queued=%d\n",
                     (int)queued);

    if(queued == -1)
        return FALSE;

    driprivate->dpSTRB = queued;

    /* DATA stage (if any) */
    if(has_data) {
        /*
         * Control transfers use separate TDs:
         *  - Setup Stage TD:   single Setup Stage TRB (no Chain bit)
         *  - Data Stage TD:    Data Stage TRB optionally chained to Normal/Event Data TRBs
         *  - Status Stage TD:  Status Stage TRB (optionally chained to Event Data TRB)
         *
         * Therefore, do not chain the DATA stage to the STATUS stage.
         */
        queued = xhciQueuePayloadTRBs(hc, ioreq, driprivate, epring, trbflags, FALSE);
        pciusbXHCIDebugTRBV("xHCI",
                            "xhciQueuePayloadTRBs (DATA) -> queued=%d\n",
                            (int)queued);
    } else {
        driprivate->dpTxSTRB = driprivate->dpSTRB;
        driprivate->dpTxETRB = (epring->next > 0) ? (epring->next - 1) : (XHCI_EVENT_RING_TRBS - 1);
        queued = driprivate->dpSTRB;
        pciusbXHCIDebugTRBV("xHCI",
                            "No DATA stage, using dpSTRB=%d\n",
                            (int)queued);
    }

    if(queued == -1)
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

        if(has_data) {
            /* Opposite of DATA direction. */
            status_tdflags ^= TRBF_FLAG_DS_DIR;
        } else {
            /* No DATA stage: Status is always IN. */
            status_tdflags |= TRBF_FLAG_DS_DIR;
        }

        pciusbXHCIDebugTRBV("xHCI",
                            "Queueing STATUS TRB\n");
        ULONG stf = xhciTDStatusFlags(status_tdflags);
        queued = xhciQueueTRB_IO(hc, epring, 0, 0,
                                 stf, &ioreq->iouh_Req);
        pciusbXHCIDebugTRBV("xHCI",
                            "xhciQueueTRB (STATUS) -> queued=%d\n",
                            (int)queued);
    }

    if(queued != -1) {
        driprivate->dpSttTRB = queued;
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
        struct Task *thisTask = FindTask(NULL);
        pciusbXHCIDebugV("xHCI",
                         DEBUGCOLOR_SET "Task @ 0x%p, IDnest %d TDNest %d" DEBUGCOLOR_RESET" \n",
                         thisTask, thisTask->tc_IDNestCnt, thisTask->tc_TDNestCnt);
    }
#endif

    /* *** Schedule Transfers *** */
    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "Scheduling new Async transfers (Type %lx)..." DEBUGCOLOR_RESET" \n",
                    txtype);

    ForeachNodeSafe(txlist, ioreq, ionext) {
        pciusbXHCIDebugV("xHCI",
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

        pciusbXHCIDebugV("xHCI",
                         "    Flags=0x%08lx, NakTO=%u, MaxPkt=%u, Interval=%u\n",
                         ioreq->iouh_Flags,
                         ioreq->iouh_NakTimeout,
                         ioreq->iouh_MaxPktSize,
                         ioreq->iouh_Interval);

        if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER) {
            pciusbXHCIDebugV("xHCI",
                             "    SETUP: bmReqType=0x%02x bReq=0x%02x wValue=0x%04x wIndex=0x%04x wLength=%u\n",
                             ioreq->iouh_SetupData.bmRequestType,
                             ioreq->iouh_SetupData.bRequest,
                             (unsigned)AROS_LE2WORD(ioreq->iouh_SetupData.wValue),
                             (unsigned)AROS_LE2WORD(ioreq->iouh_SetupData.wIndex),
                             (unsigned)AROS_LE2WORD(ioreq->iouh_SetupData.wLength));
        }

        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("xHCI",
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

        if(!driprivate || !driprivate->dpDevice) {
            pciusbError("xHCI",
                        DEBUGWARNCOLOR_SET "xHCI: Missing prepared transfer context for Dev=%u EP=%u" DEBUGCOLOR_RESET" \n",
                        ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            continue;
        }

        devCtx = driprivate->dpDevice;

        if(isStandardTRBTransfer(ioreq, txtype)) {
            pciusbXHCIDebugV("xHCI",
                             "Normal transfer path: txtype=%lx, DevAddr=%u, EP=%u\n",
                             txtype, ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);

            trbflags |= xhciBuildDataTRBFlags(ioreq, txtype);

            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
            pciusbXHCIDebugV("xHCI",
                             DEBUGCOLOR_SET "Frame=%u: Nak timeout for DevEP=%02lx set to %u" DEBUGCOLOR_RESET" \n",
                             hc->hc_FrameCounter, devadrep,
                             unit->hu_NakTimeoutFrame[devadrep]);

            if(txtype != UHCMD_CONTROLXFER) {
                driprivate->dpSTRB = (UWORD)-1;
                driprivate->dpSttTRB = (UWORD)-1;
            }

            if(xhciActivateEndpointTransfer(hc, unit, ioreq, driprivate, devadrep, &epring)) {
                if(txtype == UHCMD_CONTROLXFER) {
                    txdone = xhciQueueControlStages(hc, ioreq, driprivate, epring, trbflags);
                } else {
                    queued = xhciQueuePayloadTRBs(hc, ioreq, driprivate, epring, trbflags, TRUE);
                    if(queued != -1)
                        txdone = TRUE;
                }

                if(txdone) {
                    AddTail(&hc->hc_TDQueue,
                            (struct Node *) ioreq);
                    pciusbXHCIDebugTRB("xHCI",
                                       DEBUGCOLOR_SET "Transaction queued in TRB #%u (Dev=%u EP=%u)" DEBUGCOLOR_RESET" \n",
                                       driprivate->dpTxSTRB,
                                       ioreq->iouh_DevAddr,
                                       ioreq->iouh_Endpoint);
                }
            }

            if(!txdone) {
                pciusbError("xHCI",
                            DEBUGWARNCOLOR_SET "xHCI: txdone=FALSE, failed to submit transaction (Dev=%u EP=%u, txtype=%lx)" DEBUGCOLOR_RESET" \n",
                            ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, txtype);

                driprivate->dpSTRB   = (UWORD)-1;
                driprivate->dpTxSTRB = (UWORD)-1;
                driprivate->dpTxETRB = (UWORD)-1;
                driprivate->dpSttTRB = (UWORD)-1;

                if(ioreq->iouh_Req.io_Error) {
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                } else {
                    Disable();
                    AddHead(txlist, (struct Node *) ioreq);
                    Enable();
                }
            } else {
                pciusbXHCIDebugV("xHCI",
                                 "Ringing doorbell: slot=%u epid=%u\n",
                                 driprivate->dpDevice->dc_SlotID,
                                 driprivate->dpEPID);
                xhciRingDoorbell(hc, driprivate->dpDevice->dc_SlotID, driprivate->dpEPID);
            }
        }

        pciusbXHCIDebugV("xHCI",
                         "---- IOReq done: %p ----\n", ioreq);
    }

    pciusbXHCIDebugV("xHCI", "%s: exit\n", __func__);
}
