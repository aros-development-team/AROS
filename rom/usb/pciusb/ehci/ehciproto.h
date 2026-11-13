#ifndef EHCIPROTO_H
#define EHCIPROTO_H

#include "ehcichip.h"

/* ehcichip.c, in order of appearance */
void ehciFreeAsyncContext(struct PCIController *hc, struct IOUsbHWReq *ioreq);
void ehciFreePeriodicContext(struct PCIController *hc, struct IOUsbHWReq *ioreq);
void ehciFreeQHandTDs(struct PCIController *hc, struct EhciQH *eqh);
void ehciUpdateIntTree(struct PCIController *hc);
void ehciHandleFinishedTDs(struct PCIController *hc);
void ehciHandleIsochTDs(struct PCIController *hc);
void ehciCheckPortStatusChange(struct PCIController *hc);
void ehciScheduleCtrlTDs(struct PCIController *hc);
void ehciScheduleIntTDs(struct PCIController *hc);
void ehciScheduleBulkTDs(struct PCIController *hc);
void ehciScheduleIsoTDs(struct PCIController *hc);
void ehciUpdateFrameCounter(struct PCIController *hc);
BOOL ehciInit(struct PCIController *hc, struct PCIUnit *hu);
void ehciFree(struct PCIController *hc, struct PCIUnit *hu);

WORD ehciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
WORD ehciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void ehciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
BOOL ehciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void ehciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);

static inline struct EhciQH * ehciAllocQH(struct PCIController *hc);
static inline void ehciFreeQH(struct PCIController *hc, struct EhciQH *eqh);
static inline struct EhciTD * ehciAllocTD(struct PCIController *hc);
static inline void ehciFreeTD(struct PCIController *hc, struct EhciTD *etd);
static inline struct EhciITD * ehciAllocITD(struct PCIController *hc);
static inline void ehciFreeITD(struct PCIController *hc, struct EhciITD *itd);
static inline struct EhciSiTD * ehciAllocSiTD(struct PCIController *hc);
static inline void ehciFreeSiTD(struct PCIController *hc, struct EhciSiTD *sitd);

BOOL ehciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL ehciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL ehciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval);

#if defined(PCIUSB_EHCI_DEBUG)
#define pciusbEHCIDebug(sub,fmt,args...) pciusbDebug(sub,fmt,##args)
#else
#define pciusbEHCIDebug(sub,fmt,args...)
#endif

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

static inline struct EhciITD * ehciAllocITD(struct PCIController *hc) {
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct EhciITD *itd = ehcihcp->ehc_EhciITDPool;

    if(!itd)
    {
        KPRINTF(20, "Out of ITDs!\n");
        return NULL;
    }

    ehcihcp->ehc_EhciITDPool = itd->itd_Succ;
    return(itd);
}

static inline void ehciFreeITD(struct PCIController *hc, struct EhciITD *itd) {
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    itd->itd_Succ = ehcihcp->ehc_EhciITDPool;
    ehcihcp->ehc_EhciITDPool = itd;
}

static inline struct EhciSiTD * ehciAllocSiTD(struct PCIController *hc) {
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct EhciSiTD *sitd = ehcihcp->ehc_EhciSiTDPool;

    if(!sitd)
    {
        KPRINTF(20, "Out of SiTDs!\n");
        return NULL;
    }

    ehcihcp->ehc_EhciSiTDPool = sitd->sitd_Succ;
    return sitd;
}

static inline void ehciFreeSiTD(struct PCIController *hc, struct EhciSiTD *sitd) {
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    sitd->sitd_Succ = ehcihcp->ehc_EhciSiTDPool;
    ehcihcp->ehc_EhciSiTDPool = sitd;
}
#undef base
#if defined(AROS_USE_LOGRES)
#undef LogResBase
#undef LogHandle
#endif
#endif /* EHCIPROTO_H */
