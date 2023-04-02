#ifndef UHCIPROTO_H
#define UHCIPROTO_H

/* uhcichip.c, in order of appearance */
void uhciFreeQContext(struct PCIController *hc, struct UhciQH *uqh);
void uhciUpdateIntTree(struct PCIController *hc);
void uhciCheckPortStatusChange(struct PCIController *hc);
void uhciHandleFinishedTDs(struct PCIController *hc);
void uhciScheduleCtrlTDs(struct PCIController *hc);
void uhciScheduleIntTDs(struct PCIController *hc);
void uhciScheduleBulkTDs(struct PCIController *hc);
void uhciUpdateFrameCounter(struct PCIController *hc);
BOOL uhciInit(struct PCIController *hc, struct PCIUnit *hu);
void uhciFree(struct PCIController *hc, struct PCIUnit *hu);

static inline struct UhciQH * uhciAllocQH(struct PCIController *hc);
static inline void uhciFreeQH(struct PCIController *hc, struct UhciQH *uqh);
static inline struct UhciTD * uhciAllocTD(struct PCIController *hc);
static inline void uhciFreeTD(struct PCIController *hc, struct UhciTD *utd);

#endif /* UHCIPROTO_H */
