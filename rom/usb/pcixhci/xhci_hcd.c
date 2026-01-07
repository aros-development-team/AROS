/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver main pciusb interface
*/

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

#define XHCI_ROOT_HUB_HANDLE ((struct pciusbXHCIDevice *)-1)

#if defined(PCIUSB_XHCI_DEBUG) && defined(XHCI_ENABLE_CMDFAIL_DEBUG)
#define XHCI_CMDFAIL_LOGGING 1
#endif

static char xhciCompleteIntName[] = "xHCI CompleteInt";
static char xhciResetIntName[] = "xHCI PCI (pcixhci.device)";
static char xhciEndpointTimerName[] = "xHCI endpoint";
static char xhciEventRingTaskNameFmt[] = "usbhw<pcixhci.device/%ld> Event Ring Task";
static char xhciPortTaskNameFmt[] = "usbhw<pcixhci.device/%ld> Port Task";
static const char strPoseidonLibraryName[] = "poseidon.library";

static void xhciFreeEndpointContext(struct PCIController *hc,
                                    struct pciusbXHCIDevice *devCtx,
                                    ULONG epid,
                                    BOOL stopEndpoint,
                                    struct timerequest *timerreq);

static BOOL xhciDeviceHasEndpoints(const struct pciusbXHCIDevice *devCtx)
{
    if(!devCtx)
        return FALSE;

    for(ULONG epid = 0; epid < MAX_DEVENDPOINTS; epid++) {
        if(devCtx->dc_EPAllocs[epid].dmaa_Ptr || devCtx->dc_EPContexts[epid])
            return TRUE;
    }

    return FALSE;
}

static struct PCIController *xhciGetController(struct PCIUnit *unit)
{
    struct PCIController *hc;

    ForeachNode(&unit->hu_Controllers, hc) {
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

    /*
     * Called from the interrupt handler.
     * Do not Disable/Enable here: enabling interrupts inside an ISR can
     * lead to reentrancy and corrupt shared driver state.
     */
    framecnt = AROS_LE2LONG(rrs->mfindex) & 0x3FFF;
    if(framecnt < (hc->hc_FrameCounter & 0x3FFF)) {
        hc->hc_FrameCounter |= 0x3FFF;
        hc->hc_FrameCounter++;
        hc->hc_FrameCounter |= framecnt;
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Frame Counter Rollover %ld" DEBUGCOLOR_RESET "\n", hc->hc_FrameCounter);
    } else {
        hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xFFFFC000) | framecnt;
    }
}

static UBYTE xhciGetEPID(UBYTE endpoint, UBYTE dir)
{
    UBYTE epid = 1;

    if(endpoint > 0) {
        epid = (endpoint & 0X0F) * 2;
        epid += dir;
    }
    return epid;
}

static const char *xhciCmdTypeName(ULONG trbtype)
{
    switch(trbtype) {
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
    const BOOL superspeed = (flags & UHFF_SUPERSPEED) != 0;
    const BOOL highspeed  = (flags & UHFF_HIGHSPEED)  != 0;

    if((type != UHCMD_INTXFER) && (type != UHCMD_ISOXFER))
        return 0;

    if(interval == 0)
        return 0;

    /*
     * xHCI EP Context Interval semantics depend on speed and endpoint type:
     *
     * - HS/SS Interrupt & Isoch: Interval is an exponent in microframes, where
     *   the service interval is 2^(Interval) microframes.  USB bInterval is in
     *   the range 1..16 and directly encodes that exponent (bInterval - 1).
     *
     * - FS Isoch: bInterval is 1..16 and encodes 2^(bInterval-1) frames.
     *   Convert frames to microframes (x8) => exponent = (bInterval - 1) + 3.
     *
     * - FS/LS Interrupt: Interval is the frame count 1..255.
     */
    if(superspeed || highspeed) {
        if(interval > 16)
            interval = 16;
        return (UBYTE)(interval - 1); /* 0..15 */
    }

    if(type == UHCMD_ISOXFER) {
        UWORD exp;

        if(interval > 16)
            interval = 16;

        exp = (interval - 1) + 3; /* frames -> microframes */
        if(exp > 15)
            exp = 15;

        return (UBYTE)exp;
    }

    if(interval > 255)
        interval = 255;

    return (UBYTE)interval;
}

void xhciInitRing(struct PCIController *hc, struct pcisusbXHCIRing *ring)
{
#if defined(DEBUG)
    /*
     * The interrupt handler uses RINGFROMTRB() (masking) to map a TRB pointer
     * back to the containing ring. That requires ring allocations to be
     * aligned to XHCI_RING_ALIGN (see xhci_hcd.h).
     */
    if(((IPTR)ring & (XHCI_RING_ALIGN - 1)) != 0) {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: ring %p is misaligned (need %lu)" DEBUGCOLOR_RESET "\n",
                    ring, (ULONG)XHCI_RING_ALIGN);
    }
#endif
    memset(ring, 0, sizeof(*ring));

    ring->ringio = AllocMem(sizeof(*ring->ringio) * XHCI_EVENT_RING_TRBS, MEMF_ANY | MEMF_CLEAR);
    if(!ring->ringio) {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: unable to allocate ringio for ring %p" DEBUGCOLOR_RESET "\n",
                    ring);
        /* ring remains usable for sync/diagnostic paths, but IO correlation will suffer */
    }
    pciusbXHCIDebugRIO("xHCI",
                       DEBUGWARNCOLOR_SET "xHCI: ringio @ 0x%p" DEBUGCOLOR_RESET "\n",
                       ring->ringio);
    ring->end = RINGENDCFLAG; /* set initial cycle bit */
}

static void xhciPowerOnRootPorts(struct PCIController *hc, struct PCIUnit *hu,
                                 struct timerequest *timerreq)
{
    if(!(hc->hc_Flags & HCF_PPC))
        return;

    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);

    for(UWORD hciport = 0; hciport < hc->hc_NumPorts; hciport++) {
        ULONG origportsc = AROS_LE2LONG(xhciports[hciport].portsc);
        ULONG newportsc  = origportsc & ~(XHCIF_PR_PORTSC_OCC | XHCIF_PR_PORTSC_PRC |
                                          XHCIF_PR_PORTSC_WRC | XHCIF_PR_PORTSC_PEC |
                                          XHCIF_PR_PORTSC_CSC | XHCIF_PR_PORTSC_PLC |
                                          XHCIF_PR_PORTSC_CEC);

        /* Skip ports that are already powered */
        if(origportsc & XHCIF_PR_PORTSC_PP)
            continue;

        newportsc |= XHCIF_PR_PORTSC_PP;
        xhciports[hciport].portsc = AROS_LONG2LE(newportsc);

        /* Wait for power to latch before continuing bring-up */
        for(ULONG waitms = 0; waitms < 20; waitms++) {
            if(AROS_LE2LONG(xhciports[hciport].portsc) & XHCIF_PR_PORTSC_PP)
                break;
            uhwDelayMS(1, timerreq);
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
    /*
     * DevAddr==0 is inherently ambiguous once more than one device/slot exists.
     * Callers that need a DevAddr0 context must resolve by (route, root port)
     * via xhciFindRouteDevice()/xhciObtainDeviceCtx().
     */
    if(devaddr == 0)
        return NULL;

    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    UWORD maxslot = xhcic->xhc_NumSlots;
    if(maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for(UWORD slot = 1; slot <= maxslot; slot++) {
        struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slot];

        if(!devCtx)
            continue;

        if(devCtx->dc_DevAddr == devaddr)
            return devCtx;
    }
    return NULL;
}

struct pciusbXHCIDevice *xhciFindRouteDevice(struct PCIController *hc,
        ULONG route,
        UWORD rootPortIndex)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    UWORD maxslot = xhcic->xhc_NumSlots;

    if(maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for(UWORD slot = 1; slot <= maxslot; slot++) {
        struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slot];

        if(!devCtx)
            continue;

        if((devCtx->dc_RouteString == (route & SLOT_CTX_ROUTE_MASK)) &&
                (devCtx->dc_RootPort == rootPortIndex))
            return devCtx;
    }

    return NULL;
}

static struct pciusbXHCIDevice *xhciFindPortDevice(struct PCIController *hc, UWORD hciport)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    UWORD maxslot = xhcic->xhc_NumSlots;

    if(maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for(UWORD slot = 1; slot <= maxslot; slot++) {
        struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slot];

        if(devCtx && (devCtx->dc_RootPort == hciport))
            return devCtx;
    }

    return NULL;
}

static BOOL xhciIOReqMatchesDevice(const struct IOUsbHWReq *ioreq,
                                   const struct pciusbXHCIDevice *devCtx)
{
    if(!ioreq || !devCtx)
        return FALSE;

    struct pciusbXHCIIODevPrivate *driprivate =
        (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;

    if(driprivate && driprivate->dpDevice == devCtx)
        return TRUE;

    if(devCtx->dc_DevAddr && (ioreq->iouh_DevAddr == devCtx->dc_DevAddr))
        return TRUE;

    if(ioreq->iouh_RootPort &&
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
    while((ionext = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ)) {
        if(!xhciIOReqMatchesDevice(ioreq, devCtx)) {
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

        if(periodic)
            xhciFreePeriodicContext(hc, unit, ioreq);
        else
            xhciFreeAsyncContext(hc, unit, ioreq);

        ReplyMsg(&ioreq->iouh_Req.io_Message);

        ioreq = ionext;
    }
}

void xhciDisconnectDevice(struct PCIController *hc, struct pciusbXHCIDevice *devCtx,
                          struct timerequest *timerreq)
{
    struct PCIUnit *unit;

    if(!hc || !devCtx)
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

    if(devCtx->dc_DevAddr < USB_DEV_MAX &&
            unit->hu_DevControllers[devCtx->dc_DevAddr] == hc) {
        unit->hu_DevControllers[devCtx->dc_DevAddr] = NULL;
    }
    if(devCtx->dc_RouteString == 0 &&
            unit->hu_DevControllers[0] == hc) {
        unit->hu_DevControllers[0] = NULL;
    }

    xhciFreeDeviceCtx(hc, devCtx, TRUE, timerreq);
}

static int xhciRingEntriesFree(volatile struct pcisusbXHCIRing *ring)
{
    /* Caller must hold the ring lock. */
    ULONG last = (ring->end & ~RINGENDCFLAG);
    ULONG idx = ring->next;

    return (last > idx) ? last - idx
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
    if(ring->end & RINGENDCFLAG)
        trbflags |= TRBF_FLAG_C;

    /* Next available TRB slot */
    dst = &ring->ring[ring->next];

    /*
     * Parameter field handling:
     *  - IDT set: payload is immediate data (8 bytes), not an address.
     *  - IDT clear: payload is a CPU pointer that must become a bus address.
     */
    if(trflags & TRBF_FLAG_IDT) {
        dst->dbp.addr_lo = AROS_LONG2LE((ULONG)(payload & 0xffffffffUL));
        dst->dbp.addr_hi = AROS_LONG2LE((ULONG)((payload >> 32) & 0xffffffffUL));
    } else {
        UQUAD dma_payload = payload;
        const ULONG trbtype = (trflags >> TRBS_FLAG_TYPE) & TRB_FLAG_TYPE_SMASK;
        const BOOL raw_dma  = (trbtype == TRBB_FLAG_CRTYPE_SET_TR_DEQUEUE_PTR);

#if !defined(PCIUSB_NO_CPUTOPCI)
        if(!raw_dma && payload != 0) {
            dma_payload = (UQUAD)(IPTR)CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)(IPTR)payload);
        }
#endif
        /*
         * Set TR Dequeue Pointer uses a DMA address with embedded flag bits
         * (e.g. DCS in bit 0). Do not treat that value as a CPU pointer.
         */
        xhciSetPointer(hc, dst->dbp, dma_payload);
    }

    dst->tparams = AROS_LONG2LE(plen & TRB_TPARAMS_DS_TRBLEN_SMASK);
    dst->flags   = AROS_LONG2LE(trbflags);

    /* Ensure the controller observes the freshly written TRB */
    CacheClearE((APTR)dst, sizeof(*dst), CACRF_ClearD);
}

static inline BOOL xhciRingioMatchesIOReq(volatile struct pcisusbXHCIRing *ring, UWORD idx, APTR ioreq)
{
    if(!ring || !ring->ringio)
        return FALSE;

    return (ring->ringio[idx] == ioreq);
}

/* ------------------------------------------------------------------------- */
/* Diagnostics (rate-limited)                                                */
/* ------------------------------------------------------------------------- */

#define XHCI_DIAG_DUMP_RADIUS  4
#define XHCI_DIAG_DUMP_LIMIT   3

static UBYTE xhci_diag_missing_ioreq[USB_DEV_MAX][MAX_DEVENDPOINTS];
static UBYTE xhci_diag_cc_err[USB_DEV_MAX][MAX_DEVENDPOINTS];

static const char *xhciDiagCCName(ULONG cc)
{
    switch(cc) {
    case TRB_CC_SUCCESS:
        return "SUCCESS";
    case TRB_CC_USB_TRANSACTION_ERROR:
        return "USB_TRANSACTION_ERROR";
    case TRB_CC_TRB_ERROR:
        return "TRB_ERROR";
    case TRB_CC_STALL_ERROR:
        return "STALL_ERROR";
    case TRB_CC_RING_UNDERRUN:
        return "RING_UNDERRUN";
    case TRB_CC_RING_OVERRUN:
        return "RING_OVERRUN";
    case TRB_CC_EVENT_RING_FULL_ERROR:
        return "EVENT_RING_FULL";
    default:
        return "OTHER";
    }
}

static void xhciDiagDumpRingWindow(struct PCIController *hc,
                                   volatile struct pcisusbXHCIRing *ring,
                                   ULONG center,
                                   ULONG radius,
                                   const char *tag)
{
    (void)hc;

    if(!ring)
        return;

    ULONG start = (center > radius) ? (center - radius) : 0;
    ULONG end   = center + radius;
    if(end >= XHCI_EVENT_RING_TRBS)
        end = XHCI_EVENT_RING_TRBS - 1;

    ULONG ccs = (ring->end & RINGENDCFLAG) ? 1 : 0;

    pciusbWarn("xHCI",
               DEBUGWARNCOLOR_SET
               "DIAG[%s]: ring=%p next=%u end=%u(cyc=%lu) window=[%lu..%lu]"
               DEBUGCOLOR_RESET" \n",
               tag ? tag : "",
               ring,
               (unsigned)ring->next,
               (unsigned)(ring->end & ~RINGENDCFLAG),
               (unsigned long)ccs,
               (unsigned long)start,
               (unsigned long)end);

    for(ULONG i = start; i <= end; i++) {
        volatile struct xhci_trb *t = &ring->ring[i];
        CacheClearE((APTR)t, sizeof(*t), CACRF_InvalidateD);

        ULONG a0 = AROS_LE2LONG(t->dbp.addr_lo);
        ULONG a1 = AROS_LE2LONG(t->dbp.addr_hi);
        ULONG p2 = AROS_LE2LONG(t->tparams);
        ULONG f3 = AROS_LE2LONG(t->flags);

        ULONG type = (f3 >> TRBS_FLAG_TYPE) & TRB_FLAG_TYPE_SMASK;
        ULONG cyc  = (f3 & TRBF_FLAG_C)   ? 1 : 0;
        ULONG ioc  = (f3 & TRBF_FLAG_IOC) ? 1 : 0;
        ULONG ch   = (f3 & TRBF_FLAG_CH)  ? 1 : 0;
        ULONG idt  = (f3 & TRBF_FLAG_IDT) ? 1 : 0;
        ULONG isp  = (f3 & TRBF_FLAG_ISP) ? 1 : 0;
        ULONG len  = (p2 & TRB_TPARAMS_DS_TRBLEN_SMASK);

        pciusbWarn("xHCI",
                   DEBUGWARNCOLOR_SET
                   "  TRB[%3lu] io=%p addr=%08lx:%08lx p2=%08lx f3=%08lx type=%lu cyc=%lu ioc=%lu ch=%lu idt=%lu isp=%lu len=%lu"
                   DEBUGCOLOR_RESET" \n",
                   (unsigned long)i,
                   (ring->ringio ? ring->ringio[i] : NULL),
                   (unsigned long)a1, (unsigned long)a0,
                   (unsigned long)p2,
                   (unsigned long)f3,
                   (unsigned long)type,
                   (unsigned long)cyc,
                   (unsigned long)ioc,
                   (unsigned long)ch,
                   (unsigned long)idt,
                   (unsigned long)isp,
                   (unsigned long)len);
    }
}

static void xhciDiagDumpOutputCtx(struct PCIController *hc,
                                  struct pciusbXHCIDevice *devCtx,
                                  UBYTE epid,
                                  const char *tag)
{
    if(!hc || !devCtx || !devCtx->dc_SlotCtx.dmaa_Ptr)
        return;

    const UWORD ctxsize = (hc->hc_Flags & HCF_CTX64) ? 64 : 32;
    const ULONG ndw = (ULONG)(ctxsize / 4);

    volatile UBYTE *obase = (volatile UBYTE *)devCtx->dc_SlotCtx.dmaa_Ptr;
    volatile ULONG *slot  = (volatile ULONG *)obase;
    volatile ULONG *ep    = (volatile ULONG *)(obase + ((UWORD)epid * ctxsize));

    CacheClearE((APTR)slot, ctxsize, CACRF_InvalidateD);
    CacheClearE((APTR)ep,   ctxsize, CACRF_InvalidateD);

    pciusbWarn("xHCI",
               DEBUGWARNCOLOR_SET
               "DIAG[%s]: OutputCtx slot=%u devaddr=%u epid=%u ctxsize=%u"
               DEBUGCOLOR_RESET" \n",
               tag ? tag : "",
               (unsigned)devCtx->dc_SlotID,
               (unsigned)devCtx->dc_DevAddr,
               (unsigned)epid,
               (unsigned)ctxsize);

    /* Slot context (raw dwords) */
    for(ULONG i = 0; i < ndw; i += 4) {
        pciusbWarn("xHCI",
                   DEBUGWARNCOLOR_SET
                   "  SLOT[%2lu..%2lu]=%08lx %08lx %08lx %08lx"
                   DEBUGCOLOR_RESET" \n",
                   (unsigned long)i, (unsigned long)(i + 3),
                   (unsigned long)AROS_LE2LONG(slot[i + 0]),
                   (unsigned long)AROS_LE2LONG(slot[i + 1]),
                   (unsigned long)AROS_LE2LONG(slot[i + 2]),
                   (unsigned long)AROS_LE2LONG(slot[i + 3]));
    }

    /* Endpoint context (raw dwords) */
    for(ULONG i = 0; i < ndw; i += 4) {
        pciusbWarn("xHCI",
                   DEBUGWARNCOLOR_SET
                   "  EP%u[%2lu..%2lu]=%08lx %08lx %08lx %08lx"
                   DEBUGCOLOR_RESET" \n",
                   (unsigned)epid,
                   (unsigned long)i, (unsigned long)(i + 3),
                   (unsigned long)AROS_LE2LONG(ep[i + 0]),
                   (unsigned long)AROS_LE2LONG(ep[i + 1]),
                   (unsigned long)AROS_LE2LONG(ep[i + 2]),
                   (unsigned long)AROS_LE2LONG(ep[i + 3]));
    }
}

#define XHCI_EP_STATE_DISABLED 0
#define XHCI_EP_STATE_RUNNING  1
#define XHCI_EP_STATE_HALTED   2
#define XHCI_EP_STATE_STOPPED  3
#define XHCI_EP_STATE_ERROR    4

static inline UBYTE xhciEndpointState(volatile struct xhci_ep *epctx)
{
    return (UBYTE)(AROS_LE2LONG(epctx->ctx[0]) & 0x7U);
}

static const char *xhciEPStateName(UBYTE st)
{
    switch(st) {
    case XHCI_EP_STATE_DISABLED:
        return "Disabled";
    case XHCI_EP_STATE_RUNNING:
        return "Running";
    case XHCI_EP_STATE_HALTED:
        return "Halted";
    case XHCI_EP_STATE_STOPPED:
        return "Stopped";
    case XHCI_EP_STATE_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

static void xhciDiagDumpEndpointBrief(struct PCIController *hc,
                                      struct pciusbXHCIDevice *devCtx,
                                      UBYTE epid,
                                      const char *reason)
{
    if(!hc || !devCtx || !devCtx->dc_SlotCtx.dmaa_Ptr)
        return;

    if(epid >= MAX_DEVENDPOINTS)
        return;

    const UWORD ctxsize = (hc->hc_Flags & HCF_CTX64) ? 64 : 32;
    volatile UBYTE *obase = (volatile UBYTE *)devCtx->dc_SlotCtx.dmaa_Ptr;
    volatile struct xhci_ep *epctx = (volatile struct xhci_ep *)(obase + ((UWORD)epid * ctxsize));

    CacheClearE((APTR)epctx, ctxsize, CACRF_InvalidateD);

    const ULONG dw0 = AROS_LE2LONG(epctx->ctx[0]);
    const ULONG dw1 = AROS_LE2LONG(epctx->ctx[1]);
    const ULONG dw2 = AROS_LE2LONG(epctx->ctx[2]);
    const ULONG dw3 = AROS_LE2LONG(epctx->ctx[3]);

    const UBYTE epstate   = (UBYTE)(dw0 & 0x7U);
    const UBYTE mult      = (UBYTE)((dw0 >> 8) & 0x3U);
    const UBYTE maxps     = (UBYTE)((dw0 >> 10) & 0x1FU);
    const UBYTE interval  = (UBYTE)((dw0 >> 16) & 0xFFU);

    const UBYTE eptype    = (UBYTE)((dw1 >> 3) & 0x7U);
    const UBYTE maxburst  = (UBYTE)((dw1 >> 8) & 0xFFU);
    const UWORD maxpkt    = (UWORD)((dw1 >> 16) & 0xFFFFU);

    const UQUAD dq_raw = ((UQUAD)dw3 << 32) | (UQUAD)dw2;
    const UQUAD dq_ptr = dq_raw & ~0xFULL;
    const UBYTE dcs    = (UBYTE)(dq_raw & 0x1U);

    pciusbWarn("xHCI",
               DEBUGWARNCOLOR_SET
               "DIAG: EP ctx %s: slot=%u devaddr=%u epid=%u state=%u/%s type=%u maxpkt=%u maxburst=%u mult=%u maxps=%u interval=%u dq=%08lx:%08lx (dcs=%u)"
               DEBUGCOLOR_RESET" \n",
               reason ? reason : "?",
               devCtx->dc_SlotID,
               devCtx->dc_DevAddr,
               epid,
               (unsigned)epstate,
               xhciEPStateName(epstate),
               (unsigned)eptype,
               (unsigned)maxpkt,
               (unsigned)maxburst,
               (unsigned)mult,
               (unsigned)maxps,
               (unsigned)interval,
               (ULONG)((dq_ptr >> 32) & 0xffffffffUL),
               (ULONG)(dq_ptr        & 0xffffffffUL),
               (unsigned)dcs);

    (void)maxps;
}

static BOOL xhciResetEndpointRing(struct PCIController *hc,
                                  struct pciusbXHCIDevice *devCtx,
                                  UBYTE epid,
                                  APTR *dequeue_ptr_out,
                                  BOOL *dcs_out)
{
    (void)hc;

    if(!devCtx || (epid >= MAX_DEVENDPOINTS))
        return FALSE;

    volatile struct pcisusbXHCIRing *ring =
        (volatile struct pcisusbXHCIRing *)devCtx->dc_EPAllocs[epid].dmaa_Ptr;

    if(!ring)
        return FALSE;

    xhciRingLock();

    /*
     * Reset the transfer ring content.  Do not clear ringio here: correlation
     * teardown is handled by the completion paths and by explicit abort logic.
     */
    memset((void *)ring->ring, 0, sizeof(ring->ring));

    /*
     * After clearing, TRBs have C=0. Program the dequeue cycle state to 1 so
     * cleared TRBs are observed as empty until new TRBs are produced with C=1.
     */
    ring->next = 0;
    ring->end &= RINGENDCFLAG;
    ring->end |= RINGENDCFLAG;

    xhciRingUnlock();

    CacheClearE((APTR)ring->ring, sizeof(ring->ring), CACRF_ClearD);

    if(dequeue_ptr_out)
        *dequeue_ptr_out = (APTR)&ring->ring[0];
    if(dcs_out)
        *dcs_out = TRUE;

    return TRUE;
}

WORD xhciQueueTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload,
                  ULONG plen, ULONG trbflags)
{
    WORD queued = -1;

    if(trbflags & TRBF_FLAG_IDT) {
        pciusbXHCIDebugTRBV("xHCI", DEBUGFUNCCOLOR_SET "(0x%p, $%02x %02x %02x%02x %02x%02x %02x%02x, %u, $%08lx)"
                            DEBUGCOLOR_RESET" \n", ring,
                            ((UBYTE *)&payload)[0], ((UBYTE *)&payload)[1], ((UBYTE *)&payload)[3], ((UBYTE *)&payload)[2],
                            ((UBYTE *)&payload)[5], ((UBYTE *)&payload)[4], ((UBYTE *)&payload)[7], ((UBYTE *)&payload)[6],
                            plen, trbflags);
    } else
        pciusbXHCIDebugTRBV("xHCI", DEBUGFUNCCOLOR_SET "(0x%p, 0x%p, %u, $%08lx)" DEBUGCOLOR_RESET" \n", ring, payload, plen,
                            trbflags);

    if(!ring) {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "NO RINGSPECIFIED!!" DEBUGCOLOR_RESET" \n");
        return -1;
    }

    if(!ring->ringio) {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: ring %p has no ringio bookkeeping" DEBUGCOLOR_RESET" \n",
                    ring);
        return -1;
    }

    xhciRingLock();
    if(xhciRingEntriesFree(ring) > 1) {
        if(ring->next >= XHCI_EVENT_RING_TRBS - 1) {
            UQUAD link_dma = (UQUAD)(IPTR)&ring->ring[0];

            /*
             * If this is the last ring element, insert a link
             * back to the ring start - and update the cycle bit
             */
            pciusbXHCIDebugRIO("xHCI",
                               DEBUGWARNCOLOR_SET "xHCI: link <ringio %p>[%d] 0x%p -> 0x0000000000000000" DEBUGCOLOR_RESET "\n",
                               ring->ringio, ring->next, ring->ringio[ring->next]);
            ring->ringio[ring->next] = NULL;
            xhciInsertTRB(hc, ring,
                          link_dma,
                          TRBF_FLAG_TRTYPE_LINK | TRBF_FLAG_TC,
                          0);
            ring->next = 0;
            if(ring->end & RINGENDCFLAG)
                ring->end &= ~RINGENDCFLAG;
            else
                ring->end |= RINGENDCFLAG;
            pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "Ring Re-Linked!!" DEBUGCOLOR_RESET" \n");
        }

        queued = ring->next;
        pciusbXHCIDebugRIO("xHCI",
                           DEBUGWARNCOLOR_SET "xHCI: setting <ringio %p>[%d] 0x%p -> 0x0000000000000000" DEBUGCOLOR_RESET "\n",
                           ring->ringio, queued, ring->ringio[queued]);
        ring->ringio[queued] = NULL;
        xhciInsertTRB(hc, ring, payload, trbflags, plen);
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "ring %p <idx %d, %dbytes>" DEBUGCOLOR_RESET" \n", ring, ring->next, plen);
        ring->next++;
    } else {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "NO SPACE ON RING!! <next = %u, last = %u>" DEBUGCOLOR_RESET" \n",
                    ring->next, (ring->end & ~RINGENDCFLAG));
    }
    xhciRingUnlock();

    return queued;
}

WORD xhciQueueTRB_IO(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload,
                     ULONG plen, ULONG trbflags, struct IORequest *ioreq)
{
    WORD queued = -1;

    if(!ring) {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "NO RINGSPECIFIED!!" DEBUGCOLOR_RESET" \n");
        return -1;
    }

    if(!ring->ringio) {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: ring %p has no ringio bookkeeping" DEBUGCOLOR_RESET" \n",
                    ring);
        return -1;
    }

    xhciRingLock();
    if(xhciRingEntriesFree(ring) > 1) {
        if(ring->next >= XHCI_EVENT_RING_TRBS - 1) {
            UQUAD link_dma = (UQUAD)(IPTR)&ring->ring[0];

            pciusbXHCIDebugRIO("xHCI",
                               DEBUGWARNCOLOR_SET "xHCI: <ringio %p>[%d]  0x%p -> 0x0000000000000000" DEBUGCOLOR_RESET "\n",
                               ring->ringio, ring->next, ring->ringio[ring->next]);
            ring->ringio[ring->next] = NULL;
            xhciInsertTRB(hc, ring,
                          link_dma,
                          TRBF_FLAG_TRTYPE_LINK | TRBF_FLAG_TC,
                          0);
            ring->next = 0;
            if(ring->end & RINGENDCFLAG)
                ring->end &= ~RINGENDCFLAG;
            else
                ring->end |= RINGENDCFLAG;
            pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "Ring Re-Linked!!" DEBUGCOLOR_RESET" \n");
        }

        queued = ring->next;
        pciusbXHCIDebugRIO("xHCI",
                           DEBUGWARNCOLOR_SET "xHCI: <ringio %p>[%d] 0x%p -> 0x%p" DEBUGCOLOR_RESET "\n",
                           ring->ringio, queued, ring->ringio[queued], ioreq);
        ring->ringio[queued] = ioreq;
        xhciInsertTRB(hc, ring, payload, trbflags, plen);
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "ring %p <idx %d, %dbytes>" DEBUGCOLOR_RESET" \n", ring, ring->next, plen);
        ring->next++;
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
    if(remaining == 0) {
        ULONG txflags = base_flags | base_type;

        if(chain_beyond)
            txflags |= TRBF_FLAG_CH;

        if(ioconlast)
            txflags |= TRBF_FLAG_IOC;

        return xhciQueueTRB(hc, ring, payload, 0, txflags);
    }

    while(remaining > 0) {
        const ULONG offset = (plen - remaining);
        ULONG trblen = remaining;
        ULONG txflags;

        if(trblen > segmax)
            trblen = segmax;

        /* Select TRB type for this segment. */
        if(is_ctl_data_stage && offset != 0) {
            /*
             * Subsequent segments of a Control DATA stage use NORMAL TRBs.
             * Clear the DATA-stage-specific direction bit if present.
             */
            txflags = (base_flags & ~TRBF_FLAG_DS_DIR) | TRBF_FLAG_TRTYPE_NORMAL;
        } else {
            txflags = base_flags | base_type;
        }

        /* Chain all but the last segment; optionally chain the final segment too. */
        if(remaining > trblen)
            txflags |= TRBF_FLAG_CH;
        else if(chain_beyond)
            txflags |= TRBF_FLAG_CH;

        /* IOC only on the last segment if requested. */
        if(ioconlast && (remaining == trblen))
            txflags |= TRBF_FLAG_IOC;

        queued = xhciQueueTRB(hc, ring, payload + offset, trblen, txflags);
        if(queued == -1)
            return queued;

        if(firstqueued == -1)
            firstqueued = queued;

        remaining -= trblen;
    }

    return firstqueued;
}

WORD xhciQueueData_IO(struct PCIController *hc,
                      volatile struct pcisusbXHCIRing *ring,
                      UQUAD payload,
                      ULONG plen,
                      ULONG pmax,
                      ULONG trbflags,
                      BOOL  ioconlast,
                      struct IORequest *ioreq)
{
    ULONG remaining = plen;
    WORD  queued, firstqueued = -1;

    const ULONG base_type = (trbflags & TRB_FLAG_TYPE_MASK);
    ULONG base_flags = (trbflags & ~TRB_FLAG_TYPE_MASK);
    const BOOL chain_beyond = (base_flags & TRBF_FLAG_CH) != 0;

    base_flags &= ~(TRBF_FLAG_CH | TRBF_FLAG_IOC);

    const ULONG segmax = TRB_TPARAMS_DS_TRBLEN_SMASK;
    (void)pmax;

    const BOOL is_ctl_data_stage = (base_type == TRBF_FLAG_TRTYPE_DATA);

    if(remaining == 0) {
        ULONG txflags = base_flags | base_type;

        if(chain_beyond)
            txflags |= TRBF_FLAG_CH;

        if(ioconlast)
            txflags |= TRBF_FLAG_IOC;

        return xhciQueueTRB_IO(hc, ring, payload, 0, txflags, ioreq);
    }

    while(remaining > 0) {
        const ULONG offset = (plen - remaining);
        ULONG trblen = remaining;
        ULONG txflags;

        if(trblen > segmax)
            trblen = segmax;

        if(is_ctl_data_stage && offset != 0) {
            txflags = (base_flags & ~TRBF_FLAG_DS_DIR) | TRBF_FLAG_TRTYPE_NORMAL;
        } else {
            txflags = base_flags | base_type;
        }

        if(remaining > trblen)
            txflags |= TRBF_FLAG_CH;
        else if(chain_beyond)
            txflags |= TRBF_FLAG_CH;

        if(ioconlast && (remaining == trblen))
            txflags |= TRBF_FLAG_IOC;

        queued = xhciQueueTRB_IO(hc, ring, payload + offset, trblen, txflags, ioreq);
        if(queued == -1)
            return queued;

        if(firstqueued == -1)
            firstqueued = queued;

        remaining -= trblen;
    }

    return firstqueued;
}

/*
 * Page Size Register (PAGESIZE) - Chapter 5.4.3
 */
ULONG xhciPageSize(struct PCIController *hc)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)xhcic->xhc_XHCIOpR);
    ULONG pagesize = AROS_LE2LONG(hcopr->pagesize);
    int bit;

    for(bit = 0; bit < 16; bit++) {
        if(pagesize & (1UL << bit))
            return 1UL << (12 + bit);
    }
    return 0;
}

struct pciusbXHCIDevice *
xhciCreateDeviceCtx(struct PCIController *hc,
                    UWORD rootPortIndex,   /* 0-based */
                    ULONG route,           /* 20-bit route string (0 for root) */
                    ULONG flags,           /* UHFF_* speed / hub flags */
                    UWORD mps0,            /* initial EP0 max packet size */
                    struct timerequest *timerreq)
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
    if(!devCtx)
        return NULL;

    devCtx->dc_RootPort    = rootPortIndex;
    devCtx->dc_RouteString = route & SLOT_CTX_ROUTE_MASK;
    devCtx->dc_DevAddr     = 0;   /* default address until SET_ADDRESS */

    /*
     * One "context unit" is 32 bytes when CSZ=0, 64 bytes when CSZ=1.
     * struct xhci_inctx is declared as 32 bytes; we scale it if CTX64 is set.
     */
    ctx_unit = sizeof(struct xhci_inctx);
    if(hc->hc_Flags & HCF_CTX64) {
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

    if(!devCtx->dc_SlotCtx.dmaa_Ptr) {
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

    if(!devCtx->dc_IN.dmaa_Ptr) {
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
    if(flags & UHFF_HUB)
        islot->ctx[0] |= (1UL << 26);

    /* Speed bits + MTT bit */
    islot->ctx[0] &= ~(0xFUL << SLOTS_CTX_SPEED);
    islot->ctx[0] &= ~SLOTF_CTX_MTT;

    if(flags & UHFF_SUPERSPEED)
        islot->ctx[0] |= SLOTF_CTX_SUPERSPEED;
    else if(flags & UHFF_HIGHSPEED)
        islot->ctx[0] |= SLOTF_CTX_HIGHSPEED;
    else if(flags & UHFF_LOWSPEED)
        islot->ctx[0] |= SLOTF_CTX_LOWSPEED;
    else
        islot->ctx[0] |= SLOTF_CTX_FULLSPEED;

    /* Multi-TT hub? */
    if(flags & UHFF_TT_MULTI)
        islot->ctx[0] |= SLOTF_CTX_MTT;

    /* simple per-device max transfer tuning based on speed. */
    if(flags & UHFF_SUPERSPEED) {
        devCtx->dc_TxMax = 1024 * 16;   /* generous default for SS */
    } else if(flags & UHFF_HIGHSPEED) {
        devCtx->dc_TxMax = 512 * 13;    /* HS worst-case per microframe */
    } else if(flags & UHFF_LOWSPEED) {
        devCtx->dc_TxMax = 8 * 1;       /* LS: very small payloads */
    } else {
        devCtx->dc_TxMax = 64 * 19;     /* FS: conservative default */
    }

    /* ---- Enable Slot ---- */
    slotid = xhciCmdSlotEnable(hc, timerreq);
    if(slotid < 0) {
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
    if(TRB_CC_SUCCESS != xhciCmdDeviceAddress(hc, slotid, devCtx->dc_IN.dmaa_Ptr, 1, NULL,
            timerreq)) {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "Address Device (BSR=1) failed" DEBUGCOLOR_RESET "\n");

        xhciCmdSlotDisable(hc, slotid, timerreq);
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
    if(slotid > 0 && slotid < USB_DEV_MAX)
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
    if(!xhcic)
        return FALSE;

    volatile struct xhci_pr *xhciports =
        (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);

    UWORD port = *rootPortIndex;

    /* If caller didn't know the root port, try to find a connected one. */
    if(forceScan || (port >= hc->hc_NumPorts)) {
        UWORD found = 0xFFFF;

        /* Prefer a connected+enabled port. */
        for(UWORD i = 0; i < hc->hc_NumPorts; i++) {
            ULONG portsc = AROS_LE2LONG(xhciports[i].portsc);
            if(xhciHubPortConnected(portsc) && xhciHubPortEnabled(hc, i, portsc)) {
                found = i;
                break;
            }
        }

        /* Fallback: any connected port. */
        if(found == 0xFFFF) {
            for(UWORD i = 0; i < hc->hc_NumPorts; i++) {
                ULONG portsc = AROS_LE2LONG(xhciports[i].portsc);
                if(xhciHubPortConnected(portsc)) {
                    found = i;
                    break;
                }
            }
        }

        if(found == 0xFFFF)
            return FALSE;

        port = found;
        *rootPortIndex = port;
    }

    ULONG portsc = AROS_LE2LONG(xhciports[port].portsc);
    if(!xhciHubPortConnected(portsc))
        return FALSE;

    /* Clear any existing speed flags and set from PORTSC. */
    *flags &= ~(UHFF_SUPERSPEED | UHFF_HIGHSPEED | UHFF_LOWSPEED);

    ULONG speedBits = portsc & XHCI_PR_PORTSC_SPEED_MASK;
    if(speedBits == XHCIF_PR_PORTSC_LOWSPEED) {
        *flags |= UHFF_LOWSPEED;
        if(*mps0 == 0) *mps0 = 8;
    } else if(speedBits == XHCIF_PR_PORTSC_HIGHSPEED) {
        *flags |= UHFF_HIGHSPEED;
        if(*mps0 == 0) *mps0 = 64;
    } else if(speedBits == XHCIF_PR_PORTSC_SUPERSPEED ||
              speedBits == XHCIF_PR_PORTSC_SUPERSPEEDPLUS) {
        *flags |= UHFF_SUPERSPEED;
        if(*mps0 == 0) *mps0 = 512;
    } else {
        /* FS or unknown: start conservatively until bMaxPacketSize0 is known. */
        if(*mps0 == 0) *mps0 = 8;
    }

    return TRUE;
}

struct pciusbXHCIDevice *
xhciObtainDeviceCtx(struct PCIController *hc,
                    struct IOUsbHWReq *ioreq,
                    BOOL allowCreate,
                    struct timerequest *timerreq)
{
    struct PCIUnit *unit   = (struct PCIUnit *)ioreq->iouh_Req.io_Unit;
    UWORD devaddr       = ioreq->iouh_DevAddr;
    ULONG route         = ioreq->iouh_RouteString & SLOT_CTX_ROUTE_MASK;
    UWORD rootPortIndex = (ioreq->iouh_RootPort > 0)
                          ? (ioreq->iouh_RootPort - 1)
                          : 0;

    if(xhciIsRootHubRequest(ioreq, unit))
        return XHCI_ROOT_HUB_HANDLE;

    struct pciusbXHCIDevice *devCtx;

    /*
     * Device context lookup rules:
     *  - DevAddr != 0: prefer an exact DevAddr match. If none, allow a
     *    route/root-port match ONLY if the matched slot is still unassigned
     *    (DevAddr==0), then bind it to the new address.
     *  - DevAddr == 0: NEVER match by address (ambiguous). Match by
     *    route/root-port only.
     */
    if(devaddr != 0) {
        devCtx = xhciFindDeviceCtx(hc, devaddr);
        if(devCtx)
            return devCtx;

        devCtx = xhciFindRouteDevice(hc, route, rootPortIndex);
        if(devCtx) {
            if(devCtx->dc_DevAddr == 0) {
                devCtx->dc_DevAddr = (UBYTE)devaddr;
                return devCtx;
            }

            /* Route points at a different assigned address: inconsistent. */
            return NULL;
        }

        return NULL;
    }

    /* DevAddr==0: resolve only by route/root-port. */
    devCtx = xhciFindRouteDevice(hc, route, rootPortIndex);
    if(devCtx)
        return devCtx;

    if(!allowCreate)
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

    if(!haveSpeed || (mps0 == 0) || forceScan) {
        (void)xhciDerivePortFlagsAndMps0(hc, &rootPortIndex, &flags, &mps0, forceScan);
    }
    if(mps0 == 0)
        mps0 = 8;

    return xhciCreateDeviceCtx(hc,
                               rootPortIndex,
                               route,
                               flags,
                               mps0,
                               timerreq);
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

    BOOL ring_new = FALSE;

    /* Test if already prepared */
    if(devCtx->dc_EPAllocs[epid].dmaa_Ptr == NULL) {
        devCtx->dc_EPAllocs[epid].dmaa_Ptr =
            pciAllocAligned(hc, &devCtx->dc_EPAllocs[epid].dmaa_Entry,
                            sizeof(struct pcisusbXHCIRing),
                            XHCI_RING_ALIGN, (1 << 16));
        if(devCtx->dc_EPAllocs[epid].dmaa_Ptr) {
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
            ring_new = TRUE;
        } else {
            pciusbError("xHCI",
                        DEBUGWARNCOLOR_SET "Unable to allocate EP Ring Memory" DEBUGCOLOR_RESET" \n");
            return 0;
        }
    }
    epring = (volatile struct pcisusbXHCIRing *)devCtx->dc_EPAllocs[epid].dmaa_Ptr;
    if(ring_new)
        xhciInitRing(hc, (struct pcisusbXHCIRing *)epring);

    volatile struct xhci_inctx *in =
        (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;

    UWORD ctxoff = 1;
    if(hc->hc_Flags & HCF_CTX64)
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
    if(oslot->ctx[1] & (0xFFUL << 16))
        CopyMem(oslot, islot, ctxsize);

    /* Add/Enable this endpoint and refresh slot context */
    in->acf |= (1 << epid);
    in->acf |= 0x01;

    if(ioreq) {
        ULONG slotctx0 =
            islot->ctx[0] &
            ~((SLOT_CTX_ROUTE_MASK) | (0xF << SLOTS_CTX_SPEED) | SLOTF_CTX_MTT);
        ULONG route = (ULONG)(ioreq->iouh_RouteString & SLOT_CTX_ROUTE_MASK);

        slotctx0 |= route;
        if(flags & UHFF_SUPERSPEED)
            slotctx0 |= SLOTF_CTX_SUPERSPEED;
        else if(flags & UHFF_HIGHSPEED)
            slotctx0 |= SLOTF_CTX_HIGHSPEED;
        else if(flags & UHFF_LOWSPEED)
            slotctx0 |= SLOTF_CTX_LOWSPEED;
        else
            slotctx0 |= SLOTF_CTX_FULLSPEED;

        if(flags & UHFF_TT_MULTI)
            slotctx0 |= SLOTF_CTX_MTT;

        /* Preserve all other slot-ctx[0] bits not related to route/speed/MTT */
        slotctx0 |= (islot->ctx[0] &
                     ~(SLOT_CTX_ROUTE_MASK | (0xF << SLOTS_CTX_SPEED) | SLOTF_CTX_MTT));
        islot->ctx[0] = slotctx0;

        /* Root port number */
        islot->ctx[1] &= ~(0xFF << 16);
        islot->ctx[1] |= ((ULONG)(ioreq->iouh_RootPort & 0xFF) << 16);

        if(flags & UHFF_SPLITTRANS) {
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
    if(epid > ((islot->ctx[0] >> 27) & 0xF)) {
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
    switch(type) {
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

    /* Keep SS MaxBurst available for DW1 programming below. */
    UBYTE ssMaxBurst = 0;

    if(epid > 1) {
        /*
         * Endpoint Context DWORD4 ("tx_info") packing (xHCI spec):
         *   bits 15:0  = Average TRB Length (bytes)
         *   bits 31:16 = Max ESIT Payload (lo) (bytes; periodic endpoints only)
         *
         * For bulk/control endpoints Max ESIT Payload must be 0.
         */
        ULONG avg_trb_len      = maxpacket;
        ULONG max_esit_payload = 0;
        UBYTE multval = 0;

        if(ioreq) {
            if(flags & UHFF_SUPERSPEED) {
                multval  = ioreq->iouh_SS_Mult;
                ssMaxBurst = ioreq->iouh_SS_MaxBurst;
            } else if((flags & UHFF_HIGHSPEED) &&
                      ((flags & UHFF_MULTI_2) || (flags & UHFF_MULTI_3))) {
                UBYTE transactions = 1;

                if(flags & UHFF_MULTI_3)
                    transactions = 3;
                else if(flags & UHFF_MULTI_2)
                    transactions = 2;

                multval = transactions - 1;
            }

            /* Mult field (MaxPStreams/MULT in EP ctx) */
            ep->ctx[0] |= EPF_CTX_MULT(multval);

            if(flags & UHFF_SUPERSPEED) {
                if((type == UHCMD_ISOXFER) || (type == UHCMD_INTXFER)) {
                    if(ioreq->iouh_SS_BytesPerInterval) {
                        /* Preferred: device provides BytesPerInterval directly. */
                        max_esit_payload = ioreq->iouh_SS_BytesPerInterval;
                    } else {
                        /* Fallback: payload = MaxPkt * (MaxBurst+1) * (Mult+1) */
                        max_esit_payload = maxpacket * (ssMaxBurst + 1) * (multval + 1);
                    }
                } else {
                    /* Non-periodic SS: use wMaxPacketSize */
                    avg_trb_len = maxpacket;
                }
            } else if((type == UHCMD_ISOXFER) || (type == UHCMD_INTXFER)) {
                /* High-bandwidth HS endpoints: scale payload by Mult+1 */
                max_esit_payload = maxpacket * (ULONG)(multval + 1);
            }
        }

        if(type == UHCMD_CONTROLXFER) {
            if(avg_trb_len < 8)
                avg_trb_len = 8; /* Minimum control packet size */
        }

        /* Program DWORD4 with correct field packing */
        if((type == UHCMD_ISOXFER) || (type == UHCMD_INTXFER)) {
            if(avg_trb_len > 0xFFFFUL) avg_trb_len = 0xFFFFUL;
            if(max_esit_payload > 0xFFFFUL) max_esit_payload = 0xFFFFUL;
            ep->length = (avg_trb_len & 0xFFFFUL) | ((max_esit_payload & 0xFFFFUL) << 16);
        } else {
            if(avg_trb_len > 0xFFFFUL) avg_trb_len = 0xFFFFUL;
            ep->length = (avg_trb_len & 0xFFFFUL);
        }

        UBYTE ival = xhciCalcInterval(interval, flags, type);
        if(ival)
            ep->ctx[0] |= ((ULONG)ival << 16);
    } else
        devCtx->dc_EP0MaxPacket = maxpacket;

    /*
     * Endpoint Context DW1 programming:
     *   bit0      : reserved (must be 0)
     *   bits2:1   : CErr (must be 3 for normal operation)
     *   bits15:8  : MaxBurst (SuperSpeed only; 0 for others)
     *   bits31:16 : Max Packet Size
     */
    {
        ULONG dw1 = ep->ctx[1];
        ULONG maxburst = 0;

        if((flags & UHFF_SUPERSPEED) && ioreq && (epid > 1))
            maxburst = (ULONG)(ioreq->iouh_SS_MaxBurst & 0xFF);

        dw1 &= ~0x1UL;
        dw1 &= ~(0x3UL << 1);
        dw1 |= (3UL << 1);
        dw1 &= ~(0xFFUL << 8);
        dw1 |= (maxburst & 0xFFUL) << 8;
        dw1 &= ~(0xFFFFUL << 16);
        dw1 |= ((ULONG)maxpacket & 0xFFFFUL) << 16;
        ep->ctx[1] = dw1;
    }

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
    struct pciusbXHCIEndpointCtx *epctx = (struct pciusbXHCIEndpointCtx *)ioreq->iouh_DriverPrivate2;
    struct timerequest *timerreq = NULL;
    BOOL epctx_allocated = FALSE;

    pciusbXHCIDebugEP("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p)" DEBUGCOLOR_RESET" \n", __func__, ioreq);
    pciusbXHCIDebugEPV("xHCI", DEBUGFUNCCOLOR_SET "%s: Dev %u Endpoint %u %s" DEBUGCOLOR_RESET" \n", __func__,
                       ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT");

    if(!unit)
        return UHIOERR_BADPARAMS;

    BOOL rootHubReq = xhciIsRootHubRequest(ioreq, unit);
    pciusbXHCIDebugEPV("xHCI", DEBUGFUNCCOLOR_SET "%s: unit @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, unit);
    hc = xhciGetController(unit);
    if(!hc) {
        return UHIOERR_BADPARAMS;
    } else if(rootHubReq) {
        return 0;
    }

    if(!epctx) {
        epctx = AllocMem(sizeof(*epctx), MEMF_ANY | MEMF_CLEAR);
        if(!epctx)
            return UHIOERR_OUTOFMEMORY;

        epctx_allocated = TRUE;
    }

    if(!epctx->ectx_TimerReq) {
        if(!xhciOpenTaskTimer(&epctx->ectx_TimerPort,
                              &epctx->ectx_TimerReq,
                              xhciEndpointTimerName)) {
            if(epctx_allocated)
                FreeMem(epctx, sizeof(*epctx));
            return UHIOERR_HOSTERROR;
        }
    }

    timerreq = epctx->ectx_TimerReq;

    struct pciusbXHCIDevice *devCtx =
        xhciObtainDeviceCtx(hc, ioreq, !rootHubReq, timerreq);
    if(!devCtx) {
        pciusbWarn("xHCI",
                   DEBUGWARNCOLOR_SET "no device context" DEBUGCOLOR_RESET"\n");
        if(epctx_allocated) {
            xhciCloseTaskTimer(&epctx->ectx_TimerPort, &epctx->ectx_TimerReq);
            FreeMem(epctx, sizeof(*epctx));
        }
        return UHIOERR_HOSTERROR;
    }

    /*
     * EP0 max packet size can change after the initial 8-byte device descriptor
     * read (FS: 8/16/32/64). We must update the EP0 endpoint context when the
     * stack learns the real bMaxPacketSize0, otherwise subsequent control
     * transfers may complete with incorrect/empty data.
     */
    if((ioreq->iouh_Endpoint == 0) &&
            (ioreq->iouh_MaxPktSize != 0) &&
            ((ULONG)ioreq->iouh_MaxPktSize != devCtx->dc_EP0MaxPacket)) {
        pciusbXHCIDebugEP("xHCI",
                          DEBUGCOLOR_SET "Updating EP0 MPS: %lu -> %u (DevAddr=%u, Slot=%ld)" DEBUGCOLOR_RESET"\n",
                          devCtx->dc_EP0MaxPacket,
                          ioreq->iouh_MaxPktSize,
                          ioreq->iouh_DevAddr,
                          devCtx->dc_SlotID);

        (void)xhciInitEP(hc, devCtx,
                         ioreq,
                         0, 0,
                         UHCMD_CONTROLXFER,
                         ioreq->iouh_MaxPktSize,
                         0,
                         ioreq->iouh_Flags);

        LONG cc = xhciCmdEndpointConfigure(hc, devCtx->dc_SlotID, devCtx->dc_IN.dmaa_Ptr,
                                           timerreq);
        if(cc != TRB_CC_SUCCESS) {
            pciusbWarn("xHCI",
                       DEBUGWARNCOLOR_SET "EP0 reconfigure failed (cc=%d)" DEBUGCOLOR_RESET"\n", cc);
            if(epctx_allocated) {
                xhciCloseTaskTimer(&epctx->ectx_TimerPort, &epctx->ectx_TimerReq);
                FreeMem(epctx, sizeof(*epctx));
            }
            return UHIOERR_HOSTERROR;
        }
    }

    ULONG epid = xhciEndpointID(ioreq->iouh_Endpoint,
                                (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0);

    if(epid >= MAX_DEVENDPOINTS) {
        if(epctx_allocated) {
            xhciCloseTaskTimer(&epctx->ectx_TimerPort, &epctx->ectx_TimerReq);
            FreeMem(epctx, sizeof(*epctx));
        }
        return UHIOERR_BADPARAMS;
    }

    struct pciusbXHCIEndpointCtx *dev_epctx = devCtx->dc_EPContexts[epid];

    if(!devCtx->dc_EPAllocs[epid].dmaa_Ptr) {
        ULONG txep = xhciInitEP(hc, devCtx,
                                ioreq,
                                ioreq->iouh_Endpoint,
                                (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0,
                                ioreq->iouh_Req.io_Command,
                                ioreq->iouh_MaxPktSize,
                                ioreq->iouh_Interval,
                                ioreq->iouh_Flags);

        if(txep == 0) {
            if(epctx_allocated) {
                xhciCloseTaskTimer(&epctx->ectx_TimerPort, &epctx->ectx_TimerReq);
                FreeMem(epctx, sizeof(*epctx));
            }
            return UHIOERR_OUTOFMEMORY;
        }

        epid = txep;

        if(epid >= MAX_DEVENDPOINTS) {
            if(epctx_allocated) {
                xhciCloseTaskTimer(&epctx->ectx_TimerPort, &epctx->ectx_TimerReq);
                FreeMem(epctx, sizeof(*epctx));
            }
            return UHIOERR_BADPARAMS;
        }

        /*
         * Push the updated input context to hardware. Configure all
         * endpoints, including EP0, after initialization so the controller
         * commits the new state.
         */
        LONG cc = xhciCmdEndpointConfigure(hc, devCtx->dc_SlotID, devCtx->dc_IN.dmaa_Ptr,
                                           timerreq);
        if(cc != TRB_CC_SUCCESS) {
            pciusbWarn("xHCI",
                       DEBUGWARNCOLOR_SET "EndpointConfigure failed (cc=%d)" DEBUGCOLOR_RESET"\n", cc);
            if(epctx_allocated) {
                xhciCloseTaskTimer(&epctx->ectx_TimerPort, &epctx->ectx_TimerReq);
                FreeMem(epctx, sizeof(*epctx));
            }
            return UHIOERR_HOSTERROR;
        }
    }

    if(dev_epctx && dev_epctx != epctx) {
        if(epctx_allocated) {
            xhciCloseTaskTimer(&epctx->ectx_TimerPort, &epctx->ectx_TimerReq);
            FreeMem(epctx, sizeof(*epctx));
        }
        epctx = dev_epctx;
        epctx_allocated = FALSE;
    }

    if(epctx && !epctx->ectx_TimerReq) {
        if(!xhciOpenTaskTimer(&epctx->ectx_TimerPort,
                              &epctx->ectx_TimerReq,
                              xhciEndpointTimerName)) {
            if(epctx_allocated)
                FreeMem(epctx, sizeof(*epctx));
            return UHIOERR_HOSTERROR;
        }
    }

    if(epctx) {
        epctx->ectx_Device = devCtx;
        epctx->ectx_EPID = epid;
        if(!dev_epctx)
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
    struct pciusbXHCIEndpointCtx *epctx_free = NULL;
    struct timerequest *timerreq = NULL;
    ULONG epid;

    pciusbXHCIDebugEP("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p)" DEBUGCOLOR_RESET" \n", __func__, ioreq);
    pciusbXHCIDebugEPV("xHCI", DEBUGFUNCCOLOR_SET "%s: unit @ 0x%p, epctx @ 0x%p" DEBUGCOLOR_RESET" \n", __func__, unit,
                       epctx);

    if(!unit)
        return;

    BOOL rootHubReq = xhciIsRootHubRequest(ioreq, unit);
    hc = xhciGetController(unit);
    if(!hc || rootHubReq)
        return;

    epid = xhciEndpointID(ioreq->iouh_Endpoint, (ioreq->iouh_Dir == UHDIR_IN) ? 1 : 0);

    if(epctx) {
        devCtx = epctx->ectx_Device;
        epid = epctx->ectx_EPID;
        timerreq = epctx->ectx_TimerReq;
    } else {
        devCtx = xhciFindDeviceCtx(hc, ioreq->iouh_DevAddr);
        timerreq = unit->hu_TimerReq;
    }

    if(!devCtx || (epid >= MAX_DEVENDPOINTS))
        return;

    xhciFreeEndpointContext(hc, devCtx, epid, TRUE, timerreq);
    epctx_free = devCtx->dc_EPContexts[epid];
    if(!epctx_free)
        epctx_free = epctx;
    devCtx->dc_EPContexts[epid] = NULL;

    ioreq->iouh_DriverPrivate2 = NULL;

    /*
     * Downstream devices (route string != 0) do not generate root hub port
     * disconnect events. When their EP0 is torn down and no endpoints remain,
     * assume the device is gone and release the slot/resources.
     */
    if(devCtx->dc_RouteString != 0 &&
            epid == xhciEndpointID(0, 0) &&
            !xhciDeviceHasEndpoints(devCtx)) {
        xhciDisconnectDevice(hc, devCtx, timerreq);
    }

    if(epctx_free) {
        if(epctx_free->ectx_TimerReq || epctx_free->ectx_TimerPort) {
            xhciCloseTaskTimer(&epctx_free->ectx_TimerPort,
                               &epctx_free->ectx_TimerReq);
        }
        FreeMem(epctx_free, sizeof(*epctx_free));
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

    if((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL) {
        ioreq->iouh_DriverPrivate1 = NULL;
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);

        if(driprivate->dpBounceBuf) {
            xhciReleaseDMABuffer(hc, ioreq, 0, driprivate->dpBounceDir,
                                 driprivate->dpBounceBuf);
            driprivate->dpBounceBuf = NULL;
            driprivate->dpBounceData = NULL;
            driprivate->dpBounceLen = 0;
            driprivate->dpBounceDir = 0;
        }
        FreeMem(driprivate, sizeof(struct pciusbXHCIIODevPrivate));
    }
    devadrep = xhciDevEPKey(ioreq);
    pciusbXHCIDebugEPV("xHCI", DEBUGCOLOR_SET "%s: releasing DevEP %02lx" DEBUGCOLOR_RESET" \n", __func__, devadrep);
    if(unit->hu_DevBusyReq[devadrep] == ioreq)
        unit->hu_DevBusyReq[devadrep] = NULL;
    if(ioreq->iouh_Req.io_Command != UHCMD_ISOXFER)
        unit->hu_NakTimeoutFrame[devadrep] = 0;
}

static inline void xhciIOErrfromCC(struct IOUsbHWReq *ioreq, ULONG cc)
{
    switch(cc) {
    case TRB_CC_SUCCESS:                                        /* Success */
        ioreq->iouh_Req.io_Error = UHIOERR_NO_ERROR;
        /*
         * For periodic transfers (INT/ISO), prefer the length derived from
         * the Transfer Event. If the completion path never set iouh_Actual,
         * fall back to the requested length for backwards compatibility.
         */
        if((ioreq->iouh_Req.io_Command == UHCMD_INTXFER) &&
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

    case TRB_CC_RING_UNDERRUN:
        /*
         * Ring Underrun (commonly observed on ISO OUT when no TD is
         * available in time) is not fatal. Return success and 0 bytes so
         * the client can continue streaming.
         */
        if(ioreq->iouh_Req.io_Command == UHCMD_ISOXFER) {
            ioreq->iouh_Req.io_Error = UHIOERR_NO_ERROR;
            ioreq->iouh_Actual = 0;
        } else {
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
        }
        break;

    default: {
#if defined(AROS_USE_LOGRES)
        struct PCIUnit *unit = (struct PCIUnit *)ioreq->iouh_Req.io_Unit;
        struct PCIController *hc = xhciGetController(unit);
#endif
        pciusbWarn("xHCI", DEBUGCOLOR_SET "%s:  IOReq 0x%p - UHIOERR_HOSTERROR (cc=%ld)" DEBUGCOLOR_RESET" \n", __func__, ioreq,
                   cc);
        {
            UWORD rt  = ioreq->iouh_SetupData.bmRequestType;
            UWORD req = ioreq->iouh_SetupData.bRequest;
            /* SetupData is stored little-endian */
            UWORD val = AROS_LE2WORD(ioreq->iouh_SetupData.wValue);
            UWORD idx = AROS_LE2WORD(ioreq->iouh_SetupData.wIndex);
            UWORD len = AROS_LE2WORD(ioreq->iouh_SetupData.wLength);

            pciusbWarn("xHCI",
                       DEBUGWARNCOLOR_SET
                       "Device[%u]: Setup %02lx %02lx wValue=%04lx wIndex=%04lx wLength=%04lx!"
                       DEBUGCOLOR_RESET "\n",
                       ioreq->iouh_DevAddr,
                       (ULONG)rt, (ULONG)req, (ULONG)val, (ULONG)idx, (ULONG)len);
        }
        ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
    }
    break;
    }
}

static void xhciHandleClearFeatureEndpointHalt(struct PCIController *hc,
        struct IOUsbHWReq *ioreq,
        struct pciusbXHCIDevice *devCtx,
        struct timerequest *timerreq)
{
    if(!hc || !ioreq || !devCtx)
        return;

    if(ioreq->iouh_Req.io_Command != UHCMD_CONTROLXFER)
        return;

    if(ioreq->iouh_Req.io_Error != UHIOERR_NO_ERROR)
        return;

    const UBYTE bmRequestType = ioreq->iouh_SetupData.bmRequestType;
    const UBYTE bRequest = ioreq->iouh_SetupData.bRequest;
    const UWORD wValue = AROS_LE2WORD(ioreq->iouh_SetupData.wValue);
    const UWORD wIndex = AROS_LE2WORD(ioreq->iouh_SetupData.wIndex);

    if((bmRequestType != (URTF_STANDARD | URTF_ENDPOINT)) ||
            (bRequest != USR_CLEAR_FEATURE) ||
            (wValue != UFS_ENDPOINT_HALT))
        return;

    if(devCtx == XHCI_ROOT_HUB_HANDLE)
        return;

    const UBYTE epid = xhciEndpointIDFromIndex(wIndex);
    if(epid == 0 || epid >= MAX_DEVENDPOINTS)
        return;

#if (0)
    /* First, find and complete any pending IOReq on this endpoint */
    struct PCIUnit *unit = hc->hc_Unit;
    UWORD devadrep = (devCtx->dc_DevAddr << 5) + (wIndex & 0x0F);
    if(wIndex & 0x80) devadrep |= 0x10;

    struct IOUsbHWReq *pending = unit->hu_DevBusyReq[devadrep];
    if(pending) {
        /* Complete the failed request first */
        pending->iouh_Req.io_Error = UHIOERR_STALL;
        pending->iouh_Actual = 0;
        xhciFinishRequest(hc, unit, pending);
        /* Don't ReplyMsg - let the caller handle it */
    }
#endif

    pciusbXHCIDebugV("xHCI",
                     DEBUGCOLOR_SET "CLEAR_FEATURE(ENDPOINT_HALT) completed: dev=%u wIndex=0x%04x -> EPID=%u"
                     DEBUGCOLOR_RESET" \n",
                     (unsigned)devCtx->dc_DevAddr,
                     (unsigned)wIndex,
                     (unsigned)epid);

    if(!devCtx->dc_SlotCtx.dmaa_Ptr)
        return;

    const UWORD ctxsize = (hc->hc_Flags & HCF_CTX64) ? 64 : 32;
    volatile UBYTE *obase = (volatile UBYTE *)devCtx->dc_SlotCtx.dmaa_Ptr;
    volatile struct xhci_ep *epctx = (volatile struct xhci_ep *)(obase + (epid * ctxsize));

    CacheClearE((APTR)epctx, ctxsize, CACRF_InvalidateD);
    UBYTE epstate = xhciEndpointState(epctx);

    if(epstate != XHCI_EP_STATE_HALTED) {
        pciusbXHCIDebugV("xHCI",
                         DEBUGCOLOR_SET "Skip endpoint reset: EPID=%u state=%u (not halted)" DEBUGCOLOR_RESET" \n",
                         (unsigned)epid, (unsigned)epstate);
        return;
    }

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "Reset Endpoint: slot=%u epid=%u" DEBUGCOLOR_RESET"\n",
                    devCtx->dc_SlotID, epid);

    /* Step 1: Reset Endpoint */
    LONG cc = xhciCmdEndpointReset(hc, devCtx->dc_SlotID, epid, 0, timerreq);
    if(cc != TRB_CC_SUCCESS) {
        pciusbWarn("xHCI", "Reset Endpoint failed: cc=%ld\n", cc);
        return;
    }

    /* Step 2: Reset the transfer ring */
    APTR deqptr = NULL;
    BOOL dcs = FALSE;
    if(!xhciResetEndpointRing(hc, devCtx, epid, &deqptr, &dcs))
        return;

    /* Step 3: Set TR Dequeue Pointer */
    cc = xhciCmdSetTRDequeuePtr(hc, devCtx->dc_SlotID, epid, deqptr, dcs, timerreq);
    if(cc != TRB_CC_SUCCESS) {
        pciusbWarn("xHCI", "Set TR Dequeue failed: cc=%ld\n", cc);
        return;
    }

    /* Step 4: CRITICAL - Verify endpoint is no longer halted */
    /* Invalidate cache to see controller's updated state */
    CacheClearE((APTR)epctx, ctxsize, CACRF_InvalidateD);

    ULONG edw0 = AROS_LE2LONG(epctx->ctx[0]);
    epstate = (edw0 & 0x7U);

    pciusbXHCIDebug("xHCI",
                    "After Set TR Dequeue: epid=%u state=%u (%s)\n",
                    epid, epstate,
                    (epstate == 0) ? "Disabled" :
                    (epstate == 1) ? "Running" :
                    (epstate == 2) ? "Halted" :
                    (epstate == 3) ? "Stopped" :
                    (epstate == 4) ? "Error" : "Unknown");

    /* Endpoint should be in Stopped (3) state after Set TR Dequeue */
    if(epstate == 2) {
        pciusbWarn("xHCI",
                   DEBUGWARNCOLOR_SET
                   "Endpoint still Halted after reset sequence! Aborting recovery."
                   DEBUGCOLOR_RESET"\n");
        return;
    }

    /* Step 5: Ring doorbell to transition Stopped -> Running */
    xhciRingDoorbell(hc, devCtx->dc_SlotID, epid);

    /* Step 6: Wait briefly and verify it transitioned to Running */
    uhwDelayMS(1, timerreq);

    CacheClearE((APTR)epctx, ctxsize, CACRF_InvalidateD);
    edw0 = AROS_LE2LONG(epctx->ctx[0]);
    epstate = (edw0 & 0x7U);

    pciusbXHCIDebug("xHCI",
                    "After doorbell: epid=%u state=%u (%s)\n",
                    epid, epstate,
                    (epstate == 1) ? "Running" :
                    (epstate == 3) ? "Stopped" : "Other");
}

void xhciHandleFinishedTDs(struct PCIController *hc, struct timerequest *timerreq)
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
    while((nextioreq = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ)) {
        pciusbXHCIDebugV("xHCI",
                         DEBUGCOLOR_SET "Examining Periodic IOReq=%p (dev=%u, ep=%u, dir=%s)" DEBUGCOLOR_RESET"\n",
                         ioreq,
                         ioreq->iouh_DevAddr,
                         ioreq->iouh_Endpoint,
                         (ioreq->iouh_Dir == UHDIR_IN) ? "IN" : "OUT");

        driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        if(!driprivate) {
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
        if(driprivate->dpCC > 0) {
            pciusbXHCIDebugV("xHCI",
                             DEBUGCOLOR_SET "Periodic IOReq %p complete (CC=%u)" DEBUGCOLOR_RESET"\n",
                             ioreq, driprivate->dpCC);

            xhciIOErrfromCC(ioreq, driprivate->dpCC);

            transactiondone = TRUE;
        } else {
            /*
             * No CC yet - check for software NAK timeout
             */
            if(unit->hu_NakTimeoutFrame[devadrep] &&
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

        if(transactiondone) {
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

            struct PTDNode *ptd = (struct PTDNode *)ioreq->iouh_DriverPrivate2;
            struct RTIsoNode *rtn = ptd ? ptd->ptd_RTIsoNode : NULL;
            if(rtn && rtn->rtn_RTIso) {
                struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
                struct IOUsbHWBufferReq *bufreq = ptd ? &ptd->ptd_BufferReq : &rtn->rtn_BufferReq;
                bufreq->ubr_Length = actual;
                bufreq->ubr_Frame = ioreq->iouh_Frame;
                if(ioreq->iouh_Dir == UHDIR_IN) {
                    if(urti->urti_InDoneHook)
                        CallHookPkt(urti->urti_InDoneHook, rtn, bufreq);
                } else {
                    if(urti->urti_OutDoneHook)
                        CallHookPkt(urti->urti_OutDoneHook, rtn, bufreq);
                }

                xhciFreePeriodicContext(hc, unit, ioreq);
                ioreq->iouh_Actual = 0;
                ioreq->iouh_Req.io_Error = 0;
                if(ptd)
                    ptd->ptd_Flags &= ~(PTDF_ACTIVE | PTDF_BUFFER_VALID);

                UWORD pending = 0;
                UWORD target = (rtn->rtn_PTDCount > 1) ? 2 : 1;
                for(UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
                    struct PTDNode *scan = rtn->rtn_PTDs[idx];
                    if(scan && (scan->ptd_Flags & (PTDF_ACTIVE | PTDF_BUFFER_VALID)))
                        pending++;
                }

                while(pending < target) {
                    if(xhciQueueIsochIO(hc, rtn) != RC_OK)
                        break;
                    xhciStartIsochIO(hc, rtn);
                    pending++;
                }
                ioreq = nextioreq;
                continue;
            }

            if(!ioreq->iouh_Req.io_Error &&
                    ioreq->iouh_Data &&
                    actual > 0) {
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
                    for(i = 0; i < dump; i++) {
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
    while((nextioreq = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ)) {
        pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "Examining IOReq=0x%p" DEBUGCOLOR_RESET" \n", ioreq);

        driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        if(driprivate) {
            transactiondone = FALSE;
            devadrep = xhciDevEPKey(ioreq);

            if(driprivate->dpCC > TRB_CC_INVALID) {
                pciusbXHCIDebugV("xHCI",
                                 DEBUGCOLOR_SET "IOReq Complete (completion code %u)!"
                                 DEBUGCOLOR_RESET" \n",
                                 driprivate->dpCC);
                transactiondone = TRUE;

                xhciIOErrfromCC(ioreq, driprivate->dpCC);

                unit->hu_NakTimeoutFrame[devadrep] = 0;

            } else if(unit->hu_NakTimeoutFrame[devadrep] &&
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
            if(driprivate->dpCC == TRB_CC_STALL_ERROR &&
                    ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER &&
                    ioreq->iouh_Endpoint == 0) {
                struct pciusbXHCIDevice *devCtx = driprivate->dpDevice;

                if(devCtx && devCtx != XHCI_ROOT_HUB_HANDLE) {
                    ULONG epid = driprivate->dpEPID;
                    if(!epid) {
                        /* Fallback: EP0 is EPID 1 by spec */
                        epid = xhciGetEPID(0, 0);
                    }

                    pciusbWarn("xHCI",
                               DEBUGWARNCOLOR_SET
                               "STALL on Dev %u EP0 (CC=6), resetting EPID=%lu"
                               DEBUGCOLOR_RESET"\n",
                               ioreq->iouh_DevAddr, epid);

                    xhciCmdEndpointReset(hc, devCtx->dc_SlotID, epid, 0, timerreq);
                }
            }

            if(transactiondone) {
                struct pciusbXHCIDevice *clear_dev = driprivate ? driprivate->dpDevice : NULL;
                xhciFreeAsyncContext(hc, unit, ioreq);
                if((!ioreq->iouh_Req.io_Error) &&
                        (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)) {
                    xhciHandleClearFeatureEndpointHalt(hc, ioreq, clear_dev, timerreq);
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

    Signal(xhcic->xhc_EventTask.xet_Task,
           1L << xhcic->xhc_EventTask.xet_ProcessEventsSignal);

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

    if(!ioreq) {
        pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "IOReq missing for completion (cc=%lu)" DEBUGCOLOR_RESET" \n", (ULONG)ccode);
        return FALSE;
    }

    if((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL) {
        UWORD effdir = xhciEffectiveDataDir(ioreq);
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "IOReq TRB(s) = #%u:#%u" DEBUGCOLOR_RESET" \n", driprivate->dpTxSTRB,
                            driprivate->dpTxETRB);
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "          Ring    @ 0x%p" DEBUGCOLOR_RESET" \n",
                            driprivate->dpDevice->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr);

        driprivate->dpCC = ccode;

        /* Avoid log storms for expected ISO conditions */
        if((ccode != TRB_CC_SUCCESS) &&
                !(ccode == TRB_CC_RING_UNDERRUN && (ioreq->iouh_Req.io_Command == UHCMD_ISOXFER))) {
            pciusbWarn("xHCI",
                       DEBUGWARNCOLOR_SET
                       "cc=%d for IOReq 0x%p"
                       DEBUGCOLOR_RESET" \n", (int)ccode, ioreq);
        }

        /*
         * On error completions, dump a concise view of the endpoint context.
         * This is particularly useful to spot endpoints that remain HALTED /
         * STOPPED while new TDs are being submitted.
         */
        if((ccode != TRB_CC_SUCCESS) &&
                !(ccode == TRB_CC_RING_UNDERRUN && (ioreq->iouh_Req.io_Command == UHCMD_ISOXFER))) {
            xhciDiagDumpEndpointBrief(hc, driprivate->dpDevice, (UBYTE)driprivate->dpEPID, "completion-error");
            xhciDumpEndpointCtx(hc, driprivate->dpDevice, driprivate->dpEPID, "completion-error");
        }

        ULONG transferred = (remaining <= ioreq->iouh_Length)
                            ? (ioreq->iouh_Length - remaining)
                            : ioreq->iouh_Length;

        /*
         * For periodic ISO OUT, Ring Underrun indicates that no data was
         * serviced in time. Report 0 bytes to the client and continue.
         */
        if((ccode == TRB_CC_RING_UNDERRUN) &&
                (ioreq->iouh_Req.io_Command == UHCMD_ISOXFER) &&
                (ioreq->iouh_Dir == UHDIR_OUT)) {
            transferred = 0;
            remaining   = ioreq->iouh_Length;
        }

        /* Clear ringio bookkeeping */
        if(driprivate->dpDevice) {
            struct pciusbXHCIDevice *devCtx = driprivate->dpDevice;
            struct pcisusbXHCIRing *epRing = devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr;
            int cnt;

            if(epRing) {
                if(driprivate->dpSTRB != (UWORD) - 1) {
                    if(xhciRingioMatchesIOReq(epRing, driprivate->dpSTRB, ioreq)) {
                        pciusbXHCIDebugRIO("xHCI", DEBUGCOLOR_SET "%s: IOReq 0x%p - setup <ringio %p>[%d] 0x%p -> 0x0000000000000000"
                                           DEBUGCOLOR_RESET" \n",
                                           __func__, ioreq, epRing->ringio, driprivate->dpSTRB,
                                           epRing->ringio[driprivate->dpSTRB]);
                        epRing->ringio[driprivate->dpSTRB] = NULL;
                    }
                }

                /* dpTxSTRB/dpTxETRB can legally be -1 (no data stage). */
                if((driprivate->dpTxSTRB != (UWORD) - 1) && (driprivate->dpTxETRB != (UWORD) - 1)) {
                    if(driprivate->dpTxETRB >= driprivate->dpTxSTRB) {
                        for(cnt = driprivate->dpTxSTRB; cnt <= driprivate->dpTxETRB; cnt++) {
                            if(xhciRingioMatchesIOReq(epRing, (UWORD)cnt, ioreq)) {
                                pciusbXHCIDebugRIO("xHCI", DEBUGCOLOR_SET "%s: IOReq 0x%p - data <ringio %p>[%d] 0x%p -> 0x0000000000000000"
                                                   DEBUGCOLOR_RESET" \n",
                                                   __func__, ioreq, epRing->ringio, cnt, epRing->ringio[cnt]);
                                epRing->ringio[cnt] = NULL;
                            }
                        }
                    } else {
                        /* Wrapped range */
                        for(cnt = driprivate->dpTxSTRB; cnt < (XHCI_EVENT_RING_TRBS - 1); cnt++) {
                            if(xhciRingioMatchesIOReq(epRing, (UWORD)cnt, ioreq)) {
                                pciusbXHCIDebugRIO("xHCI", DEBUGCOLOR_SET "%s: IOReq 0x%p - data <ringio %p>[%d] 0x%p -> 0x0000000000000000"
                                                   DEBUGCOLOR_RESET" \n",
                                                   __func__, ioreq, epRing->ringio, cnt, epRing->ringio[cnt]);
                                epRing->ringio[cnt] = NULL;
                            }
                        }
                        for(cnt = 0; cnt <= driprivate->dpTxETRB; cnt++) {
                            if(xhciRingioMatchesIOReq(epRing, (UWORD)cnt, ioreq)) {
                                pciusbXHCIDebugRIO("xHCI", DEBUGCOLOR_SET "%s: IOReq 0x%p - data <ringio %p>[%d] 0x%p -> 0x0000000000000000"
                                                   DEBUGCOLOR_RESET" \n",
                                                   __func__, ioreq, epRing->ringio, cnt, epRing->ringio[cnt]);
                                epRing->ringio[cnt] = NULL;
                            }
                        }
                    }
                }

                if(driprivate->dpSttTRB != (UWORD) - 1) {
                    if(xhciRingioMatchesIOReq(epRing, driprivate->dpSttTRB, ioreq)) {
                        pciusbXHCIDebugRIO("xHCI", DEBUGCOLOR_SET "%s:  IOReq 0x%p - status <ringio %p>[%d] 0x%p -> 0x0000000000000000"
                                           DEBUGCOLOR_RESET" \n",
                                           __func__, ioreq, epRing->ringio, driprivate->dpSttTRB,
                                           epRing->ringio[driprivate->dpSttTRB]);
                        epRing->ringio[driprivate->dpSttTRB] = NULL;
                    }
                }
            }
        }

        /*
         * Release any bounce buffer regardless of completion code to avoid
         * leaks and to complete CachePostDMA on partial transfers.
         */
        if(ioreq->iouh_Data) {
            xhciReleaseDMABuffer(hc, ioreq, transferred, effdir, driprivate->dpBounceBuf);
            driprivate->dpBounceBuf = NULL;
            driprivate->dpBounceData = NULL;
            driprivate->dpBounceLen = 0;
            driprivate->dpBounceDir = 0;
        }

        ioreq->iouh_Actual = transferred;
        pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "IOReq Transfer done, %u bytes <io length %u, %u remaining>"
                            DEBUGCOLOR_RESET" \n", transferred, ioreq->iouh_Length, remaining);

        return TRUE;
    }

    /*
     * Command-ring submissions expect an IOReq but do not always
     * carry driver-private transfer state. Treat these as completed so
     * the higher layers can observe the result and progress.
     */
    ioreq->iouh_Req.io_Error = (ccode == TRB_CC_SUCCESS) ? 0 : UHIOERR_HOSTERROR;
    ioreq->iouh_Actual += (ioreq->iouh_Length - remaining);

    pciusbXHCIDebugTRBV("xHCI", DEBUGCOLOR_SET "Completion w/o private state, treating as done (cc=%lu)"
                        DEBUGCOLOR_RESET" \n", (ULONG)ccode);

    return TRUE;
}

static inline ULONG ring_advance_idx(ULONG idx)
{
    idx++;
    if(idx == XHCI_EVENT_RING_TRBS)
        idx = 0;
    return idx;
}

/*
 * Event TRBs carry physical (bus) addresses in their Parameter field.
 * When CPUTOPCI is not an identity mapping, we must translate these
 * pointers back into the driver's CPU address space before we can
 * locate the owning ring and its ringio bookkeeping.
 *
 * Transfer Event TRBs also include the Slot ID and Endpoint ID, so we
 * prefer those for robust endpoint-ring selection (and to handle cases
 * where the TRB pointer is reported as 0, e.g. Ring Underrun).
 */
static inline UQUAD xhciEventTRBDMA(volatile struct xhci_trb *etrb)
{
    /* Event TRB Parameter field is a 64-bit pointer */
    UQUAD lo = (UQUAD)AROS_LE2LONG(etrb->dbp.addr_lo);
    UQUAD hi = (UQUAD)AROS_LE2LONG(etrb->dbp.addr_hi);
    return (hi << 32) | lo;
}

static inline BOOL xhciRingAndIndexFromDMA(struct PCIController *hc,
        struct pciusbXHCIDevice *devCtx,
        UBYTE epid,
        UQUAD trb_dma,
        volatile struct pcisusbXHCIRing **outRing,
        ULONG *outIdx)
{
    (void)hc;

    if(!outRing || !outIdx)
        return FALSE;

    *outRing = NULL;
    *outIdx  = 0;

    if(!devCtx || (epid >= MAX_DEVENDPOINTS))
        return FALSE;

    if(!devCtx->dc_EPAllocs[epid].dmaa_Ptr || !devCtx->dc_EPAllocs[epid].dmaa_DMA)
        return FALSE;

    volatile struct pcisusbXHCIRing *ring =
        (volatile struct pcisusbXHCIRing *)devCtx->dc_EPAllocs[epid].dmaa_Ptr;

    UQUAD ring_dma = (UQUAD)(IPTR)devCtx->dc_EPAllocs[epid].dmaa_DMA;

    /* TRB pointer can be 0 for certain endpoint-level conditions */
    if(trb_dma == 0)
        return FALSE;

    if(trb_dma < ring_dma)
        return FALSE;

    UQUAD off = trb_dma - ring_dma;
    ULONG idx = (ULONG)(off / (UQUAD)sizeof(struct xhci_trb));

    if(idx >= XHCI_EVENT_RING_TRBS)
        return FALSE;

    *outRing = ring;
    *outIdx  = idx;
    return TRUE;
}

static inline struct IOUsbHWReq *xhciBusyReqFromSlotEpid(struct PCIController *hc,
        struct pciusbXHCIDevice *devCtx,
        UBYTE epid)
{
    if(!hc || !hc->hc_Unit || !devCtx)
        return NULL;

    /* Match xhciDevEPKey():
     *  - EP0 is a single bidirectional busy slot (no dir bit)
     *  - other EPs use 0x10 to distinguish IN
     */
    UBYTE endpoint = (epid > 1) ? (epid >> 1) : 0;
    UBYTE dirbit   = 0x00;
    if(endpoint != 0) {
        dirbit = (epid & 0x01) ? 0x10 : 0x00;
    }
    UWORD devadrep = (devCtx->dc_DevAddr << 5) + endpoint + dirbit;

    return (struct IOUsbHWReq *)hc->hc_Unit->hu_DevBusyReq[devadrep];
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
    if(status & XHCIF_USBSTS_HCH) {
        pciusbXHCIDebugTRBV("xHCI",
                            DEBUGCOLOR_SET "Host Controller Halted" DEBUGCOLOR_RESET" \n");
    }
    if(status & XHCIF_USBSTS_HCE) {
        pciusbXHCIDebugTRBV("xHCI",
                            DEBUGCOLOR_SET "Host Controller Error detected" DEBUGCOLOR_RESET" \n");
    }
    if(status & XHCIF_USBSTS_HSE) {
        pciusbXHCIDebugTRBV("xHCI",
                            DEBUGCOLOR_SET "Host System Error detected" DEBUGCOLOR_RESET" \n");
    }
    if(status & XHCIF_USBSTS_EINT) {
        pciusbXHCIDebugTRBV("xHCI",
                            DEBUGCOLOR_SET "Event Interrupt Pending" DEBUGCOLOR_RESET" \n");
    }
    if(status & XHCIF_USBSTS_PCD) {
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

    /*
     * Some controllers can signal via USBSTS even when IMAN.IP sampling is
     * unreliable; do not skip event processing just because IP wasn't seen.
     */
    if((iman & XHCIF_IR_IMAN_IE) &&
            ((iman & XHCIF_IR_IMAN_IP) || (status & (XHCIF_USBSTS_EINT | XHCIF_USBSTS_PCD)))) {
        volatile struct pcisusbXHCIRing *ering =
            (volatile struct pcisusbXHCIRing *)((IPTR)xhcic->xhc_ERSp);
        volatile struct xhci_trb *etrb;
        ULONG idx   = ering->next;
        ULONG cycle = (ering->end & RINGENDCFLAG) ? 1 : 0;
        ULONG maxwork = XHCI_EVENT_RING_TRBS;

        pciusbXHCIDebugTRBV("xHCI",
                            DEBUGCOLOR_SET "Processing events..." DEBUGCOLOR_RESET" \n");

        for(etrb = &ering->ring[idx]; maxwork > 0; maxwork--) {
            /* Make sure we see the latest TRB from the controller. */
            CacheClearE((APTR)etrb, sizeof(*etrb), CACRF_InvalidateD);

            ULONG dw2        = AROS_LE2LONG(etrb->tparams);
            ULONG dw3        = AROS_LE2LONG(etrb->flags);

            if(!xhciTRBCycleMatches(dw3, cycle))
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

            switch(trbe_type) {
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

                if(origportsc & XHCIF_PR_PORTSC_OCC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
                    newportsc |= XHCIF_PR_PORTSC_OCC;
                }
                if(origportsc & XHCIF_PR_PORTSC_PRC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
                    newportsc |= XHCIF_PR_PORTSC_PRC;
                }
                if(origportsc & XHCIF_PR_PORTSC_WRC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
                    newportsc |= XHCIF_PR_PORTSC_WRC;
                }
                if(origportsc & XHCIF_PR_PORTSC_PEC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                    newportsc |= XHCIF_PR_PORTSC_PEC;
                }
                if(origportsc & XHCIF_PR_PORTSC_CSC) {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                    newportsc |= XHCIF_PR_PORTSC_CSC;
                }
                if(origportsc & XHCIF_PR_PORTSC_PLC)
                    newportsc |= XHCIF_PR_PORTSC_PLC;
                if(origportsc & XHCIF_PR_PORTSC_CEC)
                    newportsc |= XHCIF_PR_PORTSC_CEC;

                /* Acknowledge the change bits we observed */
                xhciports[hciport].portsc = AROS_LONG2LE(newportsc);

                pciusbXHCIDebugTRBV("xHCI",
                                    DEBUGCOLOR_SET "RH Change $%08lx" DEBUGCOLOR_RESET" \n",
                                    hc->hc_PortChangeMap[hciport]);

                if(hc->hc_PortChangeMap[hciport]) {
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
                        if((hc->hc_PortChangeMap[hciport] & UPSF_PORT_CONNECTION) ||
                                (enabled && !haveDev) ||
                                (!enabled && haveDev)) {
                            signalTask = TRUE;
                        }
                    }

                    if(signalTask) {
                        pciusbXHCIDebugTRBV("xHCI",
                                            DEBUGFUNCCOLOR_SET "%s: Signaling port change handler <0x%p, %d (%08x)>..."
                                            DEBUGCOLOR_RESET" \n",
                                            __func__,
                                            xhcic->xhc_PortTask.xpt_Task,
                                            xhcic->xhc_PortTask.xpt_PortChangeSignal,
                                            1L << xhcic->xhc_PortTask.xpt_PortChangeSignal);
                        /* Connect/Disconnect the device */
                        Signal(xhcic->xhc_PortTask.xpt_Task,
                               1L << xhcic->xhc_PortTask.xpt_PortChangeSignal);
                    } else {
                        checkRHchanges = TRUE;
                    }
                }
                break;
            }

            case TRBB_FLAG_ERTYPE_TRANSFER: {
                /* Transfer Event TRB */
                UBYTE trbe_epid = (UBYTE)((dw3 >> 16) & 0x1f);
                UQUAD trb_dma = xhciEventTRBDMA(etrb);

                struct pciusbXHCIDevice *devCtx = NULL;
                volatile struct pcisusbXHCIRing *ring = NULL;
                ULONG last = 0;
                BOOL have_idx = FALSE;

                if(trbe_slot && (trbe_slot < USB_DEV_MAX))
                    devCtx = xhcic->xhc_Devices[trbe_slot];

                if(devCtx && (trbe_epid < MAX_DEVENDPOINTS) &&
                        devCtx->dc_EPAllocs[trbe_epid].dmaa_Ptr) {
                    ring = (volatile struct pcisusbXHCIRing *)devCtx->dc_EPAllocs[trbe_epid].dmaa_Ptr;
                }

                if(ring && xhciRingAndIndexFromDMA(hc, devCtx, trbe_epid, trb_dma, &ring, &last)) {
                    have_idx = TRUE;
                }

                pciusbXHCIDebugTRBV("xHCI",
                                    DEBUGCOLOR_SET "TRANSFER EVT: slot=%u epid=%u trb_dma=%p CC=%u rem=%lu"
                                    DEBUGCOLOR_RESET" \n",
                                    trbe_slot, trbe_epid, (APTR)(IPTR)trb_dma, trbe_ccode,
                                    (unsigned long)event_rem);
                xhciDumpCC(trbe_ccode);

                struct IOUsbHWReq *req = NULL;

                if(have_idx) {
                    req = (struct IOUsbHWReq *)ring->ringio[last];

                    /* Only advance ring on SUCCESS or recoverable short packets */
                    if(trbe_ccode == TRB_CC_SUCCESS ||
                            trbe_ccode == TRB_CC_SHORT_PACKET) {
                        ULONG new_end = ring_advance_idx(last);
                        ring->end = (ring->end & RINGENDCFLAG) | (new_end & ~RINGENDCFLAG);
                    }
                    /* On error, DO NOT advance - let reset/recovery handle it */
                    pciusbXHCIDebugTRBV("xHCI",
                                        DEBUGCOLOR_SET "TRANSFER EVT idx=%lu ringio=%p ring=%p" DEBUGCOLOR_RESET" \n",
                                        (unsigned long)last, req, ring);
                } else if(trbe_ccode == TRB_CC_RING_UNDERRUN) {
                    /*
                     * Ring Underrun does not reliably report a TRB pointer.
                     * Complete the currently active request for this endpoint.
                     */
                    req = xhciBusyReqFromSlotEpid(hc, devCtx, trbe_epid);
                    if(req) {
                        /* Report 0 bytes transferred */
                        event_rem = req->iouh_Length;
                    }
                }

                if(!req && devCtx && (trbe_epid < MAX_DEVENDPOINTS)) {
                    req = xhciBusyReqFromSlotEpid(hc, devCtx, trbe_epid);
                    if(req) {
                        pciusbXHCIDebugTRBV("xHCI",
                                            DEBUGCOLOR_SET
                                            "TRANSFER EVT: fallback to busy req %p (slot=%u epid=%u)"
                                            DEBUGCOLOR_RESET" \n",
                                            req, trbe_slot, trbe_epid);
                    }
                }

                if(!req) {
                    /* Avoid log storms for expected endpoint-level ISO conditions */
                    if(trbe_ccode != TRB_CC_RING_UNDERRUN) {
                        pciusbWarn("xHCI",
                                   DEBUGWARNCOLOR_SET
                                   "TRANSFER EVT: slot=%u epid=%u idx=%lu cc=%lu missing ioreq"
                                   DEBUGCOLOR_RESET" \n",
                                   trbe_slot, trbe_epid,
                                   (unsigned long)(have_idx ? last : 0),
                                   (unsigned long)trbe_ccode);
                        if(trbe_slot < USB_DEV_MAX && trbe_epid < MAX_DEVENDPOINTS) {
                            UBYTE *cnt = &xhci_diag_missing_ioreq[trbe_slot][trbe_epid];
                            if(*cnt < XHCI_DIAG_DUMP_LIMIT && devCtx && ring && have_idx) {
                                (*cnt)++;
                                pciusbWarn("xHCI",
                                           DEBUGWARNCOLOR_SET
                                           "DIAG: missing IOReq dump #%u (slot=%u epid=%u idx=%lu cc=%lu/%s trb_dma=%p ring=%p, ringio=%p)"
                                           DEBUGCOLOR_RESET" \n",
                                           (unsigned)*cnt,
                                           trbe_slot, trbe_epid,
                                           (unsigned long)last,
                                           (unsigned long)trbe_ccode,
                                           xhciDiagCCName(trbe_ccode),
                                           (APTR)(IPTR)trb_dma,
                                           ring, ring->ringio);
                                xhciDiagDumpRingWindow(hc, ring, last, XHCI_DIAG_DUMP_RADIUS, "missing-ioreq");
                                xhciDiagDumpOutputCtx(hc, devCtx, trbe_epid, "missing-ioreq");
                            }
                        }
                    }
                    break;
                }

                if(trbe_ccode != TRB_CC_SUCCESS &&
                        trbe_slot < USB_DEV_MAX && trbe_epid < MAX_DEVENDPOINTS &&
                        devCtx && ring && have_idx) {
                    UBYTE *cnt = &xhci_diag_cc_err[trbe_slot][trbe_epid];
                    if(*cnt < XHCI_DIAG_DUMP_LIMIT) {
                        (*cnt)++;
                        pciusbWarn("xHCI",
                                   DEBUGWARNCOLOR_SET
                                   "DIAG: error completion dump #%u (slot=%u epid=%u idx=%lu cc=%lu/%s rem=%lu req=%p trb_dma=%p ring=%p)"
                                   DEBUGCOLOR_RESET" \n",
                                   (unsigned)*cnt,
                                   trbe_slot, trbe_epid,
                                   (unsigned long)last,
                                   (unsigned long)trbe_ccode,
                                   xhciDiagCCName(trbe_ccode),
                                   (unsigned long)event_rem,
                                   req,
                                   (APTR)(IPTR)trb_dma,
                                   ring);
                        xhciDiagDumpRingWindow(hc, ring, last, XHCI_DIAG_DUMP_RADIUS, "cc-error");
                        xhciDiagDumpOutputCtx(hc, devCtx, trbe_epid, "cc-error");
                    }
                }

                doCompletion = xhciIntWorkProcess(hc, req, event_rem, trbe_ccode);
                break;
            }

            case TRBB_FLAG_ERTYPE_COMMAND_COMPLETE: {
                /* Command Completion Event TRB */
                UQUAD trb_dma = xhciEventTRBDMA(etrb);

                volatile struct pcisusbXHCIRing *ring =
                    (volatile struct pcisusbXHCIRing *)xhcic->xhc_OPRp;
                volatile struct xhci_trb *evt = &ring->current;

                ULONG last = 0;
                ULONG new_end = 0;

                if(trb_dma && xhcic->xhc_DMAOPR) {
                    UQUAD base_dma = (UQUAD)(IPTR)xhcic->xhc_DMAOPR;
                    if(trb_dma >= base_dma) {
                        UQUAD off = trb_dma - base_dma;
                        last = (ULONG)(off / (UQUAD)sizeof(struct xhci_trb));
                    }
                }

                if(last >= XHCI_EVENT_RING_TRBS) {
                    pciusbWarn("xHCI",
                               DEBUGWARNCOLOR_SET
                               "COMMAND COMPLETE: bad TRB ptr %p (idx=%lu)"
                               DEBUGCOLOR_RESET" \n",
                               (APTR)(IPTR)trb_dma, (unsigned long)last);
                    break;
                }

                volatile struct xhci_trb *txtrb = &ring->ring[last];
                CacheClearE((APTR)txtrb, sizeof(*txtrb), CACRF_InvalidateD);

                new_end = ring_advance_idx(last);

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

                if(trbe_ccode != TRB_CC_SUCCESS) {
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

                    if(devCtx && devCtx->dc_SlotCtx.dmaa_Ptr) {
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
                ring->end = (ring->end & RINGENDCFLAG) | (new_end & ~RINGENDCFLAG);

                if(!req) {
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


            case TRBB_FLAG_ERTYPE_HOST_CONTROLLER:
                /*
                 * Host Controller Event TRB (type 37). VMware and some hardware
                 * use this to report controller-level conditions, most notably
                 * Event Ring Full Error (cc=21). We consume the event and keep
                 * going.
                 */
                if(trbe_ccode == TRB_CC_EVENT_RING_FULL_ERROR) {
                    pciusbWarn("xHCI",
                               DEBUGWARNCOLOR_SET
                               "Host Controller Event: Event Ring Full (cc=%lu)"
                               DEBUGCOLOR_RESET" \n",
                               (unsigned long)trbe_ccode);
                } else {
                    pciusbXHCIDebugTRBV("xHCI",
                                        DEBUGCOLOR_SET
                                        "Host Controller Event (cc=%lu) consumed"
                                        DEBUGCOLOR_RESET" \n",
                                        (unsigned long)trbe_ccode);
                }
                break;

            default:
                pciusbWarn("xHCI",
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
            if(++idx == XHCI_EVENT_RING_TRBS) {
                idx = 0;
                cycle ^= 1;
                if(cycle)
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

        if(maxwork == 0) {
            pciusbWarn("xHCI",
                       DEBUGWARNCOLOR_SET
                       "Event ring processing budget exhausted; possible backlog or ring stall"
                       DEBUGCOLOR_RESET" \n");
        }

        if(doCompletion) {
            SureCause(base, &hc->hc_CompleteInt);
        }
    }

    if(checkRHchanges)
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

    if(!ioreq)
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

void xhciReset(struct PCIController *hc, struct PCIUnit *hu,
               struct timerequest *timerreq)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)xhcic->xhc_XHCIOpR);
    ULONG reg;
    ULONG cnt = 100;
    ULONG status;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, hu);

    // Tell the controller to stop if its currently running ...
    reg = AROS_LE2LONG(hcopr->usbcmd);
    if(reg & XHCIF_USBCMD_RS) {
        reg &= ~XHCIF_USBCMD_RS;
        hcopr->usbcmd = AROS_LONG2LE(reg);

        // Wait for the controller to indicate it is finished ...
        while(!(AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_HCH) && (--cnt > 0))
            uhwDelayMS(1, timerreq);
    }

    status = AROS_LE2LONG(hcopr->usbsts);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Resetting Controller (pre-reset status $%08x)..." DEBUGCOLOR_RESET" \n",
                    status);
    xhciDumpStatus(status);
    if(status & (XHCIF_USBSTS_HCE | XHCIF_USBSTS_HSE))
        pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "Pre-reset controller error bits set:%s%s" DEBUGCOLOR_RESET" \n",
                   (status & XHCIF_USBSTS_HCE) ? " HCE" : "",
                   (status & XHCIF_USBSTS_HSE) ? " HSE" : "");

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Resetting Controller..." DEBUGCOLOR_RESET" \n");
    hcopr->usbcmd = AROS_LONG2LE(XHCIF_USBCMD_HCRST);

    // Wait for the command to be accepted..
    cnt = 100;
    while((AROS_LE2LONG(hcopr->usbcmd) & XHCIF_USBCMD_HCRST) && (--cnt > 0))
        uhwDelayMS(2, timerreq);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "COMMAND = $%08x, after %ums" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd),
                    (100 - cnt) << 1);

    status = AROS_LE2LONG(hcopr->usbsts);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBSTS after HCRST write = $%08x" DEBUGCOLOR_RESET" \n", status);
    xhciDumpStatus(status);
    if(status & (XHCIF_USBSTS_HCE | XHCIF_USBSTS_HSE))
        pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "Post-reset write detected controller error bits:%s%s" DEBUGCOLOR_RESET" \n",
                   (status & XHCIF_USBSTS_HCE) ? " HCE" : "",
                   (status & XHCIF_USBSTS_HSE) ? " HSE" : "");

    // Wait for the reset to complete..
    cnt = 100;
    while((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_CNR) && (--cnt > 0))
        uhwDelayMS(2, timerreq);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBSTS = $%08x, after %ums" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbsts),
                    (100 - cnt) << 1);
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Configuring DMA pointers..." DEBUGCOLOR_RESET" \n");

    hcopr->config = AROS_LONG2LE(xhcic->xhc_NumSlots);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting DCBAA to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMADCBAA);
    xhciSetPointer(hc, hcopr->dcbaap, xhcic->xhc_DMADCBAA);
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting CRCR to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAOPR);
    xhciSetPointer(hc, hcopr->crcr, ((IPTR)xhcic->xhc_DMAOPR | 1));

    volatile struct pcisusbXHCIRing *xring = (volatile struct pcisusbXHCIRing *)xhcic->xhc_OPRp;
    xhciInitRing(hc, (struct pcisusbXHCIRing *)xring);

    volatile struct xhci_er_seg *erseg = (volatile struct xhci_er_seg *)xhcic->xhc_ERSTp;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting Event Segment Pointer to 0x%p" DEBUGCOLOR_RESET" \n",
                    xhcic->xhc_DMAERS);
    xhciSetPointer(hc, erseg->ptr, ((IPTR)xhcic->xhc_DMAERS));

    erseg->size = AROS_LONG2LE(XHCI_EVENT_RING_TRBS);

    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)xhcic->xhc_XHCIIntR);
    xhciir->erstsz = AROS_LONG2LE(1);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting ERDP to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERS);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting ERSTBA to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERST);
    xhciSetPointer(hc, xhciir->erdp, ((IPTR)xhcic->xhc_DMAERS | (IPTR)XHCIF_IR_ERDP_EHB));
    xhciSetPointer(hc, xhciir->erstba, ((IPTR)xhcic->xhc_DMAERST));

    xring = (volatile struct pcisusbXHCIRing *)xhcic->xhc_ERSp;
    xhciInitRing(hc, (struct pcisusbXHCIRing *)xring);
    /*
     * MSI/MSI-X is edge-triggered in practice: if IP/EINT are already set
     * when USBCMD.INTE becomes 1, some controllers will not emit a new MSI.
     * Clear pending latches here while enabling IE.
     */
    xhciir->imod = AROS_LONG2LE(0); /* disable moderation for bring-up */
    xhciir->iman = AROS_LONG2LE(XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP); /* W1C IP */
    (void)AROS_LE2LONG(xhciir->iman);
    ULONG st = AROS_LE2LONG(hcopr->usbsts);
    hcopr->usbsts = AROS_LONG2LE(st); /* W1C any pending EINT/PCD/etc */

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Reset complete..." DEBUGCOLOR_RESET" \n");

    xhciDumpOpR(hcopr);
    xhciDumpIR(xhciir);
}

BOOL xhciInit(struct PCIController *hc, struct PCIUnit *hu,
              struct timerequest *timerreq)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    ULONG val;
    ULONG cnt;


    struct Library *ps;
    if((ps = OpenLibrary(strPoseidonLibraryName, 5)) == NULL) {
        return FALSE;
    }
    if(!xhcic) {
        CloseLibrary(ps);
        return FALSE;
    }

    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)xhcic->xhc_XHCIOpR;

    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = xhciCompleteIntName;
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)xhciCompleteInt;

    /* install reset handler */
    hc->hc_ResetInt.is_Node.ln_Name = xhciResetIntName;
    hc->hc_ResetInt.is_Code = (VOID_FUNC)XhciResetHandler;
    hc->hc_ResetInt.is_Data = hc;
    AddResetCallback(&hc->hc_ResetInt);

    IPTR pciIntLine = 0;
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_INTLine, &pciIntLine);
    hc->hc_PCIIntLine = pciIntLine;

    {
        struct TagItem vectreqs[] = {
            { tHidd_PCIVector_Min, 1 },
            { tHidd_PCIVector_Max, 1 },
            { TAG_DONE, 0 }
        };

        if(HIDD_PCIDevice_ObtainVectors(hc->hc_PCIDeviceObject, vectreqs)) {
            struct TagItem vecAttribs[] = {
                { tHidd_PCIVector_Native, (IPTR)-1 },
                { tHidd_PCIVector_Int,    (IPTR)-1 },
                { TAG_DONE, 0 }
            };
            HIDD_PCIDevice_GetVectorAttribs(hc->hc_PCIDeviceObject, 0, vecAttribs);
            if(vecAttribs[0].ti_Data != (IPTR)-1) {
                IPTR native_vec = vecAttribs[0].ti_Data;
                IPTR int_vec    = vecAttribs[1].ti_Data;

                /* For MSI/MSI-X install handler on the actual programmed vector. */
                hc->hc_PCIIntLine = int_vec;
                hc->hc_Flags |= HCF_MSI;
            } else {
                HIDD_PCIDevice_ReleaseVectors(hc->hc_PCIDeviceObject);
            }
        }
    }
    pciusbWarn("xHCI", DEBUGCOLOR_SET "IRQ = %u%s" DEBUGCOLOR_RESET" \n",
               hc->hc_PCIIntLine, (hc->hc_Flags & HCF_MSI) ? " (MSI)" : "");

    /* add interrupt */
    hc->hc_PCIIntHandler.is_Node.ln_Name = hc->hc_ResetInt.is_Node.ln_Name;
    hc->hc_PCIIntHandler.is_Node.ln_Pri  = 5;
    hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_PCIIntHandler.is_Code         = (VOID_FUNC)xhciIntCode;
    hc->hc_PCIIntHandler.is_Data         = hc;
    if(hc->hc_Flags & HCF_MSI) {
        AddIntServer(INTB_KERNEL + hc->hc_PCIIntLine, &hc->hc_PCIIntHandler);
    } else {
        PCIXAddInterrupt(hc, &hc->hc_PCIIntHandler);
    }

    for(cnt = 0; cnt < hc->hc_NumPorts; cnt++) {
        hu->hu_PortMapX[cnt] = hc;
        hc->hc_PortNum[cnt]  = cnt;
    }

    xhciReset(hc, hu, timerreq);

    /* Ensure ports are powered per xHCI spec before enabling interrupts */
    xhciPowerOnRootPorts(hc, hu, timerreq);

    /* Enable interrupts in the xhci */
    {
        /*
         * Re-clear pending latches immediately before INTE=1 to guarantee a fresh
         * edge for MSI/MSI-X once interrupts are enabled.
         */
        volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)xhcic->xhc_XHCIIntR;
        xhciir->iman = AROS_LONG2LE(XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP);
        (void)AROS_LE2LONG(xhciir->iman);
    }
    val = AROS_LE2LONG(hcopr->usbcmd);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Enabling xHCI interrupts..." DEBUGCOLOR_RESET" \n");
    val |= XHCIF_USBCMD_INTE;
    hcopr->usbcmd = AROS_LONG2LE(val);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBCMD = $%08x..." DEBUGCOLOR_RESET" \n",
                    AROS_LE2LONG(hcopr->usbcmd));

    /* Wait for the interrupt enable bit to be visible (defensive) */
    cnt = 100;
    while(!(AROS_LE2LONG(hcopr->usbcmd) & XHCIF_USBCMD_INTE) && (--cnt > 0))
        uhwDelayMS(2, timerreq);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "COMMAND = $%08x, after %ums" DEBUGCOLOR_RESET" \n",
                    AROS_LE2LONG(hcopr->usbcmd), (100 - cnt) << 1);

    /* Wait for the controller to finish coming out of reset (CNR=0) */
    cnt = 100;
    while((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_CNR) && (--cnt > 0))
        uhwDelayMS(2, timerreq);

    /*
     * Check controller health before letting it run so that fatal errors
     * short-circuit initialisation rather than causing obscure failures later.
     */
    val = AROS_LE2LONG(hcopr->usbsts);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBSTS after interrupt enable = $%08x" DEBUGCOLOR_RESET" \n", val);
    xhciDumpStatus(val);
    if(val & (XHCIF_USBSTS_HCE | XHCIF_USBSTS_HSE)) {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "Controller reports fatal error (USBSTS=%08x), aborting init"
                    DEBUGCOLOR_RESET" \n", val);
        goto init_fail;
    }
    if(val & XHCIF_USBSTS_HCH)
        pciusbXHCIDebug("xHCI", DEBUGWARNCOLOR_SET "Controller in halted state after interrupt enable" DEBUGCOLOR_RESET" \n");
    if(val & XHCIF_USBSTS_CNR)
        pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "Warning: controller is not ready after reset" DEBUGCOLOR_RESET" \n");

#if (1)
    ULONG sigmask = SIGF_SINGLE;
    xhcic->xhc_ReadySignal = SIGB_SINGLE;
    xhcic->xhc_ReadySigTask = FindTask(NULL);
    SetSignal(0, sigmask);

    struct Task *tmptask;
    char buf[64];
    psdSafeRawDoFmt(buf, 64, xhciEventRingTaskNameFmt, hu->hu_UnitNo);
    if((tmptask = psdSpawnSubTask(buf, xhciEventRingTask, hc))) {
        sigmask = Wait(sigmask);
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Event Ring Task @ 0x%p, Sig = %u" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_EventTask.xet_Task,
                        xhcic->xhc_EventTask.xet_ProcessEventsSignal);
    }
    psdSafeRawDoFmt(buf, 64, xhciPortTaskNameFmt, hu->hu_UnitNo);
    if((tmptask = psdSpawnSubTask(buf, xhciPortTask, hc))) {
        sigmask = Wait(sigmask);
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Port Task @ 0x%p, Sig = %u" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_PortTask.xpt_Task,
                        xhcic->xhc_PortTask.xpt_PortChangeSignal);
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
    return FALSE;
}

void xhciFree(struct PCIController *hc, struct PCIUnit *hu)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);

    if(xhcic) {
        FreeMem(xhcic, sizeof(*xhcic));
        hc->hc_CPrivate = NULL;
    }
}

static void xhciFreeEndpointContext(struct PCIController *hc,
                                    struct pciusbXHCIDevice *devCtx,
                                    ULONG epid,
                                    BOOL stopEndpoint,
                                    struct timerequest *timerreq)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p, %u)" DEBUGCOLOR_RESET" \n", __func__, hc, devCtx, epid);

    if(!devCtx || (epid >= MAX_DEVENDPOINTS))
        return;

    if(devCtx->dc_EPAllocs[epid].dmaa_Ptr) {
        /*
         * Spec-safe teardown: stop/reset the endpoint and drop it from the
         * device context before freeing the transfer ring memory.
         *
         * This avoids controllers/emulators fetching TRBs from freed memory.
         */
        if(stopEndpoint && devCtx->dc_SlotID) {
            (void)xhciCmdEndpointStop(hc, devCtx->dc_SlotID, epid, TRUE, timerreq);
            (void)xhciCmdEndpointReset(hc, devCtx->dc_SlotID, epid, 0, timerreq);

            /*
             * EP0 (EPID=1) is mandatory and must not be dropped via Configure
             * Endpoint. Only drop non-control endpoints.
             */
            if((epid > 1) && devCtx->dc_IN.dmaa_Ptr && devCtx->dc_IN.dmaa_DMA) {
                volatile struct xhci_inctx *in =
                    (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
                UWORD ctxoff = 1;

                if(hc->hc_Flags & HCF_CTX64)
                    ctxoff <<= 1;

                /* Clear Add/Drop flags then request a drop for this EP. */
                in->dcf = 0;
                in->acf = 0;

                in->dcf |= (1UL << epid);
                in->acf |= 0x01; /* slot context */

                /* Preserve current slot context state (address, route, etc.). */
                if(devCtx->dc_SlotCtx.dmaa_Ptr) {
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
                                                   devCtx->dc_IN.dmaa_Ptr,
                                                   timerreq);
                if(cc != TRB_CC_SUCCESS) {
                    pciusbWarn("xHCI",
                               DEBUGWARNCOLOR_SET
                               "Configure Endpoint (drop EPID %lu) failed, cc=%ld"
                               DEBUGCOLOR_RESET" \n",
                               (unsigned long)epid, (long)cc);
                }
            }
        }

        /* Free software-only ring bookkeeping (not DMA-visible) */
        {
            volatile struct pcisusbXHCIRing *ring =
                (volatile struct pcisusbXHCIRing *)devCtx->dc_EPAllocs[epid].dmaa_Ptr;
            if(ring && ring->ringio) {
                FreeMem((APTR)ring->ringio, sizeof(*ring->ringio) * XHCI_EVENT_RING_TRBS);
                ring->ringio = NULL;
            }
        }

        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_EPAllocs[epid].dmaa_Entry.me_Un.meu_Addr);
        devCtx->dc_EPAllocs[epid].dmaa_Entry.me_Un.meu_Addr = NULL;
        devCtx->dc_EPAllocs[epid].dmaa_Ptr = NULL;
        devCtx->dc_EPAllocs[epid].dmaa_DMA = NULL;
    }

}

void xhciFreeDeviceCtx(struct PCIController *hc,
                       struct pciusbXHCIDevice *devCtx,
                       BOOL disableSlot,
                       struct timerequest *timerreq)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, devCtx);

    if(!devCtx)
        return;

    if(disableSlot && devCtx->dc_SlotID)
        xhciCmdSlotDisable(hc, devCtx->dc_SlotID, timerreq);

    if((devCtx->dc_SlotID > 0) && (devCtx->dc_SlotID < USB_DEV_MAX)) {
        if(xhcic->xhc_Devices[devCtx->dc_SlotID] == devCtx)
            xhcic->xhc_Devices[devCtx->dc_SlotID] = NULL;
    }

    if(devCtx->dc_SlotID)
        xhciSetPointer(hc, ((volatile struct xhci_address *)xhcic->xhc_DCBAAp)[devCtx->dc_SlotID], 0);

    for(ULONG epid = 0; epid < MAX_DEVENDPOINTS; epid++) {
        xhciFreeEndpointContext(hc, devCtx, epid, FALSE, timerreq);
        if(devCtx->dc_EPContexts[epid]) {
            if(devCtx->dc_EPContexts[epid]->ectx_TimerReq ||
                    devCtx->dc_EPContexts[epid]->ectx_TimerPort) {
                xhciCloseTaskTimer(&devCtx->dc_EPContexts[epid]->ectx_TimerPort,
                                   &devCtx->dc_EPContexts[epid]->ectx_TimerReq);
            }
            FreeMem(devCtx->dc_EPContexts[epid], sizeof(*devCtx->dc_EPContexts[epid]));
            devCtx->dc_EPContexts[epid] = NULL;
        }
    }

    if(devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr) {
        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr);
        devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr = NULL;
        devCtx->dc_IN.dmaa_Ptr = NULL;
        devCtx->dc_IN.dmaa_DMA = NULL;
    }

    if(devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr) {
        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr);
        devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr = NULL;
        devCtx->dc_SlotCtx.dmaa_Ptr = NULL;
        devCtx->dc_SlotCtx.dmaa_DMA = NULL;
    }

    FreeMem(devCtx, sizeof(struct pciusbXHCIDevice));
}
