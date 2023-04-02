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

BOOL uhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val);
BOOL uhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val);
BOOL uhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx);

/* /// "uhciAllocQH()" */
static inline struct UhciQH * uhciAllocQH(struct PCIController *hc)
{
    struct UhciQH *uqh = hc->hc_UhciQHPool;

    if(!uqh)
    {
        // out of QHs!
        KPRINTF(20, ("Out of QHs!\n"));
        return NULL;
    }

    hc->hc_UhciQHPool = (struct UhciQH *) uqh->uqh_Succ;

    uqh->uqh_SetupBuffer = NULL;
    uqh->uqh_DataBuffer = NULL;
    return(uqh);
}
/* \\\ */

/* /// "uhciFreeQH()" */
static inline void uhciFreeQH(struct PCIController *hc, struct UhciQH *uqh)
{
    uqh->uqh_Succ = (struct UhciXX *) hc->hc_UhciQHPool;
    hc->hc_UhciQHPool = uqh;
}
/* \\\ */

/* /// "uhciAllocTD()" */
static inline struct UhciTD * uhciAllocTD(struct PCIController *hc)
{
    struct UhciTD *utd = hc->hc_UhciTDPool;

    if(!utd)
    {
        // out of TDs!
        KPRINTF(20, ("Out of TDs!\n"));
        return NULL;
    }

    hc->hc_UhciTDPool = (struct UhciTD *) utd->utd_Succ;
    return(utd);
}
/* \\\ */

/* /// "uhciFreeTD()" */
static inline void uhciFreeTD(struct PCIController *hc, struct UhciTD *utd)
{
    utd->utd_Succ = (struct UhciXX *) hc->hc_UhciTDPool;
    hc->hc_UhciTDPool = utd;
}
/* \\\ */

#endif /* UHCIPROTO_H */
