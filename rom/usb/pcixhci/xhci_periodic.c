/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver periodic transfer support functions
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

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
        devadrep = xhciDevEPKey(ioreq);
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "New INT transfer to dev=%u ep=%u (DevEP=%02x): len=%lu dir=%s, cmd=%u" DEBUGCOLOR_RESET"\n",
                        ioreq->iouh_DevAddr,
                        ioreq->iouh_Endpoint,
                        devadrep,
                        (unsigned long)ioreq->iouh_Length,
                        (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT",
                        (unsigned)ioreq->iouh_Req.io_Command);
        pciusbXHCIDebugV("xHCI",
                         DEBUGCOLOR_SET "    IOReq=%p Flags=0x%08lx NakTO=%lu MaxPkt=%u Interval=%u" DEBUGCOLOR_RESET"\n",
                         ioreq,
                         (unsigned long)ioreq->iouh_Flags,
                         (unsigned long)ioreq->iouh_NakTimeout,
                         (unsigned)ioreq->iouh_MaxPktSize,
                         (unsigned)ioreq->iouh_Interval);

        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("xHCI", "DevEP %02lx in use!\n", devadrep);
            continue;
        }

        /****** SETUP TRANSACTION ************/
        volatile struct pcisusbXHCIRing *epring = NULL;
        struct pciusbXHCIIODevPrivate *driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        ULONG trbflags = 0;
        WORD queued = -1;

        if(!driprivate || !driprivate->dpDevice) {
            pciusbError("xHCI",
                        DEBUGWARNCOLOR_SET "xHCI: Missing prepared transfer context for Dev=%u EP=%u" DEBUGCOLOR_RESET" \n",
                        ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            continue;
        }

        {
            /* Interrupt TD flags */
            trbflags |= xhciBuildDataTRBFlags(ioreq, ioreq->iouh_Req.io_Command);
            if(ioreq->iouh_Dir == UHDIR_IN) {
                /* For INT endpoints, DS_DIR is not required; only ISP is meaningful here. */
                trbflags |= TRBF_FLAG_ISP;          /* interrupt on short packet */
            }

            /****** SETUP COMPLETE ************/
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            unit->hu_NakTimeoutFrame[devadrep] =
                (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT) : 0;
            pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%u + %u nak timeout set" DEBUGCOLOR_RESET" \n", hc->hc_FrameCounter,
                             (ioreq->iouh_NakTimeout << XHCI_NAKTOSHIFT));

            /****** INSERT TRANSACTION ************/
            BOOL txdone = FALSE;
            driprivate->dpSTRB = (UWORD)-1;
            driprivate->dpSttTRB = (UWORD)-1;

            if(xhciActivateEndpointTransfer(hc, unit, ioreq, driprivate, devadrep, &epring)) {
                queued = xhciQueuePayloadTRBs(hc, ioreq, driprivate, epring, trbflags, TRUE);

                if(queued != -1) {
                    AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);
                    pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Transaction queued in TRB #%u" DEBUGCOLOR_RESET" \n", driprivate->dpTxSTRB);
                    txdone = TRUE;
                }
            }

            /*
             * If we failed to get an endpoint,
             * or queue the transaction, requeue it
             */
            if(!txdone) {
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
