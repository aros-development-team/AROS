#ifndef EHCIPROTO_H
#define EHCIPROTO_H

/* ehcichip.c, in order of appearance */
void ehciFreeAsyncContext(struct PCIController *hc, struct IOUsbHWReq *ioreq);
void ehciFreePeriodicContext(struct PCIController *hc, struct IOUsbHWReq *ioreq);
void ehciFreeQHandTDs(struct PCIController *hc, struct EhciQH *eqh);
void ehciUpdateIntTree(struct PCIController *hc);
void ehciHandleFinishedTDs(struct PCIController *hc);
void ehciScheduleCtrlTDs(struct PCIController *hc);
void ehciScheduleIntTDs(struct PCIController *hc);
void ehciScheduleBulkTDs(struct PCIController *hc);
void ehciUpdateFrameCounter(struct PCIController *hc);
BOOL ehciInit(struct PCIController *hc, struct PCIUnit *hu);
void ehciFree(struct PCIController *hc, struct PCIUnit *hu);

static inline struct EhciQH * ehciAllocQH(struct PCIController *hc);
static inline void ehciFreeQH(struct PCIController *hc, struct EhciQH *eqh);
static inline struct EhciTD * ehciAllocTD(struct PCIController *hc);
static inline void ehciFreeTD(struct PCIController *hc, struct EhciTD *etd);

#endif /* EHCIPROTO_H */
