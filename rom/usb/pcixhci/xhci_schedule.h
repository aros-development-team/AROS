#ifndef XHCICHIP_SCHEDULE_H
#define XHCICHIP_SCHEDULE_H

#include "xhciproto.h"
#include "xhci_hcd.h"

UBYTE xhciEndpointIDFromIndex(UWORD wIndex);

APTR xhciPrepareDMABuffer(struct PCIController *hc, struct IOUsbHWReq *ioreq,
                          ULONG *dmalen, UWORD effdir, APTR *bounceOut);
void xhciReleaseDMABuffer(struct PCIController *hc, struct IOUsbHWReq *ioreq,
                          ULONG actual, UWORD effdir, APTR bounceBuf);

BOOL xhciInitIOTRBTransfer(struct PCIController *hc, struct IOUsbHWReq *ioreq,
                           struct List *ownerList, ULONG txtype, BOOL allowEp0AutoCreate,
                           struct timerequest *timerreq,
                           struct pciusbXHCIIODevPrivate **outPrivate);

ULONG xhciBuildDataTRBFlags(const struct IOUsbHWReq *ioreq, ULONG txtype);

BOOL xhciActivateEndpointTransfer(struct PCIController *hc, struct PCIUnit *unit,
                                  struct IOUsbHWReq *ioreq, struct pciusbXHCIIODevPrivate *driprivate,
                                  UWORD devadrep, volatile struct pcisusbXHCIRing **epringOut);

WORD xhciQueuePayloadTRBs(struct PCIController *hc, struct IOUsbHWReq *ioreq,
                          struct pciusbXHCIIODevPrivate *driprivate, volatile struct pcisusbXHCIRing *epring,
                          ULONG trbflags, BOOL iocOnLast);

#endif /* XHCICHIP_SCHEDULE_H */
