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

// xhcichip.c
BOOL xhciInit(struct PCIController *, struct PCIUnit *);
void xhciFree(struct PCIController *hc, struct PCIUnit *hu);

struct pciusbXHCIDevice *xhciFindDeviceCtx(struct PCIController *hc, UWORD devaddr);
struct pciusbXHCIDevice *xhciObtainDeviceCtx(struct PCIController *hc, struct IOUsbHWReq *ioreq, BOOL allowCreate);
void xhciFreeDeviceCtx(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, BOOL disableSlot);

void xhciUpdateFrameCounter(struct PCIController *hc);
void xhciAbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq);

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
        y.addr_hi = AROS_LONG2LE((ULONG)(((IPTR)(z) >> 32) & 0xFFFFFFFF)); \
    } \
    
#else
#define xhciSetPointer(x,y,z) \
    y.addr_lo = AROS_LONG2LE((ULONG)((IPTR)(z) & 0xFFFFFFFF)); \
    y.addr_hi = 0
#endif

#endif /* XHCIPROTO_H */
