#ifndef XHCIPROTO_H
#define XHCIPROTO_H

#include "xhcichip.h"

struct IOUsbHWReq;
struct pciusbXHCIDevice *
xhciCreateDeviceCtx(struct PCIController *hc, UWORD rootPortIndex, ULONG route, ULONG flags, UWORD mps0);
WORD xhciPrepareTransfer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
LONG xhciPrepareEndpoint(struct IOUsbHWReq *ioreq);
void xhciDestroyEndpoint(struct IOUsbHWReq *ioreq);

static inline UBYTE xhciEndpointID(UBYTE endpoint, UBYTE dir)
{
    UBYTE epid = 1;

    if (endpoint > 0) {
        epid = (endpoint & 0x0F) * 2;
        epid += dir;
    }

    return epid;
}

static inline volatile struct xhci_slot *
xhciInputSlotCtx(volatile struct xhci_inctx *inctx, UWORD ctxoff)
{
    return (volatile struct xhci_slot *)&inctx[ctxoff];
}

static inline volatile struct xhci_ep *
xhciInputEndpointCtx(volatile struct xhci_inctx *inctx, UWORD ctxoff, ULONG epid)
{
    /*
     * Input contexts include the control context at index 0 and the slot
     * context at index 1, so endpoint contexts begin at index 2. The endpoint
     * ID numbering already accounts for the slot context; adjust by one more
     * to skip the control context when calculating the offset.
     */
    return (volatile struct xhci_ep *)&inctx[ctxoff * (epid + 1)];
}

static inline BOOL
xhciIsRootHubRequest(struct IOUsbHWReq *ioreq, struct PCIUnit *unit)
{
    BOOL devIsRootHub = unit && (ioreq->iouh_DevAddr == unit->hu_RootHubAddr);

    /* Only treat the request as a roothub transfer when no routing info is
       present. Requests carrying a route string are for downstream devices. */
    BOOL noRoutingInfo = (ioreq->iouh_RootPort == 0) &&
                         (ioreq->iouh_RouteString == 0);

    return devIsRootHub && noRoutingInfo;
}

/*
 * Derive the effective DATA direction for a transfer.
 *
 * Poseidon uses UHDIR_SETUP for control transfers. For control transfers with a
 * DATA stage (wLength != 0), the direction is determined by bmRequestType (bit 7).
 * For non-control transfers, iouh_Dir is authoritative.
 */
static inline UWORD xhciEffectiveDataDir(const struct IOUsbHWReq *ioreq)
{
    UWORD effdir = ioreq->iouh_Dir;

    if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER) {
        UWORD wLength = AROS_LE2WORD(ioreq->iouh_SetupData.wLength);
        if (wLength != 0) {
            effdir = (ioreq->iouh_SetupData.bmRequestType & 0x80) ? UHDIR_IN : UHDIR_OUT;
        }
    }

    return effdir;
}

static inline UWORD xhciDevEPKey(const struct IOUsbHWReq *ioreq)
{
    UWORD key = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint;

    /* EP0 is bidirectional: treat it as a single busy slot */
    if (ioreq->iouh_Endpoint != 0) {
        UWORD effdir = xhciEffectiveDataDir(ioreq);
        if (effdir == UHDIR_IN)
            key |= 0x10;
    }
    return key;
}

// xhcichip.c
BOOL xhciInit(struct PCIController *, struct PCIUnit *);
void xhciFree(struct PCIController *hc, struct PCIUnit *hu);

struct pciusbXHCIDevice *xhciFindDeviceCtx(struct PCIController *hc, UWORD devaddr);
struct pciusbXHCIDevice *xhciFindRouteDevice(struct PCIController *hc, ULONG route, UWORD rootPortIndex);
struct pciusbXHCIDevice *xhciObtainDeviceCtx(struct PCIController *hc, struct IOUsbHWReq *ioreq, BOOL allowCreate);
void xhciFreeDeviceCtx(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, BOOL disableSlot);
void xhciDisconnectDevice(struct PCIController *hc, struct pciusbXHCIDevice *devCtx);

void xhciUpdateFrameCounter(struct PCIController *hc);
void xhciAbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq);

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval);

WORD xhciQueueTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG plen, ULONG trbflags);
WORD xhciQueueData(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG plen, ULONG pmax, ULONG trbflags, BOOL ioconlast);

ULONG xhciInitEP(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, struct IOUsbHWReq *ioreq, UBYTE endpoint, UBYTE dir, ULONG type, ULONG maxpacket, UWORD interval, ULONG flags);
void xhciScheduleAsyncTDs(struct PCIController *hc, struct List *txlist, ULONG txtype);
void xhciScheduleIntTDs(struct PCIController *hc);
void xhciScheduleIsoTDs(struct PCIController *hc);
void xhciHandleFinishedTDs(struct PCIController *hc);
void xhciFreeAsyncContext(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq);
void xhciFreePeriodicContext(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq);
void xhciFinishRequest(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq);

WORD xhciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
WORD xhciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void xhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void xhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void xhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);

// xhcihw.c
LONG xhciCmdSubmit(struct PCIController *hc, APTR inctx_dma, ULONG trbflags, ULONG *resflags);
LONG xhciCmdSubmitAsync(struct PCIController *hc, APTR inctx_dma, ULONG trbflags, struct IOUsbHWReq *ioreq);
LONG xhciCmdSlotEnable(struct PCIController *hc);
#if !defined(PCIUSB_INLINEXHCIOPS)
void xhciRingDoorbell(struct PCIController *hc, ULONG slot, ULONG value);
LONG xhciCmdSlotDisable(struct PCIController *hc, ULONG slot);
LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot, APTR dmaaddr, ULONG bsr, struct IOUsbHWReq *ioreq);
LONG xhciCmdEndpointStop(struct PCIController *hc, ULONG slot, ULONG epid, ULONG suspend);
LONG xhciCmdEndpointReset(struct PCIController *hc, ULONG slot, ULONG epid , ULONG preserve);
LONG xhciCmdEndpointConfigure(struct PCIController *hc, ULONG slot, APTR dmaaddr);
LONG xhciCmdContextEvaluate(struct PCIController *hc, ULONG slot, APTR dmaaddr);
LONG xhciCmdNoOp(struct PCIController *hc, ULONG slot, APTR dmaaddr);
#else
#define xhciRingDoorbell(hc,slot,value) \
     do { \
        struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc); \
        XHCI_MMIO_BARRIER(); \
         ((volatile struct xhci_dbr *)((IPTR)xhcic->xhc_XHCIDB))[(slot)].db = AROS_LONG2LE(value); \
         (void)((volatile struct xhci_dbr *)((IPTR)xhcic->xhc_XHCIDB))[(slot)].db; \
     } while (0)
#define xhciCmdSlotDisable(hc,slot)							xhciCmdSubmit(hc, NULL, (slot << 24) | TRBF_FLAG_CRTYPE_DISABLE_SLOT, NULL)
static inline LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot, APTR dmaaddr, ULONG bsr, struct IOUsbHWReq *ioreq)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_ADDRESS_DEVICE;

    if (ioreq)
        return xhciCmdSubmitAsync(hc, dmaaddr, flags, ioreq);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL);
}
#define xhciCmdEndpointStop(hc,slot,epid,suspend)		xhciCmdSubmit(hc, NULL, (slot << 24) | (suspend << 23) | (epid << 16) | TRBF_FLAG_CRTYPE_STOP_ENDPOINT, NULL)
#define xhciCmdEndpointReset(hc,slot,epid,preserve) 	xhciCmdSubmit(hc, NULL, (slot << 24) | (epid << 16) | TRBF_FLAG_CRTYPE_RESET_ENDPOINT | (preserve << 9), NULL)
#define xhciCmdEndpointConfigure(hc,slot,dmaaddr)	xhciCmdSubmit(hc, dmaaddr, (slot << 24) | TRBF_FLAG_CRTYPE_CONFIGURE_ENDPOINT, NULL)
#define xhciCmdContextEvaluate(hc,slot,dmaaddr)		xhciCmdSubmit(hc, dmaaddr, (slot << 24) | TRBF_FLAG_CRTYPE_EVALUATE_CONTEXT, NULL)
#define xhciCmdNoOp(hc,slot,dmaaddr)						xhciCmdSubmit(hc, dmaaddr, TRBF_FLAG_TRTYPE_NOOP, NULL)
#endif

#if defined(PCIUSB_XHCI_DEBUG)
#define pciusbXHCIDebug(sub,fmt,args...)                pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugTRB(sub,fmt,args...)             pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugEP(sub,fmt,args...)              pciusbDebug(sub,fmt,##args)
#if defined(DEBUG) && (DEBUG > 1)
#define pciusbXHCIDebugV(sub,fmt,args...)         pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugTRBV(sub,fmt,args...)      pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugEPV(sub,fmt,args...)       pciusbDebug(sub,fmt,##args)
#else
#define pciusbXHCIDebugV(sub,fmt,args...)
#define pciusbXHCIDebugTRBV(sub,fmt,args...)
#define pciusbXHCIDebugEPV(sub,fmt,args...)
#endif
void xhciDumpIN(volatile struct xhci_inctx *in);
void xhciDumpEP(volatile struct xhci_ep *ep);
void xhciDumpSlot(volatile struct xhci_slot *slot);
void xhciDumpEndpointCtx(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, ULONG epid, const char *reason);
void xhciDumpStatus(ULONG status);
void xhciDumpOpR(volatile struct xhci_hcopr *hcopr);
void xhciDumpIR(volatile struct xhci_ir *xhciir);
void xhciDumpPort(volatile struct xhci_pr *xhcipr);
void xhciDumpCC(UBYTE completioncode);
void xhciDebugDumpDCBAAEntry(struct PCIController *hc, ULONG slotid);
void xhciDebugDumpSlotContext(struct PCIController *hc, volatile struct xhci_slot *slot);
void xhciDebugDumpEndpointContext(struct PCIController *hc, volatile struct xhci_ep *ep, ULONG epid);
void xhciDebugControlTransfer(struct IOUsbHWReq *ioreq);
#else
#define pciusbXHCIDebug(sub,fmt,args...)
#define pciusbXHCIDebugTRB(sub,fmt,args...)
#define pciusbXHCIDebugEP(sub,fmt,args...)
#define pciusbXHCIDebugV(sub,fmt,args...)
#define pciusbXHCIDebugTRBV(sub,fmt,args...)
#define pciusbXHCIDebugEPV(sub,fmt,args...)
#define xhciDumpIN(x)
#define xhciDumpEP(x)
#define xhciDumpSlot(x)
#define xhciDumpEndpointCtx(a,b,c,d)
#define xhciDumpStatus(x)
#define xhciDumpOpR(x)
#define xhciDumpIR(x)
#define xhciDumpPort(x)
#define xhciDumpCC(x)
#define xhciDebugDumpDCBAAEntry(a,b)
#define xhciDebugDumpSlotContext(a,b)
#define xhciDebugDumpEndpointContext(a,b,c)
#define xhciDebugControlTransfer(a)
#endif

/* Support functions */
#if __WORDSIZE == 64
#define xhciSetPointer(x,y,z) \
    if ((x)->hc_Flags & HCF_ADDR64) \
        y.addr64 = AROS_QUAD2LE((UQUAD)(z)); \
    else \
    { \
        y.addr_lo = AROS_LONG2LE((ULONG)((IPTR)(z) & 0xFFFFFFFF)); \
        y.addr_hi = 0; \
    } \

#else
#define xhciSetPointer(x,y,z) \
    y.addr_lo = AROS_LONG2LE((ULONG)((IPTR)(z) & 0xFFFFFFFF)); \
    y.addr_hi = 0
#endif

static inline struct xhci_trb *
xhciTRBPointer(struct PCIController *hc, volatile struct xhci_trb *trb)
{
    if (!trb)
        return (struct xhci_trb *)0;
#if __WORDSIZE == 64
    if ((hc->hc_Flags & HCF_ADDR64) && trb) {
        ULONG lo = AROS_LE2LONG(trb->dbp.addr_lo);
        ULONG hi = AROS_LE2LONG(trb->dbp.addr_hi);

        return (struct xhci_trb *)(((UQUAD)hi << 32) | (UQUAD)lo);
    }
#else
    /* 32-bit addressing (or non-64-bit host) relies solely on the low dword. */
    (void)hc;
#endif
    return (struct xhci_trb *)(IPTR)AROS_LE2LONG(trb->dbp.addr_lo);
}

AROS_UFP0(void, xhciPortTask);
AROS_UFP0(void, xhciEventRingTask);

#endif /* XHCIPROTO_H */
