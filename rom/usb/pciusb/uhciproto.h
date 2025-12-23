#ifndef UHCIPROTO_H
#define UHCIPROTO_H

#include "uhcichip.h"

WORD uhciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
WORD uhciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void uhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void uhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void uhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn);
void uhciHandleIsochTDs(struct PCIController *hc);

/* uhcichip.c, in order of appearance */
void uhciFreeQContext(struct PCIController *hc, struct UhciQH *uqh);
void uhciUpdateIntTree(struct PCIController *hc);
void uhciCheckPortStatusChange(struct PCIController *hc);
void uhciHandleFinishedTDs(struct PCIController *hc);
void uhciScheduleCtrlTDs(struct PCIController *hc);
void uhciScheduleIntTDs(struct PCIController *hc);
void uhciScheduleBulkTDs(struct PCIController *hc);
void uhciScheduleIsoTDs(struct PCIController *hc);
void uhciUpdateFrameCounter(struct PCIController *hc);
BOOL uhciInit(struct PCIController *hc, struct PCIUnit *hu);
void uhciFree(struct PCIController *hc, struct PCIUnit *hu);

static inline struct UhciQH * uhciAllocQH(struct PCIController *hc);
static inline void uhciFreeQH(struct PCIController *hc, struct UhciQH *uqh);
static inline struct UhciTD * uhciAllocTD(struct PCIController *hc);
static inline void uhciFreeTD(struct PCIController *hc, struct UhciTD *utd);

BOOL uhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL uhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval);
BOOL uhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval);

#if defined(PCIUSB_UHCI_DEBUG)
#define pciusbUHCIDebug(sub,fmt,args...) pciusbDebug(sub,fmt,##args)
#else
#define pciusbUHCIDebug(sub,fmt,args...)
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
/* /// "uhciAllocQH()" */
static inline struct UhciQH * uhciAllocQH(struct PCIController *hc) {
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    struct UhciQH *uqh = uhcihcp->uhc_UhciQHPool;

    if(!uqh)
    {
        // out of QHs!
        KPRINTF(20, "Out of QHs!\n");
        return NULL;
    }

    uhcihcp->uhc_UhciQHPool = (struct UhciQH *) uqh->uqh_Succ;

    uqh->uqh_SetupBuffer = NULL;
    uqh->uqh_DataBuffer = NULL;
    return(uqh);
}
/* \\\ */

/* /// "uhciFreeQH()" */
static inline void uhciFreeQH(struct PCIController *hc, struct UhciQH *uqh) {
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    uqh->uqh_Succ = (struct UhciXX *) uhcihcp->uhc_UhciQHPool;
    uhcihcp->uhc_UhciQHPool = uqh;
}
/* \\\ */

/* /// "uhciAllocTD()" */
static inline struct UhciTD * uhciAllocTD(struct PCIController *hc) {
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    struct UhciTD *utd = uhcihcp->uhc_UhciTDPool;

    if(!utd)
    {
        // out of TDs!
        KPRINTF(20, "Out of TDs!\n");
        return NULL;
    }

    uhcihcp->uhc_UhciTDPool = (struct UhciTD *) utd->utd_Succ;
    return(utd);
}
/* \\\ */

/* /// "uhciFreeTD()" */
static inline void uhciFreeTD(struct PCIController *hc, struct UhciTD *utd) {
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    utd->utd_Succ = (struct UhciXX *) uhcihcp->uhc_UhciTDPool;
    uhcihcp->uhc_UhciTDPool = utd;
}
/* \\\ */
#undef base
#if defined(AROS_USE_LOGRES)
#undef LogResBase
#undef LogHandle
#endif
#endif /* UHCIPROTO_H */
