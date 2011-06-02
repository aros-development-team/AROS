void ohciFreeEDContext(struct PCIController *hc, struct OhciED *oed);
void ohciUpdateIntTree(struct PCIController *hc);
void ohciHandleFinishedTDs(struct PCIController *hc);
void ohciScheduleCtrlTDs(struct PCIController *hc);
void ohciScheduleIntTDs(struct PCIController *hc);
void ohciScheduleBulkTDs(struct PCIController *hc);
void ohciUpdateFrameCounter(struct PCIController *hc);
void ohciCompleteInt(struct PCIController *hc);
void ohciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);
void ohciAbortED(struct PCIController *hc, struct OhciED *oed);
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
    oed->oed_HeadPtr   = 0;	// Protect against ocassional reuse
    oed->oed_TailPtr   = 0;
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
