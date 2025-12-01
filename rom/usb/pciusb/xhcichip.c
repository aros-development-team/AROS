/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver main pciusb interface
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

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

// See page 395 "EWE" - to enable rollover events.
void xhciUpdateFrameCounter(struct PCIController *hc)
{
    volatile struct xhci_rrs *rrs = (volatile struct xhci_rrs *)((IPTR)hc->hc_XHCIIntR - 0x20);
    UWORD framecnt;

    Disable();
    framecnt = rrs->mfindex & 0x3FFF;
    if(framecnt < (hc->hc_FrameCounter & 0x3FFF)) {
        hc->hc_FrameCounter |= 0x3FFF;
        hc->hc_FrameCounter++;
        hc->hc_FrameCounter |= framecnt;
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Frame Counter Rollover %ld\n", hc->hc_FrameCounter);
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
    memset(ring, 0, sizeof(*ring));
    ring->end = RINGENDCFLAG; /* set initial cycle bit */
}

struct pcisusbXHCIDevice *xhciFindDeviceCtx(struct PCIController *hc, UWORD devaddr)
{
    struct pcisusbXHCIDevice *unassigned = NULL;
    UWORD maxslot = hc->hc_NumSlots;

    if (maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for (UWORD slot = 1; slot <= maxslot; slot++) {
        struct pcisusbXHCIDevice *devCtx = hc->hc_Devices[slot];

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

static struct pcisusbXHCIDevice *xhciFindPortDevice(struct PCIController *hc, UWORD hciport)
{
    UWORD maxslot = hc->hc_NumSlots;

    if (maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for (UWORD slot = 1; slot <= maxslot; slot++) {
        struct pcisusbXHCIDevice *devCtx = hc->hc_Devices[slot];

        if (devCtx && (devCtx->dc_RootPort == hciport))
            return devCtx;
    }

    return NULL;
}

static int xhciRingEntriesFree(volatile struct pcisusbXHCIRing *ring)
{
    ULONG last = (ring->end & ~RINGENDCFLAG);
    ULONG idx = ring->next;

    return (last > idx) ? last-idx
           : (USB_DEV_MAX - 1) - idx + last;
}

static void xhciInsertTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG trflags, ULONG plen)
{
    volatile struct xhci_trb *dst;
    ULONG trbflags = (trflags & ~TRBF_FLAG_C);

    // Set the cycle bit
    if (ring->end & RINGENDCFLAG)
        trbflags |= TRBF_FLAG_C;

    // Get the next available ring entry
    dst = &ring->ring[ring->next];

    //... And populate it
    xhciSetPointer(hc, dst->dbp, payload);
    dst->tparams = (plen & TRB_TPARAMS_DS_TRBLEN_SMASK);
    dst->flags = trbflags;
}

WORD xhciQueueTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload,
                  ULONG plen, ULONG trbflags)
{
    if (trbflags & TRBF_FLAG_IDT) {
        pciusbXHCIDebugTRB("xHCI", DEBUGFUNCCOLOR_SET "(0x%p, $%02x %02x %02x%02x %02x%02x %02x%02x, %u, $%08x)" DEBUGCOLOR_RESET" \n", ring,
                       ((UBYTE *)&payload)[0], ((UBYTE *)&payload)[1], ((UBYTE *)&payload)[3], ((UBYTE *)&payload)[2],
                       ((UBYTE *)&payload)[5], ((UBYTE *)&payload)[4], ((UBYTE *)&payload)[7], ((UBYTE *)&payload)[6],
                       plen, trbflags);
    } else
        pciusbXHCIDebugTRB("xHCI", DEBUGFUNCCOLOR_SET "(0x%p, 0x%p, %u, $%08x)" DEBUGCOLOR_RESET" \n", ring, payload, plen, trbflags);

    if (!ring) {
        pciusbXHCIDebugTRB("xHCI", DEBUGWARNCOLOR_SET "NO RINGSPECIFIED!!" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    if (xhciRingEntriesFree(ring) > 1) {
        if (ring->next >= USB_DEV_MAX - 1) {
            UQUAD link_dma;
#if !defined(PCIUSB_NO_CPUTOPCI)
            link_dma = (IPTR)CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)&ring->ring[0]);
#else
            link_dma = (IPTR)&ring->ring[0];
#endif

            /*
             * If this is the last ring element, insert a link
             * back to the ring start - and update the cycle bit
             */
            xhciInsertTRB(hc, ring,
                          link_dma,
                          TRBF_FLAG_TRTYPE_LINK | TRBF_FLAG_ENT,
                          0);
            ring->next = 0;
            if (ring->end & RINGENDCFLAG)
                ring->end &= ~RINGENDCFLAG;
            else
                ring->end |= RINGENDCFLAG;
            pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Ring Re-Linked!!" DEBUGCOLOR_RESET" \n");
        }

        xhciInsertTRB(hc, ring, payload, trbflags, plen);
        pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "ring %p <idx %d, %dbytes>" DEBUGCOLOR_RESET" \n", ring, ring->next, plen);
        return ring->next++;
    }

    pciusbXHCIDebugTRB("xHCI", DEBUGWARNCOLOR_SET "NO SPACE ON RING!!\nnext = %u\nlast = %u" DEBUGCOLOR_RESET" \n", ring->next, (ring->end & ~RINGENDCFLAG));
    for(;;)
        ;
    return -1;
}

WORD xhciQueueData(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload,
                   ULONG plen, ULONG pmax, ULONG trbflags, BOOL ioconlast)
{
    ULONG remaining = plen;
    WORD queued, firstqueued = -1;

    do {
        ULONG trblen = remaining, txflags = trbflags;
        if (remaining > pmax)
            trblen = pmax;

        if (ioconlast && ((remaining - trblen) == 0))
            txflags |= TRBF_FLAG_IOC;

        queued = xhciQueueTRB(hc, ring, payload + (plen - remaining), trblen, txflags);
        if (queued == -1)
            return queued;

        if (firstqueued == -1)
            firstqueued = queued;

        remaining -= trblen;
    } while (remaining > 0);

    return firstqueued;
}

/*
 * Page Size Register (PAGESIZE) - Chapter 5.4.3
 */
static ULONG xhciPageSize(struct PCIController *hc)
{
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)hc->hc_XHCIOpR);
    int bit;
    for (bit = 0; bit < 16; bit++) {
        if (hcopr->pagesize & (1 << bit))
            return 1 << (12 + bit);
    }
    return 0;
}

static struct pcisusbXHCIDevice *
xhciCreateDeviceCtx(struct PCIController *hc,
                    UWORD rootPortIndex,   /* 0-based */
                    ULONG route,           /* 20-bit route string (0 for root) */
                    ULONG flags,           /* UHFF_* speed / hub flags */
                    UWORD mps0)            /* initial EP0 max packet size */
{
    struct pcisusbXHCIDevice *devCtx;
    UWORD ctxoff = 1;
    ULONG ctxsize;
    LONG  slotid;

    devCtx = AllocMem(sizeof(struct pcisusbXHCIDevice), MEMF_ANY | MEMF_CLEAR);
    if (!devCtx)
        return NULL;

    devCtx->dc_RootPort = rootPortIndex;
    devCtx->dc_DevAddr  = 0;   /* default address until SET_ADDRESS */

    ctxsize = (MAX_DEVENDPOINTS + 1) * sizeof(struct xhci_inctx);
    if (hc->hc_Flags & HCF_CTX64) {
        ctxsize <<= 1;
        ctxoff <<= 1;
    }

    /* ---- Allocate slot context ---- */
    devCtx->dc_SlotCtx.dmaa_Ptr = pciAllocAligned(
        hc, &devCtx->dc_SlotCtx.dmaa_Entry,
        ctxsize, ALIGN_CTX, xhciPageSize(hc));

    if (!devCtx->dc_SlotCtx.dmaa_Ptr) {
        pciusbError("xHCI",
          DEBUGWARNCOLOR_SET "Unable to allocate Device Slot Ctx" DEBUGCOLOR_RESET "\n");
        FreeMem(devCtx, sizeof(*devCtx));
        return NULL;
    }

#if !defined(PCIUSB_NO_CPUTOPCI)
    devCtx->dc_SlotCtx.dmaa_DMA =
        CPUTOPCI(hc, hc->hc_PCIDriverObject, devCtx->dc_SlotCtx.dmaa_Ptr);
#else
    devCtx->dc_SlotCtx.dmaa_DMA = devCtx->dc_SlotCtx.dmaa_Ptr;
#endif

    /* ---- Allocate input context ---- */
    devCtx->dc_IN.dmaa_Ptr = pciAllocAligned(
        hc, &devCtx->dc_IN.dmaa_Entry,
        ctxsize + sizeof(struct xhci_inctx),
        ALIGN_CTX, xhciPageSize(hc));

    if (!devCtx->dc_IN.dmaa_Ptr) {
        pciusbError("xHCI",
          DEBUGWARNCOLOR_SET "Unable to allocate Device IN Ctx" DEBUGCOLOR_RESET "\n");
        /* TODO free slot ctx properly */
        FreeMem(devCtx, sizeof(*devCtx));
        return NULL;
    }

#if !defined(PCIUSB_NO_CPUTOPCI)
    devCtx->dc_IN.dmaa_DMA =
        CPUTOPCI(hc, hc->hc_PCIDriverObject, devCtx->dc_IN.dmaa_Ptr);
#else
    devCtx->dc_IN.dmaa_DMA = devCtx->dc_IN.dmaa_Ptr;
#endif

    struct xhci_inctx *inctx = (struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
    volatile struct xhci_slot *islot = (volatile struct xhci_slot *)&inctx[ctxoff];

    /* ---- Fill Slot Context ---- */

    /* Root port (1-based) */
    islot->ctx[1] &= ~(0xFFUL << 16);
    islot->ctx[1] |= (((rootPortIndex + 1) & 0xFF) << 16);

    /* Route string (20 bits) */
    islot->ctx[0] &= ~SLOT_CTX_ROUTE_MASK;
    islot->ctx[0] |= (route & SLOT_CTX_ROUTE_MASK);

    /* Hub? */
    if (flags & UHFF_HUB)
        islot->ctx[0] |= (1 << 26);

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
        devCtx->dx_TxMax = 1024 * 16;   /* generous default for SS */
    } else if (flags & UHFF_HIGHSPEED) {
        devCtx->dx_TxMax = 512 * 13;    /* HS worst-case per microframe */
    } else if (flags & UHFF_LOWSPEED) {
        devCtx->dx_TxMax = 8 * 1;       /* LS: very small payloads */
    } else {
        devCtx->dx_TxMax = 64 * 19;     /* FS: conservative default */
    }

    /* ---- Enable Slot ---- */
    slotid = xhciCmdSlotEnable(hc);
    if (slotid < 0) {
        pciusbError("xHCI",
          DEBUGWARNCOLOR_SET "Failed to enable slot" DEBUGCOLOR_RESET "\n");
        /* TODO free ctx memory */
        FreeMem(devCtx, sizeof(*devCtx));
        return NULL;
    }

    devCtx->dc_SlotID = slotid;

    volatile struct xhci_address *deviceslots =
        (volatile struct xhci_address *)hc->hc_DCBAAp;

    xhciSetPointer(hc, deviceslots[slotid], devCtx->dc_SlotCtx.dmaa_DMA);

    inctx->acf = 0x01; /* Slot + EP0 */

    /* ---- Initialize EP0 ---- */
    ULONG epid = xhciInitEP(hc, devCtx,
                            NULL,
                            0, 0,
                            UHCMD_CONTROLXFER,
                            mps0,
                            0,
                            0);

    /* ---- Address Device ---- */
    if (1 != xhciCmdDeviceAddress(hc, slotid, devCtx->dc_IN.dmaa_DMA, NULL)) {
        pciusbError("xHCI",
          DEBUGWARNCOLOR_SET "Address Device failed" DEBUGCOLOR_RESET "\n");

        xhciCmdSlotDisable(hc, slotid);
        xhciSetPointer(hc, deviceslots[slotid], 0);

        devCtx->dc_SlotID = 0;
        /* TODO free contexts */
        FreeMem(devCtx, sizeof(*devCtx));
        return NULL;
    }

    struct xhci_ep *ep =
        (struct xhci_ep *)&inctx[ctxoff * (epid + 1)];
    ep->ctx[0] = ep->ctx[1] = 0;
    ep->deq.addr_hi = ep->deq.addr_lo = 0;
    ep->length = 0;

    /* Register in slot table */
    if (slotid > 0 && slotid < USB_DEV_MAX)
        hc->hc_Devices[slotid] = devCtx;

    return devCtx;
}

struct pcisusbXHCIDevice *
xhciObtainDeviceCtx(struct PCIController *hc,
                    struct IOUsbHWReq *ioreq)
{
    UWORD devaddr       = ioreq->iouh_DevAddr;
    ULONG route         = ioreq->iouh_RouteString;
    UWORD rootPortIndex = (ioreq->iouh_RootPort > 0)
                            ? (ioreq->iouh_RootPort - 1)
                            : 0;

    struct pcisusbXHCIDevice *devCtx;

    /* Try your existing mapping first */
    devCtx = xhciFindDeviceCtx(hc, devaddr);
    if (devCtx)
        return devCtx;

    /* Non-zero address but no context = real error */
    if (devaddr != 0)
        return NULL;

    /* devaddr == 0:
     *   - route == 0  => root device, controller task MUST create it.
     *   - route != 0  => child device, we must create the slot now.
     */
    if (route == 0)
        return NULL;

    return xhciCreateDeviceCtx(hc,
                               rootPortIndex,
                               route,
                               ioreq->iouh_Flags,
                               ioreq->iouh_MaxPktSize);
}

ULONG xhciInitEP(struct PCIController *hc, struct pcisusbXHCIDevice *devCtx,
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

    pciusbXHCIDebugEP("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p, %u, %u, $%08x, %u)" DEBUGCOLOR_RESET" \n", __func__, hc, devCtx, endpoint, dir, type, maxpacket);

    epid = xhciGetEPID(endpoint, dir);

    pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "%s: EPID %u" DEBUGCOLOR_RESET" \n", __func__, epid);

    /* Test if already prepared */
    if (devCtx->dc_EPAllocs[epid].dmaa_Ptr != NULL)
        return epid;

    devCtx->dc_EPAllocs[epid].dmaa_Ptr = pciAllocAligned(hc, &devCtx->dc_EPAllocs[epid].dmaa_Entry, sizeof(struct pcisusbXHCIRing), ALIGN_EVTRING_SEG, (1 << 16));
    if (devCtx->dc_EPAllocs[epid].dmaa_Ptr) {
        pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "Allocated EP Ring @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", devCtx->dc_EPAllocs[epid].dmaa_Ptr, hc->hc_ERS.me_Un.meu_Addr, hc->hc_ERS.me_Length);
#if (0)
        devCtx->dc_EPAllocs[epid].dmaa_DMA = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)devCtx->dc_EPAllocs[epid].dmaa_Ptr);
#else
        devCtx->dc_EPAllocs[epid].dmaa_DMA = devCtx->dc_EPAllocs[epid].dmaa_Ptr;
#endif
        pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", devCtx->dc_EPAllocs[epid].dmaa_DMA);
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "Unable to allocate EP Ring Memory" DEBUGCOLOR_RESET" \n");
        return 0;
    }
    epring = (volatile struct pcisusbXHCIRing *)devCtx->dc_EPAllocs[epid].dmaa_Ptr;
    xhciInitRing((struct pcisusbXHCIRing *)epring);

    volatile struct xhci_inctx *in = (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
    in->acf |= (1 << epid);                                                                             // Add/Enable in the Add Context Flags
    in->acf |= 0x01;                                                                                    // Always refresh slot context too

    UWORD ctxoff = 1;
    if (hc->hc_Flags & HCF_CTX64)
        ctxoff <<= 1;

    struct xhci_slot *islot = (void*)&in[ctxoff];
    if (ioreq) {
        ULONG slotctx0 = islot->ctx[0] & ~((SLOT_CTX_ROUTE_MASK) | (0xF << SLOTS_CTX_SPEED) | SLOTF_CTX_MTT);
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

        slotctx0 |= (islot->ctx[0] & (~(SLOT_CTX_ROUTE_MASK | (0xF << SLOTS_CTX_SPEED) | SLOTF_CTX_MTT)));
        islot->ctx[0] = slotctx0;

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
    if (epid > ((islot->ctx[0] >> 27) & 0xF)) {
        islot->ctx[0] &= ~(0xF << 27);
        islot->ctx[0] |= (epid << 27);
    }

    struct xhci_ep *ep = (struct xhci_ep *)&in[ctxoff * (epid + 1)];

    memset((void *)ep, 0, sizeof(*ep));

    pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "EP input Ctx @ 0x%p" DEBUGCOLOR_RESET" \n", ep);

    ep->ctx[1] &= ~((ULONG)7 << EPS_CTX_TYPE);
    switch (type) {
    case UHCMD_ISOXFER:
        ep->ctx[1] |= (dir == UHDIR_IN) ? EPF_CTX_TYPE_ISOCH_I : EPF_CTX_TYPE_ISOCH_O;
        break;

    case UHCMD_BULKXFER:
        ep->ctx[1] |= (dir == UHDIR_IN) ? EPF_CTX_TYPE_BULK_I : EPF_CTX_TYPE_BULK_O;
        break;

    case UHCMD_INTXFER:
        ep->ctx[1] |= (dir == UHDIR_IN) ? EPF_CTX_TYPE_INTR_I : EPF_CTX_TYPE_INTR_O;
        break;

    case UHCMD_CONTROLXFER:
        ep->ctx[1] |= EPF_CTX_TYPE_CONTROL;
        break;
    }
    if (epid > 1) {
        ULONG avglen = maxpacket;
        UBYTE multval = 0;

        if (ioreq) {
            if (flags & UHFF_SUPERSPEED)
                multval = ioreq->iouh_SS_Mult;
            else if ((flags & UHFF_HIGHSPEED) && ((flags & UHFF_MULTI_2) || (flags & UHFF_MULTI_3))) {
                UBYTE transactions = 1;

                if (flags & UHFF_MULTI_3)
                    transactions = 3;
                else if (flags & UHFF_MULTI_2)
                    transactions = 2;

                multval = transactions - 1;
            }

            ep->ctx[0] |= EPF_CTX_MULT(multval);

            if (flags & UHFF_SUPERSPEED) {
                ep->ctx[1] &= ~(0xFF << EPS_CTX_MAXBURST);
                ep->ctx[1] |= EPF_CTX_MAXBURST(ioreq->iouh_SS_MaxBurst);

                if ((type == UHCMD_ISOXFER) || (type == UHCMD_INTXFER)) {
                    if (ioreq->iouh_SS_BytesPerInterval)
                        avglen = ioreq->iouh_SS_BytesPerInterval;
                }
            } else if ((type == UHCMD_ISOXFER) || (type == UHCMD_INTXFER)) {
                /* High-bandwidth HS endpoints: scale payload by Mult+1 */
                avglen = maxpacket * (ULONG)(multval + 1);
            }
        }

        if (type == UHCMD_CONTROLXFER) {
            if (avglen < 8)
                avglen = 8;                                                                             // Minimum control packet size
        }

        ep->length = avglen;                                                                            // Avg TRB Length / Max ESIT payload

        UBYTE ival = xhciCalcInterval(interval, flags, type);
        if (ival)
            ep->ctx[0] |= ((ULONG)ival << 16);
    }
    ep->ctx[1] |= (EP_CTX_CERR_MASK << EPS_CTX_CERR);                                                   // Set CErr's initial value, and max packets
    ep->ctx[1] |= (maxpacket << EPS_CTX_PACKETMAX);

    pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "Setting de-queue ptr to 0x%p" DEBUGCOLOR_RESET" \n", devCtx->dc_EPAllocs[epid].dmaa_DMA);

    IPTR deqptr = (IPTR)devCtx->dc_EPAllocs[epid].dmaa_DMA;
    deqptr &= ~0xF;                                                                                     // Mask reserved bits
    deqptr |= EPF_CTX_DEQ_DCS;

    xhciSetPointer(hc, ep->deq, deqptr);

    pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "%s: Endpoint Ring Initialized @ 0x%p <EPID %u>" DEBUGCOLOR_RESET" \n", __func__, devCtx->dc_EPAllocs[epid].dmaa_Ptr, epid);

    return  epid;
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

    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
    if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL) {
        ioreq->iouh_DriverPrivate1 = NULL;
        /* Deactivate the endpoint */
        if ((driprivate->dpEPID > 1) && (driprivate->dpDevice)) {
            struct pcisusbXHCIDevice *devCtx = driprivate->dpDevice;
            struct pcisusbXHCIRing *epRing = devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr;
            int cnt;

            xhciCmdEndpointStop(hc, driprivate->dpDevice->dc_SlotID, driprivate->dpEPID, TRUE);

            if (driprivate->dpSTRB != (UWORD)-1) {
                epRing->ringio[driprivate->dpSTRB] = NULL;
            }

            for (cnt = driprivate->dpTxSTRB; cnt < (driprivate->dpTxETRB + 1); cnt ++) {
                epRing->ringio[cnt] = NULL;
            }

            if (driprivate->dpSttTRB != (UWORD)-1) {
                epRing->ringio[driprivate->dpSttTRB] = NULL;
            }

            FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Entry.me_Un.meu_Addr);
            devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Entry.me_Un.meu_Addr = NULL;
            devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr = NULL;
        }
        FreeMem(driprivate, sizeof(struct pciusbXHCIIODevPrivate));
    }
    devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
    pciusbXHCIDebugEP("xHCI", DEBUGCOLOR_SET "%s: releasing DevEP %02lx" DEBUGCOLOR_RESET" \n", __func__, devadrep);
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

    default:
        ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
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
        pciusbXHCIDebug("xHCI",
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
        devadrep = (ioreq->iouh_DevAddr << 5) +
                   ioreq->iouh_Endpoint +
                   ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);

        /*
         * First, look at completion code from the event TRB
         * (set in xhciIntWorkProcess).
         */
        if (driprivate->dpCC > 0) {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "Periodic IOReq %p complete (CC=%u)" DEBUGCOLOR_RESET"\n",
                            ioreq, driprivate->dpCC);

            xhciIOErrfromCC(ioreq, driprivate->dpCC);

            transactiondone = TRUE;
        } else {
            /*
             * No CC yet – check for software NAK timeout
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

            if (!ioreq->iouh_Req.io_Error &&
                ioreq->iouh_Data &&
                actual > 0)
            {
                UBYTE *b = (UBYTE *)ioreq->iouh_Data;
                pciusbXHCIDebug("xHCI",
                                DEBUGCOLOR_SET "Periodic data (dev=%u,ep=%u): first byte=0x%02x len=%lu" DEBUGCOLOR_RESET"\n",
                                ioreq->iouh_DevAddr,
                                ioreq->iouh_Endpoint,
                                b[0],
                                (unsigned long)actual);

                /* Optional: dump a few bytes if you need */
#if 0
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
             * NOTE: do NOT adjust iouh_Actual here – that is done by
             * xhciIntWorkProcess when it decodes the Event TRB.
             */
            xhciFreePeriodicContext(hc, unit, ioreq);
            ReplyMsg(&ioreq->iouh_Req.io_Message);
        }

        ioreq = nextioreq;
    }

    /* ASYNC (CTRL/BULK) COMPLETIONS */
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Checking for Standard work done..." DEBUGCOLOR_RESET" \n");
    ioreq = (struct IOUsbHWReq *)hc->hc_TDQueue.lh_Head;
    while ((nextioreq = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ)) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Examining IOReq=0x%p" DEBUGCOLOR_RESET" \n", ioreq);

        driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        if (driprivate) {
            transactiondone = FALSE;
            devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint
                       + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);

            if (driprivate->dpCC > TRB_CC_INVALID) {
                pciusbXHCIDebug("xHCI",
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

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    xhciUpdateFrameCounter(hc);

    uhwCheckRootHubChanges(hc->hc_Unit);

    Signal(hc->hc_xHCTask, 1L<<hc->hc_DoWorkSignal);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%s Done" DEBUGCOLOR_RESET" \n", __func__);

    return FALSE;

    AROS_INTFUNC_EXIT
}

static BOOL xhciTRBCycleMatches(ULONG trbflags, ULONG cycle)
{
    if ((trbflags & TRBF_FLAG_C) == (cycle ? 1 : 0))
        return TRUE;
    return FALSE;
}

BOOL xhciIntWorkProcess(struct PCIController *hc, struct IOUsbHWReq *ioreq, ULONG remaining, ULONG ccode)
{
    struct pciusbXHCIIODevPrivate *driprivate = NULL;
    pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Examining IOReq=0x%p" DEBUGCOLOR_RESET" \n", ioreq);

    if (!ioreq) {
        pciusbXHCIDebugTRB("xHCI", DEBUGWARNCOLOR_SET "IOReq missing for completion (cc=%lu)" DEBUGCOLOR_RESET" \n", (ULONG)ccode);
        return FALSE;
    }

    if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL) {
        pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "IOReq TRB(s) = #%u:#%u\nIOReq Ring @ 0x%p" DEBUGCOLOR_RESET" \n", driprivate->dpTxSTRB, driprivate->dpTxETRB, driprivate->dpDevice->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr);

        driprivate->dpCC = ccode;

        ULONG transferred = (remaining <= ioreq->iouh_Length)
                                ? (ioreq->iouh_Length - remaining)
                                : ioreq->iouh_Length;

        /*
         * Some controllers report a full "remaining" count for INT IN
         * completions even when data was delivered. Treat a successful
         * completion with zero computed length as a short packet carrying
         * the requested payload so the caller sees the status byte.
         */
        if ((ccode == TRB_CC_SUCCESS) &&
            (transferred == 0) &&
            (ioreq->iouh_Dir == UHDIR_IN) &&
            (ioreq->iouh_Length > 0))
        {
            transferred = ioreq->iouh_Length;
        }

        ioreq->iouh_Actual = transferred;
        pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Remaining work for IO = %u bytes" DEBUGCOLOR_RESET" \n", remaining);

        return TRUE;
    }

    /*
     * Command-ring submissions expect an IOReq but do not always
     * carry driver-private transfer state. Treat these as completed so
     * the higher layers can observe the result and progress.
     */
    ioreq->iouh_Req.io_Error = (ccode == TRB_CC_SUCCESS) ? 0 : UHIOERR_HOSTERROR;
    ioreq->iouh_Actual += (ioreq->iouh_Length - remaining);

    pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Completion w/o private state, treating as done (cc=%lu)" DEBUGCOLOR_RESET" \n", (ULONG)ccode);

    return TRUE;
}

static AROS_INTH1(xhciIntCode, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)hc->hc_XHCIOpR);
    BOOL doCompletion = FALSE, checkRHchanges = FALSE;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    ULONG status = AROS_LE2LONG(hcopr->usbsts);
    xhciDumpStatus(status);

    /* First acknowledge the interrupt ..*/
    hcopr->usbsts = AROS_LONG2LE(status);

    /* Check if anything interesting happened.... */
    if (status & XHCIF_USBSTS_HCE) {
        pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Host Error detected" DEBUGCOLOR_RESET" \n");
    }
    if (status & XHCIF_USBSTS_HSE) {
        pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "System Error detected" DEBUGCOLOR_RESET" \n");
    }
    if (status & XHCIF_USBSTS_PCD) {
        pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Port Change detected" DEBUGCOLOR_RESET" \n");
    }

    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)hc->hc_XHCIIntR);
    xhciDumpIR(xhciir);

    ULONG iman = AROS_LE2LONG(xhciir->iman), tmp;
    xhciir->iman = AROS_LONG2LE(XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP);
    tmp = AROS_LE2LONG(xhciir->iman);

    if ((iman & (XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP)) == (XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP)) {
        volatile struct pcisusbXHCIRing *ering = (volatile struct pcisusbXHCIRing *)((IPTR)hc->hc_ERSp);
        volatile struct xhci_trb *etrb;
        ULONG idx = ering->next, cycle = (ering->end & RINGENDCFLAG) ? 1 : 0;
        UWORD maxwork = 10;

        pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Processing events..." DEBUGCOLOR_RESET" \n");

        for (etrb = &ering->ring[idx]; (maxwork > 0) && xhciTRBCycleMatches(etrb->flags, cycle); maxwork--) {
            ULONG trbe_type = (etrb->flags >> TRBS_FLAG_TYPE) & TRB_FLAG_TYPE_SMASK;
            ULONG trbe_ccode = (etrb->tparams >> 24) & 0XFF;

            pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Event Ring 0x%p[%u] = <%sTRB 0x%p, type %u>\n       slot %u" DEBUGCOLOR_RESET" \n",
                           ering, idx,
                           (etrb->flags & (1 << 2)) ? "Event " : "",
                           etrb, trbe_type, ((etrb->flags >> 24) & 0xFF));
            xhciDumpCC(trbe_ccode);

            switch (trbe_type) {
            case TRBB_FLAG_ERTYPE_PORT_STATUS_CHANGE: {
#if __WORDSIZE == 64
                struct xhci_trb  *txtrb = (struct xhci_trb  *)(((IPTR)etrb->dbp.addr_hi << 32) | (IPTR)etrb->dbp.addr_lo);
#else
                struct xhci_trb  *txtrb = (struct xhci_trb  *)etrb->dbp.addr_lo;
#endif
                volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
                volatile struct xhci_trb_port_status *evt = (volatile struct xhci_trb_port_status *)&ering->current;

                UWORD hciport;

                ULONG origportsc, newportsc = 0;

                *evt = *(volatile struct xhci_trb_port_status *)etrb;

                pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Port Status Change detected <Port=#%u>\nPort Status Change TRB = <%p>" DEBUGCOLOR_RESET" \n", evt->port, txtrb);
                hciport = evt->port - 1;

                xhciDumpPort(&xhciports[hciport]);
                origportsc = AROS_LE2LONG(xhciports[hciport].portsc);

                // reflect port ownership (shortcut without hc->hc_PortNum[evt->port], as usb 2.0 maps 1:1)
                hc->hc_Unit->hu_PortOwner[hciport] = HCITYPE_XHCI;

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
                if (origportsc & XHCIF_PR_PORTSC_PEC) {
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

                xhciports[hciport].portsc = AROS_LONG2LE(newportsc);
                pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "RH Change $%08lx" DEBUGCOLOR_RESET" \n", hc->hc_PortChangeMap[hciport]);
                if(hc->hc_PortChangeMap[hciport]) {
                    hc->hc_Unit->hu_RootPortChanges |= (1UL << (hciport + 1));
                    if (((origportsc & XHCIF_PR_PORTSC_PED) && (!xhciFindPortDevice(hc, hciport))) ||
                            ((!(origportsc & XHCIF_PR_PORTSC_PED)) && (xhciFindPortDevice(hc, hciport)))) {
                        pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Signaling port change handler" DEBUGCOLOR_RESET" \n");

                        /* Connect/Disconnect the device */
                        Signal(hc->hc_xHCTask, 1L<<hc->hc_PortChangeSignal);
                    } else
                        checkRHchanges = TRUE;
                }
                break;
            }
            case TRBB_FLAG_ERTYPE_TRANSFER: {
#if __WORDSIZE == 64
                struct xhci_trb  *txtrb = (struct xhci_trb  *)(((IPTR)etrb->dbp.addr_hi << 32) | (IPTR)etrb->dbp.addr_lo);
#else
                struct xhci_trb  *txtrb = (struct xhci_trb  *)etrb->dbp.addr_lo;
#endif
                struct pcisusbXHCIRing *ring = RINGFROMTRB(txtrb);
                volatile struct xhci_trb  *evt = &ring->current;
                ULONG last = txtrb - ring->ring;
                ULONG event_rem = evt->tparams & 0x00FFFFFF;

                pciusbXHCIDebugTRB("xHCI",
                    DEBUGCOLOR_SET "TRANSFER EVT: ring=%p idx=%u slot=%u CC=%u rem=%lu" DEBUGCOLOR_RESET" \n",
                    ring, last,
                    (evt->flags >> 24) & 0xFF,
                    trbe_ccode,
                    (unsigned long)event_rem);
                xhciDumpCC(trbe_ccode);

                *evt = *etrb;
                ring->end &= RINGENDCFLAG;
                ring->end |= (last & ~RINGENDCFLAG);

                doCompletion = xhciIntWorkProcess(hc, (struct IOUsbHWReq *)ring->ringio[last], event_rem, trbe_ccode);
                ring->ringio[last] = NULL;
                break;
            }
            case TRBB_FLAG_ERTYPE_COMMAND_COMPLETE: {
#if __WORDSIZE == 64
                struct xhci_trb  *txtrb = (struct xhci_trb  *)(((IPTR)etrb->dbp.addr_hi << 32) | (IPTR)etrb->dbp.addr_lo);
#else
                struct xhci_trb  *txtrb = (struct xhci_trb  *)etrb->dbp.addr_lo;
#endif
                struct pcisusbXHCIRing *ring = RINGFROMTRB(txtrb);
                volatile struct xhci_trb  *evt = &ring->current;
                ULONG last = txtrb - ring->ring;

                pciusbXHCIDebugTRB("xHCI", DEBUGCOLOR_SET "Cmd TRB  = <Cmd Ring 0x%p[%u] TRB 0x%p>" DEBUGCOLOR_RESET" \n", ring, last, txtrb);
                xhciDumpCC(trbe_ccode);

                *evt = *etrb;
                ring->end &= RINGENDCFLAG;
                ring->end |= (last & ~RINGENDCFLAG);

                hc->hc_CmdResults[last].flags = evt->flags;
                hc->hc_CmdResults[last].tparams = evt->tparams;

                doCompletion = xhciIntWorkProcess(hc, (struct IOUsbHWReq *)ring->ringio[last], (evt->tparams & 0XFFFFFF), trbe_ccode);
                ring->ringio[last] = NULL;
                break;
            }
            default: {
                pciusbXHCIDebugTRB("xHCI", DEBUGWARNCOLOR_SET "Unknown event, type %d, completion code %d" DEBUGCOLOR_RESET" \n", trbe_type, trbe_ccode);
                break;
            }
            }

            ering->end &= RINGENDCFLAG;
            ering->end |= (idx & ~RINGENDCFLAG);

            /* update the hc deque pointer.. */
            volatile struct xhci_ir *ir = (volatile struct xhci_ir *)hc->hc_XHCIIntR;
            xhciSetPointer(hc, ir->erdp, (((IPTR)etrb) | XHCIF_IR_ERDP_EHB));

            /* and adjust the ring index.. */
            if (++idx == USB_DEV_MAX) {
                cycle = cycle ? 0 : 1;
                if (cycle)
                    ering->end |= RINGENDCFLAG;
                else
                    ering->end &= ~RINGENDCFLAG;
                idx = 0;
            }
            ering->next = idx;
            etrb = &ering->ring[idx];
        }

        if(doCompletion) {
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
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, ioreq);
    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
    ReplyMsg(&ioreq->iouh_Req.io_Message);
}

void xhciReset(struct PCIController *hc, struct PCIUnit *hu)
{
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)hc->hc_XHCIOpR);
    ULONG reg;
    ULONG cnt = 100;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, hu);

    // Tell the controller to stop if its currently running ...
    reg = AROS_LE2LONG(hcopr->usbcmd);
    if (reg & XHCIF_USBCMD_RS) {
        reg &= ~XHCIF_USBCMD_RS;
        hcopr->usbcmd = AROS_LONG2LE(reg);

        // Wait for the controller to indicate it is finished ...
        while ((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_HCH) && (--cnt > 0))
            uhwDelayMS(1, hu);
    }

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Resetting Controller..." DEBUGCOLOR_RESET" \n");
    hcopr->usbcmd = AROS_LONG2LE(XHCIF_USBCMD_HCRST);

    // Wait for the command to be accepted..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbcmd) & XHCIF_USBCMD_HCRST) && (--cnt > 0))
        uhwDelayMS(2, hu);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "COMMAND = $%08x, after %ums" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd), (100 - cnt) << 1);

    // Wait for the reset to complete..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_CNR) && (--cnt > 0))
        uhwDelayMS(2, hu);

    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "after %ums" DEBUGCOLOR_RESET" \n", (100 - cnt) << 1);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Configuring DMA pointers..." DEBUGCOLOR_RESET" \n");

    hcopr->config = AROS_LONG2LE(hc->hc_NumSlots);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting DCBAA to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMADCBAA);
    xhciSetPointer(hc, hcopr->dcbaap, hc->hc_DMADCBAA);
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting CRCR to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAOPR);
    xhciSetPointer(hc, hcopr->crcr, ((IPTR)hc->hc_DMAOPR | 1));

    volatile struct pcisusbXHCIRing *xring = (volatile struct pcisusbXHCIRing *)hc->hc_OPRp;
    xhciInitRing((struct pcisusbXHCIRing *)xring);

    volatile struct xhci_er_seg *erseg = (volatile struct xhci_er_seg *)hc->hc_ERSTp;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting Event Segment Pointer to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERS);
    xhciSetPointer(hc, erseg->ptr, ((IPTR)hc->hc_DMAERS));

    erseg->size = AROS_LONG2LE(USB_DEV_MAX);

    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)hc->hc_XHCIIntR);
    xhciir->erstsz = 1;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting ERDP to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERS);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Setting ERSTBA to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERST);
    xhciSetPointer(hc, xhciir->erdp, ((IPTR)hc->hc_DMAERS));
    xhciSetPointer(hc, xhciir->erstba, ((IPTR)hc->hc_DMAERST));

    xring = (volatile struct pcisusbXHCIRing *)hc->hc_ERSp;
    xhciInitRing((struct pcisusbXHCIRing *)xring);
    xhciir->iman = XHCIF_IR_IMAN_IE;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Reset complete..." DEBUGCOLOR_RESET" \n");

    xhciDumpOpR(hcopr);
    xhciDumpIR(xhciir);
}

AROS_UFH0(void, xhciControllerTask)
{
    AROS_USERFUNC_INIT

    volatile struct xhci_pr *xhciports;
    struct PCIController *hc;
    struct Task *thistask;
    struct pcisusbXHCIDevice *devCtx;
    ULONG portsc;
    UWORD hciport;

    thistask = FindTask(NULL);
    hc = thistask->tc_UserData;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    hc->hc_xHCTask = thistask;
    hc->hc_PortChangeSignal = AllocSignal(-1);
    hc->hc_DoWorkSignal = AllocSignal(-1);

    if (hc->hc_ReadySigTask)
        Signal(hc->hc_ReadySigTask, 1L << hc->hc_ReadySignal);

    xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);

    for (;;) {
        ULONG xhcictsigs = Wait((1 << hc->hc_DoWorkSignal) | (1 << hc->hc_PortChangeSignal));
#if defined(DEBUG) && (DEBUG > 1)
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xhciControllerTask @ 0x%p, IDnest %d TDNest %d"
                        DEBUGCOLOR_RESET" \n",
                        thistask, thistask->tc_IDNestCnt, thistask->tc_TDNestCnt);
#endif

        if (xhcictsigs & (1 << hc->hc_DoWorkSignal)) {
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Processing pending HC work" DEBUGCOLOR_RESET" \n");
            xhciHandleFinishedTDs(hc);

            if (hc->hc_IntXFerQueue.lh_Head->ln_Succ)
                xhciScheduleIntTDs(hc);

            if (hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
                xhciScheduleAsyncTDs(hc, &hc->hc_CtrlXFerQueue, UHCMD_CONTROLXFER);

            if (hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
                xhciScheduleAsyncTDs(hc, &hc->hc_BulkXFerQueue, UHCMD_BULKXFER);
        }

        if (xhcictsigs & (1 << hc->hc_PortChangeSignal)) {
            for (hciport = 0; hciport < hc->hc_NumPorts; hciport++) {
                portsc = AROS_LE2LONG(xhciports[hciport].portsc);
                devCtx = xhciFindPortDevice(hc, hciport);

                if ((portsc & XHCIF_PR_PORTSC_PED) && (!devCtx)) {
                    UWORD rootPort = hciport;     /* 0-based */
                    ULONG route    = 0;           /* root-attached */
                    ULONG flags    = 0;
                    UWORD mps0     = 8;           /* safe default */

                    /* Decode port speed from PORTSC */
                    ULONG speedBits = portsc & XHCI_PR_PORTSC_SPEED_MASK;

                    if (speedBits == XHCIF_PR_PORTSC_LOWSPEED) {
                        flags |= UHFF_LOWSPEED;
                        mps0   = 8;              /* LS EP0 = 8 bytes */
                    } else if (speedBits == XHCIF_PR_PORTSC_FULLSPEED) {
                        /* FS EP0 is 8/16/32/64 – start with 8 until bMaxPacketSize0 is known */
                        mps0   = 8;
                    } else if (speedBits == XHCIF_PR_PORTSC_HIGHSPEED) {
                        flags |= UHFF_HIGHSPEED;
                        mps0   = 64;             /* HS EP0 typically 64 bytes */
                    } else if (speedBits == XHCIF_PR_PORTSC_SUPERSPEED) {
                        flags |= UHFF_SUPERSPEED;
                        mps0   = 512;            /* SS EP0 max packet size */
                    } else {
                        /* Unknown/invalid speed – leave defaults, enumeration will adjust */
                    }

                    /* Root hub device: always a hub from the HC’s perspective */
                    devCtx = xhciCreateDeviceCtx(hc,
                                                 rootPort,
                                                 route,
                                                 flags | UHFF_HUB,
                                                 mps0);
                    if (devCtx) {
                        /* Root port “device” lives on this controller */
                        hc->hc_Unit->hu_DevControllers[0] = hc;
                    }
                } else if (!(portsc & XHCIF_PR_PORTSC_PED) && devCtx) {
                    if ((devCtx->dc_SlotID > 0) && (devCtx->dc_SlotID < USB_DEV_MAX))
                        hc->hc_Devices[devCtx->dc_SlotID] = NULL;

                    pciusbXHCIDebug("xHCI",
                                    DEBUGCOLOR_SET "Detaching HCI Device Ctx @ 0x%p"
                                    DEBUGCOLOR_RESET" \n",
                                    devCtx);

                    xhciSetPointer(hc,
                                   ((volatile struct xhci_address *)hc->hc_DCBAAp)[devCtx->dc_SlotID],
                                   0);
                    if (devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr)
                        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr);
                    if (devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr)
                        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr);
                    FreeMem(devCtx, sizeof(struct pcisusbXHCIDevice));
                }
            }
            uhwCheckRootHubChanges(hc->hc_Unit);
        }
    }
    AROS_USERFUNC_EXIT
}

BOOL xhciInit(struct PCIController *hc, struct PCIUnit *hu)
{
    struct PCIDevice *hd = hu->hu_Device;
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

    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "XHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)xhciCompleteInt;

    // Initialize hardware...
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (IPTR *) &hc->hc_RegBase);
    xhciregs = (volatile struct xhci_hccapr *)hc->hc_RegBase;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "CAPLENGTH: 0x%02x" DEBUGCOLOR_RESET" \n", xhciregs->caplength);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "DBOFF: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->dboff));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "RRSOFF: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->rrsoff));

    hc->hc_XHCIOpR = (APTR)((IPTR)xhciregs + xhciregs->caplength);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Operational Registers @ 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_XHCIOpR);
    hc->hc_XHCIDB = (APTR)((IPTR)xhciregs + AROS_LE2LONG(xhciregs->dboff));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Doorbells @ 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_XHCIDB);
    hc->hc_XHCIPorts = (APTR)((IPTR)hc->hc_XHCIOpR + 0x400);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Port Registers @ 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_XHCIPorts);
    hc->hc_XHCIIntR = (APTR)((IPTR)xhciregs + AROS_LE2LONG(xhciregs->rrsoff) + 0x20);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Interrupt Registers @ 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_XHCIIntR);

    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciMemEnableAttrs); // activate memory

    xhciECPOff = (AROS_LE2LONG(xhciregs->hcsparams2) >> XHCIS_HCCPARAMS1_ECP) & XHCI_HCCPARAMS1_ECP_SMASK;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Extended Capabilties Pointer = %04x" DEBUGCOLOR_RESET" \n", xhciECPOff);
    if (xhciECPOff >= 0x40) {
        xhciUSBLegSup = READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff);
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "xhciUSBLegSup = $%08x" DEBUGCOLOR_RESET" \n", xhciUSBLegSup);
        if (xhciUSBLegSup & XHCIF_USBLEGSUP_BIOSOWNED) {
            ULONG cnt, ownershipval = xhciUSBLegSup | XHCIF_USBLEGSUP_OSOWNED;

            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Taking ownership of XHCI from BIOS" DEBUGCOLOR_RESET" \n");
takeownership:
            cnt = 100;
            /*
                 * Change the ownership flag and read back to ensure it is written
                 */
            WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff, ownershipval);
            READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff);

            /*
                 * Wait for ownership change to take place.
                 * XHCI specification doesn't say how long it can take...
                 */
            while ((READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff) & XHCIF_USBLEGSUP_BIOSOWNED) && (--cnt > 0)) {
                pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Waiting for ownership to change..." DEBUGCOLOR_RESET" \n");
                uhwDelayMS(10, hu);
            }
            if ((ownershipval != XHCIF_USBLEGSUP_OSOWNED) &&
                    (READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff) & XHCIF_USBLEGSUP_BIOSOWNED)) {
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
            WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff, XHCIF_USBLEGSUP_OSOWNED);
            READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff);
        }

        /* Clear the SMI control bits */
        WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff + 4, 0);
        READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff + 4);
    }

    UWORD xhciversion;
    char *controllername = &hc->hc_Node.ln_Name[16];
    xhciversion = AROS_LE2LONG(xhciregs->hciversion) & 0xFFFF;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "HCIVERSION: 0x%04x" DEBUGCOLOR_RESET" \n", xhciversion);
    if (xhciversion == 0x0090)
        controllername[10] = '0';
    else if (xhciversion == 0x0100)
        controllername[10] = '1';
    else if (xhciversion == 0x0320)
        controllername[10] = '2';
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "HCSPARAMS1: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hcsparams1));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "HCSPARAMS2: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hcsparams2));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "HCSPARAMS3: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hcsparams3));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "HCCPARAMS: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hcsparams3));

    ULONG hcsparams1 = AROS_LE2LONG(xhciregs->hcsparams1);
    hc->hc_NumPorts = (ULONG)((hcsparams1 >> 24) & 0XFF);
    hc->hc_NumSlots = (ULONG)(hcsparams1 & 0XFF);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%d ports, %d slots" DEBUGCOLOR_RESET" \n", hc->hc_NumPorts, hc->hc_NumSlots);

    ULONG hccparams1 = AROS_LE2LONG(xhciregs->hccparams1);
    if (hccparams1 & XHCIF_HCCPARAMS1_CSZ)
        hc->hc_Flags |= HCF_CTX64;
    if (hccparams1 & XHCIF_HCCPARAMS1_AC64)
        hc->hc_Flags |= HCF_ADDR64;
    if (hccparams1 & XHCIF_HCCPARAMS1_PPC)
        hc->hc_Flags |= HCF_PPC;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%d byte context(s), %ubit addressing" DEBUGCOLOR_RESET" \n", (hc->hc_Flags & HCF_CTX64) ? 64 : 32, (hc->hc_Flags & HCF_ADDR64) ? 64 : 32);

    // Device Context Base Address Array (Chapter 6.1)
    hc->hc_DCBAAp = pciAllocAligned(hc, &hc->hc_DCBAA, sizeof(UQUAD) * (hc->hc_NumSlots + 1), ALIGN_DCBAA, xhciPageSize(hc));
    if (hc->hc_DCBAAp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated DCBAA @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", hc->hc_DCBAAp, hc->hc_DCBAA.me_Un.meu_Addr, hc->hc_DCBAA.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        hc->hc_DMADCBAA = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)hc->hc_DCBAAp);
#else
        hc->hc_DMADCBAA = hc->hc_DCBAAp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMADCBAA);
        memset(hc->hc_DCBAAp, 0, sizeof(UQUAD) * (hc->hc_NumSlots + 1));
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate DCBAA DMA Memory" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    // Event Ring Segment Table (Chapter 6.5)
    hc->hc_ERSTp = pciAllocAligned(hc, &hc->hc_ERST, sizeof(struct xhci_er_seg), ALIGN_EVTRING_TBL, ALIGN_EVTRING_TBL);
    if (hc->hc_ERSTp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated ERST @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", hc->hc_ERSTp, hc->hc_ERST.me_Un.meu_Addr, hc->hc_ERST.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        hc->hc_DMAERST = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)hc->hc_ERSTp);
#else
        hc->hc_DMAERST = hc->hc_ERSTp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERST);
        memset((void *)hc->hc_ERSTp, 0, sizeof(struct xhci_er_seg));
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate ERST DMA Memory" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    hc->hc_OPRp = pciAllocAligned(hc, &hc->hc_OPR, sizeof(struct pcisusbXHCIRing), ALIGN_CMDRING_SEG, (1 << 16));
    if (hc->hc_OPRp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated OPR @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", hc->hc_OPRp, hc->hc_OPR.me_Un.meu_Addr, hc->hc_OPR.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        hc->hc_DMAOPR = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)hc->hc_OPRp);
#else
        hc->hc_DMAOPR = hc->hc_OPRp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAOPR);
        xhciInitRing((struct pcisusbXHCIRing *)hc->hc_OPRp);
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate OPR DMA Memory" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    hc->hc_ERSp = pciAllocAligned(hc, &hc->hc_ERS, sizeof(struct pcisusbXHCIRing), ALIGN_EVTRING_SEG, (1 << 16));
    if (hc->hc_ERSp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated ERS @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", hc->hc_ERSp, hc->hc_ERS.me_Un.meu_Addr, hc->hc_ERS.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        hc->hc_DMAERS = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)hc->hc_ERSp);
#else
        hc->hc_DMAERS = hc->hc_ERSp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERS);
        xhciInitRing((struct pcisusbXHCIRing *)hc->hc_ERSp);
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate ERS DMA Memory" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    val = AROS_LE2LONG(xhciregs->hcsparams2);
    hc->hc_NumScratchPads = ((val >> 21) & 0x1f) << 5 | (val >> 27);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "SPB = %u" DEBUGCOLOR_RESET" \n", hc->hc_NumScratchPads);

    if (hc->hc_NumScratchPads) {
        ULONG pagesize = xhciPageSize(hc);
        ULONG spba_size = sizeof(struct xhci_address) * hc->hc_NumScratchPads;
        ULONG spb_size  = pagesize * hc->hc_NumScratchPads;

        hc->hc_SPBAp = pciAllocAligned(hc, &hc->hc_SPBA, spba_size, ALIGN_SPBA, pagesize);
        if (!hc->hc_SPBAp) {
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate SPBA DMA Memory" DEBUGCOLOR_RESET" \n");
            return FALSE;
        }

        hc->hc_DMASPBA = hc->hc_SPBAp;
        memset(hc->hc_SPBAp, 0, spba_size);

        hc->hc_SPBuffersp = pciAllocAligned(hc, &hc->hc_SPBuffers, spb_size, pagesize, pagesize);
        if (!hc->hc_SPBuffersp) {
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate Scratchpad Buffers" DEBUGCOLOR_RESET" \n");
            if (hc->hc_SPBA.me_Un.meu_Addr)
                FREEPCIMEM(hc, hc->hc_PCIDriverObject, hc->hc_SPBA.me_Un.meu_Addr);
            return FALSE;
        }

        hc->hc_DMASPBuffers = hc->hc_SPBuffersp;
        memset(hc->hc_SPBuffersp, 0, spb_size);

        volatile struct xhci_address *spba = (volatile struct xhci_address *)hc->hc_SPBAp;
        for (ULONG i = 0; i < hc->hc_NumScratchPads; i++)
            xhciSetPointer(hc, spba[i], (IPTR)hc->hc_DMASPBuffers + ((IPTR)pagesize * i));

        xhciSetPointer(hc, ((volatile struct xhci_address *)hc->hc_DCBAAp)[0], hc->hc_DMASPBA);

        pciusbXHCIDebug ("xHCI", DEBUGCOLOR_SET "Scratch buffers allocated (%u x 0x%x)" DEBUGCOLOR_RESET" \n",
                         hc->hc_NumScratchPads, pagesize);
    }

    // install reset handler
    hc->hc_ResetInt.is_Node.ln_Name = "XHCI PCI (pciusb.device)";
    hc->hc_ResetInt.is_Code = (VOID_FUNC)XhciResetHandler;
    hc->hc_ResetInt.is_Data = hc;
    AddResetCallback(&hc->hc_ResetInt);

#if defined(DEBUG) && (DEBUG > 0)
    IPTR PCIIntLine = 0;
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_INTLine, &PCIIntLine);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "IRQ = %u" DEBUGCOLOR_RESET" \n",PCIIntLine);
#endif
    // add interrupt
    hc->hc_PCIIntHandler.is_Node.ln_Name = hc->hc_ResetInt.is_Node.ln_Name;
    hc->hc_PCIIntHandler.is_Node.ln_Pri = 5;
    hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_PCIIntHandler.is_Code = (VOID_FUNC)xhciIntCode;
    hc->hc_PCIIntHandler.is_Data = hc;
    PCIXAddInterrupt(hc, &hc->hc_PCIIntHandler);

    for(cnt = 0; cnt < hc->hc_NumPorts; cnt++) {
        hu->hu_PortMapX[cnt] = hc;
        hc->hc_PortNum[cnt] = cnt;
    }

    xhciReset(hc, hu);

    /* Enable interrupts in the xhci */
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)hc->hc_XHCIOpR);
    val = AROS_LE2LONG(hcopr->usbcmd);
    val |= XHCIF_USBCMD_INTE;
    hcopr->usbcmd = AROS_LONG2LE(val);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBCMD = $%08x..." DEBUGCOLOR_RESET" \n",AROS_LE2LONG(hcopr->usbcmd));

    // Wait for the command to be accepted..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbcmd) & XHCIF_USBCMD_INTE) && (--cnt > 0))
        uhwDelayMS(2, hu);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "COMMAND = $%08x, after %ums" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd), (100 - cnt) << 1);

    // Wait for the controller to finish..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_CNR) && (--cnt > 0))
        uhwDelayMS(2, hu);

#if (1)
    struct Library *ps;
    ULONG sigmask = SIGF_SINGLE;
    hc->hc_ReadySignal = SIGB_SINGLE;
    hc->hc_ReadySigTask = FindTask(NULL);
    SetSignal(0, sigmask);
    if((ps = OpenLibrary("poseidon.library", 4))) {
        struct Task *tmptask;
        if((tmptask = psdSpawnSubTask("xHCI Controller Task", xhciControllerTask, hc))) {
            sigmask = Wait(sigmask);
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Port Task @ 0x%p, Sig = %u" DEBUGCOLOR_RESET" \n", hc->hc_xHCTask, hc->hc_PortChangeSignal);
        }
    }
    hc->hc_ReadySigTask = NULL;
#endif
    /* Enable the interrupter to generate interupts */
    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)hc->hc_XHCIIntR);
    xhciir->iman = XHCIF_IR_IMAN_IE;

    /* Finally, set the "run" bit */
    val = AROS_LE2LONG(hcopr->usbcmd);
    val |= XHCIF_USBCMD_RS | XHCIF_USBCMD_INTE;
    hcopr->usbcmd = AROS_LONG2LE(val);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "USBCMD = $%08x..." DEBUGCOLOR_RESET" \n",AROS_LE2LONG(hcopr->usbcmd));

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "xhciInit returns TRUE..." DEBUGCOLOR_RESET" \n");
    return TRUE;
}

void xhciFree(struct PCIController *hc, struct PCIUnit *hu)
{
}
#endif /* PCIUSB_ENABLEXHCI */
