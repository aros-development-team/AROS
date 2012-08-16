/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#include LC_LIBDEFS_FILE

#include <devices/usb_hub.h>

#include "pciehci_hci.h"
#include "pciehci_uhw.h"

APTR pciGetPhysical(struct ehc_controller *ehc, APTR virtaddr) {
    //struct PCIDevice *hd = hc->hc_Device;
    return(HIDD_PCIDriver_CPUtoPCI(ehc->ehc_pcidriverobject, virtaddr));
}

static inline struct EhciQH * ehciAllocQH(struct ehc_controller *ehc) {
    struct EhciQH *eqh = ehc->ehc_EhciQHPool;

    if(!eqh) {
        // out of QHs!
        KPRINTF(20, ("Out of QHs!\n"));
        return NULL;
    }

    ehc->ehc_EhciQHPool = (struct EhciQH *) eqh->eqh_Succ;
    return(eqh);
}

static inline void ehciFreeQH(struct ehc_controller *ehc, struct EhciQH *eqh) {
    eqh->eqh_Succ = ehc->ehc_EhciQHPool;
    ehc->ehc_EhciQHPool = eqh;
}

static inline struct EhciTD * ehciAllocTD(struct ehc_controller *ehc) {
    struct EhciTD *etd = ehc->ehc_EhciTDPool;

    if(!etd) {
        // out of TDs!
        KPRINTF(20, ("Out of TDs!\n"));
        return NULL;
    }

    ehc->ehc_EhciTDPool = (struct EhciTD *) etd->etd_Succ;
    return(etd);
}

static inline void ehciFreeTD(struct ehc_controller *ehc, struct EhciTD *etd) {
    etd->etd_Succ = ehc->ehc_EhciTDPool;
    ehc->ehc_EhciTDPool = etd;
}

static AROS_INTH1(ehciResetHandler, struct ehc_controller *, ehc)
{
    AROS_INTFUNC_INIT

    CONSTWRITEREG32_LE(ehc->ehc_opregbase, EHCI_USBCMD, EHUF_HCRESET|(1UL<<EHUS_INTTHRESHOLD));     /* Resets the controller */

    return FALSE;

    AROS_INTFUNC_EXIT
}

void ehciFreeAsyncContext(struct ehc_controller *ehc, struct EhciQH *eqh) {

    KPRINTF(5, ("Unlinking AsyncContext %08lx\n", eqh));
    // unlink from schedule
    eqh->eqh_Pred->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
    CacheClearE(&eqh->eqh_Pred->eqh_NextQH, 32, CACRF_ClearD);
    SYNC;

    eqh->eqh_Succ->eqh_Pred = eqh->eqh_Pred;
    eqh->eqh_Pred->eqh_Succ = eqh->eqh_Succ;
    SYNC;

    // need to wait until an async schedule rollover before freeing these
    Disable();
    eqh->eqh_Succ = ehc->ehc_EhciAsyncFreeQH;
    ehc->ehc_EhciAsyncFreeQH = eqh;
    // activate doorbell
    WRITEREG32_LE(ehc->ehc_opregbase, EHCI_USBCMD, ehc->ehc_EhciUsbCmd|EHUF_ASYNCDOORBELL);
    Enable();
}

void ehciFreePeriodicContext(struct ehc_controller *ehc, struct EhciQH *eqh) {

    struct EhciTD *etd;
    struct EhciTD *nextetd;

    KPRINTF(5, ("Unlinking PeriodicContext %08lx\n", eqh));
    // unlink from schedule
    eqh->eqh_Pred->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
    CacheClearE(&eqh->eqh_Pred->eqh_NextQH, 32, CACRF_ClearD);
    SYNC;

    eqh->eqh_Succ->eqh_Pred = eqh->eqh_Pred;
    eqh->eqh_Pred->eqh_Succ = eqh->eqh_Succ;
    SYNC;

    Disable(); // avoid race condition with interrupt
    nextetd = eqh->eqh_FirstTD;
    while((etd = nextetd))
    {
        KPRINTF(1, ("FreeTD %08lx\n", nextetd));
        nextetd = etd->etd_Succ;
        ehciFreeTD(ehc, etd);
    }
    ehciFreeQH(ehc, eqh);
    Enable();
}

void ehciFreeQHandTDs(struct ehc_controller *ehc, struct EhciQH *eqh) {

    struct EhciTD *etd = NULL;
    struct EhciTD *nextetd;

    KPRINTF(5, ("Unlinking QContext %08lx\n", eqh));
    nextetd = eqh->eqh_FirstTD;
    while(nextetd) {
        KPRINTF(1, ("FreeTD %08lx\n", nextetd));
        etd = nextetd;
        nextetd = (struct EhciTD *) etd->etd_Succ;
        ehciFreeTD(ehc, etd);
    }

    ehciFreeQH(ehc, eqh);
}

void ehciUpdateIntTree(struct ehc_controller *ehc) {

    struct EhciQH *eqh;
    struct EhciQH *predeqh;
    struct EhciQH *lastusedeqh;
    UWORD cnt;

    // optimize linkage between queue heads
    predeqh = lastusedeqh = ehc->ehc_EhciTermQH;
    for(cnt = 0; cnt < 11; cnt++) {
        eqh = ehc->ehc_EhciIntQH[cnt];
        if(eqh->eqh_Succ != predeqh) {
            lastusedeqh = eqh->eqh_Succ;
        }
        eqh->eqh_NextQH = lastusedeqh->eqh_Self;
        CacheClearE(&eqh->eqh_NextQH, 32, CACRF_ClearD);
        predeqh = eqh;
    }
}

BOOL ehciInit(struct ehc_controller *ehc, struct ehu_unit *ehu) {
}

