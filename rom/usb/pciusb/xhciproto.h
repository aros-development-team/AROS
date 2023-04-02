#ifndef XHCIPROTO_H
#define XHCIPROTO_H

BOOL xhciInit(struct PCIController *, struct PCIUnit *);

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val);
BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val);
BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx);

#endif /* XHCIPROTO_H */
