/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved

    Desc:  xHCI chipset driver shared scheduling helper routines
*/

#if defined(PCIUSB_ENABLEXHCI)

#include <proto/exec.h>
#include <proto/poseidon.h>

#include <devices/usb_hub.h>
#include <exec/memory.h>

#include "uhwcmd.h"
#include "xhcichip_schedule.h"

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

UBYTE xhciEndpointIDFromIndex(UWORD wIndex)
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

APTR xhciPrepareDMABuffer(struct PCIController *hc, struct IOUsbHWReq *ioreq,
    ULONG *dmalen, UWORD effdir, APTR *bounceOut)
{
    APTR data = ioreq->iouh_Data;

    if (bounceOut)
        *bounceOut = NULL;

    if (!data || !dmalen || (*dmalen == 0))
        return data;

#if __WORDSIZE == 64
    APTR bounce = NULL;
    if (!(hc->hc_Flags & HCF_ADDR64)) {
        if ((((IPTR)data + *dmalen - 1) >> 32) != 0) {
            bounce = AllocVec(*dmalen, MEMF_31BIT|MEMF_PUBLIC);
            if (!bounce)
                return NULL;

            if (effdir == UHDIR_OUT)
                CopyMem(data, bounce, *dmalen);

            if (bounceOut)
                *bounceOut = bounce;
        }
    }
    data = bounce ? bounce : data;
#endif
    CachePreDMA(data, dmalen,
                (effdir == UHDIR_IN) ? DMAFLAGS_PREREAD : DMAFLAGS_PREWRITE);

    return data;
}

void xhciReleaseDMABuffer(struct PCIController *hc, struct IOUsbHWReq *ioreq,
    ULONG actual, UWORD effdir, APTR bounceBuf)
{
    APTR data = bounceBuf ? bounceBuf : ioreq->iouh_Data;
    ULONG postlen = actual;

    if (data && actual) {
        CachePostDMA(data, &postlen,
                     (effdir == UHDIR_IN) ? DMAFLAGS_POSTREAD : DMAFLAGS_POSTWRITE);
    }

    if (bounceBuf) {
        if ((effdir == UHDIR_IN) && actual)
            CopyMem(bounceBuf, ioreq->iouh_Data, actual);
        FreeVec(bounceBuf);
    }

    (void)hc;
}

static BOOL xhciObtainHWEndpoint(struct PCIController *hc, struct IOUsbHWReq *ioreq,
    struct List *ownerList, ULONG txtype, BOOL allowEp0AutoCreate,
    struct pciusbXHCIDevice **devCtxOut, ULONG *txepOut)
{
    struct pciusbXHCIDevice *devCtx;
    ULONG txep;
    BOOL autoCreated = FALSE;

    (void)ownerList;

    devCtx = xhciFindDeviceCtx(hc, ioreq->iouh_DevAddr);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "Device context for addr %u = 0x%p" DEBUGCOLOR_RESET" \n", ioreq->iouh_DevAddr, devCtx);

    if ((!devCtx) && allowEp0AutoCreate &&
        (ioreq->iouh_DevAddr == 0) && (ioreq->iouh_Endpoint == 0)) {
        devCtx = xhciObtainDeviceCtx(hc, ioreq, TRUE);
        if (devCtx)
            autoCreated = TRUE;
    }

    if (!devCtx) {
        pciusbWarn("xHCI",
            DEBUGWARNCOLOR_SET "xHCI: No device attached for DevAddr=%u, EP=%u" DEBUGCOLOR_RESET" \n",
            ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);
        ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
        return FALSE;
    }

    txep = xhciEndpointID(ioreq->iouh_Endpoint, (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0);

    if ((txep == 0) && allowEp0AutoCreate) {
        if (!devCtx->dc_EPAllocs[0].dmaa_Ptr) {
            ULONG initep = xhciInitEP(hc, devCtx,
                                      ioreq,
                                      0, 0,
                                      txtype,
                                      ioreq->iouh_MaxPktSize,
                                      ioreq->iouh_Interval,
                                      ioreq->iouh_Flags);

            if ((initep == 0) || !devCtx->dc_EPAllocs[0].dmaa_Ptr) {
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;

                pciusbXHCIDebugV("xHCI",
                                "Leaving %s early: failed to initialise EP0\n",
                                __func__);
                return FALSE;
            }
        }
    } else if ((txep >= MAX_DEVENDPOINTS) || !devCtx->dc_EPAllocs[txep].dmaa_Ptr) {
        ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;

        pciusbXHCIDebugV("xHCI",
                        "Leaving %s early: endpoint not prepared (txep=%u)\n",
                        __func__, (unsigned)txep);
        return FALSE;
    }

    if (autoCreated) {
        pciusbWarn("xHCI",
            DEBUGCOLOR_SET "xHCI: Auto-created DevAddr0/EP0 context for pending transfer" DEBUGCOLOR_RESET" \n");
    }

    *devCtxOut = devCtx;
    *txepOut   = txep;
    return TRUE;
}

BOOL xhciInitIOTRBTransfer(struct PCIController *hc, struct IOUsbHWReq *ioreq,
    struct List *ownerList, ULONG txtype, BOOL allowEp0AutoCreate,
    struct pciusbXHCIIODevPrivate **outPrivate)
{
    struct pciusbXHCIDevice *devCtx = NULL;
    ULONG txep = 0;
    struct pciusbXHCIIODevPrivate *driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;

    if (!driprivate) {
        if (!xhciObtainHWEndpoint(hc, ioreq, ownerList, txtype, allowEp0AutoCreate, &devCtx, &txep))
            return FALSE;

        driprivate = AllocMem(sizeof(struct pciusbXHCIIODevPrivate), MEMF_ANY|MEMF_CLEAR);
        if (!driprivate) {
            pciusbError("xHCI",
                DEBUGWARNCOLOR_SET "%s: Failed to allocate IO Driver Data!" DEBUGCOLOR_RESET" \n",
                __func__);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            return FALSE;
        }

        driprivate->dpDevice = devCtx;
        driprivate->dpEPID   = txep;
    } else {
        if (!xhciObtainHWEndpoint(hc, ioreq, ownerList, txtype, allowEp0AutoCreate, &devCtx, &txep))
            return FALSE;

        driprivate->dpDevice = devCtx;
        driprivate->dpEPID   = txep;
        driprivate->dpBounceBuf = NULL;
        driprivate->dpBounceData = NULL;
        driprivate->dpBounceLen = 0;
        driprivate->dpBounceDir = 0;
        pciusbXHCIDebugV("xHCI",
                        "Reusing existing driver private=%p, devCtx=%p, refreshed EPID=%u\n",
                        driprivate, devCtx, driprivate->dpEPID);
    }

#if defined(DEBUG) && (DEBUG > 1)
    if (driprivate && driprivate->dpDevice)
        xhciDumpEndpointCtx(hc, driprivate->dpDevice, driprivate->dpEPID, "shared schedule");
#endif

    /* Mark IO as prepared for scheduling; activation/busy tracking is done later
     * in xhciActivateEndpointTransfer().
     */
    ioreq->iouh_DriverPrivate1 = driprivate;
    *outPrivate = driprivate;
    return TRUE;
}

ULONG xhciBuildDataTRBFlags(const struct IOUsbHWReq *ioreq, ULONG txtype)
{
    ULONG trbflags = 0;

    if (txtype == UHCMD_CONTROLXFER) {
        UWORD wLength = AROS_LE2WORD(ioreq->iouh_SetupData.wLength);
        BOOL  setup_in = (ioreq->iouh_SetupData.bmRequestType & 0x80) != 0;

        if (wLength != 0) {
            if (setup_in)
                trbflags |= TRBF_FLAG_DS_DIR;

            trbflags |= TRBF_FLAG_TRTYPE_DATA;
        } else {
            trbflags |= TRBF_FLAG_TRTYPE_NORMAL;
        }
    } else {
        trbflags |= TRBF_FLAG_TRTYPE_NORMAL;
    }

    return trbflags;
}

BOOL xhciActivateEndpointTransfer(struct PCIController *hc, struct PCIUnit *unit,
    struct IOUsbHWReq *ioreq, struct pciusbXHCIIODevPrivate *driprivate,
    UWORD devadrep, volatile struct pcisusbXHCIRing **epringOut)
{
    BOOL ok = FALSE;

    Disable();
    if ((ioreq->iouh_DriverPrivate1 = driprivate) != NULL) {
        unit->hu_DevBusyReq[devadrep] = ioreq;

        if (driprivate->dpDevice) {
            *epringOut = driprivate->dpDevice->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr;

            if (*epringOut)
                ok = TRUE;
        }
    }
    Enable();

    if (!ok) {
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
        ReplyMsg(&ioreq->iouh_Req.io_Message);
        return FALSE;
    }

    pciusbXHCIDebugEPV("xHCI",
                      DEBUGCOLOR_SET "%s: EP ring @ 0x%p (EPID=%u)" DEBUGCOLOR_RESET" \n",
                      __func__, *epringOut, driprivate->dpEPID);

    return TRUE;
}

WORD xhciQueuePayloadTRBs(struct PCIController *hc, struct IOUsbHWReq *ioreq,
    struct pciusbXHCIIODevPrivate *driprivate, volatile struct pcisusbXHCIRing *epring,
    ULONG trbflags, BOOL iocOnLast)
{
    ULONG dmalen = ioreq->iouh_Length;
    WORD queued;

    UWORD effdir = xhciEffectiveDataDir(ioreq);
    APTR bounce = NULL;
    APTR dmaptr = xhciPrepareDMABuffer(hc, ioreq, &dmalen, effdir, &bounce);
    if (!dmaptr && dmalen) {
        ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
        return -1;
    }

    driprivate->dpBounceBuf = bounce;
    driprivate->dpBounceData = bounce ? ioreq->iouh_Data : NULL;
    driprivate->dpBounceLen = dmalen;
    driprivate->dpBounceDir = effdir;
    queued = xhciQueueData(hc, epring,
                           (UQUAD)(IPTR)dmaptr,
                           dmalen,
                           ioreq->iouh_MaxPktSize,
                           trbflags,
                           iocOnLast);
    if (queued != -1) {
        int cnt;

        driprivate->dpTxSTRB = queued;
        driprivate->dpTxETRB = (epring->next > 0) ? (epring->next - 1) : (XHCI_EVENT_RING_TRBS - 1);

        xhciRingLock();
        if (driprivate->dpTxETRB >= driprivate->dpTxSTRB) {
            for (cnt = driprivate->dpTxSTRB; cnt < (driprivate->dpTxETRB + 1); cnt++)
                epring->ringio[cnt] = &ioreq->iouh_Req;
        } else {
            for (cnt = driprivate->dpTxSTRB; cnt < (XHCI_EVENT_RING_TRBS - 1); cnt++)
                epring->ringio[cnt] = &ioreq->iouh_Req;
            for (cnt = 0; cnt < (driprivate->dpTxETRB + 1); cnt++)
                epring->ringio[cnt] = &ioreq->iouh_Req;
        }
        xhciRingUnlock();
    } else if (driprivate->dpBounceBuf) {
        xhciReleaseDMABuffer(hc, ioreq, 0, effdir, driprivate->dpBounceBuf);
        driprivate->dpBounceBuf = NULL;
        driprivate->dpBounceData = NULL;
        driprivate->dpBounceLen = 0;
        driprivate->dpBounceDir = 0;
    }

    return queued;
}

#endif /* PCIUSB_ENABLEXHCI */
