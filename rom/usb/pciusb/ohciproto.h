void ohciUpdateFrameCounter(struct PCIController *hc);
void ohciAbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq);
BOOL ohciInit(struct PCIController *hc, struct PCIUnit *hu);
void ohciFree(struct PCIController *hc, struct PCIUnit *hu);

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
    oed->oed_HeadPtr = oed->oed_TailPtr;	// Protect against ocassional reuse
    CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
    SYNC;

    oed->oed_IOReq     = NULL;
    oed->oed_Buffer    = NULL;
    oed->oed_SetupData = NULL;
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
    otd->otd_NextTD = 0; // Protect against looped TD list in ocassion of TD reuse ("Rogue TD" state)
    SYNC;

    otd->otd_ED = NULL;
    otd->otd_Succ = hc->hc_OhciTDPool;
    hc->hc_OhciTDPool = otd;
}
/* \\\ */

static inline void ohciDisableED(struct OhciED *oed)
{
    ULONG ctrlstatus;

    // disable ED
    ctrlstatus = READMEM32_LE(&oed->oed_EPCaps);
    ctrlstatus |= OECF_SKIP;
    WRITEMEM32_LE(&oed->oed_EPCaps, ctrlstatus);

    // unlink from schedule
    oed->oed_Succ->oed_Pred = oed->oed_Pred;
    oed->oed_Pred->oed_Succ = oed->oed_Succ;
    oed->oed_Pred->oed_NextED = oed->oed_Succ->oed_Self;
    oed->oed_IOReq = NULL;
    CacheClearE(&oed->oed_Pred->oed_EPCaps, 16, CACRF_ClearD);
    SYNC;
}

static inline void ohciDisableInt(struct PCIController *hc, ULONG mask)
{
    WRITEREG32_LE(hc->hc_RegBase, OHCI_INTDIS, mask);
    hc->hc_PCIIntEnMask &= ~mask;
}

static inline void ohciEnableInt(struct PCIController *hc, ULONG mask)
{
    WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, mask); // Clear potential dangling status
    hc->hc_PCIIntEnMask |= mask;
    WRITEREG32_LE(hc->hc_RegBase, OHCI_INTEN, mask);
}
