#ifndef XHCIPROTO_H
#define XHCIPROTO_H

#include "xhci_hcd.h"

static const char xhciTimerDeviceName[] = "timer.device";

#if !defined(AROS_USE_LOGRES)
#if defined(DEBUG) && (DEBUG > 1)
#define XHCI_ENABLEINDEBUG
//#define XHCI_ENABLESLOTDEBUG
#define XHCI_ENABLEEPDEBUG
#define XHCI_ENABLESTATUSDEBUG
//#define XHCI_ENABLEOPRDEBUG
//#define XHCI_ENABLEIMANDEBUG
//#define XHCI_ENABLEIRDEBUG
#define XHCI_ENABLEPORTDEBUG
//#define XHCI_ENABLECCDEBUG
#endif
#endif

struct IOUsbHWReq;
struct timerequest;
struct pciusbXHCIDevice *
xhciCreateDeviceCtx(struct PCIController *hc, UWORD rootPortIndex, ULONG route, ULONG flags, UWORD mps0,
                    struct timerequest *timerreq);
WORD xhciPrepareTransfer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
LONG xhciPrepareEndpoint(struct IOUsbHWReq *ioreq);
void xhciDestroyEndpoint(struct IOUsbHWReq *ioreq);
ULONG xhciPageSize(struct PCIController *hc);
void xhciInitRing(struct PCIController *hc, struct pcisusbXHCIRing *ring);

static inline BOOL xhciOpenTaskTimer(struct MsgPort **msgport,
                                     struct timerequest **timerreq,
                                     const char *name)
{
    if((*msgport = CreateMsgPort())) {
        *timerreq = (struct timerequest *)CreateIORequest(*msgport, sizeof(struct timerequest));
        if(*timerreq) {
            if(!OpenDevice(xhciTimerDeviceName, UNIT_MICROHZ, (struct IORequest *)*timerreq, 0)) {
                (*timerreq)->tr_node.io_Message.mn_Node.ln_Name = (STRPTR)name;
                (*timerreq)->tr_node.io_Command = TR_ADDREQUEST;
                return TRUE;
            }
            DeleteIORequest((struct IORequest *)*timerreq);
            *timerreq = NULL;
        }
        DeleteMsgPort(*msgport);
        *msgport = NULL;
    }

    return FALSE;
}

static inline void xhciCloseTaskTimer(struct MsgPort **msgport, struct timerequest **timerreq)
{
    if(*timerreq) {
        CloseDevice((APTR)*timerreq);
        DeleteIORequest((struct IORequest *)*timerreq);
        *timerreq = NULL;
    }
    if(*msgport) {
        DeleteMsgPort(*msgport);
        *msgport = NULL;
    }
}

static inline UBYTE xhciEndpointID(UBYTE endpoint, UBYTE dir)
{
    UBYTE epid = 1;

    if(endpoint > 0) {
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

    if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER) {
        UWORD wLength = AROS_LE2WORD(ioreq->iouh_SetupData.wLength);
        if(wLength != 0) {
            effdir = (ioreq->iouh_SetupData.bmRequestType & 0x80) ? UHDIR_IN : UHDIR_OUT;
        }
    }

    return effdir;
}

static inline UWORD xhciDevEPKey(const struct IOUsbHWReq *ioreq)
{
    UWORD key = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint;

    /* EP0 is bidirectional: treat it as a single busy slot */
    if(ioreq->iouh_Endpoint != 0) {
        UWORD effdir = xhciEffectiveDataDir(ioreq);
        if(effdir == UHDIR_IN)
            key |= 0x10;
    }
    return key;
}

// xhcichip.c
BOOL xhciInit(struct PCIController *, struct PCIUnit *, struct timerequest *timerreq);
void xhciFree(struct PCIController *hc, struct PCIUnit *hu);

struct pciusbXHCIDevice *xhciFindDeviceCtx(struct PCIController *hc, UWORD devaddr);
struct pciusbXHCIDevice *xhciFindRouteDevice(struct PCIController *hc, ULONG route, UWORD rootPortIndex);
struct pciusbXHCIDevice *xhciObtainDeviceCtx(struct PCIController *hc, struct IOUsbHWReq *ioreq,
        BOOL allowCreate, struct timerequest *timerreq);
void xhciFreeDeviceCtx(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, BOOL disableSlot,
                       struct timerequest *timerreq);
void xhciDisconnectDevice(struct PCIController *hc, struct pciusbXHCIDevice *devCtx,
                          struct timerequest *timerreq);

void xhciUpdateFrameCounter(struct PCIController *hc);
void xhciAbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq);

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val,
                      WORD *retval);
BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval);

WORD xhciQueueTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG plen,
                  ULONG trbflags);
WORD xhciQueueTRB_IO(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload,
                     ULONG plen, ULONG trbflags, struct IORequest *ioreq);
WORD xhciQueueData(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG plen,
                   ULONG pmax, ULONG trbflags, BOOL ioconlast);
WORD xhciQueueData_IO(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG plen,
                      ULONG pmax, ULONG trbflags, BOOL ioconlast, struct IORequest *ioreq);

ULONG xhciInitEP(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, struct IOUsbHWReq *ioreq, UBYTE endpoint,
                 UBYTE dir, ULONG type, ULONG maxpacket, UWORD interval, ULONG flags);
void xhciScheduleAsyncTDs(struct PCIController *hc, struct List *txlist, ULONG txtype);
void xhciScheduleIntTDs(struct PCIController *hc);
void xhciScheduleIsoTDs(struct PCIController *hc);
void xhciHandleFinishedTDs(struct PCIController *hc, struct timerequest *timerreq);
void xhciFreeAsyncContext(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq);
void xhciFreePeriodicContext(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq);
void xhciFinishRequest(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq);

WORD xhciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
WORD xhciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void xhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void xhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void xhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);

// xhcihw.c
LONG xhciCmdSubmit(struct PCIController *hc, APTR inctx_dma, ULONG trbflags, ULONG *resflags,
                   struct timerequest *timerreq);
LONG xhciCmdSubmitAsync(struct PCIController *hc, APTR inctx_dma, ULONG trbflags, struct IOUsbHWReq *ioreq);
LONG xhciCmdSlotEnable(struct PCIController *hc, struct timerequest *timerreq);
#if !defined(PCIUSB_INLINEXHCIOPS)
void xhciRingDoorbell(struct PCIController *hc, ULONG slot, ULONG value);
LONG xhciCmdSlotDisable(struct PCIController *hc, ULONG slot, struct timerequest *timerreq);
LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot, APTR dmaaddr, ULONG bsr,
                          struct IOUsbHWReq *ioreq, struct timerequest *timerreq);
LONG xhciCmdEndpointStop(struct PCIController *hc, ULONG slot, ULONG epid, ULONG suspend,
                         struct timerequest *timerreq);
LONG xhciCmdEndpointReset(struct PCIController *hc, ULONG slot, ULONG epid, ULONG preserve,
                          struct timerequest *timerreq);
LONG xhciCmdSetTRDequeuePtr(struct PCIController *hc, ULONG slot, ULONG epid, APTR dequeue_ptr,
                            BOOL dcs, struct timerequest *timerreq);
LONG xhciCmdEndpointConfigure(struct PCIController *hc, ULONG slot, APTR dmaaddr,
                              struct timerequest *timerreq);
LONG xhciCmdContextEvaluate(struct PCIController *hc, ULONG slot, APTR dmaaddr,
                            struct timerequest *timerreq);
LONG xhciCmdNoOp(struct PCIController *hc, ULONG slot, APTR dmaaddr,
                 struct timerequest *timerreq);
#else
#define xhciRingDoorbell(hc,slot,value) \
     do { \
        struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc); \
        XHCI_MMIO_BARRIER(); \
         ((volatile struct xhci_dbr *)((IPTR)xhcic->xhc_XHCIDB))[(slot)].db = AROS_LONG2LE(value); \
         (void)((volatile struct xhci_dbr *)((IPTR)xhcic->xhc_XHCIDB))[(slot)].db; \
     } while (0)
#define xhciCmdSlotDisable(hc,slot,timerreq) \
    xhciCmdSubmit(hc, NULL, (slot << 24) | TRBF_FLAG_CRTYPE_DISABLE_SLOT, NULL, timerreq)
static inline LONG xhciCmdDeviceAddress(struct PCIController *hc, ULONG slot, APTR dmaaddr, ULONG bsr,
                                        struct IOUsbHWReq *ioreq, struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | TRBF_FLAG_CRTYPE_ADDRESS_DEVICE;

    /* Address Device Command TRB: bit 9 = BSR (Block SetAddress Request) */
    if(bsr)
        flags |= TRBF_FLAG_ADDRDEV_BSR;

    if(ioreq)
        return xhciCmdSubmitAsync(hc, dmaaddr, flags, ioreq);

    return xhciCmdSubmit(hc, dmaaddr, flags, NULL, timerreq);
}
#define xhciCmdEndpointStop(hc,slot,epid,suspend,timerreq) \
    xhciCmdSubmit(hc, NULL, (slot << 24) | (suspend << 23) | (epid << 16) | TRBF_FLAG_CRTYPE_STOP_ENDPOINT, NULL, timerreq)
#define xhciCmdEndpointReset(hc,slot,epid,preserve,timerreq) \
    xhciCmdSubmit(hc, NULL, (slot << 24) | (epid << 16) | TRBF_FLAG_CRTYPE_RESET_ENDPOINT | (preserve << 9), NULL, timerreq)
static inline LONG xhciCmdSetTRDequeuePtr(struct PCIController *hc, ULONG slot, ULONG epid, APTR dequeue_ptr,
        BOOL dcs, struct timerequest *timerreq)
{
    ULONG flags = (slot << 24) | (epid << 16) | TRBF_FLAG_CRTYPE_SET_TR_DEQUEUE_PTR;
    UQUAD dma = 0;

    if(dequeue_ptr) {
#if !defined(PCIUSB_NO_CPUTOPCI)
        dma = (UQUAD)(IPTR)CPUTOPCI(hc, hc->hc_PCIDriverObject, dequeue_ptr);
#else
        dma = (UQUAD)(IPTR)dequeue_ptr;
#endif
    }

    if(dcs)
        dma |= 0x1ULL;

    return xhciCmdSubmit(hc, (APTR)(IPTR)dma, flags, NULL, timerreq);
}
#define xhciCmdEndpointConfigure(hc,slot,dmaaddr,timerreq) \
    xhciCmdSubmit(hc, dmaaddr, (slot << 24) | TRBF_FLAG_CRTYPE_CONFIGURE_ENDPOINT, NULL, timerreq)
#define xhciCmdContextEvaluate(hc,slot,dmaaddr,timerreq) \
    xhciCmdSubmit(hc, dmaaddr, (slot << 24) | TRBF_FLAG_CRTYPE_EVALUATE_CONTEXT, NULL, timerreq)
#define xhciCmdNoOp(hc,slot,dmaaddr,timerreq) \
    xhciCmdSubmit(hc, dmaaddr, TRBF_FLAG_TRTYPE_NOOP, NULL, timerreq)
#endif

#if defined(PCIUSB_XHCI_DEBUG)
#define pciusbXHCIDebug(sub,fmt,args...)                pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugTRB(sub,fmt,args...)             pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugRIO(sub,fmt,args...)             pciusbDebug(sub,fmt,##args)
#if defined(DEBUG) && (DEBUG > 1)
#define pciusbXHCIDebugV(sub,fmt,args...)         pciusbDebug(sub,fmt,##args)
#define pciusbXHCIDebugTRBV(sub,fmt,args...)      pciusbDebug(sub,fmt,##args)
#else
#define pciusbXHCIDebugV(sub,fmt,args...)
#define pciusbXHCIDebugTRBV(sub,fmt,args...)
#endif
void xhciDebugDumpDCBAAEntry(struct PCIController *hc, ULONG slotid);
void xhciDebugControlTransfer(struct IOUsbHWReq *ioreq);
#else
#define pciusbXHCIDebug(sub,fmt,args...)
#define pciusbXHCIDebugTRB(sub,fmt,args...)
#define pciusbXHCIDebugV(sub,fmt,args...)
#define pciusbXHCIDebugTRBV(sub,fmt,args...)
#define pciusbXHCIDebugRIO(sub,fmt,args...)
#define xhciDebugDumpDCBAAEntry(a,b)
#define xhciDebugControlTransfer(a)
#endif

#if defined(XHCI_ENABLEINDEBUG)
void xhciDumpIN(volatile struct xhci_inctx *in);
#else
#define xhciDumpIN(x)
#endif
#if defined(XHCI_ENABLESLOTDEBUG)
void xhciDumpSlot(volatile struct xhci_slot *slot, int);
void xhciDebugDumpSlotContext(struct PCIController *hc, volatile struct xhci_slot *slot);
#else
#define xhciDumpSlot(x,y)
#define xhciDebugDumpSlotContext(a,b)
#endif
#if defined(XHCI_ENABLEEPDEBUG)
#define pciusbXHCIDebugEP(sub,fmt,args...)              pciusbDebug(sub,fmt,##args)
#if defined(DEBUG) && (DEBUG > 1)
#define pciusbXHCIDebugEPV(sub,fmt,args...)       pciusbDebug(sub,fmt,##args)
#else
#define pciusbXHCIDebugEPV(sub,fmt,args...)
#endif
void xhciDumpEP(volatile struct xhci_ep *ep);
void xhciDumpEndpointCtx(struct PCIController *hc, struct pciusbXHCIDevice *devCtx, ULONG epid, const char *reason);
void xhciDebugDumpEndpointContext(struct PCIController *hc, volatile struct xhci_ep *ep, ULONG epid);
#else
#define pciusbXHCIDebugEP(sub,fmt,args...)
#define pciusbXHCIDebugEPV(sub,fmt,args...)
#define xhciDumpEP(x)
#define xhciDumpEndpointCtx(a,b,c,d)
#define xhciDebugDumpEndpointContext(a,b,c)
#endif
#if defined(XHCI_ENABLESTATUSDEBUG)
void xhciDumpStatus(ULONG status);
#else
#define xhciDumpStatus(x)
#endif
#if defined(XHCI_ENABLEOPRDEBUG)
void xhciDumpOpR(volatile struct xhci_hcopr *hcopr);
#else
#define xhciDumpOpR(x)
#endif
#if defined(XHCI_ENABLEIMANDEBUG)
#else
#endif
#if defined(XHCI_ENABLEIRDEBUG)
void xhciDumpIR(volatile struct xhci_ir *xhciir);
#else
#define xhciDumpIR(x)
#endif
#if defined(XHCI_ENABLEPORTDEBUG)
void xhciDumpPort(volatile struct xhci_pr *xhcipr);
#else
#define xhciDumpPort(x)
#endif
#if defined(XHCI_ENABLECCDEBUG)
void xhciDumpCC(UBYTE completioncode);
#else
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
    UQUAD dma;

    if(!trb)
        return (struct xhci_trb *)NULL;

    /*
     * Event TRBs report TRB pointers in DMA/bus address space.  Convert those
     * addresses back into a CPU pointer before using them for ring lookups or
     * IOReq association.
     *
     * This is required regardless of whether the controller supports 64-bit
     * addressing: the pointer is still a DMA address.
     */
    dma  = (UQUAD)AROS_LE2LONG(trb->dbp.addr_lo);
    dma |= ((UQUAD)AROS_LE2LONG(trb->dbp.addr_hi)) << 32;

#if !defined(PCIUSB_NO_CPUTOPCI)
    return (struct xhci_trb *)PCITOCPU(hc, hc->hc_PCIDriverObject, (APTR)(IPTR)dma);
#else
    return (struct xhci_trb *)(IPTR)dma;
#endif
}

AROS_UFP0(void, xhciPortTask);
AROS_UFP0(void, xhciEventRingTask);
#endif /* XHCIPROTO_H */
