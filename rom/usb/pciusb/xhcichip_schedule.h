#ifndef XHCICHIP_SCHEDULE_H
#define XHCICHIP_SCHEDULE_H

#include "xhciproto.h"
#include "xhcichip.h"

UBYTE xhciEndpointIDFromIndex(UWORD wIndex);

BOOL xhciInitIOTRBTransfer(struct PCIController *hc, struct IOUsbHWReq *ioreq,
        struct List *ownerList, ULONG txtype, BOOL allowEp0AutoCreate,
        struct pciusbXHCIIODevPrivate **outPrivate);

ULONG xhciBuildDataTRBFlags(const struct IOUsbHWReq *ioreq, ULONG txtype);

BOOL xhciActivateEndpointTransfer(struct PCIController *hc, struct PCIUnit *unit,
        struct IOUsbHWReq *ioreq, struct pciusbXHCIIODevPrivate *driprivate,
        UWORD devadrep, volatile struct pcisusbXHCIRing **epringOut);

WORD xhciQueuePayloadTRBs(struct PCIController *hc, struct IOUsbHWReq *ioreq,
        struct pciusbXHCIIODevPrivate *driprivate, volatile struct pcisusbXHCIRing *epring,
        ULONG trbflags, BOOL iocOnLast);

#endif /* XHCICHIP_SCHEDULE_H */
