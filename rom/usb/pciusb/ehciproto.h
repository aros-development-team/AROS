#ifndef EHCIPROTO_H
#define EHCIPROTO_H

#include "ehcichip.h"

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

BOOL ehciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, UWORD *retval);
BOOL ehciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, UWORD *retval);
BOOL ehciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, UWORD *retval);

/* /// "ehciAllocQH()" */
static inline struct EhciQH * ehciAllocQH(struct PCIController *hc)
{
    struct EhciQH *eqh = hc->hc_EhciQHPool;

    if(!eqh)
    {
        // out of QHs!
        KPRINTF(20, ("Out of QHs!\n"));
        return NULL;
    }

    hc->hc_EhciQHPool = (struct EhciQH *) eqh->eqh_Succ;
    return(eqh);
}
/* \\\ */

/* /// "ehciFreeQH()" */
static inline void ehciFreeQH(struct PCIController *hc, struct EhciQH *eqh)
{
    eqh->eqh_Succ = hc->hc_EhciQHPool;
    hc->hc_EhciQHPool = eqh;
}
/* \\\ */

/* /// "ehciAllocTD()" */
static inline struct EhciTD * ehciAllocTD(struct PCIController *hc)
{
    struct EhciTD *etd = hc->hc_EhciTDPool;

    if(!etd)
    {
        // out of TDs!
        KPRINTF(20, ("Out of TDs!\n"));
        return NULL;
    }

    hc->hc_EhciTDPool = (struct EhciTD *) etd->etd_Succ;
    return(etd);
}
/* \\\ */

/* /// "ehciFreeTD()" */
static inline void ehciFreeTD(struct PCIController *hc, struct EhciTD *etd)
{
    etd->etd_Succ = hc->hc_EhciTDPool;
    hc->hc_EhciTDPool = etd;
}
/* \\\ */

#endif /* EHCIPROTO_H */
