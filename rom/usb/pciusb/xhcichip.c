/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver main pciusb interface
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <exec/errors.h>
#include <hidd/pci.h>

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

#define XHCI_ROOT_HUB_HANDLE ((struct pciusbXHCIDevice *)-1)

#if defined(PCIUSB_XHCI_DEBUG) && defined(XHCI_ENABLE_CMDFAIL_DEBUG)
#define XHCI_CMDFAIL_LOGGING 1
#endif

static void xhciFreeEndpointContext(struct PCIController *hc,
                                    struct pciusbXHCIDevice *devCtx,
                                    ULONG epid,
                                    BOOL stopEndpoint);

static BOOL xhciDeviceHasEndpoints(const struct pciusbXHCIDevice *devCtx)
{
    if (!devCtx)
        return FALSE;

    for (ULONG epid = 0; epid < MAX_DEVENDPOINTS; epid++) {
        if (devCtx->dc_EPAllocs[epid].dmaa_Ptr || devCtx->dc_EPContexts[epid])
            return TRUE;
    }

    return FALSE;
}

static struct PCIController *xhciGetController(struct PCIUnit *unit)
{
    struct PCIController *hc;

    ForeachNode(&unit->hu_Controllers, hc) {
        if (hc->hc_HCIType == HCITYPE_XHCI)
            return hc;
    }

    return NULL;
}

// See page 395 "EWE" - to enable rollover events.
void xhciUpdateFrameCounter(struct PCIController *hc)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_rrs *rrs = (volatile struct xhci_rrs *)((IPTR)xhcic->xhc_XHCIIntR - 0x20);
    UWORD framecnt;

    Disable();
    framecnt = AROS_LE2LONG(rrs->mfindex) & 0x3FFF;
    if(framecnt < (hc->hc_FrameCounter & 0x3FFF)) {
        hc->hc_FrameCounter |= 0x3FFF;
        hc->hc_FrameCounter++;
        hc->hc_FrameCounter |= framecnt;
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Frame Counter Rollover %ld" DEBUGCOLOR_RESET "\n", hc->hc_FrameCounter);
    } else {
        hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xFFFFC000)|framecnt;
    }
    Enable();
}

static UBYTE xhciGetEPID(UBYTE endpoint, UBYTE dir)
{
    UBYTE epid = 1;

    if (endpoint > 0) {
        epid = (endpoint & 0X0F) * 2;
        epid += dir;
    }
    return epid;
}

static const char *xhciCmdTypeName(ULONG trbtype)
{
    switch (trbtype) {
    case TRBB_FLAG_CRTYPE_ENABLE_SLOT:
        return "Enable Slot";
    case TRBB_FLAG_CRTYPE_DISABLE_SLOT:
        return "Disable Slot";
    case TRBB_FLAG_CRTYPE_ADDRESS_DEVICE:
        return "Address Device";
    case TRBB_FLAG_CRTYPE_CONFIGURE_ENDPOINT:
        return "Configure Endpoint";
    case TRBB_FLAG_CRTYPE_EVALUATE_CONTEXT:
        return "Evaluate Context";
    case TRBB_FLAG_CRTYPE_RESET_ENDPOINT:
        return "Reset Endpoint";
    case TRBB_FLAG_CRTYPE_STOP_ENDPOINT:
        return "Stop Endpoint";
    case TRBB_FLAG_CRTYPE_SET_TR_DEQUEUE_PTR:
        return "Set TR Dequeue";
    case TRBB_FLAG_CRTYPE_RESET_DEVICE:
        return "Reset Device";
    case TRBB_FLAG_CRTYPE_FORCE_EVENT:
        return "Force Event";
    case TRBB_FLAG_CRTYPE_NEGOTIATE_BANDWIDTH:
        return "Negotiate Bandwidth";
    case TRBB_FLAG_CRTYPE_SET_LATENCY_TOLERANCE:
        return "Set Latency Tolerance";
    case TRBB_FLAG_CRTYPE_GET_PORT_BANDWIDTH:
        return "Get Port Bandwidth";
    case TRBB_FLAG_CRTYPE_FORCE_HEADER:
        return "Force Header";
    case TRBB_FLAG_CRTYPE_NOOP:
        return "Command No-Op";
    default:
        return "Unknown";
    }
}

static UBYTE xhciCalcInterval(UWORD interval, ULONG flags, ULONG type)
{
    BOOL superspeed = (flags & UHFF_SUPERSPEED) != 0;
    BOOL highspeed  = (flags & UHFF_HIGHSPEED)  != 0;

    if ((type != UHCMD_INTXFER) && (type != UHCMD_ISOXFER))
        return 0;

    if (interval == 0)
        return 0;

    /*
     * For SuperSpeed/HighSpeed endpoints the interval is an exponent in
     * microframes. For full/low-speed interrupt endpoints the field uses the
     * frame-count value directly.
     */
    if (superspeed || highspeed) {
        UWORD microframes = interval;
        UBYTE exp = 0;

        while (((1U << exp) < microframes) && (exp < 10))
            exp++;

        /* Interval exponents below 3 mean "every microframe". */
        if (exp < 3)
            exp = 3;

        return exp;
    }

    if (interval > 255)
        interval = 255;

    return (UBYTE)interval;
}

static void xhciInitRing(struct pcisusbXHCIRing *ring)
{
#if defined(DEBUG)
    /*
     * The interrupt handler uses RINGFROMTRB() (masking) to map a TRB pointer
     * back to the containing ring. That requires ring allocations to be
     * aligned to XHCI_RING_ALIGN (see xhcichip.h).
     */
    if (((IPTR)ring & (XHCI_RING_ALIGN - 1)) != 0) {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: ring %p is misaligned (need %lu)" DEBUGCOLOR_RESET "\n",
                    ring, (ULONG)XHCI_RING_ALIGN);
    }
#endif
    memset(ring, 0, sizeof(*ring));
    ring->end = RINGENDCFLAG; /* set initial cycle bit */
}

static void xhciPowerOnRootPorts(struct PCIController *hc, struct PCIUnit *hu)
{
    if (!(hc->hc_Flags & HCF_PPC))
        return;

    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);

    for (UWORD hciport = 0; hciport < hc->hc_NumPorts; hciport++) {
        ULONG origportsc = AROS_LE2LONG(xhciports[hciport].portsc);
        ULONG newportsc  = origportsc & ~(XHCIF_PR_PORTSC_OCC | XHCIF_PR_PORTSC_PRC |
                                         XHCIF_PR_PORTSC_WRC | XHCIF_PR_PORTSC_PEC |
                                         XHCIF_PR_PORTSC_CSC | XHCIF_PR_PORTSC_PLC |
                                         XHCIF_PR_PORTSC_CEC);

        /* Skip ports that are already powered */
        if (origportsc & XHCIF_PR_PORTSC_PP)
            continue;

        newportsc |= XHCIF_PR_PORTSC_PP;
        xhciports[hciport].portsc = AROS_LONG2LE(newportsc);

        /* Wait for power to latch before continuing bring-up */
        for (ULONG waitms = 0; waitms < 20; waitms++) {
            if (AROS_LE2LONG(xhciports[hciport].portsc) & XHCIF_PR_PORTSC_PP)
                break;
            uhwDelayMS(1, hu);
        }

        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "Port %u powered: %08lx -> %08lx" DEBUGCOLOR_RESET" \n",
                        hciport + 1,
                        origportsc,
                        AROS_LE2LONG(xhciports[hciport].portsc));
    }
}

struct pciusbXHCIDevice *xhciFindDeviceCtx(struct PCIController *hc, UWORD devaddr)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    struct pciusbXHCIDevice *unassigned = NULL;
    UWORD maxslot = xhcic->xhc_NumSlots;

    if (maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for (UWORD slot = 1; slot <= maxslot; slot++) {
        struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slot];

        if (!devCtx)
            continue;

        if (devCtx->dc_DevAddr == devaddr)
            return devCtx;

        if ((devaddr != 0) && (devCtx->dc_DevAddr == 0))
            unassigned = devCtx;
    }

    if (unassigned) {
        unassigned->dc_DevAddr = (UBYTE)devaddr;
        return unassigned;
    }

    return NULL;
}

struct pciusbXHCIDevice *xhciFindRouteDevice(struct PCIController *hc,
                                                    ULONG route,
                                                    UWORD rootPortIndex)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    UWORD maxslot = xhcic->xhc_NumSlots;

    if (maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for (UWORD slot = 1; slot <= maxslot; slot++) {
        struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slot];

        if (!devCtx)
            continue;

        if ((devCtx->dc_RouteString == (route & SLOT_CTX_ROUTE_MASK)) &&
            (devCtx->dc_RootPort == rootPortIndex))
            return devCtx;
    }

    return NULL;
}

static struct pciusbXHCIDevice *xhciFindPortDevice(struct PCIController *hc, UWORD hciport)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    UWORD maxslot = xhcic->xhc_NumSlots;

    if (maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for (UWORD slot = 1; slot <= maxslot; slot++) {
        struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slot];

        if (devCtx && (devCtx->dc_RootPort == hciport))
            return devCtx;
    }

    return NULL;
}

static BOOL xhciIOReqMatchesDevice(const struct IOUsbHWReq *ioreq,
                                   const struct pciusbXHCIDevice *devCtx)
{
    if (!ioreq || !devCtx)
        return FALSE;

    struct pciusbXHCIIODevPrivate *driprivate =
        (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;

    if (driprivate && driprivate->dpDevice == devCtx)
        return TRUE;

    if (devCtx->dc_DevAddr && (ioreq->iouh_DevAddr == devCtx->dc_DevAddr))
        return TRUE;

    if (ioreq->iouh_RootPort &&
        (devCtx->dc_RootPort == (ioreq->iouh_RootPort - 1)) &&
        ((ioreq->iouh_RouteString & SLOT_CTX_ROUTE_MASK) == devCtx->dc_RouteString))
        return TRUE;

    return FALSE;
}

static void xhciAbortDeviceQueue(struct PCIController *hc,
                                 struct PCIUnit *unit,
                                 struct List *queue,
                                 struct pciusbXHCIDevice *devCtx,
                                 BOOL periodic)
{
    struct IOUsbHWReq *ioreq, *ionext;

    ioreq = (struct IOUsbHWReq *)queue->lh_Head;
    while ((ionext = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ)) {
        if (!xhciIOReqMatchesDevice(ioreq, devCtx)) {
            ioreq = ionext;
            continue;
        }

        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "Aborting pending IOReq %p for dev %u"
                        DEBUGCOLOR_RESET " \n",
                        ioreq,
                        ioreq->iouh_DevAddr);

        ioreq->iouh_Req.io_Error = IOERR_ABORTED;
        ioreq->iouh_Actual = 0;

        if (periodic)
            xhciFreePeriodicContext(hc, unit, ioreq);
        else
            xhciFreeAsyncContext(hc, unit, ioreq);

        ReplyMsg(&ioreq->iouh_Req.io_Message);

        ioreq = ionext;
    }
}

void xhciDisconnectDevice(struct PCIController *hc, struct pciusbXHCIDevice *devCtx)
{
    struct PCIUnit *unit;

    if (!hc || !devCtx)
        return;

    unit = hc->hc_Unit;

    pciusbXHCIDebug("xHCI",
                    DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n",
                    __func__, hc, devCtx);

    xhciAbortDeviceQueue(hc, unit, &hc->hc_CtrlXFerQueue, devCtx, FALSE);
    xhciAbortDeviceQueue(hc, unit, &hc->hc_BulkXFerQueue, devCtx, FALSE);
    xhciAbortDeviceQueue(hc, unit, &hc->hc_IntXFerQueue, devCtx, TRUE);
    xhciAbortDeviceQueue(hc, unit, &hc->hc_IsoXFerQueue, devCtx, TRUE);
    xhciAbortDeviceQueue(hc, unit, &hc->hc_TDQueue, devCtx, FALSE);
    xhciAbortDeviceQueue(hc, unit, &hc->hc_PeriodicTDQueue, devCtx, TRUE);

    if (devCtx->dc_DevAddr < USB_DEV_MAX &&
        unit->hu_DevControllers[devCtx->dc_DevAddr] == hc) {
        unit->hu_DevControllers[devCtx->dc_DevAddr] = NULL;
    }
    if (devCtx->dc_RouteString == 0 &&
        unit->hu_DevControllers[0] == hc) {
        unit->hu_DevControllers[0] = NULL;
    }

    xhciFreeDeviceCtx(hc, devCtx, TRUE);
}

static int xhciRingEntriesFree(volatile struct pcisusbXHCIRing *ring)
{
    /* Caller must hold the ring lock. */
    ULONG last = (ring->end & ~RINGENDCFLAG);
    ULONG idx = ring->next;

    return (last > idx) ? last-idx
           : (XHCI_EVENT_RING_TRBS - 1) - idx + last;
}

static void xhciInsertTRB(struct PCIController *hc,
                          volatile struct pcisusbXHCIRing *ring,
                          UQUAD payload, ULONG trflags, ULONG plen)
{
    volatile struct xhci_trb *dst;
    ULONG trbflags;

    /* Start from caller flags, but never copy the cycle bit verbatim */
    trbflags = (trflags & ~TRBF_FLAG_C);

    /* Apply the current producer cycle state */
    if (ring->end & RINGENDCFLAG)
        trbflags |= TRBF_FLAG_C;

    /* Next available TRB slot */
    dst = &ring->ring[ring->next];

    /*
     * Parameter field handling:
     *  - IDT set: payload is immediate data (8 bytes), not an address.
     *  - IDT clear: payload is a CPU pointer that must become a bus address.
     */
    if (trflags & TRBF_FLAG_IDT) {
        dst->dbp.addr_lo = AROS_LONG2LE((ULONG)(payload & 0xffffffffUL));
        dst->dbp.addr_hi = AROS_LONG2LE((ULONG)((payload >> 32) & 0xffffffffUL));
    } else {
        UQUAD dma_payload = payload;
#if !defined(PCIUSB_NO_CPUTOPCI)
        if (payload != 0) {
            dma_payload = (UQUAD)(IPTR)CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)(IPTR)payload);
        }
#endif
        xhciSetPointer(hc, dst->dbp, dma_payload);
    }

    dst->tparams = AROS_LONG2LE(plen & TRB_TPARAMS_DS_TRBLEN_SMASK);
    dst->flags   = AROS_LONG2LE(trbflags);

    /* Ensure the controller observes the freshly written TRB */
    CacheClearE((APTR)dst, sizeof(*dst), CACRF_ClearD);
}

WORD xhciQueueTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload,
                  ULONG plen, ULONG trbflags)
{
    WORD queued = -1;

    if (trbflags & TRBF_FLAG_IDT) {
        pciusbXHCIDebugTRBV("xHCI", DEBUGFUNCCOLOR_SET "(0x%p, $%02x %02x %02x%02x %02x%02x %02x%02x, %u, $%08lx)" DEBUGCOLOR_RESET" \n", ring,
                       ((UBYTE *)&payload)[0], ((UBYTE *)&payload)[1], ((UBYTE *)&payload)[3], ((UBYTE *)&payload)[2],
                       ((UBYTE *)&payload)[5], ((UBYTE *)&payload)[4], ((UBYTE *)&payload)[7], ((UBYTE *)&payload)[6],
                       plen, trbflags);
    } else
        pciusbXHCIDebugTRBV("xHCI", DEBUGFUNCCOLOR_SET "(0x%p, 0x%p, %u, $%08lx)" DEBUGCOLOR_RESET" \n", ring, payload, plen, trbflags);

    if (!ring) {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "NO RINGSPECIFIED!!" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    xhciRingLock();
    if (xhciRingEntriesFree(ring) > 1) {
        if (ring->next >= XHCI_EVENT_RING_TRBS - 1) {
            UQUAD link_dma = (UQUAD)(IPTR)&ring->ring[0];

            /*
             * If this is the last ring element, insert a link
             * back to the ring start - and update the cycle bit
             */
            xhciInsertTRB(hc, ring,
                          link_dma,
                          TRBF_FLAG_TRTYPE_LINK | TRBF_FLAG_TC,
                          0);
            ring->next = 0;
            if (ring->end & RINGENDCFLAG)
                ring->end &= ~RINGENDCFLAG;
            else
                ring->end |= RINGENDCFLAG;
            pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "Ring Re-Linked!!" DEBUGCOLOR_RESET" \n");
        }

        xhciInsertTRB(hc, ring, payload, trbflags, plen);
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "ring %p <idx %d, %dbytes>" DEBUGCOLOR_RESET" \n", ring, ring->next, plen);
        queued = ring->next++;
    } else {
        pciusbError("xHCI",
                            DEBUGWARNCOLOR_SET "NO SPACE ON RING!! <next = %u, last = %u>" DEBUGCOLOR_RESET" \n",
                            ring->next, (ring->end & ~RINGENDCFLAG));
    }
    xhciRingUnlock();

    return queued;
}

WORD xhciQueueData(struct PCIController *hc,
                   volatile struct pcisusbXHCIRing *ring,
                   UQUAD payload,
                   ULONG plen,
                   ULONG pmax,
                   ULONG trbflags,
                   BOOL  ioconlast)
{
    ULONG remaining = plen;
    WORD  queued, firstqueued = -1;

    /* Caller-selected TRB type (NORMAL vs. DATA stage, etc.). */
    const ULONG base_type = (trbflags & TRB_FLAG_TYPE_MASK);

    /*
     * The caller may supply TRBF_FLAG_CH to indicate that the final TRB
     * generated by this helper must remain chained to a subsequent TRB that
     * is queued by the caller (e.g. Control DATA stage chained into STATUS).
     */
    ULONG base_flags = (trbflags & ~TRB_FLAG_TYPE_MASK);
    const BOOL chain_beyond = (base_flags & TRBF_FLAG_CH) != 0;

    /*
     * CH/IOC are managed locally per segment.  Do not inherit IOC directly
     * from the caller; use ioconlast for that policy.
     */
    base_flags &= ~(TRBF_FLAG_CH | TRBF_FLAG_IOC);

    /*
     * xHCI limits the per-TRB transfer length field to 17 bits.  Do not split
     * by MaxPacketSize; xHCI TRBs are not packet-sized.
     */
    const ULONG segmax = TRB_TPARAMS_DS_TRBLEN_SMASK;
    (void)pmax;

    /*
     * If this is a Control DATA Stage TRB, only the first segment is a DATA
     * TRB; any additional segments must be NORMAL TRBs chained to it.
     */
    const BOOL is_ctl_data_stage = (base_type == TRBF_FLAG_TRTYPE_DATA);

    /* Zero-length payload: queue a single TRB with len=0. */
    if (remaining == 0) {
        ULONG txflags = base_flags | base_type;

        if (chain_beyond)
            txflags |= TRBF_FLAG_CH;

        if (ioconlast)
            txflags |= TRBF_FLAG_IOC;

        return xhciQueueTRB(hc, ring, payload, 0, txflags);
    }

    while (remaining > 0) {
        const ULONG offset = (plen - remaining);
        ULONG trblen = remaining;
        ULONG txflags;

        if (trblen > segmax)
            trblen = segmax;

        /* Select TRB type for this segment. */
        if (is_ctl_data_stage && offset != 0) {
            /*
             * Subsequent segments of a Control DATA stage use NORMAL TRBs.
             * Clear the DATA-stage-specific direction bit if present.
             */
            txflags = (base_flags & ~TRBF_FLAG_DS_DIR) | TRBF_FLAG_TRTYPE_NORMAL;
        } else {
            txflags = base_flags | base_type;
        }

        /* Chain all but the last segment; optionally chain the final segment too. */
        if (remaining > trblen)
            txflags |= TRBF_FLAG_CH;
        else if (chain_beyond)
            txflags |= TRBF_FLAG_CH;

        /* IOC only on the last segment if requested. */
        if (ioconlast && (remaining == trblen))
            txflags |= TRBF_FLAG_IOC;

        queued = xhciQueueTRB(hc, ring, payload + offset, trblen, txflags);
        if (queued == -1)
            return queued;

        if (firstqueued == -1)
            firstqueued = queued;

        remaining -= trblen;
    }

    return firstqueued;
}

/*
 * Page Size Register (PAGESIZE) - Chapter 5.4.3
 */
static ULONG xhciPageSize(struct PCIController *hc)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)xhcic->xhc_XHCIOpR);
    ULONG pagesize = AROS_LE2LONG(hcopr->pagesize);
    int bit;

    for (bit = 0; bit < 16; bit++) {
        if (pagesize & (1UL << bit))
            return 1UL << (12 + bit);
    }
    return 0;
}

struct pciusbXHCIDevice *
xhciCreateDeviceCtx(struct PCIController *hc,
                    UWORD rootPortIndex,   /* 0-based */
                    ULONG route,           /* 20-bit route string (0 for root) */
                    ULONG flags,           /* UHFF_* speed / hub flags */
                    UWORD mps0)            /* initial EP0 max packet size */
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    struct pciusbXHCIDevice *devCtx;
    UWORD ctxoff = 1;
    volatile struct xhci_ep *iep0;
    ULONG ctx_unit;
    ULONG devctx_size;
    ULONG inctx_size;
    LONG  slotid;

    pciusbXHCIDebug("xHCI",
                    "%s(0x%p, %04x, %08x, %08x, %d)\n",
                    __func__,
                    hc, rootPortIndex, route, flags, mps0);

    devCtx = AllocMem(sizeof(struct pciusbXHCIDevice), MEMF_ANY | MEMF_CLEAR);
    if (!devCtx)
        return NULL;

    devCtx->dc_RootPort    = rootPortIndex;
    devCtx->dc_RouteString = route & SLOT_CTX_ROUTE_MASK;
    devCtx->dc_DevAddr     = 0;   /* default address until SET_ADDRESS */

    /*
     * One "context unit" is 32 bytes when CSZ=0, 64 bytes when CSZ=1.
     * struct xhci_inctx is declared as 32 bytes; we scale it if CTX64 is set.
     */
    ctx_unit = sizeof(struct xhci_inctx);
    if (hc->hc_Flags & HCF_CTX64) {
        ctx_unit <<= 1;
        ctxoff <<= 1;
    }

    /* Device Context: Slot + MAX_DEVENDPOINTS Endpoint Contexts */
    devctx_size = ctx_unit * (MAX_DEVENDPOINTS + 1);

    /* Input Context: Input Control + Slot + MAX_DEVENDPOINTS Endpoint Contexts */
    inctx_size  = ctx_unit * (MAX_DEVENDPOINTS + 2);

    /* ---- Allocate slot (device) context ---- */
    devCtx->dc_SlotCtx.dmaa_Ptr = pciAllocAligned(
        hc, &devCtx->dc_SlotCtx.dmaa_Entry,
        devctx_size, ALIGN_CTX, xhciPageSize(hc));

    if (!devCtx->dc_SlotCtx.dmaa_Ptr) {
        pciusbError("xHCI",
          DEBUGWARNCOLOR_SET "Unable to allocate Device Slot Ctx" DEBUGCOLOR_RESET "\n");
        FreeMem(devCtx, sizeof(*devCtx));
        return NULL;
    }

    memset(devCtx->dc_SlotCtx.dmaa_Ptr, 0, devctx_size);
#if !defined(PCIUSB_NO_CPUTOPCI)
    devCtx->dc_SlotCtx.dmaa_DMA =
        CPUTOPCI(hc, hc->hc_PCIDriverObject, devCtx->dc_SlotCtx.dmaa_Ptr);
#else
    devCtx->dc_SlotCtx.dmaa_DMA = devCtx->dc_SlotCtx.dmaa_Ptr;
#endif

    /* ---- Allocate input context ---- */
    devCtx->dc_IN.dmaa_Ptr = pciAllocAligned(
        hc, &devCtx->dc_IN.dmaa_Entry,
        inctx_size, ALIGN_CTX, xhciPageSize(hc));

    if (!devCtx->dc_IN.dmaa_Ptr) {
        pciusbError("xHCI",
          DEBUGWARNCOLOR_SET "Unable to allocate Device IN Ctx" DEBUGCOLOR_RESET "\n");
        /* TODO: free devCtx->dc_SlotCtx DMA properly */
        FreeMem(devCtx, sizeof(*devCtx));
        return NULL;
    }

    memset(devCtx->dc_IN.dmaa_Ptr, 0, inctx_size);
#if !defined(PCIUSB_NO_CPUTOPCI)
    devCtx->dc_IN.dmaa_DMA =
        CPUTOPCI(hc, hc->hc_PCIDriverObject, devCtx->dc_IN.dmaa_Ptr);
#else
    devCtx->dc_IN.dmaa_DMA = devCtx->dc_IN.dmaa_Ptr;
#endif

    struct xhci_inctx *inctx = (struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
    volatile struct xhci_slot *islot = xhciInputSlotCtx(inctx, ctxoff);

    /* ---- Fill Slot Context (input) ---- */

    /* Root port (1-based) */
    islot->ctx[1] &= ~(0xFFUL << 16);
    islot->ctx[1] |= (((rootPortIndex + 1) & 0xFF) << 16);

    /* Route string (20 bits) */
    islot->ctx[0] &= ~SLOT_CTX_ROUTE_MASK;
    islot->ctx[0] |= (route & SLOT_CTX_ROUTE_MASK);

    /* Hub? */
    if (flags & UHFF_HUB)
        islot->ctx[0] |= (1UL << 26);

    /* Speed bits + MTT bit */
    islot->ctx[0] &= ~(0xFUL << SLOTS_CTX_SPEED);
    islot->ctx[0] &= ~SLOTF_CTX_MTT;

    if (flags & UHFF_SUPERSPEED)
        islot->ctx[0] |= SLOTF_CTX_SUPERSPEED;
    else if (flags & UHFF_HIGHSPEED)
        islot->ctx[0] |= SLOTF_CTX_HIGHSPEED;
    else if (flags & UHFF_LOWSPEED)
        islot->ctx[0] |= SLOTF_CTX_LOWSPEED;
    else
        islot->ctx[0] |= SLOTF_CTX_FULLSPEED;

    /* Multi-TT hub? */
    if (flags & UHFF_TT_MULTI)
        islot->ctx[0] |= SLOTF_CTX_MTT;

    /* simple per-device max transfer tuning based on speed. */
    if (flags & UHFF_SUPERSPEED) {
        devCtx->dc_TxMax = 1024 * 16;   /* generous default for SS */
    } else if (flags & UHFF_HIGHSPEED) {
        devCtx->dc_TxMax = 512 * 13;    /* HS worst-case per microframe */
    } else if (flags & UHFF_LOWSPEED) {
        devCtx->dc_TxMax = 8 * 1;       /* LS: very small payloads */
    } else {
        devCtx->dc_TxMax = 64 * 19;     /* FS: conservative default */
    }

    /* ---- Enable Slot ---- */
    slotid = xhciCmdSlotEnable(hc);
    if (slotid < 0) {
        pciusbError("xHCI",
          DEBUGWARNCOLOR_SET "Failed to enable slot" DEBUGCOLOR_RESET "\n");
        /* TODO: free devCtx->dc_SlotCtx / dc_IN DMA properly */
        FreeMem(devCtx, sizeof(*devCtx));
        return NULL;
    }

    devCtx->dc_SlotID = slotid;

    volatile struct xhci_address *deviceslots =
        (volatile struct xhci_address *)xhcic->xhc_DCBAAp;

    xhciSetPointer(hc, deviceslots[slotid], devCtx->dc_SlotCtx.dmaa_DMA);

    pciusbXHCIDebug("xHCI",
        "DCBAA[%lu] = 0x%p (DMA), devCtx slot ctx dmaa_Ptr = 0x%p\n",
        slotid,
        (void *)(IPTR)devCtx->dc_SlotCtx.dmaa_DMA,
        devCtx->dc_SlotCtx.dmaa_Ptr);

    /*
     * Build EP0 in the input context.
     * xhciInitEP() will set Add Context Flags (slot + EP0) as needed.
     */
    (void)xhciInitEP(hc, devCtx,
                     NULL,
                     0, 0,
                     UHCMD_CONTROLXFER,
                     mps0,
                     0,
                     flags);

    /*
     * Address Device requires the input EP0 EPSTATE to be Disabled (0) and
     * the Input Control Context add flags to include Slot+EP0. Be explicit
     * here to avoid stale/incorrect state leading to CC=19 on some controllers.
     */
    iep0 = xhciInputEndpointCtx(inctx, ctxoff, 1);
    {
        ULONG edw0 = AROS_LE2LONG(iep0->ctx[0]);
        edw0 &= ~0x7UL; /* EPSTATE bits [2:0] -> Disabled */
        iep0->ctx[0] = AROS_LONG2LE(edw0);
    }
    inctx->dcf = 0;
    inctx->acf = 0;
    inctx->acf |= 0x01;       /* Slot context */
    inctx->acf |= (1UL << 1); /* EP0 context */
    CacheClearE((APTR)devCtx->dc_IN.dmaa_Ptr, inctx_size, CACRF_ClearD);

    /* ---- Address Device ---- */
    if (TRB_CC_SUCCESS != xhciCmdDeviceAddress(hc, slotid, devCtx->dc_IN.dmaa_Ptr, 1, NULL)) {
        pciusbError("xHCI",
          DEBUGWARNCOLOR_SET "Address Device (BSR=1) failed" DEBUGCOLOR_RESET "\n");

        xhciCmdSlotDisable(hc, slotid);
        xhciSetPointer(hc, deviceslots[slotid], 0);

        devCtx->dc_SlotID = 0;
        /* TODO free contexts */
        FreeMem(devCtx, sizeof(*devCtx));
        return NULL;
    }

    /* Copy the updated output contexts to the input context for future commands.
     *
     * NOTE: ctxoff is expressed in 32-byte "inctx units" (see xhciInputSlotCtx /
     * xhciInputEndpointCtx). Do not multiply ctxoff by ctxsize when CSZ=1, otherwise
     * you will land 64 bytes too far and corrupt the context image.
     */
    UWORD ctxsize = (hc->hc_Flags & HCF_CTX64) ? 64 : 32;
    CacheClearE(devCtx->dc_SlotCtx.dmaa_Ptr,  ctxsize * 2, CACRF_InvalidateD);

    /* Slot context starts at inctx[ctxoff * 1] (offset by the header). */
    CopyMem((const void *)devCtx->dc_SlotCtx.dmaa_Ptr, (void *)(APTR)islot, ctxsize);
    {
        /* EP0 output context is immediately after the slot context in the device context. */
        volatile struct xhci_ep *iep0 = xhciInputEndpointCtx(inctx, ctxoff, 1);
        volatile struct xhci_ep *oep0 = (volatile struct xhci_ep *)((UBYTE *)devCtx->dc_SlotCtx.dmaa_Ptr + ctxsize);
        CopyMem((const void *)(APTR)oep0, (void *)(APTR)iep0, ctxsize);

        /* Address Device (BSR=0) requires EP0 state = Disabled in the input context. */
        ULONG edw0 = AROS_LE2LONG(iep0->ctx[0]);
        edw0 &= ~0x7UL; /* EPSTATE is bits [2:0] */
        iep0->ctx[0] = AROS_LONG2LE(edw0);
    }

    /* Flush the updated input context back to memory */
    CacheClearE((APTR)devCtx->dc_IN.dmaa_Ptr, inctx_size, CACRF_ClearD);

    xhciDumpEndpointCtx(hc, devCtx, xhciGetEPID(0, 0), "post-address");

    /* Register in slot table */
    if (slotid > 0 && slotid < USB_DEV_MAX)
        xhcic->xhc_Devices[slotid] = devCtx;

    return devCtx;
}

/*
 * Derive speed flags and a safe EP0 MPS from the current PORTSC state.
 *
 * This is required for the "auto-create DevAddr0" path, because the first
 * control transfer can arrive before Poseidon has populated ioreq->iouh_Flags
 * / ioreq->iouh_MaxPktSize. Creating the slot context with a wrong speed is
 * sufficient to trigger cc=4 (USB Transaction Error) on the very first
 * GET_DESCRIPTOR(8).
 */
static BOOL xhciDerivePortFlagsAndMps0(struct PCIController *hc,
                                      UWORD *rootPortIndex,
                                      ULONG *flags,
                                      UWORD *mps0,
                                      BOOL forceScan)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    if (!xhcic)
        return FALSE;

    volatile struct xhci_pr *xhciports =
        (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);

    UWORD port = *rootPortIndex;

    /* If caller didn't know the root port, try to find a connected one. */
    if (forceScan || (port >= hc->hc_NumPorts)) {
        UWORD found = 0xFFFF;

        /* Prefer a connected+enabled port. */
        for (UWORD i = 0; i < hc->hc_NumPorts; i++) {
            ULONG portsc = AROS_LE2LONG(xhciports[i].portsc);
            if (xhciHubPortConnected(portsc) && xhciHubPortEnabled(hc, i, portsc)) {
                found = i;
                break;
            }
        }

        /* Fallback: any connected port. */
        if (found == 0xFFFF) {
            for (UWORD i = 0; i < hc->hc_NumPorts; i++) {
                ULONG portsc = AROS_LE2LONG(xhciports[i].portsc);
                if (xhciHubPortConnected(portsc)) {
                    found = i;
                    break;
                }
            }
        }

        if (found == 0xFFFF)
            return FALSE;

        port = found;
        *rootPortIndex = port;
    }

    ULONG portsc = AROS_LE2LONG(xhciports[port].portsc);
    if (!xhciHubPortConnected(portsc))
        return FALSE;

    /* Clear any existing speed flags and set from PORTSC. */
    *flags &= ~(UHFF_SUPERSPEED | UHFF_HIGHSPEED | UHFF_LOWSPEED);

    ULONG speedBits = portsc & XHCI_PR_PORTSC_SPEED_MASK;
    if (speedBits == XHCIF_PR_PORTSC_LOWSPEED) {
        *flags |= UHFF_LOWSPEED;
        if (*mps0 == 0) *mps0 = 8;
    } else if (speedBits == XHCIF_PR_PORTSC_HIGHSPEED) {
        *flags |= UHFF_HIGHSPEED;
        if (*mps0 == 0) *mps0 = 64;
    } else if (speedBits == XHCIF_PR_PORTSC_SUPERSPEED) {
        *flags |= UHFF_SUPERSPEED;
        if (*mps0 == 0) *mps0 = 512;
    } else {
        /* FS or unknown: start conservatively until bMaxPacketSize0 is known. */
        if (*mps0 == 0) *mps0 = 8;
    }

    return TRUE;
}

struct pciusbXHCIDevice *
xhciObtainDeviceCtx(struct PCIController *hc,
                    struct IOUsbHWReq *ioreq,
                    BOOL allowCreate)
{
    struct PCIUnit *unit   = (struct PCIUnit *)ioreq->iouh_Req.io_Unit;
    UWORD devaddr       = ioreq->iouh_DevAddr;
    ULONG route         = ioreq->iouh_RouteString & SLOT_CTX_ROUTE_MASK;
    UWORD rootPortIndex = (ioreq->iouh_RootPort > 0)
                            ? (ioreq->iouh_RootPort - 1)
                            : 0;

    if (xhciIsRootHubRequest(ioreq, unit))
        return XHCI_ROOT_HUB_HANDLE;

    struct pciusbXHCIDevice *devCtx;

    /* Try existing mappings first */
    devCtx = xhciFindDeviceCtx(hc, devaddr);
    if (devCtx)
        return devCtx;

    devCtx = xhciFindRouteDevice(hc, route, rootPortIndex);
    if (devCtx)
        return devCtx;

    /* Non-zero address but no context = real error */
    if ((devaddr != 0) || !allowCreate)
        return NULL;

    /*
     * Auto-create DevAddr0 context: do not trust ioreq speed flags/MPS here.
     * The first control transfer can arrive with flags==0 (as seen in logs),
     * which would otherwise create a FullSpeed slot context and fail the first
     * GET_DESCRIPTOR with cc=4.
     */
    ULONG flags = ioreq->iouh_Flags;
    UWORD mps0  = ioreq->iouh_MaxPktSize;

    const BOOL forceScan = (ioreq->iouh_RootPort == 0);
    const BOOL haveSpeed = (flags & (UHFF_SUPERSPEED | UHFF_HIGHSPEED | UHFF_LOWSPEED)) != 0;

    if (!haveSpeed || (mps0 == 0) || forceScan) {
        (void)xhciDerivePortFlagsAndMps0(hc, &rootPortIndex, &flags, &mps0, forceScan);
    }
    if (mps0 == 0)
        mps0 = 8;

    return xhciCreateDeviceCtx(hc,
                               rootPortIndex,
                               route,
                               flags,
                               mps0);
}

ULONG xhciInitEP(struct PCIController *hc, struct pciusbXHCIDevice *devCtx,
                 struct IOUsbHWReq *ioreq,
                 UBYTE endpoint,
                 UBYTE dir,
                 ULONG type,
                 ULONG maxpacket,
                 UWORD interval,
                 ULONG flags)
{
    volatile struct pcisusbXHCIRing *epring;
    ULONG epid;

    pciusbXHCIDebugEP("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p, %u, %u, $%08x, %u)" DEBUGCOLOR_RESET" \n",
                      __func__, hc, devCtx, endpoint, dir, type, maxpacket);

    epid = xhciGetEPID(endpoint, dir);

    pciusbXHCIDebugEPV("xHCI", DEBUGCOLOR_SET "%s: EPID %u" DEBUGCOLOR_RESET" \n", __func__, epid);

    /* Test if already prepared */
    if (devCtx->dc_EPAllocs[epid].dmaa_Ptr == NULL) {
        devCtx->dc_EPAllocs[epid].dmaa_Ptr =
            pciAllocAligned(hc, &devCtx->dc_EPAllocs[epid].dmaa_Entry,
                            sizeof(struct pcisusbXHCIRing),
                            XHCI_RING_ALIGN, (1 << 16));
        if (devCtx->dc_EPAllocs[epid].dmaa_Ptr) {
            pciusbXHCIDebugEPV("xHCI",
                              DEBUGCOLOR_SET "Allocated EP Ring @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                              devCtx->dc_EPAllocs[epid].dmaa_Ptr,
                              devCtx->dc_EPAllocs[epid].dmaa_Entry.me_Un.meu_Addr,
                              devCtx->dc_EPAllocs[epid].dmaa_Entry.me_Length);
    #if !defined(PCIUSB_NO_CPUTOPCI)
            devCtx->dc_EPAllocs[epid].dmaa_DMA =
                CPUTOPCI(hc, hc->hc_PCIDriverObject,
                         (APTR)devCtx->dc_EPAllocs[epid].dmaa_Ptr);
    #else
            devCtx->dc_EPAllocs[epid].dmaa_DMA = devCtx->dc_EPAllocs[epid].dmaa_Ptr;
    #endif
            pciusbXHCIDebugEPV("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n",
                              devCtx->dc_EPAllocs[epid].dmaa_DMA);
        } else {
            pciusbError("xHCI",
                        DEBUGWARNCOLOR_SET "Unable to allocate EP Ring Memory" DEBUGCOLOR_RESET" \n");
            return 0;
        }
    }
    epring = (volatile struct pcisusbXHCIRing *)devCtx->dc_EPAllocs[epid].dmaa_Ptr;
    xhciInitRing((struct pcisusbXHCIRing *)epring);

    volatile struct xhci_inctx *in =
        (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;

    UWORD ctxoff = 1;
    if (hc->hc_Flags & HCF_CTX64)
        ctxoff <<= 1;
    UWORD ctxsize = (hc->hc_Flags & HCF_CTX64) ? 64 : 32;

    /* 
     * Invalidate cache before reading output context to ensure we see
     * any controller updates from previous commands.
     */
    CacheClearE(devCtx->dc_SlotCtx.dmaa_Ptr, 
                devCtx->dc_SlotCtx.dmaa_Entry.me_Length, 
                CACRF_InvalidateD);

    /* Clear add/drop flags for fresh configuration */
    in->dcf = 0;
    in->acf = 0;

    struct xhci_slot *islot = (void *)xhciInputSlotCtx(in, ctxoff);
    struct xhci_slot *oslot = (struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;

    /*
     * Avoid overwriting a freshly prepared slot context with an empty output
     * context before the controller has committed any state. Once the host
     * has written the slot context (port is non-zero), mirror it into the
     * next input build to keep the device address and routing in sync.
     */
    if (oslot->ctx[1] & (0xFFUL << 16))
        CopyMem(oslot, islot, ctxsize);

    /* Add/Enable this endpoint and refresh slot context */
    in->acf |= (1 << epid);
    in->acf |= 0x01;

    if (ioreq) {
        ULONG slotctx0 =
            islot->ctx[0] &
            ~((SLOT_CTX_ROUTE_MASK) | (0xF << SLOTS_CTX_SPEED) | SLOTF_CTX_MTT);
        ULONG route = (ULONG)(ioreq->iouh_RouteString & SLOT_CTX_ROUTE_MASK);

        slotctx0 |= route;
        if (flags & UHFF_SUPERSPEED)
            slotctx0 |= SLOTF_CTX_SUPERSPEED;
        else if (flags & UHFF_HIGHSPEED)
            slotctx0 |= SLOTF_CTX_HIGHSPEED;
        else if (flags & UHFF_LOWSPEED)
            slotctx0 |= SLOTF_CTX_LOWSPEED;
        else
            slotctx0 |= SLOTF_CTX_FULLSPEED;

        if (flags & UHFF_TT_MULTI)
            slotctx0 |= SLOTF_CTX_MTT;

        /* Preserve all other slot-ctx[0] bits not related to route/speed/MTT */
        slotctx0 |= (islot->ctx[0] &
                     ~(SLOT_CTX_ROUTE_MASK | (0xF << SLOTS_CTX_SPEED) | SLOTF_CTX_MTT));
        islot->ctx[0] = slotctx0;

        /* Root port number */
        islot->ctx[1] &= ~(0xFF << 16);
        islot->ctx[1] |= ((ULONG)(ioreq->iouh_RootPort & 0xFF) << 16);

        if (flags & UHFF_SPLITTRANS) {
            ULONG ttinfo = 0;
            UWORD ttt = (UWORD)((flags >> UHFS_THINKTIME) & 0x3);

            ttinfo |= ((ULONG)(ioreq->iouh_SplitHubAddr & 0xFF) << SLOT_CTX_TT_SLOT_SHIFT);
            ttinfo |= ((ULONG)(ioreq->iouh_SplitHubPort & 0xFF) << SLOT_CTX_TT_PORT_SHIFT);
            ttinfo |= ((ULONG)ttt << SLOT_CTX_TTT_SHIFT);

            islot->ctx[2] = ttinfo;
        } else {
            islot->ctx[2] = 0;
        }
    }

    /* Update context entries field if we are using a higher EPID */
    if (epid > ((islot->ctx[0] >> 27) & 0xF)) {
        islot->ctx[0] &= ~(0xF << 27);
        islot->ctx[0] |= (epid << 27);
    }

    /* Endpoint context */
    struct xhci_ep *ep =
        (struct xhci_ep *)xhciInputEndpointCtx(in, ctxoff, epid);
    /*
     * Reset the entire endpoint context unit.
     *
     * The hardware endpoint context size follows CSZ (32 or 64 bytes).
     * struct xhci_ep models only the first 32 bytes; when CSZ=1, the upper
     * dwords must also be cleared, otherwise stale garbage can trigger
     * controller-side context validation failures (e.g. CC=19).
     */
    memset((void *)ep, 0, ctxsize);

    pciusbXHCIDebugEPV("xHCI", DEBUGCOLOR_SET "EP input Ctx @ 0x%p" DEBUGCOLOR_RESET" \n", ep);

    ep->ctx[1] &= ~((ULONG)7 << EPS_CTX_TYPE);
    switch (type) {
    case UHCMD_ISOXFER:
        ep->ctx[1] |= (dir == 1) ? EPF_CTX_TYPE_ISOCH_I : EPF_CTX_TYPE_ISOCH_O;
        break;

    case UHCMD_BULKXFER:
        ep->ctx[1] |= (dir == 1) ? EPF_CTX_TYPE_BULK_I : EPF_CTX_TYPE_BULK_O;
        break;

    case UHCMD_INTXFER:
        ep->ctx[1] |= (dir == 1) ? EPF_CTX_TYPE_INTR_I : EPF_CTX_TYPE_INTR_O;
        break;

    case UHCMD_CONTROLXFER:
        ep->ctx[1] |= EPF_CTX_TYPE_CONTROL;
        break;
    }

    if (epid > 1) {
        ULONG avglen = maxpacket;
        UBYTE multval = 0;
        UBYTE maxBurst = 0;

        if (ioreq) {
            if (flags & UHFF_SUPERSPEED) {
                multval  = ioreq->iouh_SS_Mult;
                maxBurst = ioreq->iouh_SS_MaxBurst;
            } else if ((flags & UHFF_HIGHSPEED) &&
                       ((flags & UHFF_MULTI_2) || (flags & UHFF_MULTI_3))) {
                UBYTE transactions = 1;

                if (flags & UHFF_MULTI_3)
                    transactions = 3;
                else if (flags & UHFF_MULTI_2)
                    transactions = 2;

                multval = transactions - 1;
            }

            /* Mult field (MaxPStreams/MULT in EP ctx) */
            ep->ctx[0] |= EPF_CTX_MULT(multval);

            if (flags & UHFF_SUPERSPEED) {
                if ((type == UHCMD_ISOXFER) || (type == UHCMD_INTXFER)) {
                    if (ioreq->iouh_SS_BytesPerInterval) {
                        /* Preferred: device provides BytesPerInterval directly. */
                        avglen = ioreq->iouh_SS_BytesPerInterval;
                    } else {
                        /* Fallback: compute Max ESIT payload from descriptors. */
                        ULONG multPlus1 = (ULONG)(multval + 1);
                        avglen = maxpacket * (ULONG)(maxBurst + 1) * multPlus1;
                    }
                } else {
                    /* Non-periodic SS: use wMaxPacketSize */
                    avglen = maxpacket;
                }
            } else if ((type == UHCMD_ISOXFER) || (type == UHCMD_INTXFER)) {
                /* High-bandwidth HS endpoints: scale payload by Mult+1 */
                avglen = maxpacket * (ULONG)(multval + 1);
            }
        }

        if (type == UHCMD_CONTROLXFER) {
            if (avglen < 8)
                avglen = 8; /* Minimum control packet size */
        }

        /* Avg TRB Length / Max ESIT payload */
        ep->length = avglen;

        UBYTE ival = xhciCalcInterval(interval, flags, type);
        if (ival)
            ep->ctx[0] |= ((ULONG)ival << 16);
    } else
        devCtx->dc_EP0MaxPacket = maxpacket;

    /* Set CErr = 3 (2-bit field starting at bit 0) */
    ep->ctx[1] |= (EP_CTX_CERR_MASK << EPS_CTX_CERR);

    /* wMaxPacketSize field */
    ep->ctx[1] |= (maxpacket << EPS_CTX_PACKETMAX);

    pciusbXHCIDebugEPV("xHCI",
                      DEBUGCOLOR_SET "Setting de-queue ptr to 0x%p" DEBUGCOLOR_RESET" \n",
                      devCtx->dc_EPAllocs[epid].dmaa_DMA);

    IPTR deqptr = (IPTR)devCtx->dc_EPAllocs[epid].dmaa_DMA;
    deqptr &= ~0xF;               /* Mask reserved bits */
    deqptr |= EPF_CTX_DEQ_DCS;    /* DCS = 1 */

    xhciSetPointer(hc, ep->deq, deqptr);

    /* Flush the complete input context to memory before issuing command */
    CacheClearE((APTR)devCtx->dc_IN.dmaa_Ptr, 
                devCtx->dc_IN.dmaa_Entry.me_Length, 
                CACRF_ClearD);

    pciusbXHCIDebugEP("xHCI",
                      DEBUGCOLOR_SET "%s: Endpoint Ring Initialized @ 0x%p <EPID %u>" DEBUGCOLOR_RESET" \n",
                      __func__, devCtx->dc_EPAllocs[epid].dmaa_Ptr, epid);

    return epid;
}

LONG xhciPrepareEndpoint(struct IOUsbHWReq *ioreq)
{
    struct PCIUnit *unit = (struct PCIUnit *)ioreq->iouh_Req.io_Unit;
    struct PCIController *hc;

    pciusbXHCIDebugEP("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p)" DEBUGCOLOR_RESET" \n", __func__, ioreq);
    pciusbXHCIDebugEPV("xHCI", DEBUGFUNCCOLOR_SET "%s: Dev %u Endpoint %u %s" DEBUGCOLOR_RESET" \n", __func__, ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT");

    if (!unit)
        return UHIOERR_BADPARAMS;

    BOOL rootHubReq = xhciIsRootHubRequest(ioreq, unit);
    pciusbXHCIDebugEPV("xHCI", DEBUGFUNCCOLOR_SET "%s: unit @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, unit);
    hc = xhciGetController(unit);
    if (!hc) {
        return UHIOERR_BADPARAMS;
    } else if (rootHubReq) {
        return 0;
    }

    struct pciusbXHCIDevice *devCtx = xhciObtainDeviceCtx(hc, ioreq, !rootHubReq);
    if (!devCtx) {
        pciusbWarn("xHCI",
                   DEBUGWARNCOLOR_SET "no device context" DEBUGCOLOR_RESET"\n");
        return UHIOERR_HOSTERROR;
    }

    ULONG epid = xhciEndpointID(ioreq->iouh_Endpoint,
                                (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0);

    if (epid >= MAX_DEVENDPOINTS)
        return UHIOERR_BADPARAMS;

    struct pciusbXHCIEndpointCtx *epctx = devCtx->dc_EPContexts[epid];

    if (!devCtx->dc_EPAllocs[epid].dmaa_Ptr) {
        ULONG txep = xhciInitEP(hc, devCtx,
                                ioreq,
                                ioreq->iouh_Endpoint,
                                (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0,
                                ioreq->iouh_Req.io_Command,
                                ioreq->iouh_MaxPktSize,
                                ioreq->iouh_Interval,
                                ioreq->iouh_Flags);

        if (txep == 0)
            return UHIOERR_OUTOFMEMORY;

        epid = txep;

        if (epid >= MAX_DEVENDPOINTS)
            return UHIOERR_BADPARAMS;

        /*
         * Push the updated input context to hardware. Configure all
         * endpoints, including EP0, after initialization so the controller
         * commits the new state.
         */
        LONG cc = xhciCmdEndpointConfigure(hc, devCtx->dc_SlotID, devCtx->dc_IN.dmaa_Ptr);
        if (cc != TRB_CC_SUCCESS) {
            pciusbWarn("xHCI",
                       DEBUGWARNCOLOR_SET "EndpointConfigure failed (cc=%d)" DEBUGCOLOR_RESET"\n", cc);
            return UHIOERR_HOSTERROR;
        }
    }

    if (!epctx) {
        epctx = AllocMem(sizeof(*epctx), MEMF_ANY|MEMF_CLEAR);
        if (!epctx)
            return UHIOERR_OUTOFMEMORY;

        epctx->ectx_Device = devCtx;
        epctx->ectx_EPID = epid;
        devCtx->dc_EPContexts[epid] = epctx;
    }

    ioreq->iouh_DriverPrivate2 = epctx;

    return UHIOERR_NO_ERROR;
}

void xhciDestroyEndpoint(struct IOUsbHWReq *ioreq)
{
    struct PCIUnit *unit = (struct PCIUnit *)ioreq->iouh_Req.io_Unit;
    struct PCIController *hc;
    struct pciusbXHCIDevice *devCtx = NULL;
    struct pciusbXHCIEndpointCtx *epctx = (struct pciusbXHCIEndpointCtx *)ioreq->iouh_DriverPrivate2;
    ULONG epid;

    pciusbXHCIDebugEP("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p)" DEBUGCOLOR_RESET" \n", __func__, ioreq);
    pciusbXHCIDebugEPV("xHCI", DEBUGFUNCCOLOR_SET "%s: unit @ 0x%p, epctx @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, unit, epctx);

    if (!unit)
        return;

    BOOL rootHubReq = xhciIsRootHubRequest(ioreq, unit);
    hc = xhciGetController(unit);
    if (!hc || rootHubReq)
        return;

    epid = xhciEndpointID(ioreq->iouh_Endpoint, (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0);

    if (epctx) {
        devCtx = epctx->ectx_Device;
        epid = epctx->ectx_EPID;
    } else {
        devCtx = xhciFindDeviceCtx(hc, ioreq->iouh_DevAddr);
    }

    if (!devCtx || (epid >= MAX_DEVENDPOINTS))
        return;

    xhciFreeEndpointContext(hc, devCtx, epid, TRUE);

    ioreq->iouh_DriverPrivate2 = NULL;

    /*
     * Downstream devices (route string != 0) do not generate root hub port
     * disconnect events. When their EP0 is torn down and no endpoints remain,
     * assume the device is gone and release the slot/resources.
     */
    if (devCtx->dc_RouteString != 0 &&
        epid == xhciEndpointID(0, 0) &&
        !xhciDeviceHasEndpoints(devCtx)) {
        xhciDisconnectDevice(hc, devCtx);
    }
}

/* Shutdown and Interrupt handlers */

static AROS_INTH1(XhciResetHandler, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    return FALSE;

    AROS_INTFUNC_EXIT
}

void xhciFinishRequest(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    struct pciusbXHCIIODevPrivate *driprivate;
    UWORD devadrep;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p)" DEBUGCOLOR_RESET" \n", __func__, ioreq);

    if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL) {
        ioreq->iouh_DriverPrivate1 = NULL;
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);

        if (driprivate->dpBounceBuf) {
            xhciReleaseDMABuffer(hc, ioreq, 0, driprivate->dpBounceDir,
                                 driprivate->dpBounceBuf);
            driprivate->dpBounceBuf = NULL;
            driprivate->dpBounceData = NULL;
            driprivate->dpBounceLen = 0;
            driprivate->dpBounceDir = 0;
        }

        /* Deactivate the endpoint */
        if (driprivate->dpDevice) {
            struct pciusbXHCIDevice *devCtx = driprivate->dpDevice;
            struct pcisusbXHCIRing *epRing = devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr;
            int cnt;

            if (epRing) {
                xhciRingLock();
                if (driprivate->dpSTRB != (UWORD)-1) {
                    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s: IOReq 0x%p - clearing ringio[%d] setup (was 0x%p)" DEBUGCOLOR_RESET" \n", __func__, ioreq, driprivate->dpSTRB, epRing->ringio[driprivate->dpSTRB]);
                    epRing->ringio[driprivate->dpSTRB] = NULL;
                }

                for (cnt = driprivate->dpTxSTRB; cnt < (driprivate->dpTxETRB + 1); cnt ++) {
                    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s: IOReq 0x%p - clearing ringio[%d] data (was 0x%p)" DEBUGCOLOR_RESET" \n", __func__, ioreq, cnt, epRing->ringio[cnt]);
                    epRing->ringio[cnt] = NULL;
                }

                if (driprivate->dpSttTRB != (UWORD)-1) {
                    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "%s:  IOReq 0x%p - clearing ringio[%d] status (was 0x%p)" DEBUGCOLOR_RESET" \n", __func__, ioreq, driprivate->dpSttTRB, epRing->ringio[driprivate->dpSttTRB]);
                    epRing->ringio[driprivate->dpSttTRB] = NULL;
                }
                xhciRingUnlock();
            }
        }
        FreeMem(driprivate, sizeof(struct pciusbXHCIIODevPrivate));
    }
    devadrep = xhciDevEPKey(ioreq);
    pciusbXHCIDebugEPV("xHCI", DEBUGCOLOR_SET "%s: releasing DevEP %02lx" DEBUGCOLOR_RESET" \n", __func__, devadrep);
    unit->hu_DevBusyReq[devadrep] = NULL;
    unit->hu_NakTimeoutFrame[devadrep] = 0;
}

static inline void xhciIOErrfromCC(struct IOUsbHWReq *ioreq, ULONG cc)
{
    switch (cc) {
    case TRB_CC_SUCCESS:                                        /* Success */
        ioreq->iouh_Req.io_Error = UHIOERR_NO_ERROR;
        /*
         * For periodic transfers (INT/ISO), prefer the length derived from
         * the Transfer Event. If the completion path never set iouh_Actual,
         * fall back to the requested length for backwards compatibility.
         */
        if ((ioreq->iouh_Req.io_Command == UHCMD_INTXFER) &&
            (ioreq->iouh_Actual == 0))
            ioreq->iouh_Actual = ioreq->iouh_Length;
        break;

    case TRB_CC_BABBLE_DETECTED_ERROR:                          /* Data Buffer Error / Babble */
        ioreq->iouh_Req.io_Error = UHIOERR_BABBLE;
        break;

    case TRB_CC_STALL_ERROR:                                    /* Stall */
        ioreq->iouh_Req.io_Error = UHIOERR_STALL;
        break;

    case TRB_CC_PARAMETER_ERROR:                                /* Parameter Error */
        ioreq->iouh_Req.io_Error = UHIOERR_BADPARAMS;
        break;

    case TRB_CC_NO_PING_RESPONSE_ERROR:                         /* No Ping Response / NAK timeout equivalent */
        ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
        break;

    default: {
            pciusbWarn("xHCI", DEBUGCOLOR_SET "%s:  IOReq 0x%p - UHIOERR_HOSTERROR (cc=%ld)" DEBUGCOLOR_RESET" \n", __func__, ioreq, cc);
            {
                UWORD rt  = ioreq->iouh_SetupData.bmRequestType;
                UWORD req = ioreq->iouh_SetupData.bRequest;
                UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
                UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
                UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

                pciusbWarn("xHCI",
                            DEBUGWARNCOLOR_SET
                            "Device[%u]: Command %02lx %02lx %04lx %04lx %04lx!"
                            DEBUGCOLOR_RESET "\n",
                            ioreq->iouh_DevAddr,
                            (ULONG)rt, (ULONG)req, (ULONG)idx, (ULONG)val, (ULONG)len);
            }
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
        }
        break;
    }
}

void xhciHandleFinishedTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq, *nextioreq;
    struct pciusbXHCIIODevPrivate *driprivate;
    UWORD devadrep;
    BOOL transactiondone;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    /* PERIODIC (INT/ISO) COMPLETIONS */
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Checking for Periodic work done..." DEBUGCOLOR_RESET"\n");

    ioreq = (struct IOUsbHWReq *)hc->hc_PeriodicTDQueue.lh_Head;
    while ((nextioreq = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ)) {
        pciusbXHCIDebugV("xHCI",
                        DEBUGCOLOR_SET "Examining Periodic IOReq=%p (dev=%u, ep=%u, dir=%s)" DEBUGCOLOR_RESET"\n",
                        ioreq,
                        ioreq->iouh_DevAddr,
                        ioreq->iouh_Endpoint,
                        (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT");

        driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        if (!driprivate) {
            pciusbWarn("xHCI",
                       DEBUGWARNCOLOR_SET "Periodic IOReq %p has no driver private, skipping" DEBUGCOLOR_RESET"\n",
                       ioreq);
            ioreq = nextioreq;
            continue;
        }

        transactiondone = FALSE;
        devadrep = xhciDevEPKey(ioreq);

        /*
         * First, look at completion code from the event TRB
         * (set in xhciIntWorkProcess).
         */
        if (driprivate->dpCC > 0) {
            pciusbXHCIDebugV("xHCI",
                            DEBUGCOLOR_SET "Periodic IOReq %p complete (CC=%u)" DEBUGCOLOR_RESET"\n",
                            ioreq, driprivate->dpCC);

            xhciIOErrfromCC(ioreq, driprivate->dpCC);

            transactiondone = TRUE;
        } else {
            /*
             * No CC yet - check for software NAK timeout
             */
            if (unit->hu_NakTimeoutFrame[devadrep] &&
                (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep])) {
                pciusbWarn("xHCI",
                           DEBUGWARNCOLOR_SET "xHCI: Periodic NAK timeout for DevEP %02x (frame %u > %u)" DEBUGCOLOR_RESET"\n",
                           devadrep,
                           hc->hc_FrameCounter,
                           unit->hu_NakTimeoutFrame[devadrep]);
                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                transactiondone = TRUE;
            }
        }

        if (transactiondone) {
            /*
             * At this point xhciIntWorkProcess should have set iouh_Actual.
             * Log what we are about to return to hub.class (or any other client).
             */
            ULONG actual = ioreq->iouh_Actual;

            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "Periodic IOReq %p DONE: err=%ld, actual=%lu" DEBUGCOLOR_RESET"\n",
                            ioreq,
                            (LONG)ioreq->iouh_Req.io_Error,
                            (unsigned long)actual);

            struct RTIsoNode *rtn = (struct RTIsoNode *)ioreq->iouh_DriverPrivate2;
            if (rtn && rtn->rtn_RTIso) {
                struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
                rtn->rtn_BufferReq.ubr_Length = actual;
                rtn->rtn_BufferReq.ubr_Frame = ioreq->iouh_Frame;
                if (ioreq->iouh_Dir == UHDIR_IN) {
                    if (urti->urti_InDoneHook)
                        CallHookPkt(urti->urti_InDoneHook, rtn, &rtn->rtn_BufferReq);
                } else {
                    if (urti->urti_OutDoneHook)
                        CallHookPkt(urti->urti_OutDoneHook, rtn, &rtn->rtn_BufferReq);
                }

                xhciFreePeriodicContext(hc, unit, ioreq);
                ioreq->iouh_Actual = 0;
                ioreq->iouh_Req.io_Error = 0;
                if (xhciQueueIsochIO(hc, rtn) == RC_OK)
                    xhciStartIsochIO(hc, rtn);
                ioreq = nextioreq;
                continue;
            }

            if (!ioreq->iouh_Req.io_Error &&
                ioreq->iouh_Data &&
                actual > 0)
            {
                UBYTE *b = (UBYTE *)ioreq->iouh_Data;
                pciusbXHCIDebugV("xHCI",
                                DEBUGCOLOR_SET "Periodic data (dev=%u,ep=%u): first byte=0x%02x len=%lu" DEBUGCOLOR_RESET"\n",
                                ioreq->iouh_DevAddr,
                                ioreq->iouh_Endpoint,
                                b[0],
                                (unsigned long)actual);

                /* Optional: dump a few bytes */
#if defined(DEBUG) && (DEBUG > 1)
                {
                    int i, dump = (actual < 8) ? actual : 8;
                    for (i = 0; i < dump; i++) {
                        pciusbXHCIDebug("xHCI",
                            "    [%d] = 0x%02x\n", i, b[i]);
                    }
                }
#endif
            }

            /*
             * Release hardware state and complete the IO.
             * NOTE: do NOT adjust iouh_Actual here - that is done by
             * xhciIntWorkProcess when it decodes the Event TRB.
             */
            xhciFreePeriodicContext(hc, unit, ioreq);
            ReplyMsg(&ioreq->iouh_Req.io_Message);
        }

        ioreq = nextioreq;
    }

    /* ASYNC (CTRL/BULK) COMPLETIONS */
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "Checking for Standard work done..." DEBUGCOLOR_RESET" \n");
    ioreq = (struct IOUsbHWReq *)hc->hc_TDQueue.lh_Head;
    while ((nextioreq = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ)) {
        pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "Examining IOReq=0x%p" DEBUGCOLOR_RESET" \n", ioreq);

        driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        if (driprivate) {
            transactiondone = FALSE;
            devadrep = xhciDevEPKey(ioreq);

            if (driprivate->dpCC > TRB_CC_INVALID) {
                pciusbXHCIDebugV("xHCI",
                                DEBUGCOLOR_SET "IOReq Complete (completion code %u)!"
                                DEBUGCOLOR_RESET" \n",
                                driprivate->dpCC);
                transactiondone = TRUE;
                
                xhciIOErrfromCC(ioreq, driprivate->dpCC);

                unit->hu_NakTimeoutFrame[devadrep] = 0;

            } else if (unit->hu_NakTimeoutFrame[devadrep] &&
                       (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep])) {
                pciusbWarn("xHCI",
                           DEBUGWARNCOLOR_SET "xHCI: Async NAK timeout (%u)"
                           DEBUGCOLOR_RESET" \n",
                           unit->hu_NakTimeoutFrame[devadrep]);
                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                transactiondone = TRUE;
                unit->hu_NakTimeoutFrame[devadrep] = 0;
            }

            /*
             * If we see a STALL on a control endpoint (EP0), the xHC will place
             * that endpoint into the Halted state and will not execute further
             * TDs until software issues Reset Endpoint / Set TR Dequeue.
             *
             * During enumeration, upper layers (Poseidon hubss.class) do not
             * send CLEAR_FEATURE(ENDPOINT_HALT) on EP0, so a stalled
             * SET_CONFIGURATION would permanently wedge control traffic for
             * that device. To prevent the hang, we automatically reset EP0
             * here for non-root devices when a STALL is reported.
             */
            if (driprivate->dpCC == TRB_CC_STALL_ERROR &&
                ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER &&
                ioreq->iouh_Endpoint == 0)
            {
                struct pciusbXHCIDevice *devCtx = driprivate->dpDevice;

                if (devCtx && devCtx != XHCI_ROOT_HUB_HANDLE) {
                    ULONG epid = driprivate->dpEPID;
                    if (!epid) {
                        /* Fallback: EP0 is EPID 1 by spec */
                        epid = xhciGetEPID(0, 0);
                    }

                    pciusbWarn("xHCI",
                        DEBUGWARNCOLOR_SET
                        "STALL on Dev %u EP0 (CC=6), resetting EPID=%lu"
                        DEBUGCOLOR_RESET"\n",
                        ioreq->iouh_DevAddr, epid);

                    xhciCmdEndpointReset(hc, devCtx->dc_SlotID, epid, 0);
                }
            }

            if (transactiondone) {
                xhciFreeAsyncContext(hc, unit, ioreq);
                if ((!ioreq->iouh_Req.io_Error) &&
                    (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)) {
                    uhwCheckSpecialCtrlTransfers(hc, ioreq);
                }
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        }
        ioreq = nextioreq;
    }
}

static AROS_INTH1(xhciCompleteInt, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    xhciUpdateFrameCounter(hc);

    uhwCheckRootHubChanges(hc->hc_Unit);

    Signal(xhcic->xhc_xHCERTask, 1L << xhcic->xhc_DoWorkSignal);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s Done" DEBUGCOLOR_RESET" \n", __func__);

    return FALSE;

    AROS_INTFUNC_EXIT
}

static BOOL xhciTRBCycleMatches(ULONG trbflags, ULONG cycle)
{
    return (((trbflags & TRBF_FLAG_C) ? 1 : 0) == (cycle ? 1 : 0)) ? TRUE : FALSE;
}

BOOL xhciIntWorkProcess(struct PCIController *hc, struct IOUsbHWReq *ioreq, ULONG remaining, ULONG ccode)
{
    struct pciusbXHCIIODevPrivate *driprivate = NULL;
    pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "Examining IOReq=0x%p" DEBUGCOLOR_RESET" \n", ioreq);

    if (!ioreq) {
        pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "IOReq missing for completion (cc=%lu)" DEBUGCOLOR_RESET" \n", (ULONG)ccode);
        return FALSE;
    }

    if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL) {
        UWORD effdir = xhciEffectiveDataDir(ioreq);
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "IOReq TRB(s) = #%u:#%u" DEBUGCOLOR_RESET" \n", driprivate->dpTxSTRB, driprivate->dpTxETRB);
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "          Ring    @ 0x%p" DEBUGCOLOR_RESET" \n", driprivate->dpDevice->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr);

        if ((driprivate->dpCC = ccode) != TRB_CC_SUCCESS) {
            pciusbWarn("xHCI",
                       DEBUGWARNCOLOR_SET
                       "cc=%d for IOReq 0x%p"
                       DEBUGCOLOR_RESET" \n", driprivate->dpCC, ioreq);
        }

        ULONG transferred = (remaining <= ioreq->iouh_Length)
                                ? (ioreq->iouh_Length - remaining)
                                : ioreq->iouh_Length;

        /*
         * VirtualBox xHCI occasionally reports "remaining == requested length"
         * on successful CONTROL/INT IN completions, even though the device
         * delivered data into the buffer. If we leave transferred as zero in
         * that case, upper layers will ignore the data and we may also skip
         * CachePostDMA() depending on the logic below.
         *
         * Restrict the workaround to CONTROL and INT transfers to avoid
         * perturbing BULK/ISO semantics (where a ZLP is legitimate).
         */
        if ((ccode == TRB_CC_SUCCESS) &&
            (effdir == UHDIR_IN) &&
            (ioreq->iouh_Length > 0) &&
            (remaining == ioreq->iouh_Length) &&
            ((ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER) ||
             (ioreq->iouh_Req.io_Command == UHCMD_INTXFER)))
        {
            transferred = ioreq->iouh_Length;
        }

        if ((ccode == TRB_CC_SUCCESS) && ioreq->iouh_Data) {
            xhciReleaseDMABuffer(hc, ioreq, transferred, effdir, driprivate->dpBounceBuf);
            driprivate->dpBounceBuf = NULL;
            driprivate->dpBounceData = NULL;
            driprivate->dpBounceLen = 0;
            driprivate->dpBounceDir = 0;
        }

        ioreq->iouh_Actual = transferred;
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "Transfer IO done/remaining = %u/%u bytes" DEBUGCOLOR_RESET" \n", transferred, remaining);

        return TRUE;
    }

    /*
     * Command-ring submissions expect an IOReq but do not always
     * carry driver-private transfer state. Treat these as completed so
     * the higher layers can observe the result and progress.
     */
    ioreq->iouh_Req.io_Error = (ccode == TRB_CC_SUCCESS) ? 0 : UHIOERR_HOSTERROR;
    ioreq->iouh_Actual += (ioreq->iouh_Length - remaining);

    pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "Completion w/o private state, treating as done (cc=%lu)" DEBUGCOLOR_RESET" \n", (ULONG)ccode);

    return TRUE;
}

static AROS_INTH1(xhciIntCode, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_hcopr *hcopr =
        (volatile struct xhci_hcopr *)((IPTR)xhcic->xhc_XHCIOpR);
    BOOL doCompletion = FALSE, checkRHchanges = FALSE;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    ULONG status = AROS_LE2LONG(hcopr->usbsts);
    xhciDumpStatus(status);

    /* First acknowledge the interrupt ..*/
    hcopr->usbsts = AROS_LONG2LE(status);

    /* Check if anything interesting happened.... */
    if (status & XHCIF_USBSTS_HCH) {
        pciusbXHCIDebugTRBV("xHCI",
            DEBUGCOLOR_SET "Host Controller Halted" DEBUGCOLOR_RESET" \n");
    }
    if (status & XHCIF_USBSTS_HCE) {
        pciusbXHCIDebugTRBV("xHCI",
            DEBUGCOLOR_SET "Host Controller Error detected" DEBUGCOLOR_RESET" \n");
    }
    if (status & XHCIF_USBSTS_HSE) {
        pciusbXHCIDebugTRBV("xHCI",
            DEBUGCOLOR_SET "Host System Error detected" DEBUGCOLOR_RESET" \n");
    }
    if (status & XHCIF_USBSTS_EINT) {
        pciusbXHCIDebugTRBV("xHCI",
            DEBUGCOLOR_SET "Event Interrupt Pending" DEBUGCOLOR_RESET" \n");
    }
    if (status & XHCIF_USBSTS_PCD) {
        pciusbXHCIDebugTRBV("xHCI",
            DEBUGCOLOR_SET "Port Change Detect" DEBUGCOLOR_RESET" \n");
    }

    volatile struct xhci_ir *xhciir =
        (volatile struct xhci_ir *)((IPTR)xhcic->xhc_XHCIIntR);
    xhciDumpIR(xhciir);

    ULONG iman = AROS_LE2LONG(xhciir->iman), tmp;
    /* Clear IP (W1C) while keeping IE set */
    xhciir->iman = AROS_LONG2LE(XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP);
    tmp = AROS_LE2LONG(xhciir->iman);

    if ((iman & (XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP)) ==
        (XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP)) {
        volatile struct pcisusbXHCIRing *ering =
            (volatile struct pcisusbXHCIRing *)((IPTR)xhcic->xhc_ERSp);
        volatile struct xhci_trb *etrb;
        ULONG idx   = ering->next;
        ULONG cycle = (ering->end & RINGENDCFLAG) ? 1 : 0;
        ULONG maxwork = XHCI_EVENT_RING_TRBS;

        pciusbXHCIDebugTRBV("xHCI",
                           DEBUGCOLOR_SET "Processing events..." DEBUGCOLOR_RESET" \n");

        for (etrb = &ering->ring[idx]; maxwork > 0; maxwork--) {
            /* Make sure we see the latest TRB from the controller. */
            CacheClearE((APTR)etrb, sizeof(*etrb), CACRF_InvalidateD);

            ULONG dw2        = AROS_LE2LONG(etrb->tparams);
            ULONG dw3        = AROS_LE2LONG(etrb->flags);

            if (!xhciTRBCycleMatches(dw3, cycle))
                break;

            ULONG trbe_type  = (dw3 >> TRBS_FLAG_TYPE) & TRB_FLAG_TYPE_SMASK;
            ULONG trbe_slot  = (dw3 >> 24) & 0xFF;
            ULONG trbe_ccode = (dw2 >> 24) & 0xFF;
            ULONG event_rem  = dw2 & 0x00FFFFFF;

            pciusbXHCIDebugTRBV("xHCI",
                DEBUGCOLOR_SET "Event Ring 0x%p[%u] = <%sTRB 0x%p, type %u>\n       slot %u"
                DEBUGCOLOR_RESET" \n",
                ering, idx,
                (dw3 & (1 << 2)) ? "Event " : "",
                etrb, trbe_type, trbe_slot);
            xhciDumpCC(trbe_ccode);

            switch (trbe_type) {
            case TRBB_FLAG_ERTYPE_PORT_STATUS_CHANGE: {
                struct xhci_trb  *txtrb = xhciTRBPointer(hc, etrb);
                volatile struct xhci_pr *xhciports =
                    (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);
                volatile struct xhci_trb_port_status *evt =
                    (volatile struct xhci_trb_port_status *)&ering->current;

                UWORD hciport;
                ULONG origportsc, newportsc = 0;

                *evt = *(volatile struct xhci_trb_port_status *)etrb;

                pciusbXHCIDebugTRBV("xHCI",
                    DEBUGCOLOR_SET
                    "Port Status Change detected <Port=#%u>\nPort Status Change TRB = <%p>"
                    DEBUGCOLOR_RESET" \n",
                    evt->port, txtrb);

                hciport = evt->port - 1;

                xhciDumpPort(&xhciports[hciport]);
                origportsc = AROS_LE2LONG(xhciports[hciport].portsc);

                /* reflect port ownership (shortcut without hc->hc_PortNum[evt->port]) */
                hc->hc_Unit->hu_PortOwner[hciport] = HCITYPE_XHCI;

                if (origportsc & XHCIF_PR_PORTSC_OCC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
                    newportsc |= XHCIF_PR_PORTSC_OCC;
                }
                if (origportsc & XHCIF_PR_PORTSC_PRC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
                    newportsc |= XHCIF_PR_PORTSC_PRC;
                }
                if (origportsc & XHCIF_PR_PORTSC_WRC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
                    newportsc |= XHCIF_PR_PORTSC_WRC;
                }
                if (origportsc & XHCIF_PR_PORTSC_PEC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                    newportsc |= XHCIF_PR_PORTSC_PEC;
                }
                if (origportsc & XHCIF_PR_PORTSC_CSC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                    newportsc |= XHCIF_PR_PORTSC_CSC;
                }
                if (origportsc & XHCIF_PR_PORTSC_PLC)
                    newportsc |= XHCIF_PR_PORTSC_PLC;
                if (origportsc & XHCIF_PR_PORTSC_CEC)
                    newportsc |= XHCIF_PR_PORTSC_CEC;

                /* Acknowledge the change bits we observed */
                xhciports[hciport].portsc = AROS_LONG2LE(newportsc);

                pciusbXHCIDebugTRBV("xHCI",
                    DEBUGCOLOR_SET "RH Change $%08lx" DEBUGCOLOR_RESET" \n",
                    hc->hc_PortChangeMap[hciport]);

                if (hc->hc_PortChangeMap[hciport]) {
                    BOOL signalTask = FALSE;

                    hc->hc_Unit->hu_RootPortChanges |= (1UL << (hciport + 1));

                    {
                        const BOOL enabled = xhciHubPortEnabled(hc, hciport, origportsc);
                        const BOOL haveDev = (xhciFindPortDevice(hc, hciport) != NULL);

                        /*
                         * For USB3 ports, don't key off PED. Treat "enabled" as
                         * "connected and operational (U0)" so SS/SS+ devices can
                         * trigger the port-change task.
                         */
                        if ((hc->hc_PortChangeMap[hciport] & UPSF_PORT_CONNECTION) ||
                            (enabled && !haveDev) ||
                            (!enabled && haveDev))
                        {
                            signalTask = TRUE;
                        }
                    }

                    if (signalTask) {
                        pciusbXHCIDebugTRBV("xHCI", DEBUGFUNCCOLOR_SET "%s: Signaling port change handler <0x%p, %d (%08x)>..." DEBUGCOLOR_RESET" \n", __func__, xhcic->xhc_xHCPortTask, xhcic->xhc_PortChangeSignal, 1L << xhcic->xhc_PortChangeSignal);
                        /* Connect/Disconnect the device */
                        Signal(xhcic->xhc_xHCPortTask, 1L << xhcic->xhc_PortChangeSignal);
                    } else {
                        checkRHchanges = TRUE;
                    }
                }
                break;
            }

            case TRBB_FLAG_ERTYPE_TRANSFER: {
                struct xhci_trb  *txtrb = xhciTRBPointer(hc, etrb);
                struct pcisusbXHCIRing *ring = RINGFROMTRB(txtrb);
                volatile struct xhci_trb  *evt = &ring->current;
                ULONG last = (ULONG)(txtrb - ring->ring);

                /* Cache the event TRB before using its fields. */
                *evt = *etrb;

                pciusbXHCIDebugTRBV("xHCI",
                    DEBUGCOLOR_SET "TRANSFER EVT: ring=%p idx=%u slot=%u CC=%u rem=%lu"
                    DEBUGCOLOR_RESET" \n",
                    ring, last,
                    trbe_slot,
                    trbe_ccode,
                    (unsigned long)event_rem);
                xhciDumpCC(trbe_ccode);

                struct IOUsbHWReq *req;
                req = (struct IOUsbHWReq *)ring->ringio[last];
                ring->end &= RINGENDCFLAG;
                ring->end |= (last & ~RINGENDCFLAG);
                ring->ringio[last] = NULL;
                if (!req) {
                    pciusbWarn("xHCI",
                               DEBUGWARNCOLOR_SET
                               "Transfer #%d with completion code %lu missing ioreq\n"
                               DEBUGCOLOR_RESET" \n",
                               last, trbe_ccode);
                }
                doCompletion |= xhciIntWorkProcess(hc, req, event_rem, trbe_ccode);
                break;
            }

            case TRBB_FLAG_ERTYPE_COMMAND_COMPLETE: {
                struct xhci_trb  *txtrb = xhciTRBPointer(hc, etrb);
                struct pcisusbXHCIRing *ring = RINGFROMTRB(txtrb);
                volatile struct xhci_trb  *evt = &ring->current;
                ULONG last = (ULONG)(txtrb - ring->ring);
                ULONG txdw3    = AROS_LE2LONG(txtrb->flags);
                ULONG cmd_type = (txdw3 >> TRBS_FLAG_TYPE) & TRB_FLAG_TYPE_SMASK;
                ULONG slotid   = (txdw3 >> 24) & 0xFF;
                ULONG epid     = (txdw3 >> 16) & 0x1F;
                struct pciusbXHCIDevice *devCtx =
                    (slotid < USB_DEV_MAX) ? xhcic->xhc_Devices[slotid] : NULL;
                ULONG port = devCtx ? (devCtx->dc_RootPort + 1) : 0;

                pciusbXHCIDebugTRBV("xHCI",
                    DEBUGCOLOR_SET
                    "Cmd TRB  = <Cmd Ring 0x%p[%u] TRB 0x%p> type %lu (%s) slot %lu ep %lu port %lu"
                    DEBUGCOLOR_RESET" \n",
                    ring,
                    last,
                    txtrb,
                    cmd_type,
                    xhciCmdTypeName(cmd_type),
                    slotid,
                    epid,
                    port);
                xhciDumpCC(trbe_ccode);

                if (trbe_ccode != TRB_CC_SUCCESS) {
                    volatile struct xhci_ir *ir =
                        (volatile struct xhci_ir *)xhcic->xhc_XHCIIntR;
                    ULONG crcr_lo = AROS_LE2LONG(hcopr->crcr.addr_lo);
                    ULONG crcr_hi = AROS_LE2LONG(hcopr->crcr.addr_hi);
                    ULONG erdp_lo = AROS_LE2LONG(ir->erdp.addr_lo);
                    ULONG erdp_hi = AROS_LE2LONG(ir->erdp.addr_hi);
                    ULONG erst_lo = AROS_LE2LONG(ir->erstba.addr_lo);
                    ULONG erst_hi = AROS_LE2LONG(ir->erstba.addr_hi);

                    pciusbWarn("xHCI",
                        DEBUGWARNCOLOR_SET
                        "Command %s completion code %lu (slot %lu ep %lu port %lu)\n"
                        "CRCR=0x%08lx%08lx ERDP=0x%08lx%08lx ERSTBA=0x%08lx%08lx"
                        DEBUGCOLOR_RESET" \n",
                        xhciCmdTypeName(cmd_type),
                        trbe_ccode,
                        slotid,
                        epid,
                        port,
                        crcr_hi,
                        crcr_lo,
                        erdp_hi,
                        erdp_lo,
                        erst_hi,
                        erst_lo);

#if defined(XHCI_CMDFAIL_LOGGING)
                    xhciDebugDumpDCBAAEntry(hc, slotid);

                    if (devCtx && devCtx->dc_SlotCtx.dmaa_Ptr) {
                        /*
                         * Output Device Context layout differs from Input Context:
                         * there is no Input Control Context at index 0.
                         *
                         * Index 0: Slot Context
                         * Index N: Endpoint Context for EPID==N
                         */
                        const UWORD ctxsize = (hc->hc_Flags & HCF_CTX64) ? 64 : 32;
                        volatile UBYTE *obase = (volatile UBYTE *)devCtx->dc_SlotCtx.dmaa_Ptr;
                        volatile struct xhci_slot *slotctx = (volatile struct xhci_slot *)obase;
                        volatile struct xhci_ep *ep0ctx =
                            (volatile struct xhci_ep *)(obase + ((UWORD)xhciGetEPID(0, 0) * ctxsize));

                        xhciDebugDumpSlotContext(slotctx);
                        xhciDebugDumpEndpointContext(ep0ctx, xhciGetEPID(0, 0));
                    }
#endif
                }

                *evt = *etrb;

                xhcic->xhc_CmdResults[last].flags   = AROS_LE2LONG(evt->flags);
                xhcic->xhc_CmdResults[last].tparams = AROS_LE2LONG(evt->tparams);
                xhcic->xhc_CmdResults[last].status  = trbe_ccode;

                struct IOUsbHWReq *req;
                req = (struct IOUsbHWReq *)ring->ringio[last];
                ring->end &= RINGENDCFLAG;
                ring->end |= (last & ~RINGENDCFLAG);
                ring->ringio[last] = NULL;

                if (!req) {
                    /*
                     * Synchronous command submissions (xhciCmdSubmit) do not
                     * populate ringio[], because they complete by polling
                     * xhc_CmdResults[]. Do not treat these completions as
                     * missing-IOReq errors and do not run the transfer
                     * completion path.
                     */
                    pciusbXHCIDebugTRBV("xHCI",
                        DEBUGCOLOR_SET
                        "Command #%d completed (cc=%lu) without IOReq (sync submit)\n"
                        DEBUGCOLOR_RESET" \n",
                        last, trbe_ccode);
                } else {
                    doCompletion |= xhciIntWorkProcess(hc, req, event_rem, trbe_ccode);
                }
                break;
            }

            default:
                pciusbXHCIDebugTRB("xHCI",
                    DEBUGWARNCOLOR_SET
                    "Unknown event, type %u, completion code %u"
                    DEBUGCOLOR_RESET" \n",
                    trbe_type, trbe_ccode);
                break;
            }

            /* Update cached index + cycle state for the event ring itself. */
            ering->end &= RINGENDCFLAG;
            ering->end |= (idx & ~RINGENDCFLAG);

            /* Advance to the next TRB, handling wrap and cycle bit. */
            if (++idx == XHCI_EVENT_RING_TRBS) {
                idx = 0;
                cycle ^= 1;
                if (cycle)
                    ering->end |= RINGENDCFLAG;
                else
                    ering->end &= ~RINGENDCFLAG;
            }
            ering->next = idx;
            etrb = &ering->ring[idx];

            /* Update the hardware dequeue pointer to the next TRB. */
            {
                volatile struct xhci_ir *ir =
                    (volatile struct xhci_ir *)xhcic->xhc_XHCIIntR;
                UQUAD next_dma;

                next_dma  = (UQUAD)(IPTR)xhcic->xhc_DMAERS;
                next_dma += (UQUAD)ering->next * (UQUAD)sizeof(struct xhci_trb);

                xhciSetPointer(hc, ir->erdp,
                               (IPTR)(next_dma | (UQUAD)XHCIF_IR_ERDP_EHB));
            }
        }

        if (maxwork == 0) {
            pciusbWarn("xHCI",
                               DEBUGWARNCOLOR_SET
                               "Event ring processing budget exhausted; possible backlog or ring stall"
                               DEBUGCOLOR_RESET" \n");
        }

        if (doCompletion) {
            SureCause(base, &hc->hc_CompleteInt);
        }
    }

    if (checkRHchanges)
        uhwCheckRootHubChanges(hc->hc_Unit);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s: Exiting" DEBUGCOLOR_RESET" \n", __func__);

    return FALSE;

    AROS_INTFUNC_EXIT
}

void xhciAbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct PCIUnit *unit;

    pciusbXHCIDebug("xHCI",
        DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n",
        __func__, hc, ioreq);

    if (!ioreq)
        return;

    /* Get the host's root unit for this controller */
    unit = hc->hc_Unit;

    /*
     * 1. Free controller-side async context and release DevEP.
     *    This will:
     *      - Deactivate the endpoint for this IOReq
     *      - Clear hu->hu_DevBusyReq[DevEP]
     *      - Clear hu->hu_NakTimeoutFrame[DevEP]
     *      - Remove the IOReq from the HC's internal queue
     */
    xhciFreeAsyncContext(hc, unit, ioreq);

    /*
     * 2. Return the IOReq to the caller.
     *    The NAK timeout logic in uhw should already have set
     *    iouh_Req.io_Error appropriately (e.g. timeout / aborted).
     */
    ReplyMsg(&ioreq->iouh_Req.io_Message);
}

void xhciReset(struct PCIController *hc, struct PCIUnit *hu)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)xhcic->xhc_XHCIOpR);
    ULONG reg;
    ULONG cnt = 100;
    ULONG status;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, hu);

    // Tell the controller to stop if its currently running ...
    reg = AROS_LE2LONG(hcopr->usbcmd);
    if (reg & XHCIF_USBCMD_RS) {
        reg &= ~XHCIF_USBCMD_RS;
        hcopr->usbcmd = AROS_LONG2LE(reg);

        // Wait for the controller to indicate it is finished ...
        while (!(AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_HCH) && (--cnt > 0))
            uhwDelayMS(1, hu);
    }

    status = AROS_LE2LONG(hcopr->usbsts);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Resetting Controller (pre-reset status $%08x)..." DEBUGCOLOR_RESET" \n", status);
    xhciDumpStatus(status);
    if (status & (XHCIF_USBSTS_HCE | XHCIF_USBSTS_HSE))
        pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "Pre-reset controller error bits set:%s%s" DEBUGCOLOR_RESET" \n",
                        (status & XHCIF_USBSTS_HCE) ? " HCE" : "",
                        (status & XHCIF_USBSTS_HSE) ? " HSE" : "");

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Resetting Controller..." DEBUGCOLOR_RESET" \n");
    hcopr->usbcmd = AROS_LONG2LE(XHCIF_USBCMD_HCRST);

    // Wait for the command to be accepted..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbcmd) & XHCIF_USBCMD_HCRST) && (--cnt > 0))
        uhwDelayMS(2, hu);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "COMMAND = $%08x, after %ums" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd), (100 - cnt) << 1);

    status = AROS_LE2LONG(hcopr->usbsts);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBSTS after HCRST write = $%08x" DEBUGCOLOR_RESET" \n", status);
    xhciDumpStatus(status);
    if (status & (XHCIF_USBSTS_HCE | XHCIF_USBSTS_HSE))
        pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "Post-reset write detected controller error bits:%s%s" DEBUGCOLOR_RESET" \n",
                        (status & XHCIF_USBSTS_HCE) ? " HCE" : "",
                        (status & XHCIF_USBSTS_HSE) ? " HSE" : "");

    // Wait for the reset to complete..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_CNR) && (--cnt > 0))
        uhwDelayMS(2, hu);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBSTS = $%08x, after %ums" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbsts), (100 - cnt) << 1);
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Configuring DMA pointers..." DEBUGCOLOR_RESET" \n");

    hcopr->config = AROS_LONG2LE(xhcic->xhc_NumSlots);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting DCBAA to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMADCBAA);
    xhciSetPointer(hc, hcopr->dcbaap, xhcic->xhc_DMADCBAA);
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting CRCR to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAOPR);
    xhciSetPointer(hc, hcopr->crcr, ((IPTR)xhcic->xhc_DMAOPR | 1));

    volatile struct pcisusbXHCIRing *xring = (volatile struct pcisusbXHCIRing *)xhcic->xhc_OPRp;
    xhciInitRing((struct pcisusbXHCIRing *)xring);

    volatile struct xhci_er_seg *erseg = (volatile struct xhci_er_seg *)xhcic->xhc_ERSTp;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting Event Segment Pointer to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERS);
    xhciSetPointer(hc, erseg->ptr, ((IPTR)xhcic->xhc_DMAERS));

    erseg->size = AROS_LONG2LE(XHCI_EVENT_RING_TRBS);

    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)xhcic->xhc_XHCIIntR);
    xhciir->erstsz = AROS_LONG2LE(1);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting ERDP to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERS);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting ERSTBA to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERST);
    xhciSetPointer(hc, xhciir->erdp, ((IPTR)xhcic->xhc_DMAERS | (IPTR)XHCIF_IR_ERDP_EHB));
    xhciSetPointer(hc, xhciir->erstba, ((IPTR)xhcic->xhc_DMAERST));

    xring = (volatile struct pcisusbXHCIRing *)xhcic->xhc_ERSp;
    xhciInitRing((struct pcisusbXHCIRing *)xring);
    xhciir->iman = AROS_LONG2LE(XHCIF_IR_IMAN_IE);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Reset complete..." DEBUGCOLOR_RESET" \n");

    xhciDumpOpR(hcopr);
    xhciDumpIR(xhciir);
}

BOOL xhciInit(struct PCIController *hc, struct PCIUnit *hu)
{
    struct PCIDevice *hd = hu->hu_Device;
    struct XhciHCPrivate *xhcic = NULL;
    volatile struct xhci_hccapr *xhciregs;
    UBYTE *memptr;
    ULONG xhciUSBLegSup = 0;
    ULONG xhciECPOff;
    ULONG val;
    ULONG cnt;

    struct TagItem pciMemEnableAttrs[] = {
        { aHidd_PCIDevice_isMEM,    TRUE },
        { aHidd_PCIDevice_isMaster, TRUE },
        { TAG_DONE, 0UL },
    };

    struct TagItem pciDeactivateBusmaster[] = {
        { aHidd_PCIDevice_isMaster, FALSE },
        { TAG_DONE, 0UL },
    };

    struct Library *ps;
    if ((ps = OpenLibrary("poseidon.library", 5)) == NULL) {
        return FALSE;
    }

    xhcic = AllocMem(sizeof(*xhcic), MEMF_CLEAR);
    if (!xhcic) {
        CloseLibrary(ps);
        return FALSE;
    }
    hc->hc_CPrivate = xhcic;

    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "XHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)xhciCompleteInt;

    /* Initialize hardware... */
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (IPTR *)&hc->hc_RegBase);
    xhciregs = (volatile struct xhci_hccapr *)hc->hc_RegBase;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Initializing hardware for unit #%d" DEBUGCOLOR_RESET" \n", hu->hu_UnitNo);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  CAPLENGTH: 0x%02x" DEBUGCOLOR_RESET" \n", xhciregs->caplength);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  DBOFF: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->dboff));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  RRSOFF: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->rrsoff));

    xhcic->xhc_XHCIOpR   = (APTR)((IPTR)xhciregs + xhciregs->caplength);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Operational Registers @ 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_XHCIOpR);
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)xhcic->xhc_XHCIOpR;
    xhcic->xhc_XHCIDB    = (APTR)((IPTR)xhciregs + AROS_LE2LONG(xhciregs->dboff));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Doorbells @ 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_XHCIDB);
    xhcic->xhc_XHCIPorts = (APTR)((IPTR)xhcic->xhc_XHCIOpR + 0x400);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Port Registers @ 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_XHCIPorts);
    xhcic->xhc_XHCIIntR  = (APTR)((IPTR)xhciregs + AROS_LE2LONG(xhciregs->rrsoff) + 0x20);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Interrupt Registers @ 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_XHCIIntR);

    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *)pciMemEnableAttrs); /* activate memory */

    /* Cache capability parameters once */
    ULONG hcsparams1 = AROS_LE2LONG(xhciregs->hcsparams1);
    ULONG hcsparams2 = AROS_LE2LONG(xhciregs->hcsparams2);
    ULONG hcsparams3 = AROS_LE2LONG(xhciregs->hcsparams3);
    ULONG hccparams1 = AROS_LE2LONG(xhciregs->hccparams1);
    ULONG hccparams2 = AROS_LE2LONG(xhciregs->hccparams2);
    ULONG xhciMaxIntrs = (hcsparams1 >> 8) & 0x7FF;
    UWORD xhciPortLimit = (UWORD)((hcsparams1 >> 24) & 0xFF);

    if (xhciPortLimit > MAX_ROOT_PORTS) {
        xhciPortLimit = MAX_ROOT_PORTS;
    }

    memset(xhcic->xhc_PortProtocol, XHCI_PORT_PROTOCOL_UNKNOWN, sizeof(xhcic->xhc_PortProtocol));
    xhcic->xhc_PortProtocolValid = FALSE;

    /* Extended Capabilities Pointer comes from HCCPARAMS1 (DWORD offset) */
    xhciECPOff = ((hccparams1 >> XHCIS_HCCPARAMS1_ECP) & XHCI_HCCPARAMS1_ECP_SMASK) << 2;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Extended Capabilties Pointer = %04x" DEBUGCOLOR_RESET" \n", xhciECPOff);

    while (xhciECPOff >= 0x40) {
        volatile ULONG *capreg = (volatile ULONG *)((IPTR)xhciregs + xhciECPOff);
        ULONG caphdr = AROS_LE2LONG(*capreg);
        ULONG nextcap = (caphdr >> XHCIS_EXT_CAP_NEXT) & XHCI_EXT_CAP_NEXT_MASK;
        UBYTE capid = caphdr & 0xFF;

        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  ExtCap @%04lx: ID=%02x next=%02lx" DEBUGCOLOR_RESET" \n",
                        xhciECPOff, capid, nextcap);

        if (capid == XHCI_EXT_CAP_ID_LEGACY_SUPPORT) {
            xhciUSBLegSup = caphdr;
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  xhciUSBLegSup = $%08x" DEBUGCOLOR_RESET" \n", xhciUSBLegSup);

            if (xhciUSBLegSup & XHCIF_USBLEGSUP_BIOSOWNED) {
                ULONG ownershipval = xhciUSBLegSup | XHCIF_USBLEGSUP_OSOWNED;

                pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Taking ownership of XHCI from BIOS" DEBUGCOLOR_RESET" \n");
takeownership:
                cnt = 100;
                /*
                 * Change the ownership flag and read back to ensure it is written
                 */
                *capreg = AROS_LONG2LE(ownershipval);
                (void)*capreg;

                /*
                 * Wait for ownership change to take place.
                 * XHCI specification doesn't say how long it can take...
                 */
                while ((AROS_LE2LONG(*capreg) & XHCIF_USBLEGSUP_BIOSOWNED) && (--cnt > 0)) {
                    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Waiting for ownership to change..." DEBUGCOLOR_RESET" \n");
                    uhwDelayMS(10, hu);
                }
                if ((ownershipval != XHCIF_USBLEGSUP_OSOWNED) &&
                    (AROS_LE2LONG(*capreg) & XHCIF_USBLEGSUP_BIOSOWNED)) {
                    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Ownership of XHCI still with BIOS" DEBUGCOLOR_RESET" \n");

                    /* Try to force ownership */
                    ownershipval = XHCIF_USBLEGSUP_OSOWNED;
                    goto takeownership;
                }
            } else if (xhciUSBLegSup & XHCIF_USBLEGSUP_OSOWNED) {
                pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Ownership already with OS!" DEBUGCOLOR_RESET" \n");
            } else {
                pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Forcing ownership of XHCI from (unknown)" DEBUGCOLOR_RESET" \n");
                /* Try to force ownership */
                *capreg = AROS_LONG2LE(XHCIF_USBLEGSUP_OSOWNED);
                (void)*capreg;
            }

            /* Clear the SMI control bits */
            volatile ULONG *smictl = (volatile ULONG *)((IPTR)capreg + 4);
            *smictl = AROS_LONG2LE(0);
            (void)*smictl;
        } else if (capid == XHCI_EXT_CAP_ID_SUPPORTED_PROTOCOL) {
            ULONG capprot = AROS_LE2LONG(*(capreg + 1));
            ULONG capports = AROS_LE2LONG(*(capreg + 2));
            UBYTE major = (capprot >> XHCIS_XCP_REV_MAJOR) & XHCI_XCP_REV_MAJOR_MASK;
            UBYTE minor = (capprot >> XHCIS_XCP_REV_MINOR) & XHCI_XCP_REV_MINOR_MASK;
            UWORD name = (UWORD)(capprot & XHCI_XCP_NAMESTRING_MASK);
            UBYTE portOffset = (capports >> XHCIS_XCP_PORT_OFFSET) & XHCI_XCP_PORT_OFFSET_MASK;
            UBYTE portCount = (capports >> XHCIS_XCP_PORT_COUNT) & XHCI_XCP_PORT_COUNT_MASK;

            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "  XCP protocol '%c%c' rev %u.%u ports %u..%u"
                            DEBUGCOLOR_RESET" \n",
                            (name & 0xFF),
                            (name >> 8) & 0xFF,
                            major, minor,
                            portOffset,
                            (UWORD)(portOffset + portCount - 1));

            if (portOffset > 0 && portCount > 0) {
                UWORD start = (UWORD)(portOffset - 1);
                UWORD end = (UWORD)(start + portCount);
                UBYTE proto = XHCI_PORT_PROTOCOL_UNKNOWN;

                if (major >= XHCI_PORT_PROTOCOL_USB3) {
                    proto = XHCI_PORT_PROTOCOL_USB3;
                } else if (major == XHCI_PORT_PROTOCOL_USB2) {
                    proto = XHCI_PORT_PROTOCOL_USB2;
                }

                if (proto != XHCI_PORT_PROTOCOL_UNKNOWN) {
                    for (UWORD port = start; port < end && port < xhciPortLimit; port++) {
                        xhcic->xhc_PortProtocol[port] = proto;
                    }
                    xhcic->xhc_PortProtocolValid = TRUE;
                }
            }
        }

        if (nextcap == 0) {
            break;
        }
        xhciECPOff = nextcap << 2;
    }

    UWORD xhciversion;
    char *controllername = &hc->hc_Node.ln_Name[16];
    xhciversion = AROS_LE2LONG(xhciregs->hciversion) & 0xFFFF;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  HCIVERSION: 0x%04x" DEBUGCOLOR_RESET" \n", xhciversion);
    if (xhciversion == 0x0090)
        controllername[10] = '0';
    else if (xhciversion == 0x0100)
        controllername[10] = '1';
    else if (xhciversion == 0x0320)
        controllername[10] = '2';

    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  HCSPARAMS1: 0x%08x (MaxIntr %lu)" DEBUGCOLOR_RESET" \n", hcsparams1, xhciMaxIntrs);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  HCSPARAMS2: 0x%08x" DEBUGCOLOR_RESET" \n", hcsparams2);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  HCSPARAMS3: 0x%08x" DEBUGCOLOR_RESET" \n", hcsparams3);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  HCCPARAMS1: 0x%08x" DEBUGCOLOR_RESET" \n", hccparams1);
    pciusbXHCIDebugV("xHCI",
                    DEBUGCOLOR_SET "  HCCPARAMS2: 0x%08x (PRS=%u CPSM=%u)" DEBUGCOLOR_RESET" \n",
                    hccparams2,
                    (hccparams2 & XHCIF_HCCPARAMS2_PRS) ? 1 : 0,
                    (hccparams2 & XHCIF_HCCPARAMS2_CPSM) ? 1 : 0);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  OPR.CONFIG: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->config));

    hc->hc_NumPorts = (ULONG)((hcsparams1 >> 24) & 0xFF);
    xhcic->xhc_NumSlots = (ULONG)(hcsparams1 & 0xFF);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%d ports, %d slots" DEBUGCOLOR_RESET" \n",
                    hc->hc_NumPorts, xhcic->xhc_NumSlots);

    if (hccparams1 & XHCIF_HCCPARAMS1_CSZ)
        hc->hc_Flags |= HCF_CTX64;
    if (hccparams1 & XHCIF_HCCPARAMS1_AC64)
        hc->hc_Flags |= HCF_ADDR64;
    if (hccparams1 & XHCIF_HCCPARAMS1_PPC)
        hc->hc_Flags |= HCF_PPC;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%d byte context(s), %ubit addressing" DEBUGCOLOR_RESET" \n",
                    (hc->hc_Flags & HCF_CTX64) ? 64 : 32,
                    (hc->hc_Flags & HCF_ADDR64) ? 64 : 32);

    /* Device Context Base Address Array (Chapter 6.1) */
    xhcic->xhc_DCBAAp = pciAllocAligned(hc, &xhcic->xhc_DCBAA,
                                    sizeof(UQUAD) * (xhcic->xhc_NumSlots + 1),
                                    ALIGN_DCBAA,
                                    xhciPageSize(hc));
    if (xhcic->xhc_DCBAAp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated DCBAA @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_DCBAAp, xhcic->xhc_DCBAA.me_Un.meu_Addr, xhcic->xhc_DCBAA.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMADCBAA = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)xhcic->xhc_DCBAAp);
#else
        xhcic->xhc_DMADCBAA = xhcic->xhc_DCBAAp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMADCBAA);
        memset(xhcic->xhc_DCBAAp, 0, sizeof(UQUAD) * (xhcic->xhc_NumSlots + 1));
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate DCBAA DMA Memory" DEBUGCOLOR_RESET" \n");
        goto init_fail;
    }

    /* Event Ring Segment Table (Chapter 6.5) */
    xhcic->xhc_ERSTp = pciAllocAligned(hc, &xhcic->xhc_ERST,
                                   sizeof(struct xhci_er_seg),
                                   ALIGN_EVTRING_TBL,
                                   ALIGN_EVTRING_TBL);
    if (xhcic->xhc_ERSTp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated ERST @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_ERSTp, xhcic->xhc_ERST.me_Un.meu_Addr, xhcic->xhc_ERST.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMAERST = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)xhcic->xhc_ERSTp);
#else
        xhcic->xhc_DMAERST = xhcic->xhc_ERSTp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERST);
        memset((void *)xhcic->xhc_ERSTp, 0, sizeof(struct xhci_er_seg));
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate ERST DMA Memory" DEBUGCOLOR_RESET" \n");
        goto init_fail;
    }

    /* Command Ring */
    xhcic->xhc_OPRp = pciAllocAligned(hc, &xhcic->xhc_OPR,
                                  sizeof(struct pcisusbXHCIRing),
                                  XHCI_RING_ALIGN,
                                  (1 << 16));
    if (xhcic->xhc_OPRp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated OPR @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_OPRp, xhcic->xhc_OPR.me_Un.meu_Addr, xhcic->xhc_OPR.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMAOPR = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)xhcic->xhc_OPRp);
#else
        xhcic->xhc_DMAOPR = xhcic->xhc_OPRp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAOPR);
        xhciInitRing((struct pcisusbXHCIRing *)xhcic->xhc_OPRp);
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate OPR DMA Memory" DEBUGCOLOR_RESET" \n");
        goto init_fail;
    }

    /* Event Ring */
    xhcic->xhc_ERSp = pciAllocAligned(hc, &xhcic->xhc_ERS,
                                  sizeof(struct pcisusbXHCIRing),
                                  XHCI_RING_ALIGN,
                                  (1 << 16));
    if (xhcic->xhc_ERSp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated ERS @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_ERSp, xhcic->xhc_ERS.me_Un.meu_Addr, xhcic->xhc_ERS.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMAERS = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)xhcic->xhc_ERSp);
#else
        xhcic->xhc_DMAERS = xhcic->xhc_ERSp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERS);
        xhciInitRing((struct pcisusbXHCIRing *)xhcic->xhc_ERSp);
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate ERS DMA Memory" DEBUGCOLOR_RESET" \n");
        goto init_fail;
    }

    /* Scratchpad buffer count decode (Hi/Lo per spec) */
    val = hcsparams2;
    {
        ULONG sp_lo = (val >> 21) & 0x1F;
        ULONG sp_hi = (val >> 27) & 0x1F;
        xhcic->xhc_NumScratchPads = (sp_hi << 5) | sp_lo;
    }
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "SPB = %u" DEBUGCOLOR_RESET" \n", xhcic->xhc_NumScratchPads);

    if (xhcic->xhc_NumScratchPads) {
        ULONG pagesize   = xhciPageSize(hc);
        ULONG spba_size  = sizeof(struct xhci_address) * xhcic->xhc_NumScratchPads;
        ULONG spb_size   = pagesize * xhcic->xhc_NumScratchPads;

        xhcic->xhc_SPBAp = pciAllocAligned(hc, &xhcic->xhc_SPBA,
                                       spba_size,
                                       ALIGN_SPBA,
                                       pagesize);
        if (!xhcic->xhc_SPBAp) {
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate SPBA DMA Memory" DEBUGCOLOR_RESET" \n");
            goto init_fail;
        }

#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMASPBA = CPUTOPCI(hc, hc->hc_PCIDriverObject, xhcic->xhc_SPBAp);
#else
        xhcic->xhc_DMASPBA = xhcic->xhc_SPBAp;
#endif
        memset(xhcic->xhc_SPBAp, 0, spba_size);

        xhcic->xhc_SPBuffersp = pciAllocAligned(hc, &xhcic->xhc_SPBuffers,
                                            spb_size,
                                            pagesize,
                                            pagesize);
        if (!xhcic->xhc_SPBuffersp) {
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate Scratchpad Buffers" DEBUGCOLOR_RESET" \n");
            if (xhcic->xhc_SPBA.me_Un.meu_Addr)
                FREEPCIMEM(hc, hc->hc_PCIDriverObject, xhcic->xhc_SPBA.me_Un.meu_Addr);
            goto init_fail;
        }

#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMASPBuffers = CPUTOPCI(hc, hc->hc_PCIDriverObject, xhcic->xhc_SPBuffersp);
#else
        xhcic->xhc_DMASPBuffers = xhcic->xhc_SPBuffersp;
#endif
        memset(xhcic->xhc_SPBuffersp, 0, spb_size);

        {
            volatile struct xhci_address *spba = (volatile struct xhci_address *)xhcic->xhc_SPBAp;
            ULONG i;
            for (i = 0; i < xhcic->xhc_NumScratchPads; i++) {
                xhciSetPointer(hc, spba[i],
                               (IPTR)xhcic->xhc_DMASPBuffers + ((IPTR)pagesize * i));
            }
        }

        xhciSetPointer(hc,
                       ((volatile struct xhci_address *)xhcic->xhc_DCBAAp)[0],
                       xhcic->xhc_DMASPBA);

        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Scratch buffers allocated (%u x 0x%x)" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_NumScratchPads, pagesize);
    }

    /* install reset handler */
    hc->hc_ResetInt.is_Node.ln_Name = "XHCI PCI (pciusb.device)";
    hc->hc_ResetInt.is_Code = (VOID_FUNC)XhciResetHandler;
    hc->hc_ResetInt.is_Data = hc;
    AddResetCallback(&hc->hc_ResetInt);

    IPTR pciIntLine = 0;
    BOOL use_msi = FALSE;
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_INTLine, &pciIntLine);
    hc->hc_PCIIntLine = pciIntLine;

    if (use_msi) {
        struct TagItem vectreqs[] = {
            { tHidd_PCIVector_Min, 1 },
            { tHidd_PCIVector_Max, 1 },
            { TAG_DONE, 0 }
        };

        if (HIDD_PCIDevice_ObtainVectors(hc->hc_PCIDeviceObject, vectreqs)) {
            struct TagItem vecAttribs[] = {
                { tHidd_PCIVector_Int, (IPTR)-1 },
                { TAG_DONE, 0 }
            };
            HIDD_PCIDevice_GetVectorAttribs(hc->hc_PCIDeviceObject, 0, vecAttribs);
            if (vecAttribs[0].ti_Data != (IPTR)-1) {
                hc->hc_PCIIntLine = vecAttribs[0].ti_Data;
                hc->hc_Flags |= HCF_MSI;
            } else {
                HIDD_PCIDevice_ReleaseVectors(hc->hc_PCIDeviceObject);
            }
        }
    }
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "IRQ = %u%s" DEBUGCOLOR_RESET" \n",
                    hc->hc_PCIIntLine, (hc->hc_Flags & HCF_MSI) ? " (MSI)" : "");

    /* add interrupt */
    hc->hc_PCIIntHandler.is_Node.ln_Name = hc->hc_ResetInt.is_Node.ln_Name;
    hc->hc_PCIIntHandler.is_Node.ln_Pri  = 5;
    hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_PCIIntHandler.is_Code         = (VOID_FUNC)xhciIntCode;
    hc->hc_PCIIntHandler.is_Data         = hc;
    if (hc->hc_Flags & HCF_MSI) {
        AddIntServer(INTB_KERNEL + hc->hc_PCIIntLine, &hc->hc_PCIIntHandler);
    } else {
        PCIXAddInterrupt(hc, &hc->hc_PCIIntHandler);
    }

    for (cnt = 0; cnt < hc->hc_NumPorts; cnt++) {
        hu->hu_PortMapX[cnt] = hc;
        hc->hc_PortNum[cnt]  = cnt;
    }

    xhciReset(hc, hu);

    /* Ensure ports are powered per xHCI spec before enabling interrupts */
    xhciPowerOnRootPorts(hc, hu);

    /* Enable interrupts in the xhci */
    val = AROS_LE2LONG(hcopr->usbcmd);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Enabling xHCI interrupts..." DEBUGCOLOR_RESET" \n");
    val |= XHCIF_USBCMD_INTE;
    hcopr->usbcmd = AROS_LONG2LE(val);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBCMD = $%08x..." DEBUGCOLOR_RESET" \n",
                    AROS_LE2LONG(hcopr->usbcmd));

    /* Wait for the interrupt enable bit to be visible (defensive) */
    cnt = 100;
    while (!(AROS_LE2LONG(hcopr->usbcmd) & XHCIF_USBCMD_INTE) && (--cnt > 0))
        uhwDelayMS(2, hu);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "COMMAND = $%08x, after %ums" DEBUGCOLOR_RESET" \n",
                    AROS_LE2LONG(hcopr->usbcmd), (100 - cnt) << 1);

    /* Wait for the controller to finish coming out of reset (CNR=0) */
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_CNR) && (--cnt > 0))
        uhwDelayMS(2, hu);

    /*
     * Check controller health before letting it run so that fatal errors
     * short-circuit initialisation rather than causing obscure failures later.
     */
    val = AROS_LE2LONG(hcopr->usbsts);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBSTS after interrupt enable = $%08x" DEBUGCOLOR_RESET" \n", val);
    xhciDumpStatus(val);
    if (val & (XHCIF_USBSTS_HCE | XHCIF_USBSTS_HSE)) {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "Controller reports fatal error (USBSTS=%08x), aborting init" DEBUGCOLOR_RESET" \n", val);
        goto init_fail;
    }
    if (val & XHCIF_USBSTS_HCH)
        pciusbXHCIDebug("xHCI", DEBUGWARNCOLOR_SET "Controller in halted state after interrupt enable" DEBUGCOLOR_RESET" \n");
    if (val & XHCIF_USBSTS_CNR)
        pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "Warning: controller is not ready after reset" DEBUGCOLOR_RESET" \n");

#if (1)
    ULONG sigmask = SIGF_SINGLE;
    xhcic->xhc_ReadySignal = SIGB_SINGLE;
    xhcic->xhc_ReadySigTask = FindTask(NULL);
    SetSignal(0, sigmask);

    struct Task *tmptask;
    char buf[64];
    psdSafeRawDoFmt(buf, 64, "usbhw<pciusb.device/%ld> Event Ring Task", hu->hu_UnitNo);
    if ((tmptask = psdSpawnSubTask(buf, xhciEventRingTask, hc))) {
        sigmask = Wait(sigmask);
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Event Ring Task @ 0x%p, Sig = %u" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_xHCERTask, xhcic->xhc_DoWorkSignal);
    }
    psdSafeRawDoFmt(buf, 64, "usbhw<pciusb.device/%ld> Port Task", hu->hu_UnitNo);
    if ((tmptask = psdSpawnSubTask(buf, xhciPortTask, hc))) {
        sigmask = Wait(sigmask);
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Port Task @ 0x%p, Sig = %u" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_xHCPortTask, xhcic->xhc_PortChangeSignal);
    }
    xhcic->xhc_ReadySigTask = NULL;
#endif

    /* Enable the interrupter to generate interrupts */
    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)xhcic->xhc_XHCIIntR);
    xhciir->iman = AROS_LONG2LE(XHCIF_IR_IMAN_IE);

    /* Finally, set the "run" bit */
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Starting controller run state..." DEBUGCOLOR_RESET" \n");
    val = AROS_LE2LONG(hcopr->usbcmd);
    val |= XHCIF_USBCMD_RS | XHCIF_USBCMD_INTE;
    hcopr->usbcmd = AROS_LONG2LE(val);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBCMD = $%08x..." DEBUGCOLOR_RESET" \n",
                    AROS_LE2LONG(hcopr->usbcmd));

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "xhciInit returns TRUE..." DEBUGCOLOR_RESET" \n");
    return TRUE;

init_fail:
    if (xhcic) {
        FreeMem(xhcic, sizeof(*xhcic));
        hc->hc_CPrivate = NULL;
    }
    return FALSE;
}

void xhciFree(struct PCIController *hc, struct PCIUnit *hu)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);

    if (xhcic) {
        FreeMem(xhcic, sizeof(*xhcic));
        hc->hc_CPrivate = NULL;
    }
}

static void xhciFreeEndpointContext(struct PCIController *hc,
                                    struct pciusbXHCIDevice *devCtx,
                                    ULONG epid,
                                    BOOL stopEndpoint)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p, %u)" DEBUGCOLOR_RESET" \n", __func__, hc, devCtx, epid);

    if (!devCtx || (epid >= MAX_DEVENDPOINTS))
        return;

    if (devCtx->dc_EPAllocs[epid].dmaa_Ptr) {
        /*
         * Spec-safe teardown: stop/reset the endpoint and drop it from the
         * device context before freeing the transfer ring memory.
         *
         * This avoids controllers/emulators fetching TRBs from freed memory.
         */
        if (stopEndpoint && devCtx->dc_SlotID) {
            (void)xhciCmdEndpointStop(hc, devCtx->dc_SlotID, epid, TRUE);
            (void)xhciCmdEndpointReset(hc, devCtx->dc_SlotID, epid, 0);

            /*
             * EP0 (EPID=1) is mandatory and must not be dropped via Configure
             * Endpoint. Only drop non-control endpoints.
             */
            if ((epid > 1) && devCtx->dc_IN.dmaa_Ptr && devCtx->dc_IN.dmaa_DMA) {
                volatile struct xhci_inctx *in =
                    (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
                UWORD ctxoff = 1;

                if (hc->hc_Flags & HCF_CTX64)
                    ctxoff <<= 1;

                /* Clear Add/Drop flags then request a drop for this EP. */
                in->dcf = 0;
                in->acf = 0;

                in->dcf |= (1UL << epid);
                in->acf |= 0x01; /* slot context */

                /* Preserve current slot context state (address, route, etc.). */
                if (devCtx->dc_SlotCtx.dmaa_Ptr) {
                    volatile struct xhci_slot *islot = xhciInputSlotCtx(in, ctxoff);
                    volatile struct xhci_slot *oslot =
                        (volatile struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;

                    CopyMem((const void *)oslot, (void *)islot, sizeof(*oslot));
                }

                /* Ensure the controller observes the updated input context. */
                CacheClearE((APTR)in,
                            devCtx->dc_IN.dmaa_Entry.me_Length,
                            CACRF_ClearD);

                LONG cc = xhciCmdEndpointConfigure(hc,
                                                  devCtx->dc_SlotID,
                                                  devCtx->dc_IN.dmaa_Ptr);
                if (cc != TRB_CC_SUCCESS) {
                    pciusbWarn("xHCI",
                        DEBUGWARNCOLOR_SET
                        "Configure Endpoint (drop EPID %lu) failed, cc=%ld"
                        DEBUGCOLOR_RESET" \n",
                        (unsigned long)epid, (long)cc);
                }
            }
        }

        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_EPAllocs[epid].dmaa_Entry.me_Un.meu_Addr);
        devCtx->dc_EPAllocs[epid].dmaa_Entry.me_Un.meu_Addr = NULL;
        devCtx->dc_EPAllocs[epid].dmaa_Ptr = NULL;
        devCtx->dc_EPAllocs[epid].dmaa_DMA = NULL;
    }

    if (devCtx->dc_EPContexts[epid]) {
        FreeMem(devCtx->dc_EPContexts[epid], sizeof(*devCtx->dc_EPContexts[epid]));
        devCtx->dc_EPContexts[epid] = NULL;
    }
}

void xhciFreeDeviceCtx(struct PCIController *hc,
                              struct pciusbXHCIDevice *devCtx,
                              BOOL disableSlot)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, devCtx);

    if (!devCtx)
        return;

    if (disableSlot && devCtx->dc_SlotID)
        xhciCmdSlotDisable(hc, devCtx->dc_SlotID);

    if ((devCtx->dc_SlotID > 0) && (devCtx->dc_SlotID < USB_DEV_MAX)) {
        if (xhcic->xhc_Devices[devCtx->dc_SlotID] == devCtx)
            xhcic->xhc_Devices[devCtx->dc_SlotID] = NULL;
    }

    if (devCtx->dc_SlotID)
        xhciSetPointer(hc, ((volatile struct xhci_address *)xhcic->xhc_DCBAAp)[devCtx->dc_SlotID], 0);

    for (ULONG epid = 0; epid < MAX_DEVENDPOINTS; epid++)
        xhciFreeEndpointContext(hc, devCtx, epid, FALSE);

    if (devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr) {
        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr);
        devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr = NULL;
        devCtx->dc_IN.dmaa_Ptr = NULL;
        devCtx->dc_IN.dmaa_DMA = NULL;
    }

    if (devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr) {
        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr);
        devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr = NULL;
        devCtx->dc_SlotCtx.dmaa_Ptr = NULL;
        devCtx->dc_SlotCtx.dmaa_DMA = NULL;
    }

    FreeMem(devCtx, sizeof(struct pciusbXHCIDevice));
}

#endif /* PCIUSB_ENABLEXHCI */
