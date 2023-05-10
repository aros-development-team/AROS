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

WORD ehciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
WORD ehciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void ehciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void ehciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void ehciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);

static inline struct EhciQH * ehciAllocQH(struct PCIController *hc);
static inline void ehciFreeQH(struct PCIController *hc, struct EhciQH *eqh);
static inline struct EhciTD * ehciAllocTD(struct PCIController *hc);
static inline void ehciFreeTD(struct PCIController *hc, struct EhciTD *etd);

BOOL ehciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL ehciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL ehciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval);

#ifdef base
#undef base
#endif
#define base (hc->hc_Device)
#if defined(AROS_USE_LOGRES)
#ifdef LogResBase
#undef LogResBase
#endif
#define LogResBase (base->hd_LogResBase)
#ifdef LogHandle
#undef LogHandle
#endif
#define LogHandle (hc->hc_LogRHandle)
#endif
/* /// "ehciAllocQH()" */
static inline struct EhciQH * ehciAllocQH(struct PCIController *hc) {
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct EhciQH *eqh = ehcihcp->ehc_EhciQHPool;

    if(!eqh)
    {
        // out of QHs!
        KPRINTF(20, "Out of QHs!\n");
        return NULL;
    }

    ehcihcp->ehc_EhciQHPool = (struct EhciQH *) eqh->eqh_Succ;
    return(eqh);
}
/* \\\ */

/* /// "ehciFreeQH()" */
static inline void ehciFreeQH(struct PCIController *hc, struct EhciQH *eqh) {
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    eqh->eqh_Succ = ehcihcp->ehc_EhciQHPool;
    ehcihcp->ehc_EhciQHPool = eqh;
}
/* \\\ */

/* /// "ehciAllocTD()" */
static inline struct EhciTD * ehciAllocTD(struct PCIController *hc) {
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct EhciTD *etd = ehcihcp->ehc_EhciTDPool;

    if(!etd)
    {
        // out of TDs!
        KPRINTF(20, "Out of TDs!\n");
        return NULL;
    }

    ehcihcp->ehc_EhciTDPool = (struct EhciTD *) etd->etd_Succ;
    return(etd);
}
/* \\\ */

/* /// "ehciFreeTD()" */
static inline void ehciFreeTD(struct PCIController *hc, struct EhciTD *etd) {
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    etd->etd_Succ = ehcihcp->ehc_EhciTDPool;
    ehcihcp->ehc_EhciTDPool = etd;
}
/* \\\ */
#undef base
#if defined(AROS_USE_LOGRES)
#undef LogResBase
#undef LogHandle
#endif
#endif /* EHCIPROTO_H */
