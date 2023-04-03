#ifndef XHCIPROTO_H
#define XHCIPROTO_H

#include "xhcichip.h"

BOOL xhciInit(struct PCIController *, struct PCIUnit *);
void xhciFree(struct PCIController *hc, struct PCIUnit *hu);

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, UWORD *retval);
BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, UWORD *retval);
BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, UWORD *retval);

#endif /* XHCIPROTO_H */
