/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <utility/hooks.h>
#include <exec/memory.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "ehciproto.h"
#include "ohci/ohcichip.h"
#include "uhci/uhcichip.h"

#ifdef base
#undef base
#endif
#define base (hc->hc_Device)
#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (hc->hc_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif

static AROS_INTH1(ehciResetHandler, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    // reset controller
    CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, EHUF_HCRESET|(1UL<<EHUS_INTTHRESHOLD));

    return FALSE;

    AROS_INTFUNC_EXIT
}


static inline APTR ehciGetDMABuffer(struct EhciHCPrivate *ehcihcp, APTR buffer, ULONG length, UWORD dir)
{
#if __WORDSIZE == 64
    if(!ehcihcp->ehc_64BitCapable) {
        return usbGetBuffer(buffer, length, dir);
    }
#endif
    return buffer;
}

static inline void ehciReleaseDMABuffer(APTR dmabuffer, APTR original, ULONG length, UWORD dir)
{
#if __WORDSIZE == 64
    if(dmabuffer && dmabuffer != original) {
        usbReleaseBuffer(dmabuffer, original, length, dir);
    }
#else
    (void)dmabuffer;
    (void)original;
    (void)length;
    (void)dir;
#endif
}

static BOOL ehciScheduleSegmentValid(struct PCIController *hc, APTR addr, ULONG size, ULONG *segment)
{
#if __WORDSIZE == 64
    UQUAD phys_base = (UQUAD)(IPTR)pciGetPhysical(hc, addr);
    UQUAD phys_end = phys_base + (UQUAD)size - 1;
    ULONG seg = (ULONG)(phys_base >> 32);

    if ((phys_end >> 32) != seg) {
        return FALSE;
    }

    if (segment) {
        *segment = seg;
    }
#else
    (void)hc;
    (void)addr;
    (void)size;
    if (segment) {
        *segment = 0;
    }
#endif
    return TRUE;
}

static void ehciFillScatterGather(struct PCIController *hc, struct EhciHCPrivate *ehcihcp,
                                  struct EhciTD *etd, APTR buffer, ULONG length)
{
    UBYTE *cursor = (UBYTE *)buffer;
    IPTR phys;
    ULONG remaining;
    int idx;

    /* First buffer pointer can include the offset inside the physical page */
    phys = (IPTR)pciGetPhysical(hc, cursor);
    ehciSetPointer(ehcihcp, etd->etd_BufferPtr[0], etd->etd_ExtBufferPtr[0], phys);

    /* Calculate how much of the first page is actually used */
    ULONG firstpage = EHCI_PAGE_SIZE - (phys & (EHCI_PAGE_SIZE - 1));
    if(firstpage > length)
        firstpage = length;

    remaining = (length > firstpage) ? length - firstpage : 0;
    cursor += firstpage;

    for(idx = 1; idx < 5; idx++) {
        if(!remaining) {
            WRITEMEM32_LE(&etd->etd_BufferPtr[idx], 0);
#if __WORDSIZE == 64
            WRITEMEM32_LE(&etd->etd_ExtBufferPtr[idx], 0);
#endif
            continue;
        }

        phys = (IPTR)pciGetPhysical(hc, cursor) & EHCI_PAGE_MASK;
        ehciSetPointer(ehcihcp, etd->etd_BufferPtr[idx], etd->etd_ExtBufferPtr[idx], phys);

        if(remaining > EHCI_PAGE_SIZE) {
            remaining -= EHCI_PAGE_SIZE;
        } else {
            remaining = 0;
        }
        cursor += EHCI_PAGE_SIZE;
    }
}

static void ehciFinishRequest(struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    struct EhciQH *eqh = ioreq->iouh_DriverPrivate1;
    UWORD devadrep;
    UWORD dir;

    // unlink from schedule
    WRITEMEM32_LE(&eqh->eqh_Pred->eqh_NextQH, eqh->eqh_Succ->eqh_Self);
    CacheClearE(&eqh->eqh_Pred->eqh_NextQH, 32, CACRF_ClearD);
    SYNC;

    eqh->eqh_Succ->eqh_Pred = eqh->eqh_Pred;
    eqh->eqh_Pred->eqh_Succ = eqh->eqh_Succ;
    SYNC;

    /* Deactivate the endpoint */
    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
    devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
    unit->hu_DevBusyReq[devadrep] = NULL;

    /* Release bounce buffers */
    if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
        dir = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT;
    else
        dir = ioreq->iouh_Dir;

    ehciReleaseDMABuffer(eqh->eqh_Buffer, ioreq->iouh_Data, ioreq->iouh_Actual, dir);
    ehciReleaseDMABuffer(eqh->eqh_SetupBuf, &ioreq->iouh_SetupData, 8, UHDIR_OUT);
    eqh->eqh_Buffer   = NULL;
    eqh->eqh_SetupBuf = NULL;
}

void ehciFreeAsyncContext(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct EhciQH *eqh = ioreq->iouh_DriverPrivate1;

    pciusbEHCIDebug("EHCI", "Freeing AsyncContext 0x%p\n", eqh);

    Disable();
    ehciFinishRequest(hc->hc_Unit, ioreq);
    // need to wait until an async schedule rollover before freeing these
    eqh->eqh_Succ = ehcihcp->ehc_EhciAsyncFreeQH;
    ehcihcp->ehc_EhciAsyncFreeQH = eqh;
    // activate doorbell
    WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, ehcihcp->ehc_EhciUsbCmd|EHUF_ASYNCDOORBELL);
    Enable();
}

void ehciFreePeriodicContext(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct EhciQH *eqh = ioreq->iouh_DriverPrivate1;
    struct EhciTD *etd;
    struct EhciTD *nextetd;

    pciusbEHCIDebug("EHCI", "Freeing PeriodicContext 0x%p\n", eqh);

    Disable(); // avoid race condition with interrupt
    ehciFinishRequest(hc->hc_Unit, ioreq);
    nextetd = eqh->eqh_FirstTD;
    while((etd = nextetd)) {
        pciusbEHCIDebug("EHCI", "FreeTD 0x%p\n", nextetd);
        nextetd = etd->etd_Succ;
        ehciFreeTD(hc, etd);
    }
    ehciFreeQH(hc, eqh);
    Enable();
}

void ehciFreeQHandTDs(struct PCIController *hc, struct EhciQH *eqh)
{

    struct EhciTD *etd = NULL;
    struct EhciTD *nextetd;

    pciusbEHCIDebug("EHCI", "Unlinking QContext 0x%p\n", eqh);
    nextetd = eqh->eqh_FirstTD;
    while(nextetd) {
        pciusbEHCIDebug("EHCI", "FreeTD 0x%p\n", nextetd);
        etd = nextetd;
        nextetd = (struct EhciTD *) etd->etd_Succ;
        ehciFreeTD(hc, etd);
    }

    ehciFreeQH(hc, eqh);
}

void ehciUpdateIntTree(struct PCIController *hc)
{
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct EhciQH *eqh;
    struct EhciQH *predeqh;
    struct EhciQH *lastusedeqh;
    UWORD cnt;

    // optimize linkage between queue heads
    predeqh = lastusedeqh = ehcihcp->ehc_EhciTermQH;
    for(cnt = 0; cnt < 11; cnt++) {
        eqh = ehcihcp->ehc_EhciIntQH[cnt];
        if(eqh->eqh_Succ != predeqh) {
            lastusedeqh = eqh->eqh_Succ;
        }
        eqh->eqh_NextQH = lastusedeqh->eqh_Self;
        CacheClearE(&eqh->eqh_NextQH, 32, CACRF_ClearD);
        predeqh = eqh;
    }
}

void ehciHandleFinishedTDs(struct PCIController *hc)
{
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    struct IOUsbHWReq *nextioreq;
    struct EhciQH *eqh;
    struct EhciTD *etd;
    struct EhciTD *predetd;
    UWORD devadrep;
    ULONG len;
    UWORD inspect;
    ULONG nexttd;
    BOOL shortpkt;
    ULONG ctrlstatus;
    ULONG epctrlstatus;
    ULONG actual;
    IPTR phyaddr;
    BOOL halted;
    BOOL updatetree = FALSE;
    BOOL zeroterm;

    pciusbEHCIDebug("EHCI", "Checking for Async work done...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ)) {
        eqh = (struct EhciQH *) ioreq->iouh_DriverPrivate1;
        if(eqh) {
            pciusbEHCIDebug("EHCI", "Examining IOReq=0x%p with EQH=0x%p\n", ioreq, eqh);
            SYNC;

            CacheClearE(&eqh->eqh_NextQH, 32, CACRF_InvalidateD);
            epctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
            nexttd = READMEM32_LE(&eqh->eqh_NextTD);
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            halted = ((epctrlstatus & (ETCF_ACTIVE|ETSF_HALTED)) == ETSF_HALTED);
            if(halted || (!(epctrlstatus & ETCF_ACTIVE) && (nexttd & EHCI_TERMINATE))) {
                pciusbEHCIDebug("EHCI", "AS: CS=%08lx CP=%08lx NX=%08lx\n", epctrlstatus, READMEM32_LE(&eqh->eqh_CurrTD), nexttd);
                shortpkt = FALSE;
                actual = 0;
                inspect = 1;
                etd = eqh->eqh_FirstTD;
                do {
                    ctrlstatus = READMEM32_LE(&etd->etd_CtrlStatus);
                    pciusbEHCIDebug("EHCI", "AS: CS=%08lx SL=%08lx TD=0x%p\n", ctrlstatus, READMEM32_LE(&etd->etd_Self), etd);
                    if(ctrlstatus & ETCF_ACTIVE) {
                        if(halted) {
                            pciusbError("EHCI", "Async: Halted before TD\n");
                            //ctrlstatus = eqh->eqh_CtrlStatus;
                            inspect = 0;
                            if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep])) {
                                pciusbWarn("EHCI", "NAK timeout\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            }
                            break;
                        } else {
                            // what happened here? The host controller was just updating the fields and has not finished yet
                            ctrlstatus = epctrlstatus;

                            /*KPRINTF(20, ("AS: CS=%08lx CP=%08lx NX=%08lx\n", epctrlstatus, READMEM32_LE(&eqh->eqh_CurrTD), nexttd));
                            KPRINTF(20, ("AS: CS=%08lx CP=%08lx NX=%08lx\n", READMEM32_LE(&eqh->eqh_CtrlStatus), READMEM32_LE(&eqh->eqh_CurrTD), READMEM32_LE(&eqh->eqh_NextTD)));
                            KPRINTF(20, ("AS: CS=%08lx SL=%08lx TD=%08lx\n", ctrlstatus, READMEM32_LE(&etd->etd_Self), etd));
                            etd = eqh->eqh_FirstTD;
                            do
                            {
                                KPRINTF(20, ("XX: CS=%08lx SL=%08lx TD=%08lx\n", READMEM32_LE(&etd->etd_CtrlStatus), READMEM32_LE(&etd->etd_Self), etd));
                            } while(etd = etd->etd_Succ);
                            KPRINTF(20, ("Async: Internal error! Still active?!\n"));
                            inspect = 2;
                            break;*/
                        }
                    }

                    if(ctrlstatus & (ETSF_HALTED|ETSF_TRANSERR|ETSF_BABBLE|ETSF_DATABUFFERERR)) {
                        if(ctrlstatus & ETSF_BABBLE) {
                            pciusbError("EHCI", "Babble error %08lx\n", ctrlstatus);
                            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                        } else if(ctrlstatus & ETSF_DATABUFFERERR) {
                            pciusbError("EHCI", "Databuffer error\n");
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                        } else if(ctrlstatus & ETSF_TRANSERR) {
                            if((ctrlstatus & ETCM_ERRORLIMIT)>>ETCS_ERRORLIMIT) {
                                pciusbError("EHCI", "other kind of STALLED!\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            } else {
                                pciusbError("EHCI", "TIMEOUT!\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            }
                        } else {
                            pciusbError("EHCI", "STALLED!\n");
                            ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                        }
                        inspect = 0;
                        break;
                    }

                    len = etd->etd_Length - ((ctrlstatus & ETSM_TRANSLENGTH)>>ETSS_TRANSLENGTH);
                    if((ctrlstatus & ETCM_PIDCODE) != ETCF_PIDCODE_SETUP) { // don't count setup packet
                        actual += len;
                    }
                    if(ctrlstatus & ETSM_TRANSLENGTH) {
                        pciusbWarn("EHCI", "Short packet: %ld < %ld\n", len, etd->etd_Length);
                        shortpkt = TRUE;
                        break;
                    }
                    etd = etd->etd_Succ;
                } while(etd && (!(ctrlstatus & ETCF_READYINTEN)));
                /*if(inspect == 2)
                {
                    // phantom halted
                    ioreq = nextioreq;
                    continue;
                }*/

                if(((actual + ioreq->iouh_Actual) < eqh->eqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS))) {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                ioreq->iouh_Actual += actual;
                if(inspect && (!shortpkt) && (eqh->eqh_Actual < ioreq->iouh_Length)) {
                    pciusbEHCIDebug("EHCI", "Reloading BULK at %ld/%ld\n", eqh->eqh_Actual, ioreq->iouh_Length);
                    // reload
                    ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
                    phyaddr = (IPTR)pciGetPhysical(hc, eqh->eqh_Buffer + ioreq->iouh_Actual);
                    predetd = etd = eqh->eqh_FirstTD;

                    CONSTWRITEMEM32_LE(&eqh->eqh_CurrTD, EHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&eqh->eqh_NextTD, EHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&eqh->eqh_AltNextTD, EHCI_TERMINATE);
                    CacheClearE(&eqh->eqh_CurrTD, 16, CACRF_ClearD);
                    do {
                        len = ioreq->iouh_Length - eqh->eqh_Actual;
                        if(len > 4*EHCI_PAGE_SIZE) {
                            len = 4*EHCI_PAGE_SIZE;
                        }
                        etd->etd_Length = len;
                        pciusbEHCIDebug("EHCI", "Reload Bulk TD 0x%p len %ld (%ld/%ld) phy=0x%p\n",
                                    etd, len, eqh->eqh_Actual, ioreq->iouh_Length, phyaddr);
                        WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
                        ehciFillScatterGather(hc, ehcihcp, etd, (UBYTE *)eqh->eqh_Buffer + eqh->eqh_Actual, len);

                        phyaddr += len;
                        eqh->eqh_Actual += len;
                        zeroterm = (len && (ioreq->iouh_Dir == UHDIR_OUT) && (eqh->eqh_Actual == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((eqh->eqh_Actual % ioreq->iouh_MaxPktSize) == 0));
                        predetd = etd;
                        etd = etd->etd_Succ;
                        if((!etd) && zeroterm) {
                            // rare case where the zero packet would be lost, allocate etd and append zero packet.
                            etd = ehciAllocTD(hc);
                            if(!etd) {
                                pciusbWarn("EHCI", "INTERNAL ERROR! This should not happen! Could not allocate zero packet TD\n");
                                break;
                            }
                            predetd->etd_Succ = etd;
                            predetd->etd_NextTD = etd->etd_Self;
                            predetd->etd_AltNextTD = ehcihcp->ehc_ShortPktEndTD->etd_Self;
                            etd->etd_Succ = NULL;
                            CONSTWRITEMEM32_LE(&etd->etd_NextTD, EHCI_TERMINATE);
                            CONSTWRITEMEM32_LE(&etd->etd_AltNextTD, EHCI_TERMINATE);
                        }
                    } while(etd && ((eqh->eqh_Actual < ioreq->iouh_Length) || zeroterm));
                    ctrlstatus |= ETCF_READYINTEN|(predetd->etd_Length<<ETSS_TRANSLENGTH);
                    WRITEMEM32_LE(&predetd->etd_CtrlStatus, ctrlstatus);
                    CONSTWRITEMEM32_LE(&predetd->etd_NextTD, EHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&predetd->etd_AltNextTD, EHCI_TERMINATE);
                    SYNC;
                    etd = eqh->eqh_FirstTD;
                    WRITEMEM32_LE(&eqh->eqh_NextTD, etd->etd_Self);
                    CacheClearE(&eqh->eqh_NextTD, 16, CACRF_ClearD);
                    SYNC;
                    unit->hu_NakTimeoutFrame[devadrep] =
                        (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<ehcihcp->ehc_EhciTimeoutShift) : 0;
                } else {
                    ehciFreeAsyncContext(hc, ioreq);
                    // use next data toggle bit based on last successful transaction
                    pciusbEHCIDebug("EHCI", "Old Toggle %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]);
                    unit->hu_DevDataToggle[devadrep] = (ctrlstatus & ETCF_DATA1) ? TRUE : FALSE;
                    pciusbEHCIDebug("EHCI", "Toggle now %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]);
                    if(inspect) {
                        if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER) {
                            // check for sucessful clear feature and set address ctrl transfers
                            uhwCheckSpecialCtrlTransfers(hc, ioreq);
                        }
                    }
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                }
            }
        } else {
            pciusbEHCIDebug("EHCI", "IOReq=0x%p has no UQH!\n", ioreq);
        }
        ioreq = nextioreq;
    }

    pciusbEHCIDebug("EHCI", "Checking for Periodic work done...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ)) {
        eqh = (struct EhciQH *) ioreq->iouh_DriverPrivate1;
        if(eqh) {
            pciusbEHCIDebug("EHCI", "Examining IOReq=0x%p with EQH=0x%p\n", ioreq, eqh);
            nexttd = READMEM32_LE(&eqh->eqh_NextTD);
            etd = eqh->eqh_FirstTD;
            ctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            halted = ((ctrlstatus & (ETCF_ACTIVE|ETSF_HALTED)) == ETSF_HALTED);
            if(halted || (!(ctrlstatus & ETCF_ACTIVE) && (nexttd & EHCI_TERMINATE))) {
                pciusbEHCIDebug("EHCI", "EQH not active %08lx\n", ctrlstatus);
                shortpkt = FALSE;
                actual = 0;
                inspect = 1;
                do {
                    ctrlstatus = READMEM32_LE(&etd->etd_CtrlStatus);
                    pciusbEHCIDebug("EHCI", "Periodic: TD=0x%p CS=%08lx\n", etd, ctrlstatus);
                    if(ctrlstatus & ETCF_ACTIVE) {
                        if(halted) {
                            pciusbError("EHCI", "Periodic: Halted before TD\n");
                            //ctrlstatus = eqh->eqh_CtrlStatus;
                            inspect = 0;
                            if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep])) {
                                pciusbWarn("EHCI", "NAK timeout\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            }
                            break;
                        } else {
                            pciusbError("EHCI", "Periodic: Internal error! Still active?!\n");
                            break;
                        }
                    }

                    if(ctrlstatus & (ETSF_HALTED|ETSF_TRANSERR|ETSF_BABBLE|ETSF_DATABUFFERERR|ETSF_MISSEDCSPLIT)) {
                        if(ctrlstatus & ETSF_BABBLE) {
                            pciusbError("EHCI", "Babble error %08lx\n", ctrlstatus);
                            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                        } else if(ctrlstatus & ETSF_MISSEDCSPLIT) {
                            pciusbError("EHCI", "Missed CSplit %08lx\n", ctrlstatus);
                            ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                        } else if(ctrlstatus & ETSF_DATABUFFERERR) {
                            pciusbError("EHCI", "Databuffer error\n");
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                        } else if(ctrlstatus & ETSF_TRANSERR) {
                            if((ctrlstatus & ETCM_ERRORLIMIT)>>ETCS_ERRORLIMIT) {
                                pciusbError("EHCI", "STALLED!\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            } else {
                                pciusbError("EHCI", "TIMEOUT!\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            }
                        } else if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep])) {
                            ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                        }
                        inspect = 0;
                        break;
                    }

                    len = etd->etd_Length - ((ctrlstatus & ETSM_TRANSLENGTH)>>ETSS_TRANSLENGTH);
                    actual += len;
                    if(ctrlstatus & ETSM_TRANSLENGTH) {
                        pciusbWarn("EHCI", "Short packet: %ld < %ld\n", len, etd->etd_Length);
                        shortpkt = TRUE;
                        break;
                    }
                    etd = etd->etd_Succ;
                } while(etd);
                if((actual < eqh->eqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS))) {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                ioreq->iouh_Actual += actual;
                ehciFreePeriodicContext(hc, ioreq);
                updatetree = TRUE;
                // use next data toggle bit based on last successful transaction
                pciusbEHCIDebug("EHCI", "Old Toggle %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]);
                unit->hu_DevDataToggle[devadrep] = (ctrlstatus & ETCF_DATA1) ? TRUE : FALSE;
                pciusbEHCIDebug("EHCI", "Toggle now %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]);
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        } else {
            pciusbEHCIDebug("EHCI", "IOReq=0x%p has no UQH!\n", ioreq);
        }
        ioreq = nextioreq;
    }
    if(updatetree) {
        ehciUpdateIntTree(hc);
    }
}


void ehciScheduleCtrlTDs(struct PCIController *hc)
{
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct EhciQH *eqh;
    struct EhciTD *setupetd;
    struct EhciTD *dataetd;
    struct EhciTD *termetd;
    struct EhciTD *predetd;
    ULONG epcaps;
    ULONG ctrlstatus;
    ULONG len;
    IPTR phyaddr;

    /* *** CTRL Transfers *** */
    pciusbEHCIDebug("EHCI", "Scheduling new CTRL transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        pciusbEHCIDebug("EHCI", "New CTRL transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("EHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh) {
            break;
        }

        setupetd = ehciAllocTD(hc);
        if(!setupetd) {
            ehciFreeQH(hc, eqh);
            break;
        }
        termetd = ehciAllocTD(hc);
        if(!termetd) {
            ehciFreeTD(hc, setupetd);
            ehciFreeQH(hc, eqh);
            break;
        }
        eqh->eqh_IOReq = ioreq;
        eqh->eqh_FirstTD = setupetd;
        eqh->eqh_Actual = 0;

        epcaps = ((0<<EQES_RELOAD)|EQEF_TOGGLEFROMTD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS) {
            pciusbEHCIDebug("EHCI", "*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr);
            // full speed and low speed handling
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
            epcaps |= EQEF_SPLITCTRLEP;
            if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
                pciusbEHCIDebug("EHCI", "*** LOW SPEED ***\n");
                epcaps |= EQEF_LOWSPEED;
            }
        } else {
            CONSTWRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1);
            epcaps |= EQEF_HIGHSPEED;
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        CONSTWRITEMEM32_LE(&eqh->eqh_CtrlStatus, 0);
        CONSTWRITEMEM32_LE(&eqh->eqh_CurrTD, 0);
        WRITEMEM32_LE(&eqh->eqh_AltNextTD, setupetd->etd_Self);
        WRITEMEM32_LE(&eqh->eqh_NextTD, setupetd->etd_Self);
        CacheClearE(&eqh->eqh_CurrTD, 16, CACRF_ClearD);
        CacheClearE(&eqh->eqh_NextTD, 16, CACRF_ClearD);

        //termetd->etd_QueueHead = setupetd->etd_QueueHead = eqh;

        pciusbEHCIDebug("EHCI", "SetupTD=0x%p, TermTD=0x%p\n", setupetd, termetd);

        // fill setup td
        setupetd->etd_Length = 8;

        CONSTWRITEMEM32_LE(&setupetd->etd_CtrlStatus, (8<<ETSS_TRANSLENGTH)|ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_SETUP);

        eqh->eqh_SetupBuf = ehciGetDMABuffer(ehcihcp, &ioreq->iouh_SetupData, 8, UHDIR_OUT);
        phyaddr = (IPTR) pciGetPhysical(hc, eqh->eqh_SetupBuf);

        ehciSetPointer(ehcihcp, setupetd->etd_BufferPtr[0], setupetd->etd_ExtBufferPtr[0], phyaddr);
        ehciSetPointer(ehcihcp, setupetd->etd_BufferPtr[1], setupetd->etd_ExtBufferPtr[1], (phyaddr + 8) & EHCI_PAGE_MASK); // theoretically, setup data may cross one page
        setupetd->etd_BufferPtr[2] = 0; // clear for overlay bits

        ctrlstatus = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        predetd = setupetd;
        if(ioreq->iouh_Length) {
            eqh->eqh_Buffer = ehciGetDMABuffer(ehcihcp, ioreq->iouh_Data, ioreq->iouh_Length, (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT);
            phyaddr = (IPTR)pciGetPhysical(hc, eqh->eqh_Buffer);
            do {
                dataetd = ehciAllocTD(hc);
                if(!dataetd) {
                    break;
                }
                ctrlstatus ^= ETCF_DATA1; // toggle bit
                predetd->etd_Succ = dataetd;
                predetd->etd_NextTD = dataetd->etd_Self;
                dataetd->etd_AltNextTD = termetd->etd_Self;

                len = ioreq->iouh_Length - eqh->eqh_Actual;
                if(len > 4*EHCI_PAGE_SIZE) {
                    len = 4*EHCI_PAGE_SIZE;
                }
                dataetd->etd_Length = len;
                WRITEMEM32_LE(&dataetd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
                ehciFillScatterGather(hc, ehcihcp, dataetd, (UBYTE *)eqh->eqh_Buffer + eqh->eqh_Actual, len);

                phyaddr += len;
                eqh->eqh_Actual += len;
                predetd = dataetd;
            } while(eqh->eqh_Actual < ioreq->iouh_Length);
            if(!dataetd) {
                // not enough dataetds? try again later
                ehciReleaseDMABuffer(eqh->eqh_Buffer, ioreq->iouh_Data, ioreq->iouh_Length,
                    (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT);
                ehciReleaseDMABuffer(eqh->eqh_SetupBuf, &ioreq->iouh_SetupData, 8, UHDIR_OUT);
                ehciFreeQHandTDs(hc, eqh);
                ehciFreeTD(hc, termetd); // this one's not linked yet
                break;
            }
        }
        // TERM packet
        ctrlstatus |= ETCF_DATA1|ETCF_READYINTEN;
        ctrlstatus ^= (ETCF_PIDCODE_IN^ETCF_PIDCODE_OUT);

        predetd->etd_NextTD = termetd->etd_Self;
        predetd->etd_Succ = termetd;
        CONSTWRITEMEM32_LE(&termetd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&termetd->etd_AltNextTD, EHCI_TERMINATE);
        WRITEMEM32_LE(&termetd->etd_CtrlStatus, ctrlstatus);
        termetd->etd_Length = 0;
        termetd->etd_BufferPtr[0] = 0; // clear for overlay bits
        termetd->etd_BufferPtr[1] = 0; // clear for overlay bits
        termetd->etd_BufferPtr[2] = 0; // clear for overlay bits
        termetd->etd_ExtBufferPtr[0] = 0; // clear for overlay bits
        termetd->etd_ExtBufferPtr[1] = 0; // clear for overlay bits
        termetd->etd_ExtBufferPtr[2] = 0; // clear for overlay bits
        termetd->etd_Succ = NULL;

        if (hc->hc_Quirks & HCQ_EHCI_OVERLAY_CTRL_FILL) {
            // due to sillicon bugs, we fill in the first overlay ourselves.
            WRITEMEM32_LE(&eqh->eqh_CurrTD, setupetd->etd_Self);
            WRITEMEM32_LE(&eqh->eqh_NextTD, setupetd->etd_NextTD);
            WRITEMEM32_LE(&eqh->eqh_AltNextTD, setupetd->etd_AltNextTD);
            WRITEMEM32_LE(&eqh->eqh_CtrlStatus, setupetd->etd_CtrlStatus);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[0], setupetd->etd_BufferPtr[0]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[1], setupetd->etd_BufferPtr[1]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[2], 0);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[0], setupetd->etd_ExtBufferPtr[0]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[1], setupetd->etd_ExtBufferPtr[1]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[2], 0);
            CacheClearE(&eqh->eqh_CurrTD, 32, CACRF_ClearD);
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] =
            (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<ehcihcp->ehc_EhciTimeoutShift) : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the asyncQH)
        eqh->eqh_Succ = ehcihcp->ehc_EhciAsyncQH->eqh_Succ;
        eqh->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
        SYNC;

        eqh->eqh_Pred = ehcihcp->ehc_EhciAsyncQH;
        eqh->eqh_Succ->eqh_Pred = eqh;
        ehcihcp->ehc_EhciAsyncQH->eqh_Succ = eqh;
        ehcihcp->ehc_EhciAsyncQH->eqh_NextQH = eqh->eqh_Self;
        SYNC;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    }
}

void ehciScheduleIntTDs(struct PCIController *hc)
{
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    UWORD cnt;
    struct EhciQH *eqh;
    struct EhciQH *inteqh;
    struct EhciTD *etd;
    struct EhciTD *predetd;
    ULONG epcaps;
    ULONG ctrlstatus;
    ULONG splitctrl;
    ULONG len;

    /* *** INT Transfers *** */
    pciusbEHCIDebug("EHCI", "Scheduling new INT transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbEHCIDebug("EHCI", "New INT transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("EHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh) {
            break;
        }

        eqh->eqh_IOReq = ioreq;
        eqh->eqh_Actual = 0;

        epcaps = (0<<EQES_RELOAD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS) {
            pciusbEHCIDebug("EHCI", "*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr);
            // full speed and low speed handling
            if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
                pciusbEHCIDebug("EHCI", "*** LOW SPEED ***\n");
                epcaps |= EQEF_LOWSPEED;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, (EQSF_MULTI_1|(0x01<<EQSS_MUSOFACTIVE)|(0x1c<<EQSS_MUSOFCSPLIT))|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
            if(ioreq->iouh_Interval >= 255) {
                inteqh = ehcihcp->ehc_EhciIntQH[8]; // 256ms interval
            } else {
                cnt = 0;
                do {
                    inteqh = ehcihcp->ehc_EhciIntQH[cnt++];
                } while(ioreq->iouh_Interval >= (1<<cnt));
            }
        } else {
            epcaps |= EQEF_HIGHSPEED;
            if(ioreq->iouh_Flags & UHFF_MULTI_3) {
                splitctrl = EQSF_MULTI_3;
            } else if(ioreq->iouh_Flags & UHFF_MULTI_2) {
                splitctrl = EQSF_MULTI_2;
            } else {
                splitctrl = EQSF_MULTI_1;
            }
            if(ioreq->iouh_Interval < 2) { // 0-1 microframes
                splitctrl |= (0xff<<EQSS_MUSOFACTIVE);
            } else if(ioreq->iouh_Interval < 4) { // 2-3 microframes
                splitctrl |= (0x55<<EQSS_MUSOFACTIVE);
            } else if(ioreq->iouh_Interval < 8) { // 4-7 microframes
                splitctrl |= (0x22<<EQSS_MUSOFACTIVE);
            } else if(ioreq->iouh_Interval > 511) { // 64ms and higher
                splitctrl |= (0x10<<EQSS_MUSOFACTIVE);
            } else { //if(ioreq->iouh_Interval >= 8) // 1-64ms
                splitctrl |= (0x01<<EQSS_MUSOFACTIVE);
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, splitctrl);
            if(ioreq->iouh_Interval >= 1024) {
                inteqh = ehcihcp->ehc_EhciIntQH[10]; // 1024 microframes interval
            } else {
                cnt = 0;
                do {
                    inteqh = ehcihcp->ehc_EhciIntQH[cnt++];
                } while(ioreq->iouh_Interval >= (1<<cnt));
            }
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        CONSTWRITEMEM32_LE(&eqh->eqh_CtrlStatus, 0);
        CONSTWRITEMEM32_LE(&eqh->eqh_CurrTD, 0);
        eqh->eqh_FirstTD = NULL; // clear for ehciFreeQHandTDs()

        ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        if(unit->hu_DevDataToggle[devadrep]) {
            // continue with data toggle 0
            ctrlstatus |= ETCF_DATA1;
        }
        predetd = NULL;
        eqh->eqh_Buffer = ehciGetDMABuffer(ehcihcp, ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
        do {
            etd = ehciAllocTD(hc);
            if(!etd) {
                break;
            }
            if(predetd) {
                predetd->etd_Succ = etd;
                predetd->etd_NextTD = etd->etd_Self;
                predetd->etd_AltNextTD = ehcihcp->ehc_ShortPktEndTD->etd_Self;
            } else {
                eqh->eqh_FirstTD = etd;
                WRITEMEM32_LE(&eqh->eqh_AltNextTD, etd->etd_Self);
                WRITEMEM32_LE(&eqh->eqh_NextTD, etd->etd_Self);
                CacheClearE(&eqh->eqh_CurrTD, 16, CACRF_ClearD);
                CacheClearE(&eqh->eqh_NextTD, 16, CACRF_ClearD);
            }

            len = ioreq->iouh_Length - eqh->eqh_Actual;
            if(len > 4*EHCI_PAGE_SIZE) {
                len = 4*EHCI_PAGE_SIZE;
            }
            etd->etd_Length = len;
            WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
            if(len) {
                ehciFillScatterGather(hc, ehcihcp, etd, (UBYTE *)eqh->eqh_Buffer + eqh->eqh_Actual, len);
            } else {
                for(UWORD idx = 0; idx < 5; idx++) {
                    WRITEMEM32_LE(&etd->etd_BufferPtr[idx], 0);
#if __WORDSIZE == 64
                    WRITEMEM32_LE(&etd->etd_ExtBufferPtr[idx], 0);
#endif
                }
            }

            eqh->eqh_Actual += len;
            predetd = etd;
        } while(eqh->eqh_Actual < ioreq->iouh_Length);

        if(!etd) {
            // not enough etds? try again later
            ehciReleaseDMABuffer(eqh->eqh_Buffer, ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
            ehciFreeQHandTDs(hc, eqh);
            break;
        }
        ctrlstatus |= ETCF_READYINTEN|(etd->etd_Length<<ETSS_TRANSLENGTH);
        WRITEMEM32_LE(&predetd->etd_CtrlStatus, ctrlstatus);

        CONSTWRITEMEM32_LE(&predetd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&predetd->etd_AltNextTD, EHCI_TERMINATE);
        predetd->etd_Succ = NULL;

        if (hc->hc_Quirks & HCQ_EHCI_OVERLAY_INT_FILL) {
            // due to sillicon bugs, we fill in the first overlay ourselves.
            etd = eqh->eqh_FirstTD;
            WRITEMEM32_LE(&eqh->eqh_CurrTD, etd->etd_Self);
            WRITEMEM32_LE(&eqh->eqh_NextTD, etd->etd_NextTD);
            WRITEMEM32_LE(&eqh->eqh_AltNextTD, etd->etd_AltNextTD);
            WRITEMEM32_LE(&eqh->eqh_CtrlStatus, etd->etd_CtrlStatus);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[0], etd->etd_BufferPtr[0]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[1], etd->etd_BufferPtr[1]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[2], etd->etd_BufferPtr[2]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[3], etd->etd_BufferPtr[3]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[4], etd->etd_BufferPtr[4]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[0], etd->etd_ExtBufferPtr[0]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[1], etd->etd_ExtBufferPtr[1]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[2], etd->etd_ExtBufferPtr[2]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[3], etd->etd_ExtBufferPtr[3]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[4], etd->etd_ExtBufferPtr[4]);
            CacheClearE(&eqh->eqh_CurrTD, 32, CACRF_ClearD);
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] =
            (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<ehcihcp->ehc_EhciTimeoutShift) : 0;

        Disable();
        AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry in the right IntQH
        eqh->eqh_Succ = inteqh->eqh_Succ;
        eqh->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
        SYNC;

        eqh->eqh_Pred = inteqh;
        eqh->eqh_Succ->eqh_Pred = eqh;
        inteqh->eqh_Succ = eqh;
        inteqh->eqh_NextQH = eqh->eqh_Self;
        SYNC;
        Enable();

        ehciUpdateIntTree(hc);

        ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    }
}


void ehciScheduleBulkTDs(struct PCIController *hc)
{
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct EhciQH *eqh;
    struct EhciTD *etd = NULL;
    struct EhciTD *predetd;
    ULONG epcaps;
    ULONG ctrlstatus;
    ULONG splitctrl;
    ULONG len;
    IPTR phyaddr;

    /* *** BULK Transfers *** */
    pciusbEHCIDebug("EHCI", "Scheduling new BULK transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbEHCIDebug("EHCI", "New BULK transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("EHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh) {
            break;
        }

        eqh->eqh_IOReq = ioreq;
        eqh->eqh_Actual = 0;

        epcaps = (0<<EQES_RELOAD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS) {
            pciusbEHCIDebug("EHCI", "*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr);
            // full speed and low speed handling
            if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
                pciusbEHCIDebug("EHCI", "*** LOW SPEED ***\n");
                epcaps |= EQEF_LOWSPEED;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
        } else {
            epcaps |= EQEF_HIGHSPEED;
            if(ioreq->iouh_Flags & UHFF_MULTI_3) {
                splitctrl = EQSF_MULTI_3;
            } else if(ioreq->iouh_Flags & UHFF_MULTI_2) {
                splitctrl = EQSF_MULTI_2;
            } else {
                splitctrl = EQSF_MULTI_1;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, splitctrl);
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        CONSTWRITEMEM32_LE(&eqh->eqh_CtrlStatus, 0);
        CONSTWRITEMEM32_LE(&eqh->eqh_CurrTD, 0);
        eqh->eqh_FirstTD = NULL; // clear for ehciFreeQHandTDs()

        ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        if(unit->hu_DevDataToggle[devadrep]) {
            // continue with data toggle 0
            ctrlstatus |= ETCF_DATA1;
        }
        predetd = NULL;
        eqh->eqh_Buffer = ehciGetDMABuffer(ehcihcp, ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
        phyaddr = (ioreq->iouh_Length && eqh->eqh_Buffer) ? (IPTR)pciGetPhysical(hc, eqh->eqh_Buffer) : 0;
        do {
            if((eqh->eqh_Actual >= EHCI_TD_BULK_LIMIT) && (eqh->eqh_Actual < ioreq->iouh_Length)) {
                pciusbEHCIDebug("EHCI", "Bulk too large, splitting...\n");
                break;
            }
            etd = ehciAllocTD(hc);
            if(!etd) {
                break;
            }
            if(predetd) {
                predetd->etd_Succ = etd;
                predetd->etd_NextTD = etd->etd_Self;
                predetd->etd_AltNextTD = ehcihcp->ehc_ShortPktEndTD->etd_Self;
            } else {
                eqh->eqh_FirstTD = etd;
                WRITEMEM32_LE(&eqh->eqh_AltNextTD, etd->etd_Self);
                WRITEMEM32_LE(&eqh->eqh_NextTD, etd->etd_Self);
                CacheClearE(&eqh->eqh_CurrTD, 16, CACRF_ClearD);
                CacheClearE(&eqh->eqh_NextTD, 16, CACRF_ClearD);
            }

            len = ioreq->iouh_Length - eqh->eqh_Actual;
            if(len > 4*EHCI_PAGE_SIZE) {
                len = 4*EHCI_PAGE_SIZE;
            }
            etd->etd_Length = len;
            pciusbEHCIDebug("EHCI", "Bulk TD 0x%p len %ld (%ld/%ld) phy=0x%p\n",
                        etd, len, eqh->eqh_Actual, ioreq->iouh_Length, phyaddr);
            WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
            if(len) {
                ehciFillScatterGather(hc, ehcihcp, etd, (UBYTE *)eqh->eqh_Buffer + eqh->eqh_Actual, len);
            } else {
                for(UWORD idx = 0; idx < 5; idx++) {
                    WRITEMEM32_LE(&etd->etd_BufferPtr[idx], 0);
#if __WORDSIZE == 64
                    WRITEMEM32_LE(&etd->etd_ExtBufferPtr[idx], 0);
#endif
                }
            }

            phyaddr += len;
            eqh->eqh_Actual += len;

            predetd = etd;
        } while((eqh->eqh_Actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (eqh->eqh_Actual == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((eqh->eqh_Actual % ioreq->iouh_MaxPktSize) == 0)));

        if(!etd) {
            // not enough etds? try again later
            ehciReleaseDMABuffer(eqh->eqh_Buffer, ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
            ehciFreeQHandTDs(hc, eqh);
            break;
        }
        ctrlstatus |= ETCF_READYINTEN|(predetd->etd_Length<<ETSS_TRANSLENGTH);
        WRITEMEM32_LE(&predetd->etd_CtrlStatus, ctrlstatus);

        predetd->etd_Succ = NULL;
        CONSTWRITEMEM32_LE(&predetd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&predetd->etd_AltNextTD, EHCI_TERMINATE);

        if (hc->hc_Quirks & HCQ_EHCI_OVERLAY_BULK_FILL) {
            // due to sillicon bugs, we fill in the first overlay ourselves.
            etd = eqh->eqh_FirstTD;
            WRITEMEM32_LE(&eqh->eqh_CurrTD, etd->etd_Self);
            WRITEMEM32_LE(&eqh->eqh_NextTD, etd->etd_NextTD);
            WRITEMEM32_LE(&eqh->eqh_AltNextTD, etd->etd_AltNextTD);
            WRITEMEM32_LE(&eqh->eqh_CtrlStatus, etd->etd_CtrlStatus);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[0], etd->etd_BufferPtr[0]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[1], etd->etd_BufferPtr[1]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[2], etd->etd_BufferPtr[2]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[3], etd->etd_BufferPtr[3]);
            WRITEMEM32_LE(&eqh->eqh_BufferPtr[4], etd->etd_BufferPtr[4]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[0], etd->etd_ExtBufferPtr[0]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[1], etd->etd_ExtBufferPtr[1]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[2], etd->etd_ExtBufferPtr[2]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[3], etd->etd_ExtBufferPtr[3]);
            WRITEMEM32_LE(&eqh->eqh_ExtBufferPtr[4], etd->etd_ExtBufferPtr[4]);
            CacheClearE(&eqh->eqh_CurrTD, 32, CACRF_ClearD);
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] =
            (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<ehcihcp->ehc_EhciTimeoutShift) : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the asyncQH)
        eqh->eqh_Succ = ehcihcp->ehc_EhciAsyncQH->eqh_Succ;
        eqh->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
        SYNC;

        eqh->eqh_Pred = ehcihcp->ehc_EhciAsyncQH;
        eqh->eqh_Succ->eqh_Pred = eqh;
        ehcihcp->ehc_EhciAsyncQH->eqh_Succ = eqh;
        ehcihcp->ehc_EhciAsyncQH->eqh_NextQH = eqh->eqh_Self;
        SYNC;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }
}

void ehciUpdateFrameCounter(struct PCIController *hc)
{

    ULONG fc;
    Disable();
    fc = READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT);
    if ((hc->hc_Quirks & HCQ_EHCI_MOSC_FRAMECOUNTBUG) && ((fc & 7) == 0))
        fc = READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT);
    hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xffffc000)|(fc & 0x3fff);
    Enable();
}

static AROS_INTH1(ehciCompleteInt, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;

    pciusbEHCIDebug("EHCI", "CompleteInt!\n");

    ehciUpdateFrameCounter(hc);

    /* **************** PROCESS DONE TRANSFERS **************** */

    if(ehcihcp->ehc_AsyncAdvanced) {
        struct EhciQH *eqh;
        struct EhciTD *etd;
        struct EhciTD *nextetd;

        ehcihcp->ehc_AsyncAdvanced = FALSE;

        pciusbEHCIDebug("EHCI", "AsyncAdvance 0x%p\n", ehcihcp->ehc_EhciAsyncFreeQH);

        while((eqh = ehcihcp->ehc_EhciAsyncFreeQH)) {
            pciusbEHCIDebug("EHCI", "FreeQH 0x%p\n", eqh);
            nextetd = eqh->eqh_FirstTD;
            while((etd = nextetd)) {
                pciusbEHCIDebug("EHCI", "FreeTD 0x%p\n", nextetd);
                nextetd = etd->etd_Succ;
                ehciFreeTD(hc, etd);
            }
            ehcihcp->ehc_EhciAsyncFreeQH = eqh->eqh_Succ;
            ehciFreeQH(hc, eqh);
        }
    }

    ehciHandleFinishedTDs(hc);
    ehciHandleIsochTDs(hc);

    if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ) {
        ehciScheduleCtrlTDs(hc);
    }

    if(hc->hc_IntXFerQueue.lh_Head->ln_Succ) {
        ehciScheduleIntTDs(hc);
    }

    if(hc->hc_IsoXFerQueue.lh_Head->ln_Succ) {
        ehciScheduleIsoTDs(hc);
    }

    if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ) {
        ehciScheduleBulkTDs(hc);
    }

    pciusbEHCIDebug("EHCI", "CompleteDone\n");

    return FALSE;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(ehciIntCode, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
#ifndef base
    struct PCIDevice *base = hc->hc_Device;
#endif
    ULONG intr;

    //KPRINTF(1, ("pciEhciInt()\n"));
    intr = READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS);
    if(intr & hc->hc_PCIIntEnMask) {
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS, intr);
        //KPRINTF(1, ("INT=%04lx\n", intr));
        if (!(hc->hc_Flags & HCF_ONLINE)) {
            return FALSE;
        }
        if(intr & EHSF_FRAMECOUNTOVER) {
            hc->hc_FrameCounter |= 0x3fff;
            hc->hc_FrameCounter++;
            hc->hc_FrameCounter |= READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT) & 0x3fff;
            pciusbEHCIDebug("EHCI", "Frame Counter Rollover %ld\n", hc->hc_FrameCounter);
        }
        if(intr & EHSF_ASYNCADVANCE) {
            pciusbEHCIDebug("EHCI", "AsyncAdvance\n");
            ehcihcp->ehc_AsyncAdvanced = TRUE;
        }
        if(intr & EHSF_HOSTERROR) {
            pciusbError("EHCI", "Host ERROR!\n");
        }
        if(intr & EHSF_PORTCHANGED) {
            ehciCheckPortStatusChange(hc);
        }
        if(intr & (EHSF_TDDONE|EHSF_TDERROR|EHSF_ASYNCADVANCE)) {
            SureCause(base, &hc->hc_CompleteInt);
        }
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}







#undef base
#define base (hc->hc_Device)

BOOL ehciInit(struct PCIController *hc, struct PCIUnit *hu)
{
    struct EhciHCPrivate *ehcihcp;
    struct EhciQH *eqh;
    struct EhciQH *predeqh;
    struct EhciTD *etd;
    ULONG *tabptr;
    UBYTE *memptr;
    ULONG bitcnt;
    ULONG hcsparams;
    ULONG hccparams;
    volatile APTR pciregbase;
    ULONG extcapoffset;
    ULONG legsup;
    ULONG timeout;
    ULONG tmp;
    ULONG fls;
    ULONG schedule_bytes;
    ULONG schedule_segment = 0;
    BOOL schedule_ok = TRUE;

    ULONG cnt;

    struct TagItem pciActivateMem[] = {
        { aHidd_PCIDevice_isMEM,    TRUE },
        { TAG_DONE, 0UL },
    };

    struct TagItem pciActivateBusmaster[] = {
        { aHidd_PCIDevice_isMaster, TRUE },
        { TAG_DONE, 0UL },
    };

    struct TagItem pciDeactivateBusmaster[] = {
        { aHidd_PCIDevice_isMaster, FALSE },
        { TAG_DONE, 0UL },
    };

    ehcihcp = AllocMem(sizeof(struct EhciHCPrivate), MEMF_CLEAR);
    if (!ehcihcp)
        return FALSE;

    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateMem); // activate memory
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (IPTR *) &pciregbase);
    pciregbase = (APTR) (((IPTR) pciregbase) & (~0xf));

    // we use the operational registers as RegBase.
    hc->hc_RegBase = (APTR) ((IPTR) pciregbase + READREG16_LE(pciregbase, EHCI_CAPLENGTH));
    pciusbEHCIDebug("EHCI", "RegBase = 0x%p\n", hc->hc_RegBase);

    hccparams = READREG32_LE(pciregbase, EHCI_HCCPARAMS);
    fls = 0;
    if(hccparams & EHCF_PROGFRAMELIST) {
        fls = (READREG32_LE(hc->hc_RegBase, EHCI_USBCMD) & EHUM_FRAMELISTSIZE) >> EHUS_FRAMELISTSIZE;
        if(fls > 2)
            fls = 0;
    }

    ehcihcp->ehc_FrameListSize = EHCI_FRAMELIST_SIZE >> fls;
    ehcihcp->ehc_FrameListMask = ehcihcp->ehc_FrameListSize - 1;

    ehcihcp->ehc_IsoAnchor = AllocMem(sizeof(ULONG) * ehcihcp->ehc_FrameListSize, MEMF_CLEAR);
    ehcihcp->ehc_IsoHead = AllocMem(sizeof(struct PTDNode *) * ehcihcp->ehc_FrameListSize, MEMF_CLEAR);
    ehcihcp->ehc_IsoTail = AllocMem(sizeof(struct PTDNode *) * ehcihcp->ehc_FrameListSize, MEMF_CLEAR);
    if(!ehcihcp->ehc_IsoAnchor || !ehcihcp->ehc_IsoHead || !ehcihcp->ehc_IsoTail) {
        if(ehcihcp->ehc_IsoAnchor)
            FreeMem(ehcihcp->ehc_IsoAnchor, sizeof(ULONG) * ehcihcp->ehc_FrameListSize);
        if(ehcihcp->ehc_IsoHead)
            FreeMem(ehcihcp->ehc_IsoHead, sizeof(struct PTDNode *) * ehcihcp->ehc_FrameListSize);
        if(ehcihcp->ehc_IsoTail)
            FreeMem(ehcihcp->ehc_IsoTail, sizeof(struct PTDNode *) * ehcihcp->ehc_FrameListSize);
        FreeMem(ehcihcp, sizeof(struct EhciHCPrivate));
        return FALSE;
    }

    hc->hc_CPrivate = ehcihcp;
    hc->hc_portroute = 0;

    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "EHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)ehciCompleteInt;

    /*
        Frame list size was determined from HCCPARAMS/USBCMD when programmable,
        falling back to EHCI_FRAMELIST_SIZE when not.
    */
    schedule_bytes = sizeof(ULONG) * ehcihcp->ehc_FrameListSize;
    schedule_bytes += sizeof(struct EhciQH) * EHCI_QH_POOLSIZE;
    schedule_bytes += sizeof(struct EhciTD) * EHCI_TD_POOLSIZE;
    schedule_bytes += sizeof(struct EhciITD) * EHCI_ITD_POOLSIZE;
    schedule_bytes += sizeof(struct EhciSiTD) * EHCI_SITD_POOLSIZE;

    hc->hc_PCIMem.me_Length = sizeof(ULONG) * ehcihcp->ehc_FrameListSize + EHCI_FRAMELIST_ALIGNMENT + 1;
    hc->hc_PCIMem.me_Length += sizeof(struct EhciQH) * EHCI_QH_POOLSIZE;
    hc->hc_PCIMem.me_Length += sizeof(struct EhciTD) * EHCI_TD_POOLSIZE;
    hc->hc_PCIMem.me_Length += sizeof(struct EhciITD) * EHCI_ITD_POOLSIZE;
    hc->hc_PCIMem.me_Length += sizeof(struct EhciSiTD) * EHCI_SITD_POOLSIZE;

    /*
        EHCI registers are read before allocating memory to size the periodic list.
    */
    memptr = ALLOCPCIMEM(hc, hc->hc_PCIDriverObject, hc->hc_PCIMem.me_Length);
    hc->hc_PCIMem.me_Un.meu_Addr = (APTR) memptr;
    hc->hc_PCIMemIsExec = FALSE;

    if(memptr) {
        // align memory
        memptr = (UBYTE *) ((((IPTR) hc->hc_PCIMem.me_Un.meu_Addr) + EHCI_FRAMELIST_ALIGNMENT) & (~EHCI_FRAMELIST_ALIGNMENT));
#if __WORDSIZE == 64
        schedule_ok = ehciScheduleSegmentValid(hc, memptr, schedule_bytes, &schedule_segment);
        if(schedule_ok && !(hccparams & EHCF_64BITS) && schedule_segment != 0) {
            schedule_ok = FALSE;
        }
        if(!schedule_ok) {
            pciusbWarn("EHCI", "Schedule allocation crosses 4GB boundary, retrying in low memory\n");
            FREEPCIMEM(hc, hc->hc_PCIDriverObject, hc->hc_PCIMem.me_Un.meu_Addr);
            hc->hc_PCIMem.me_Un.meu_Addr = NULL;
            hc->hc_PCIMemIsExec = TRUE;
            memptr = AllocMem(hc->hc_PCIMem.me_Length, MEMF_31BIT | MEMF_CLEAR);
            hc->hc_PCIMem.me_Un.meu_Addr = (APTR)memptr;
            if(memptr) {
                memptr = (UBYTE *) ((((IPTR) hc->hc_PCIMem.me_Un.meu_Addr) + EHCI_FRAMELIST_ALIGNMENT) & (~EHCI_FRAMELIST_ALIGNMENT));
                schedule_ok = ehciScheduleSegmentValid(hc, memptr, schedule_bytes, &schedule_segment);
                if(schedule_ok && !(hccparams & EHCF_64BITS) && schedule_segment != 0) {
                    schedule_ok = FALSE;
                }
            }
        }
#endif
        if(!memptr || !schedule_ok) {
            goto init_fail;
        }

        // PhysicalAddress - VirtualAdjust = VirtualAddress
        // VirtualAddress  + VirtualAdjust = PhysicalAddress
        hc->hc_PCIVirtualAdjust = pciGetPhysical(hc, hc->hc_PCIMem.me_Un.meu_Addr) - (APTR)hc->hc_PCIMem.me_Un.meu_Addr;
        pciusbEHCIDebug("EHCI", "VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust);

        ehcihcp->ehc_EhciFrameList = (ULONG *) memptr;
        pciusbEHCIDebug("EHCI", "FrameListBase 0x%p\n", ehcihcp->ehc_EhciFrameList);
        memptr += sizeof(ULONG) * ehcihcp->ehc_FrameListSize;

        // build up QH pool
        eqh = (struct EhciQH *) memptr;
        ehcihcp->ehc_EhciQHPool = eqh;
        cnt = EHCI_QH_POOLSIZE - 1;
        do {
            // minimal initalization
            eqh->eqh_Succ = (eqh + 1);
            WRITEMEM32_LE(&eqh->eqh_Self, (IPTR) (&eqh->eqh_NextQH) + hc->hc_PCIVirtualAdjust + EHCI_QUEUEHEAD);
            CONSTWRITEMEM32_LE(&eqh->eqh_NextTD, EHCI_TERMINATE);
            CONSTWRITEMEM32_LE(&eqh->eqh_AltNextTD, EHCI_TERMINATE);
            eqh++;
        } while(--cnt);
        eqh->eqh_Succ = NULL;
        WRITEMEM32_LE(&eqh->eqh_Self, (IPTR) (&eqh->eqh_NextQH) + hc->hc_PCIVirtualAdjust + EHCI_QUEUEHEAD);
        CONSTWRITEMEM32_LE(&eqh->eqh_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&eqh->eqh_AltNextTD, EHCI_TERMINATE);
        memptr += sizeof(struct EhciQH) * EHCI_QH_POOLSIZE;

        // build up TD pool
        etd = (struct EhciTD *) memptr;
        ehcihcp->ehc_EhciTDPool = etd;
        cnt = EHCI_TD_POOLSIZE - 1;
        do {
            etd->etd_Succ = (etd + 1);
            WRITEMEM32_LE(&etd->etd_Self, (IPTR) (&etd->etd_NextTD) + hc->hc_PCIVirtualAdjust);
            etd++;
        } while(--cnt);
        etd->etd_Succ = NULL;
        WRITEMEM32_LE(&etd->etd_Self, (IPTR) (&etd->etd_NextTD) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct EhciTD) * EHCI_TD_POOLSIZE;

        struct EhciITD *itd;
        struct EhciSiTD *sitd;

        itd = (struct EhciITD *) memptr;
        ehcihcp->ehc_EhciITDPool = itd;
        cnt = EHCI_ITD_POOLSIZE - 1;
        do {
            itd->itd_Succ = (itd + 1);
            WRITEMEM32_LE(&itd->itd_Self, (IPTR) (&itd->itd_Next) + hc->hc_PCIVirtualAdjust);
            itd++;
        } while(--cnt);
        itd->itd_Succ = NULL;
        WRITEMEM32_LE(&itd->itd_Self, (IPTR) (&itd->itd_Next) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct EhciITD) * EHCI_ITD_POOLSIZE;

        sitd = (struct EhciSiTD *) memptr;
        ehcihcp->ehc_EhciSiTDPool = sitd;
        cnt = EHCI_SITD_POOLSIZE - 1;
        do {
            sitd->sitd_Succ = (sitd + 1);
            WRITEMEM32_LE(&sitd->sitd_Self, (IPTR) (&sitd->sitd_Next) + hc->hc_PCIVirtualAdjust);
            sitd++;
        } while(--cnt);
        sitd->sitd_Succ = NULL;
        WRITEMEM32_LE(&sitd->sitd_Self, (IPTR) (&sitd->sitd_Next) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct EhciSiTD) * EHCI_SITD_POOLSIZE;

        // empty async queue head
        ehcihcp->ehc_EhciAsyncFreeQH = NULL;
        ehcihcp->ehc_EhciAsyncQH = eqh = ehciAllocQH(hc);
        eqh->eqh_Succ = eqh;
        eqh->eqh_Pred = eqh;
        CONSTWRITEMEM32_LE(&eqh->eqh_EPCaps, EQEF_RECLAMHEAD);
        eqh->eqh_NextQH = eqh->eqh_Self;

        // empty terminating queue head
        ehcihcp->ehc_EhciTermQH = eqh = ehciAllocQH(hc);
        eqh->eqh_Succ = NULL;
        CONSTWRITEMEM32_LE(&eqh->eqh_NextQH, EHCI_TERMINATE);
        predeqh = eqh;

        // 1 ms INT QH
        ehcihcp->ehc_EhciIntQH[0] = eqh = ehciAllocQH(hc);
        eqh->eqh_Succ = predeqh;
        predeqh->eqh_Pred = eqh;
        eqh->eqh_Pred = NULL; // who knows...
        //eqh->eqh_NextQH = predeqh->eqh_Self;
        predeqh = eqh;

        // make 11 levels of QH interrupts
        for(cnt = 1; cnt < 11; cnt++) {
            ehcihcp->ehc_EhciIntQH[cnt] = eqh = ehciAllocQH(hc);
            eqh->eqh_Succ = predeqh;
            eqh->eqh_Pred = NULL; // who knows...
            //eqh->eqh_NextQH = predeqh->eqh_Self; // link to previous int level
            predeqh = eqh;
        }

        ehciUpdateIntTree(hc);

        // fill in framelist with IntQH entry points based on interval
        tabptr = ehcihcp->ehc_EhciFrameList;
        for(cnt = 0; cnt < ehcihcp->ehc_FrameListSize; cnt++) {
            eqh = ehcihcp->ehc_EhciIntQH[10];
            bitcnt = 0;
            do {
                if(cnt & (1UL<<bitcnt)) {
                    eqh = ehcihcp->ehc_EhciIntQH[bitcnt];
                    break;
                }
            } while(++bitcnt < 11);
            *tabptr = eqh->eqh_Self;
            ehcihcp->ehc_IsoAnchor[cnt] = eqh->eqh_Self;
            ehcihcp->ehc_IsoHead[cnt] = NULL;
            ehcihcp->ehc_IsoTail[cnt] = NULL;
            tabptr++;
        }

        etd = ehcihcp->ehc_ShortPktEndTD = ehciAllocTD(hc);
        etd->etd_Succ = NULL;
        CONSTWRITEMEM32_LE(&etd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&etd->etd_AltNextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&etd->etd_CtrlStatus, 0);

        // time to initialize hardware...
        extcapoffset = (READREG32_LE(pciregbase, EHCI_HCCPARAMS) & EHCM_EXTCAPOFFSET)>>EHCS_EXTCAPOFFSET;
        while(extcapoffset >= 0x40) {
            pciusbEHCIDebug("EHCI", "Extended Caps at 0x%08lx\n", extcapoffset);
            legsup = READCONFIGLONG(hc, hc->hc_PCIDeviceObject, extcapoffset);
            if(((legsup & EHLM_CAP_ID) >> EHLS_CAP_ID) == 0x01) {
                if(legsup & EHLF_BIOS_OWNER) {
                    pciusbEHCIDebug("EHCI", "BIOS still has hands on EHCI, trying to get rid of it\n");
                    legsup |= EHLF_OS_OWNER;
                    WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, extcapoffset, legsup);
                    timeout = 100;
                    do {
                        legsup = READCONFIGLONG(hc, hc->hc_PCIDeviceObject, extcapoffset);
                        if(!(legsup & EHLF_BIOS_OWNER)) {
                            pciusbEHCIDebug("EHCI", "BIOS gave up on EHCI. Pwned!\n");
                            break;
                        }
                        uhwDelayMS(10, hu->hu_TimerReq);
                    } while(--timeout);
                    if(!timeout) {
                        pciusbWarn("EHCI", "BIOS didn't release EHCI. Forcing and praying...\n");
                        legsup |= EHLF_OS_OWNER;
                        legsup &= ~EHLF_BIOS_OWNER;
                        WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, extcapoffset, legsup);
                    }
                }
                /* disable all SMIs */
                WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, extcapoffset + 4, 0);
                break;
            }
            extcapoffset = (legsup & EHCM_EXTCAPOFFSET)>>EHCS_EXTCAPOFFSET;
        }

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivateBusmaster); // no busmaster yet

        // we use the operational registers as RegBase.
        pciusbEHCIDebug("EHCI", "Resetting HC\n");
        pciusbEHCIDebug("EHCI", "CMD: 0x%08x STS: 0x%08x\n", READREG32_LE(hc->hc_RegBase, EHCI_USBCMD), READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS));
        /* Step 1: Stop the HC */
        tmp = READREG32_LE(hc->hc_RegBase, EHCI_USBCMD);
        tmp &= ~EHUF_RUNSTOP;
        CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, tmp);

        /* Step 2. Wait for the controller to halt */
        cnt = 100;
        do {
            uhwDelayMS(10, hu->hu_TimerReq);
            if(READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS) & EHSF_HCHALTED) {
                break;
            }
        } while (cnt--);
        if (cnt == 0) {
            pciusbWarn("EHCI", "Timeout waiting for controller to halt\n");
        }

        /* Step 3. Reset the controller */
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, tmp | EHUF_HCRESET);

        /* Step 4. Wait for the reset bit to clear */
        cnt = 100;
        do {
            uhwDelayMS(10, hu->hu_TimerReq);
            if(!(READREG32_LE(hc->hc_RegBase, EHCI_USBCMD) & EHUF_HCRESET)) {
                break;
            }
        } while(--cnt);

#ifdef DEBUG
        if(cnt == 0) {
            pciusbEHCIDebug("EHCI", "Reset Timeout!\n");
        } else {
            pciusbEHCIDebug("EHCI", "Reset finished after %ld ticks\n", 100-cnt);
        }
#endif

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateBusmaster); // enable busmaster

        // Read HCSPARAMS register to obtain number of downstream ports
        hcsparams = READREG32_LE(pciregbase, EHCI_HCSPARAMS);
        ehcihcp->ehc_64BitCapable = (hccparams & EHCF_64BITS) != 0;

        hc->hc_NumPorts = (hcsparams & EHSM_NUM_PORTS)>>EHSS_NUM_PORTS;

        const char *str64bit;
#if __WORDSIZE==64
        str64bit = " 64bit ";
#else
        str64bit = " ";
#endif
        /* Cache companion/routing information for later unit-level port mapping. */
        ehcihcp->ehc_EhciNumCompanions = (UWORD)((hcsparams & EHSM_NUM_COMPANIONS) >> EHSS_NUM_COMPANIONS);
        ehcihcp->ehc_EhciPortsPerComp  = (UWORD)((hcsparams & EHSM_PORTS_PER_COMP) >> EHSS_PORTS_PER_COMP);

        pciusbEHCIDebug("EHCI", "Found%sController @ 0x%p with %lu ports (%lu companions with %lu ports each)\n",
                    (ehcihcp->ehc_64BitCapable) ? str64bit : " ",
                    hc->hc_PCIDeviceObject, hc->hc_NumPorts,
                    ehcihcp->ehc_EhciNumCompanions,
                    ehcihcp->ehc_EhciPortsPerComp);

        /* Ensure schedule allocations stay within a 4GB segment when needed. */

        if(hcsparams & EHSF_EXTPORTROUTING) {
            ehcihcp->ehc_ComplexRouting = TRUE;
            hc->hc_portroute = READREG32_LE(pciregbase, EHCI_HCSPPORTROUTE);
#ifdef DEBUG
            for(cnt = 0; cnt < hc->hc_NumPorts; cnt++) {
                pciusbEHCIDebug("EHCI", "Port %ld maps to controller %ld\n", cnt, ((hc->hc_portroute >> (cnt<<2)) & 0xf));
            }
#endif
        } else {
            ehcihcp->ehc_ComplexRouting = FALSE;
        }

        pciusbEHCIDebug("EHCI", "HCCParams: 64 Bit=%s, ProgFrameList=%s, AsyncSchedPark=%s\n",
                    (hccparams & EHCF_64BITS) ? "Yes" : "No",
                    (hccparams & EHCF_PROGFRAMELIST) ? "Yes" : "No",
                    (hccparams & EHCF_ASYNCSCHEDPARK) ? "Yes" : "No");

        ehcihcp->ehc_EhciUsbCmd = (fls << EHUS_FRAMELISTSIZE) | (1UL<<EHUS_INTTHRESHOLD);
        ehcihcp->ehc_EhciTimeoutShift = 3;
        if (hc->hc_Quirks & HCQ_EHCI_VBOX_FRAMEROOLOVER) {
            ehcihcp->ehc_EhciTimeoutShift += 2;
        }

        if(hccparams & EHCF_ASYNCSCHEDPARK) {
            pciusbEHCIDebug("EHCI", "Enabling AsyncSchedParkMode with MULTI_3\n");
            ehcihcp->ehc_EhciUsbCmd |= EHUF_ASYNCSCHEDPARK|(3<<EHUS_ASYNCPARKCOUNT);
        }

        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, ehcihcp->ehc_EhciUsbCmd);

        CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT, 0);

#if __WORDSIZE == 64
        if(ehcihcp->ehc_64BitCapable) {
            IPTR framelistphys = (IPTR)pciGetPhysical(hc, ehcihcp->ehc_EhciFrameList);
            WRITEREG32_LE(hc->hc_RegBase, EHCI_CTRLDSSEGMENT, AROS_LONG2LE(schedule_segment));
            WRITEREG32_LE(hc->hc_RegBase, EHCI_PERIODICLIST, framelistphys);
        } else {
            WRITEREG32_LE(hc->hc_RegBase, EHCI_PERIODICLIST, (IPTR)pciGetPhysical(hc, ehcihcp->ehc_EhciFrameList));
            WRITEREG32_LE(hc->hc_RegBase, EHCI_CTRLDSSEGMENT, 0);
        }
#else
        WRITEREG32_LE(hc->hc_RegBase, EHCI_PERIODICLIST, (IPTR)pciGetPhysical(hc, ehcihcp->ehc_EhciFrameList));
        WRITEREG32_LE(hc->hc_RegBase, EHCI_CTRLDSSEGMENT, 0);
#endif
        WRITEREG32_LE(hc->hc_RegBase, EHCI_ASYNCADDR, AROS_LONG2LE(ehcihcp->ehc_EhciAsyncQH->eqh_Self));
        CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS, EHSF_ALL_INTS);

        // install reset handler
        hc->hc_ResetInt.is_Node.ln_Name = "EHCI PCI (pciusb.device)";
        hc->hc_ResetInt.is_Code = (VOID_FUNC)ehciResetHandler;
        hc->hc_ResetInt.is_Data = hc;
        AddResetCallback(&hc->hc_ResetInt);

        // add interrupt
        hc->hc_PCIIntHandler.is_Node.ln_Name = hc->hc_ResetInt.is_Node.ln_Name;
        hc->hc_PCIIntHandler.is_Node.ln_Pri = 5;
        hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
        hc->hc_PCIIntHandler.is_Code = (VOID_FUNC)ehciIntCode;
        hc->hc_PCIIntHandler.is_Data = hc;
        PCIXAddInterrupt(hc, &hc->hc_PCIIntHandler);

        hc->hc_PCIIntEnMask = EHSF_ALL_INTS;
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBINTEN, hc->hc_PCIIntEnMask);

        CacheClearE(ehcihcp->ehc_EhciFrameList, sizeof(ULONG) * ehcihcp->ehc_FrameListSize,      CACRF_ClearD);
        CacheClearE(ehcihcp->ehc_EhciQHPool,    sizeof(struct EhciQH) * EHCI_QH_POOLSIZE, CACRF_ClearD);
        CacheClearE(ehcihcp->ehc_EhciTDPool,    sizeof(struct EhciTD) * EHCI_TD_POOLSIZE, CACRF_ClearD);

        CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_CONFIGFLAG, EHCF_CONFIGURED);
        ehcihcp->ehc_EhciUsbCmd |= EHUF_RUNSTOP|EHUF_PERIODICENABLE|EHUF_ASYNCENABLE;
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, ehcihcp->ehc_EhciUsbCmd);
        SYNC;

        /*
         * VirtualBox (and some BIOSes) may leave the per-port Port Owner (PO) bit set,
         * even after CONFIGFLAG is asserted. With PO=1, the EHCI controller will not
         * report connect status for full/low-speed devices on those ports, because the
         * companion controller owns them. Clear PO here to ensure EHCI initially owns
         * all ports; the EHCI root-hub code will hand ports back to companions as needed.
         */
        for (cnt = 0; cnt < hc->hc_NumPorts; cnt++) {
            UWORD portreg = EHCI_PORTSC1 + (cnt << 2);
            ULONG ps = READREG32_LE(hc->hc_RegBase, portreg);
            if (ps & 0x00002000UL) { /* PO bit */
                WRITEREG32_LE(hc->hc_RegBase, portreg, ps & ~0x00002000UL);
                SYNC;
            }
        }

        pciusbEHCIDebug("EHCI", "HW Init done\n");

        pciusbEHCIDebug("EHCI", "HW Regs USBCMD=%04lx\n", READREG32_LE(hc->hc_RegBase, EHCI_USBCMD));
        pciusbEHCIDebug("EHCI", "HW Regs USBSTS=%04lx\n", READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS));
        pciusbEHCIDebug("EHCI", "HW Regs FRAMECOUNT=%04lx\n", READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT));

        /* Global port mapping is unit-level and may span multiple EHCI instances.
         * It is constructed in pciAllocUnit() once all controllers for the PCI
         * device are initialized.
         */
        for (cnt = 0; cnt < hc->hc_NumPorts && cnt < MAX_ROOT_PORTS; cnt++) {
            hc->hc_PortNum[cnt] = 0xFF;
        }

        pciusbEHCIDebug("EHCI", "ehciInit returns TRUE...\n");
        return TRUE;
    }

init_fail:
    if(hc->hc_PCIMem.me_Un.meu_Addr) {
        if(hc->hc_PCIMemIsExec)
            FreeMem(hc->hc_PCIMem.me_Un.meu_Addr, hc->hc_PCIMem.me_Length);
        else
            FREEPCIMEM(hc, hc->hc_PCIDriverObject, hc->hc_PCIMem.me_Un.meu_Addr);
        hc->hc_PCIMem.me_Un.meu_Addr = NULL;
        hc->hc_PCIMemIsExec = FALSE;
    }
    if(ehcihcp) {
        if(ehcihcp->ehc_IsoAnchor)
            FreeMem(ehcihcp->ehc_IsoAnchor, sizeof(ULONG) * ehcihcp->ehc_FrameListSize);
        if(ehcihcp->ehc_IsoHead)
            FreeMem(ehcihcp->ehc_IsoHead, sizeof(struct PTDNode *) * ehcihcp->ehc_FrameListSize);
        if(ehcihcp->ehc_IsoTail)
            FreeMem(ehcihcp->ehc_IsoTail, sizeof(struct PTDNode *) * ehcihcp->ehc_FrameListSize);
        FreeMem(ehcihcp, sizeof(struct EhciHCPrivate));
    }
    hc->hc_CPrivate = NULL;
    pciusbEHCIDebug("EHCI", "ehciInit returns FALSE...\n");
    return FALSE;
}

void ehciFree(struct PCIController *hc, struct PCIUnit *hu)
{
    UWORD portreg;
    UWORD hciport;
    pciusbEHCIDebug("EHCI", "Shutting down HC @ 0x%p\n", hc);
    CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBINTEN, 0);
    // disable all ports
    for(hciport = 0; hciport < hc->hc_NumPorts; hciport++) {
        portreg = EHCI_PORTSC1 + (hciport<<2);
        WRITEREG32_LE(hc->hc_RegBase, portreg, 0);
    }
    CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, 1UL<<EHUS_INTTHRESHOLD);
    uhwDelayMS(10, hu->hu_TimerReq);
    CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_CONFIGFLAG, 0);
    CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, EHUF_HCRESET|(1UL<<EHUS_INTTHRESHOLD));
    SYNC;

    uhwDelayMS(50, hu->hu_TimerReq);
    CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, 1UL<<EHUS_INTTHRESHOLD);
    SYNC;

    uhwDelayMS(10, hu->hu_TimerReq);

    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    hc->hc_CPrivate = NULL;
    if (ehcihcp) {
        if(ehcihcp->ehc_IsoAnchor)
            FreeMem(ehcihcp->ehc_IsoAnchor, sizeof(ULONG) * ehcihcp->ehc_FrameListSize);
        if(ehcihcp->ehc_IsoHead)
            FreeMem(ehcihcp->ehc_IsoHead, sizeof(struct PTDNode *) * ehcihcp->ehc_FrameListSize);
        if(ehcihcp->ehc_IsoTail)
            FreeMem(ehcihcp->ehc_IsoTail, sizeof(struct PTDNode *) * ehcihcp->ehc_FrameListSize);
        FreeMem(ehcihcp, sizeof(struct EhciHCPrivate));
    }

    pciusbEHCIDebug("EHCI", "Shut down complete\n");
}
