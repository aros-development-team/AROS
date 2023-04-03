#ifndef XHCIPROTO_H
#define XHCIPROTO_H

#include "xhcichip.h"

BOOL xhciInit(struct PCIController *, struct PCIUnit *);
void xhciFree(struct PCIController *hc, struct PCIUnit *hu);

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval);

#endif /* XHCIPROTO_H */
