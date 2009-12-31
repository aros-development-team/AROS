#ifndef UHWCMD_H
#define UHWCMD_H

#include "debug.h"
#include "pci_aros.h"

#include "uhcichip.h"
#include "ohcichip.h"
#include "ehcichip.h"
#include "pciusb.h"

struct Unit *Open_Unit(struct IOUsbHWReq *ioreq,
                       LONG unitnr,
                       struct PCIDevice *base);
void Close_Unit(struct PCIDevice *base, struct PCIUnit *unit,
                struct IOUsbHWReq *ioreq);

void uhwDelayMS(ULONG milli, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdReset(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdUsbReset(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdUsbResume(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdUsbSuspend(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdUsbOper(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdControlXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdBulkXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdIntXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdIsoXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdFlush(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base);

void TermIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base);

AROS_UFP1(void, uhwNakTimeoutInt,
          AROS_UFPA(struct PCIUnit *,  unit, A1));

BOOL pciInit(struct PCIDevice *hd);
void pciExpunge(struct PCIDevice *hd);
BOOL pciAllocUnit(struct PCIUnit *hu);
void pciFreeUnit(struct PCIUnit *hu);
APTR pciGetPhysical(struct PCIController *hc, APTR virtaddr);

void uhciCompleteInt(struct PCIController *hc);
void uhciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

void uhciUpdateIntTree(struct PCIController *hc);
void uhciFreeQContext(struct PCIController *hc, struct UhciQH *uqh);
static inline struct UhciQH * uhciAllocQH(struct PCIController *hc);
static inline void uhciFreeQH(struct PCIController *hc, struct UhciQH *uqh);
static inline struct UhciTD * uhciAllocTD(struct PCIController *hc);
static inline void uhciFreeTD(struct PCIController *hc, struct UhciTD *utd);


void ohciCompleteInt(struct PCIController *hc);
void ohciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

void ohciUpdateIntTree(struct PCIController *hc);
void ohciFreeEDContext(struct PCIController *hc, struct OhciED *oed);
static inline struct OhciED * ohciAllocED(struct PCIController *hc);
static inline void ohciFreeED(struct PCIController *hc, struct OhciED *oed);
static inline struct OhciTD * ohciAllocTD(struct PCIController *hc);
static inline void ohciFreeTD(struct PCIController *hc, struct OhciTD *otd);


void ehciCompleteInt(struct PCIController *hc);
void ehciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

void ehciUpdateIntTree(struct PCIController *hc);
void ehciFreeAsyncContext(struct PCIController *hc, struct EhciQH *eqh);
void ehciFreePeriodicContext(struct PCIController *hc, struct EhciQH *eqh);
static inline struct EhciQH * ehciAllocQH(struct PCIController *hc);
static inline void ehciFreeQH(struct PCIController *hc, struct EhciQH *eqh);
static inline struct EhciTD * ehciAllocTD(struct PCIController *hc);
static inline void ehciFreeTD(struct PCIController *hc, struct EhciTD *etd);

struct my_NSDeviceQueryResult
{
    ULONG   DevQueryFormat;         /* this is type 0               */
    ULONG   SizeAvailable;          /* bytes available              */
    UWORD   DeviceType;             /* what the device does         */
    UWORD   DeviceSubType;          /* depends on the main type     */
    const UWORD *SupportedCommands; /* 0 terminated list of cmd's   */
};

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

/* /// "ohciAllocED()" */
static inline struct OhciED * ohciAllocED(struct PCIController *hc)
{
    struct OhciED *oed = hc->hc_OhciEDPool;

    if(!oed)
    {
        // out of QHs!
        KPRINTF(20, ("Out of EDs!\n"));
        return NULL;
    }

    hc->hc_OhciEDPool = oed->oed_Succ;
    return(oed);
}
/* \\\ */

/* /// "ohciFreeED()" */
static inline void ohciFreeED(struct PCIController *hc, struct OhciED *oed)
{
    oed->oed_IOReq = NULL;
    oed->oed_Succ = hc->hc_OhciEDPool;
    hc->hc_OhciEDPool = oed;
}
/* \\\ */

/* /// "ohciAllocTD()" */
static inline struct OhciTD * ohciAllocTD(struct PCIController *hc)
{
    struct OhciTD *otd = hc->hc_OhciTDPool;

    if(!otd)
    {
        // out of TDs!
        KPRINTF(20, ("Out of TDs!\n"));
        return NULL;
    }

    hc->hc_OhciTDPool = otd->otd_Succ;
    return(otd);
}
/* \\\ */

/* /// "ohciFreeTD()" */
static inline void ohciFreeTD(struct PCIController *hc, struct OhciTD *otd)
{
    otd->otd_ED = NULL;
    otd->otd_Succ = hc->hc_OhciTDPool;
    hc->hc_OhciTDPool = otd;
}
/* \\\ */

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

#endif /* UHWCMD_H */

