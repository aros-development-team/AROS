#ifndef XHCIPROTO_H
#define XHCIPROTO_H

#include "xhcichip.h"

struct IOUsbHWReq;

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

// xhcichip.c
BOOL xhciInit(struct PCIController *, struct PCIUnit *);
void xhciFree(struct PCIController *hc, struct PCIUnit *hu);

struct pciusbXHCIDevice *xhciFindDeviceCtx(struct PCIController *hc, UWORD devaddr);
struct pciusbXHCIDevice *xhciObtainDeviceCtx(struct PCIController *hc, struct IOUsbHWReq *ioreq, BOOL allowCreate);
void xhciFreeDeviceCtx(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, BOOL disableSlot);

void xhciUpdateFrameCounter(struct PCIController *hc);
void xhciAbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq);
void xhciDumpEndpointCtx(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, ULONG epid, const char *reason);

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval);

BOOL xhciQueueTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG plen, ULONG trbflags);
WORD xhciQueueData(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG plen, ULONG pmax, ULONG trbflags, BOOL ioconlast);

ULONG xhciInitEP(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, struct IOUsbHWReq *ioreq, UBYTE endpoint, UBYTE dir, ULONG type, ULONG maxpacket, UWORD interval, ULONG flags);
void xhciScheduleAsyncTDs(struct PCIController *hc, struct List *txlist, ULONG txtype);
void xhciScheduleIntTDs(struct PCIController *hc);
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
LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot, APTR dmaaddr, struct IOUsbHWReq *ioreq);
LONG xhciCmdEndpointStop(struct PCIController *hc, ULONG slot, ULONG epid, ULONG suspend);
LONG xhciCmdEndpointReset(struct PCIController *hc, ULONG slot, ULONG epid , ULONG preserve);
LONG xhciCmdEndpointConfigure(struct PCIController *hc, ULONG slot, APTR dmaaddr);
LONG xhciCmdContextEvaluate(struct PCIController *hc, ULONG slot, APTR dmaaddr);
LONG xhciCmdNoOp(struct PCIController *hc, ULONG slot, APTR dmaaddr);
#else
#define xhciRingDoorbell(hc,slot,value)						((volatile struct xhci_dbr *)((IPTR)hc->hc_XHCIDB))[slot].db = value;
#define xhciCmdSlotDisable(hc,slot)							xhciCmdSubmit(hc, NULL, (slot << 24) | TRBF_FLAG_CRTYPE_DISABLE_SLOT, NULL)
static inline LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot, APTR dmaaddr, struct IOUsbHWReq *ioreq)
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

struct pciusbXHCIDevice *xhciFindDeviceCtx(struct PCIController *hc, UWORD devaddr);

#if defined(PCIUSB_XHCI_DEBUG)
#define pciusbXHCIDebug(sub,fmt,args...) pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugTRB(sub,fmt,args...) pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugEP(sub,fmt,args...) pciusbDebug(sub,fmt,##args)
void xhciDumpIN(volatile struct xhci_inctx *in);
void xhciDumpEP(volatile struct xhci_ep *ep);
void xhciDumpSlot(volatile struct xhci_slot *slot);
void xhciDumpStatus(ULONG status);
void xhciDumpOpR(volatile struct xhci_hcopr *hcopr);
void xhciDumpIR(volatile struct xhci_ir *xhciir);
void xhciDumpPort(volatile struct xhci_pr *xhcipr);
void xhciDumpCC(UBYTE completioncode);
#else
#define pciusbXHCIDebug(sub,fmt,args...)
#define pciusbXHCIDebugTRB(sub,fmt,args...)
#define pciusbXHCIDebugEP(sub,fmt,args...)
#define xhciDumpIN(x)
#define xhciDumpEP(x)
#define xhciDumpSlot(x)
#define xhciDumpStatus(x)
#define xhciDumpOpR(x)
#define xhciDumpIR(x)
#define xhciDumpPort(x)
#define xhciDumpCC(x)
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
#if __WORDSIZE == 64
    if ((hc->hc_Flags & HCF_ADDR64) && trb)
        return (struct xhci_trb *)(((UQUAD)trb->dbp.addr_hi << 32) |
                                   (UQUAD)trb->dbp.addr_lo);
#else
    /* 32-bit addressing (or non-64-bit host) relies solely on the low dword. */
    (void)hc;
#endif
    return (struct xhci_trb *)(IPTR)(trb ? trb->dbp.addr_lo : 0);
}

#endif /* XHCIPROTO_H */
