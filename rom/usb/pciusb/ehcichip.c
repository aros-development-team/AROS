/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved
    $Id$
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase (hd->hd_HiddPCIDeviceAB)
#undef HiddAttrBase
#define HiddAttrBase (hd->hd_HiddAB)

static AROS_INTH1(EhciResetHandler, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    // reset controller
    CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, EHUF_HCRESET|(1UL<<EHUS_INTTHRESHOLD));

    return FALSE;

    AROS_INTFUNC_EXIT
}

static void ehciFinishRequest(struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    struct EhciQH *eqh = ioreq->iouh_DriverPrivate1;
    UWORD devadrep;
    UWORD dir;

    // unlink from schedule
    eqh->eqh_Pred->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
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

    usbReleaseBuffer(eqh->eqh_Buffer, ioreq->iouh_Data, ioreq->iouh_Actual, dir);
    usbReleaseBuffer(eqh->eqh_SetupBuf, &ioreq->iouh_SetupData, 8, UHDIR_IN);
    eqh->eqh_Buffer   = NULL;
    eqh->eqh_SetupBuf = NULL;
}

void ehciFreeAsyncContext(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct EhciQH *eqh = ioreq->iouh_DriverPrivate1;

    KPRINTF(5, ("Freeing AsyncContext 0x%p\n", eqh));
    ehciFinishRequest(hc->hc_Unit, ioreq);

    // need to wait until an async schedule rollover before freeing these
    Disable();
    eqh->eqh_Succ = hc->hc_EhciAsyncFreeQH;
    hc->hc_EhciAsyncFreeQH = eqh;
    // activate doorbell
    WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, hc->hc_EhciUsbCmd|EHUF_ASYNCDOORBELL);
    Enable();
}

void ehciFreePeriodicContext(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct EhciQH *eqh = ioreq->iouh_DriverPrivate1;
    struct EhciTD *etd;
    struct EhciTD *nextetd;

    KPRINTF(5, ("Freeing PeriodicContext 0x%p\n", eqh));
    ehciFinishRequest(hc->hc_Unit, ioreq);

    Disable(); // avoid race condition with interrupt
    nextetd = eqh->eqh_FirstTD;
    while((etd = nextetd))
    {
        KPRINTF(1, ("FreeTD 0x%p\n", nextetd));
        nextetd = etd->etd_Succ;
        ehciFreeTD(hc, etd);
    }
    ehciFreeQH(hc, eqh);
    Enable();
}

void ehciFreeQHandTDs(struct PCIController *hc, struct EhciQH *eqh) {

    struct EhciTD *etd = NULL;
    struct EhciTD *nextetd;

    KPRINTF(5, ("Unlinking QContext 0x%p\n", eqh));
    nextetd = eqh->eqh_FirstTD;
    while(nextetd)
    {
        KPRINTF(1, ("FreeTD 0x%p\n", nextetd));
        etd = nextetd;
        nextetd = (struct EhciTD *) etd->etd_Succ;
        ehciFreeTD(hc, etd);
    }

    ehciFreeQH(hc, eqh);
}

void ehciUpdateIntTree(struct PCIController *hc) {

    struct EhciQH *eqh;
    struct EhciQH *predeqh;
    struct EhciQH *lastusedeqh;
    UWORD cnt;

    // optimize linkage between queue heads
    predeqh = lastusedeqh = hc->hc_EhciTermQH;
    for(cnt = 0; cnt < 11; cnt++)
    {
        eqh = hc->hc_EhciIntQH[cnt];
        if(eqh->eqh_Succ != predeqh)
        {
            lastusedeqh = eqh->eqh_Succ;
        }
        eqh->eqh_NextQH = lastusedeqh->eqh_Self;
        CacheClearE(&eqh->eqh_NextQH, 32, CACRF_ClearD);
        predeqh = eqh;
    }
}

void ehciHandleFinishedTDs(struct PCIController *hc) {

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
    BOOL halted;
    BOOL updatetree = FALSE;
    BOOL zeroterm;
    IPTR phyaddr;

    KPRINTF(1, ("Checking for Async work done...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        eqh = (struct EhciQH *) ioreq->iouh_DriverPrivate1;
        if(eqh)
        {
            KPRINTF(1, ("Examining IOReq=0x%p with EQH=0x%p\n", ioreq, eqh));
            SYNC;

            CacheClearE(&eqh->eqh_NextQH, 32, CACRF_InvalidateD);
            epctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
            nexttd = READMEM32_LE(&eqh->eqh_NextTD);
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            halted = ((epctrlstatus & (ETCF_ACTIVE|ETSF_HALTED)) == ETSF_HALTED);
            if(halted || (!(epctrlstatus & ETCF_ACTIVE) && (nexttd & EHCI_TERMINATE)))
            {
                KPRINTF(1, ("AS: CS=%08lx CP=%08lx NX=%08lx\n", epctrlstatus, READMEM32_LE(&eqh->eqh_CurrTD), nexttd));
                shortpkt = FALSE;
                actual = 0;
                inspect = 1;
                etd = eqh->eqh_FirstTD;
                do
                {
                    ctrlstatus = READMEM32_LE(&etd->etd_CtrlStatus);
                    KPRINTF(1, ("AS: CS=%08lx SL=%08lx TD=0x%p\n", ctrlstatus, READMEM32_LE(&etd->etd_Self), etd));
                    if(ctrlstatus & ETCF_ACTIVE)
                    {
                        if(halted)
                        {
                            KPRINTF(20, ("Async: Halted before TD\n"));
                            //ctrlstatus = eqh->eqh_CtrlStatus;
                            inspect = 0;
                            if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
                            {
                                KPRINTF(20, ("NAK timeout\n"));
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

                    if(ctrlstatus & (ETSF_HALTED|ETSF_TRANSERR|ETSF_BABBLE|ETSF_DATABUFFERERR))
                    {
                        if(ctrlstatus & ETSF_BABBLE)
                        {
                            KPRINTF(20, ("Babble error %08lx\n", ctrlstatus));
                            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                        }
                        else if(ctrlstatus & ETSF_DATABUFFERERR)
                        {
                            KPRINTF(20, ("Databuffer error\n"));
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                        }
                        else if(ctrlstatus & ETSF_TRANSERR)
                        {
                            if((ctrlstatus & ETCM_ERRORLIMIT)>>ETCS_ERRORLIMIT)
                            {
                                KPRINTF(20, ("other kind of STALLED!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            } else {
                                KPRINTF(20, ("TIMEOUT!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            }
                        } else {
                            KPRINTF(20, ("STALLED!\n"));
                            ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                        }
                        inspect = 0;
                        break;
                    }

                    len = etd->etd_Length - ((ctrlstatus & ETSM_TRANSLENGTH)>>ETSS_TRANSLENGTH);
                    if((ctrlstatus & ETCM_PIDCODE) != ETCF_PIDCODE_SETUP) // don't count setup packet
                    {
                        actual += len;
                    }
                    if(ctrlstatus & ETSM_TRANSLENGTH)
                    {
                        KPRINTF(10, ("Short packet: %ld < %ld\n", len, etd->etd_Length));
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

                if(((actual + ioreq->iouh_Actual) < eqh->eqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                ioreq->iouh_Actual += actual;
                if(inspect && (!shortpkt) && (eqh->eqh_Actual < ioreq->iouh_Length))
                {
                    KPRINTF(10, ("Reloading BULK at %ld/%ld\n", eqh->eqh_Actual, ioreq->iouh_Length));
                    // reload
                    ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
                    phyaddr = (IPTR)pciGetPhysical(hc, eqh->eqh_Buffer + ioreq->iouh_Actual);
                    predetd = etd = eqh->eqh_FirstTD;

                    CONSTWRITEMEM32_LE(&eqh->eqh_CurrTD, EHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&eqh->eqh_NextTD, EHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&eqh->eqh_AltNextTD, EHCI_TERMINATE);
                    do
                    {
                        len = ioreq->iouh_Length - eqh->eqh_Actual;
                        if(len > 4*EHCI_PAGE_SIZE)
                        {
                            len = 4*EHCI_PAGE_SIZE;
                        }
                        etd->etd_Length = len;
                        KPRINTF(1, ("Reload Bulk TD 0x%p len %ld (%ld/%ld) phy=0x%p\n",
                                    etd, len, eqh->eqh_Actual, ioreq->iouh_Length, phyaddr));
                        WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
                        // FIXME need quark scatter gather mechanism here
                        WRITEMEM32_LE(&etd->etd_BufferPtr[0], phyaddr);
                        WRITEMEM32_LE(&etd->etd_BufferPtr[1], (phyaddr & EHCI_PAGE_MASK) + (1*EHCI_PAGE_SIZE));
                        WRITEMEM32_LE(&etd->etd_BufferPtr[2], (phyaddr & EHCI_PAGE_MASK) + (2*EHCI_PAGE_SIZE));
                        WRITEMEM32_LE(&etd->etd_BufferPtr[3], (phyaddr & EHCI_PAGE_MASK) + (3*EHCI_PAGE_SIZE));
                        WRITEMEM32_LE(&etd->etd_BufferPtr[4], (phyaddr & EHCI_PAGE_MASK) + (4*EHCI_PAGE_SIZE));

                        // FIXME Make use of these on 64-bit-capable hardware
                        etd->etd_ExtBufferPtr[0] = 0;
                        etd->etd_ExtBufferPtr[1] = 0;
                        etd->etd_ExtBufferPtr[2] = 0;
                        etd->etd_ExtBufferPtr[3] = 0;
                        etd->etd_ExtBufferPtr[4] = 0;
                        
                        phyaddr += len;
                        eqh->eqh_Actual += len;
                        zeroterm = (len && (ioreq->iouh_Dir == UHDIR_OUT) && (eqh->eqh_Actual == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((eqh->eqh_Actual % ioreq->iouh_MaxPktSize) == 0));
                        predetd = etd;
                        etd = etd->etd_Succ;
                        if((!etd) && zeroterm)
                        {
                            // rare case where the zero packet would be lost, allocate etd and append zero packet.
                            etd = ehciAllocTD(hc);
                            if(!etd)
                            {
                                KPRINTF(200, ("INTERNAL ERROR! This should not happen! Could not allocate zero packet TD\n"));
                                break;
                            }
                            predetd->etd_Succ = etd;
                            predetd->etd_NextTD = etd->etd_Self;
                            predetd->etd_AltNextTD = hc->hc_ShortPktEndTD->etd_Self;
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
                    eqh->eqh_NextTD = etd->etd_Self;
                    SYNC;
                    unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<3) : 0;
                }
                else
                {
                    ehciFreeAsyncContext(hc, ioreq);
                    // use next data toggle bit based on last successful transaction
                    KPRINTF(1, ("Old Toggle %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                    unit->hu_DevDataToggle[devadrep] = (ctrlstatus & ETCF_DATA1) ? TRUE : FALSE;
                    KPRINTF(1, ("Toggle now %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                    if(inspect)
                    {
                        if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                        {
                            // check for sucessful clear feature and set address ctrl transfers
                            uhwCheckSpecialCtrlTransfers(hc, ioreq);
                        }
                    }
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                }
            }
        } else {
            KPRINTF(20, ("IOReq=0x%p has no UQH!\n", ioreq));
        }
        ioreq = nextioreq;
    }

    KPRINTF(1, ("Checking for Periodic work done...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        eqh = (struct EhciQH *) ioreq->iouh_DriverPrivate1;
        if(eqh)
        {
            KPRINTF(1, ("Examining IOReq=0x%p with EQH=0x%p\n", ioreq, eqh));
            nexttd = READMEM32_LE(&eqh->eqh_NextTD);
            etd = eqh->eqh_FirstTD;
            ctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            halted = ((ctrlstatus & (ETCF_ACTIVE|ETSF_HALTED)) == ETSF_HALTED);
            if(halted || (!(ctrlstatus & ETCF_ACTIVE) && (nexttd & EHCI_TERMINATE)))
            {
                KPRINTF(1, ("EQH not active %08lx\n", ctrlstatus));
                shortpkt = FALSE;
                actual = 0;
                inspect = 1;
                do
                {
                    ctrlstatus = READMEM32_LE(&etd->etd_CtrlStatus);
                    KPRINTF(1, ("Periodic: TD=0x%p CS=%08lx\n", etd, ctrlstatus));
                    if(ctrlstatus & ETCF_ACTIVE)
                    {
                        if(halted)
                        {
                            KPRINTF(20, ("Periodic: Halted before TD\n"));
                            //ctrlstatus = eqh->eqh_CtrlStatus;
                            inspect = 0;
                            if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
                            {
                                KPRINTF(20, ("NAK timeout\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            }
                            break;
                        } else {
                            KPRINTF(20, ("Periodic: Internal error! Still active?!\n"));
                            break;
                        }
                    }

                    if(ctrlstatus & (ETSF_HALTED|ETSF_TRANSERR|ETSF_BABBLE|ETSF_DATABUFFERERR|ETSF_MISSEDCSPLIT))
                    {
                        if(ctrlstatus & ETSF_BABBLE)
                        {
                            KPRINTF(20, ("Babble error %08lx\n", ctrlstatus));
                            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                        }
                        else if(ctrlstatus & ETSF_MISSEDCSPLIT)
                        {
                            KPRINTF(20, ("Missed CSplit %08lx\n", ctrlstatus));
                            ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                        }
                        else if(ctrlstatus & ETSF_DATABUFFERERR)
                        {
                            KPRINTF(20, ("Databuffer error\n"));
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                        }
                        else if(ctrlstatus & ETSF_TRANSERR)
                        {
                            if((ctrlstatus & ETCM_ERRORLIMIT)>>ETCS_ERRORLIMIT)
                            {
                                KPRINTF(20, ("STALLED!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            } else {
                                KPRINTF(20, ("TIMEOUT!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            }
                        }
                        else if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
                        {
                            ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                        }
                        inspect = 0;
                        break;
                    }

                    len = etd->etd_Length - ((ctrlstatus & ETSM_TRANSLENGTH)>>ETSS_TRANSLENGTH);
                    actual += len;
                    if(ctrlstatus & ETSM_TRANSLENGTH)
                    {
                        KPRINTF(10, ("Short packet: %ld < %ld\n", len, etd->etd_Length));
                        shortpkt = TRUE;
                        break;
                    }
                    etd = etd->etd_Succ;
                } while(etd);
                if((actual < eqh->eqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                ioreq->iouh_Actual += actual;
                ehciFreePeriodicContext(hc, ioreq);
                updatetree = TRUE;
                // use next data toggle bit based on last successful transaction
                KPRINTF(1, ("Old Toggle %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                unit->hu_DevDataToggle[devadrep] = (ctrlstatus & ETCF_DATA1) ? TRUE : FALSE;
                KPRINTF(1, ("Toggle now %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        } else {
            KPRINTF(20, ("IOReq=0x%p has no UQH!\n", ioreq));
        }
        ioreq = nextioreq;
    }
    if(updatetree)
    {
        ehciUpdateIntTree(hc);
    }
}

void ehciScheduleCtrlTDs(struct PCIController *hc) {

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
    KPRINTF(1, ("Scheduling new CTRL transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        KPRINTF(10, ("New CTRL transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh)
        {
            break;
        }

        setupetd = ehciAllocTD(hc);
        if(!setupetd)
        {
            ehciFreeQH(hc, eqh);
            break;
        }
        termetd = ehciAllocTD(hc);
        if(!termetd)
        {
            ehciFreeTD(hc, setupetd);
            ehciFreeQH(hc, eqh);
            break;
        }
        eqh->eqh_IOReq = ioreq;
        eqh->eqh_FirstTD = setupetd;
        eqh->eqh_Actual = 0;

        epcaps = ((0<<EQES_RELOAD)|EQEF_TOGGLEFROMTD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
            epcaps |= EQEF_SPLITCTRLEP;
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                epcaps |= EQEF_LOWSPEED;
            }
        } else {
            CONSTWRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1);
            epcaps |= EQEF_HIGHSPEED;
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        //eqh->eqh_CtrlStatus = eqh->eqh_CurrTD = 0;
        //eqh->eqh_AltNextTD = eqh->eqh_NextTD = setupetd->etd_Self;

        //termetd->etd_QueueHead = setupetd->etd_QueueHead = eqh;

        KPRINTF(1, ("SetupTD=0x%p, TermTD=0x%p\n", setupetd, termetd));

        // fill setup td
        setupetd->etd_Length = 8;

        CONSTWRITEMEM32_LE(&setupetd->etd_CtrlStatus, (8<<ETSS_TRANSLENGTH)|ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_SETUP);
        
        eqh->eqh_SetupBuf = usbGetBuffer(&ioreq->iouh_SetupData, 8, UHDIR_OUT);
        phyaddr = (IPTR) pciGetPhysical(hc, eqh->eqh_SetupBuf);

        WRITEMEM32_LE(&setupetd->etd_BufferPtr[0], phyaddr);
        WRITEMEM32_LE(&setupetd->etd_BufferPtr[1], (phyaddr + 8) & EHCI_PAGE_MASK); // theoretically, setup data may cross one page
        setupetd->etd_BufferPtr[2] = 0; // clear for overlay bits

        // FIXME Make use of these on 64-bit-capable hardware
        setupetd->etd_ExtBufferPtr[0] = 0;
        setupetd->etd_ExtBufferPtr[1] = 0;
        setupetd->etd_ExtBufferPtr[2] = 0;

        ctrlstatus = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        predetd = setupetd;
        if(ioreq->iouh_Length)
        {
            eqh->eqh_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT);
            phyaddr = (IPTR)pciGetPhysical(hc, eqh->eqh_Buffer);
            do
            {
                dataetd = ehciAllocTD(hc);
                if(!dataetd)
                {
                    break;
                }
                ctrlstatus ^= ETCF_DATA1; // toggle bit
                predetd->etd_Succ = dataetd;
                predetd->etd_NextTD = dataetd->etd_Self;
                dataetd->etd_AltNextTD = termetd->etd_Self;

                len = ioreq->iouh_Length - eqh->eqh_Actual;
                if(len > 4*EHCI_PAGE_SIZE)
                {
                    len = 4*EHCI_PAGE_SIZE;
                }
                dataetd->etd_Length = len;
                WRITEMEM32_LE(&dataetd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
                // FIXME need quark scatter gather mechanism here
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[0], phyaddr);
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[1], (phyaddr & EHCI_PAGE_MASK) + (1*EHCI_PAGE_SIZE));
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[2], (phyaddr & EHCI_PAGE_MASK) + (2*EHCI_PAGE_SIZE));
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[3], (phyaddr & EHCI_PAGE_MASK) + (3*EHCI_PAGE_SIZE));
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[4], (phyaddr & EHCI_PAGE_MASK) + (4*EHCI_PAGE_SIZE));

                // FIXME Make use of these on 64-bit-capable hardware
                dataetd->etd_ExtBufferPtr[0] = 0;
                dataetd->etd_ExtBufferPtr[1] = 0;
                dataetd->etd_ExtBufferPtr[2] = 0;
                dataetd->etd_ExtBufferPtr[3] = 0;
                dataetd->etd_ExtBufferPtr[4] = 0;

                phyaddr += len;
                eqh->eqh_Actual += len;
                predetd = dataetd;
            } while(eqh->eqh_Actual < ioreq->iouh_Length);
            if(!dataetd)
            {
                // not enough dataetds? try again later
                usbReleaseBuffer(eqh->eqh_Buffer, ioreq->iouh_Data, 0, 0);
                usbReleaseBuffer(eqh->eqh_SetupBuf, &ioreq->iouh_SetupData, 0, 0);
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

        // due to sillicon bugs, we fill in the first overlay ourselves.
        eqh->eqh_CurrTD = setupetd->etd_Self;
        eqh->eqh_NextTD = setupetd->etd_NextTD;
        eqh->eqh_AltNextTD = setupetd->etd_AltNextTD;
        eqh->eqh_CtrlStatus = setupetd->etd_CtrlStatus;
        eqh->eqh_BufferPtr[0] = setupetd->etd_BufferPtr[0];
        eqh->eqh_BufferPtr[1] = setupetd->etd_BufferPtr[1];
        eqh->eqh_BufferPtr[2] = 0;
        eqh->eqh_ExtBufferPtr[0] = setupetd->etd_ExtBufferPtr[0];
        eqh->eqh_ExtBufferPtr[1] = setupetd->etd_ExtBufferPtr[1];
        eqh->eqh_ExtBufferPtr[2] = 0;

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<3) : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the asyncQH)
        eqh->eqh_Succ = hc->hc_EhciAsyncQH->eqh_Succ;
        eqh->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
        SYNC;

        eqh->eqh_Pred = hc->hc_EhciAsyncQH;
        eqh->eqh_Succ->eqh_Pred = eqh;
        hc->hc_EhciAsyncQH->eqh_Succ = eqh;
        hc->hc_EhciAsyncQH->eqh_NextQH = eqh->eqh_Self;
        SYNC;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    }
}

void ehciScheduleIntTDs(struct PCIController *hc) {

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
    IPTR phyaddr;

    /* *** INT Transfers *** */
    KPRINTF(1, ("Scheduling new INT transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New INT transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh)
        {
            break;
        }

        eqh->eqh_IOReq = ioreq;
        eqh->eqh_Actual = 0;

        epcaps = (0<<EQES_RELOAD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                epcaps |= EQEF_LOWSPEED;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, (EQSF_MULTI_1|(0x01<<EQSS_MUSOFACTIVE)|(0x1c<<EQSS_MUSOFCSPLIT))|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
            if(ioreq->iouh_Interval >= 255)
            {
                inteqh = hc->hc_EhciIntQH[8]; // 256ms interval
            } else {
                cnt = 0;
                do
                {
                    inteqh = hc->hc_EhciIntQH[cnt++];
                } while(ioreq->iouh_Interval >= (1<<cnt));
            }
        } else {
            epcaps |= EQEF_HIGHSPEED;
            if(ioreq->iouh_Flags & UHFF_MULTI_3)
            {
                splitctrl = EQSF_MULTI_3;
            }
            else if(ioreq->iouh_Flags & UHFF_MULTI_2)
            {
                splitctrl = EQSF_MULTI_2;
            } else {
                splitctrl = EQSF_MULTI_1;
            }
            if(ioreq->iouh_Interval < 2) // 0-1 µFrames
            {
                splitctrl |= (0xff<<EQSS_MUSOFACTIVE);
            }
            else if(ioreq->iouh_Interval < 4) // 2-3 µFrames
            {
                splitctrl |= (0x55<<EQSS_MUSOFACTIVE);
            }
            else if(ioreq->iouh_Interval < 8) // 4-7 µFrames
            {
                splitctrl |= (0x22<<EQSS_MUSOFACTIVE);
            }
            else if(ioreq->iouh_Interval > 511) // 64ms and higher
            {
                splitctrl |= (0x10<<EQSS_MUSOFACTIVE);
            }
            else //if(ioreq->iouh_Interval >= 8) // 1-64ms
            {
                splitctrl |= (0x01<<EQSS_MUSOFACTIVE);
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, splitctrl);
            if(ioreq->iouh_Interval >= 1024)
            {
                inteqh = hc->hc_EhciIntQH[10]; // 1024 µFrames interval
            } else {
                cnt = 0;
                do
                {
                    inteqh = hc->hc_EhciIntQH[cnt++];
                } while(ioreq->iouh_Interval >= (1<<cnt));
            }
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        //eqh->eqh_CtrlStatus = eqh->eqh_CurrTD = 0;
        eqh->eqh_FirstTD = NULL; // clear for ehciFreeQHandTDs()

        ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 0
            ctrlstatus |= ETCF_DATA1;
        }
        predetd = NULL;
        eqh->eqh_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
        phyaddr = (IPTR) pciGetPhysical(hc, eqh->eqh_Buffer);
        do
        {
            etd = ehciAllocTD(hc);
            if(!etd)
            {
                break;
            }
            if(predetd)
            {
                predetd->etd_Succ = etd;
                predetd->etd_NextTD = etd->etd_Self;
                predetd->etd_AltNextTD = hc->hc_ShortPktEndTD->etd_Self;
            } else {
                eqh->eqh_FirstTD = etd;
                //eqh->eqh_AltNextTD = eqh->eqh_NextTD = etd->etd_Self;
            }

            len = ioreq->iouh_Length - eqh->eqh_Actual;
            if(len > 4*EHCI_PAGE_SIZE)
            {
                len = 4*EHCI_PAGE_SIZE;
            }
            etd->etd_Length = len;
            WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
            // FIXME need quark scatter gather mechanism here
            WRITEMEM32_LE(&etd->etd_BufferPtr[0], phyaddr);
            WRITEMEM32_LE(&etd->etd_BufferPtr[1], (phyaddr & EHCI_PAGE_MASK) + (1*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[2], (phyaddr & EHCI_PAGE_MASK) + (2*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[3], (phyaddr & EHCI_PAGE_MASK) + (3*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[4], (phyaddr & EHCI_PAGE_MASK) + (4*EHCI_PAGE_SIZE));

            // FIXME Use these on 64-bit-capable hardware
            etd->etd_ExtBufferPtr[0] = 0;
            etd->etd_ExtBufferPtr[1] = 0;
            etd->etd_ExtBufferPtr[2] = 0;
            etd->etd_ExtBufferPtr[3] = 0;
            etd->etd_ExtBufferPtr[4] = 0;

            phyaddr += len;
            eqh->eqh_Actual += len;
            predetd = etd;
        } while(eqh->eqh_Actual < ioreq->iouh_Length);

        if(!etd)
        {
            // not enough etds? try again later
            usbReleaseBuffer(eqh->eqh_Buffer, ioreq->iouh_Data, 0, 0);
            ehciFreeQHandTDs(hc, eqh);
            break;
        }
        ctrlstatus |= ETCF_READYINTEN|(etd->etd_Length<<ETSS_TRANSLENGTH);
        WRITEMEM32_LE(&predetd->etd_CtrlStatus, ctrlstatus);

        CONSTWRITEMEM32_LE(&predetd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&predetd->etd_AltNextTD, EHCI_TERMINATE);
        predetd->etd_Succ = NULL;

        // due to sillicon bugs, we fill in the first overlay ourselves.
        etd = eqh->eqh_FirstTD;
        eqh->eqh_CurrTD = etd->etd_Self;
        eqh->eqh_NextTD = etd->etd_NextTD;
        eqh->eqh_AltNextTD = etd->etd_AltNextTD;
        eqh->eqh_CtrlStatus = etd->etd_CtrlStatus;
        eqh->eqh_BufferPtr[0] = etd->etd_BufferPtr[0];
        eqh->eqh_BufferPtr[1] = etd->etd_BufferPtr[1];
        eqh->eqh_BufferPtr[2] = etd->etd_BufferPtr[2];
        eqh->eqh_BufferPtr[3] = etd->etd_BufferPtr[3];
        eqh->eqh_BufferPtr[4] = etd->etd_BufferPtr[4];
        eqh->eqh_ExtBufferPtr[0] = etd->etd_ExtBufferPtr[0];
        eqh->eqh_ExtBufferPtr[1] = etd->etd_ExtBufferPtr[1];
        eqh->eqh_ExtBufferPtr[2] = etd->etd_ExtBufferPtr[2];
        eqh->eqh_ExtBufferPtr[3] = etd->etd_ExtBufferPtr[3];
        eqh->eqh_ExtBufferPtr[4] = etd->etd_ExtBufferPtr[4];

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<3) : 0;

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

void ehciScheduleBulkTDs(struct PCIController *hc) {

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
    KPRINTF(1, ("Scheduling new BULK transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New BULK transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh)
        {
            break;
        }

        eqh->eqh_IOReq = ioreq;
        eqh->eqh_Actual = 0;

        epcaps = (0<<EQES_RELOAD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                epcaps |= EQEF_LOWSPEED;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
        } else {
            epcaps |= EQEF_HIGHSPEED;
            if(ioreq->iouh_Flags & UHFF_MULTI_3)
            {
                splitctrl = EQSF_MULTI_3;
            }
            else if(ioreq->iouh_Flags & UHFF_MULTI_2)
            {
                splitctrl = EQSF_MULTI_2;
            } else {
                splitctrl = EQSF_MULTI_1;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, splitctrl);
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        //eqh->eqh_CtrlStatus = eqh->eqh_CurrTD = 0;
        eqh->eqh_FirstTD = NULL; // clear for ehciFreeQHandTDs()

        ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 0
            ctrlstatus |= ETCF_DATA1;
        }
        predetd = NULL;
        eqh->eqh_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
        phyaddr = (IPTR)pciGetPhysical(hc, eqh->eqh_Buffer);
        do
        {
            if((eqh->eqh_Actual >= EHCI_TD_BULK_LIMIT) && (eqh->eqh_Actual < ioreq->iouh_Length))
            {
                KPRINTF(10, ("Bulk too large, splitting...\n"));
                break;
            }
            etd = ehciAllocTD(hc);
            if(!etd)
            {
                break;
            }
            if(predetd)
            {
                predetd->etd_Succ = etd;
                predetd->etd_NextTD = etd->etd_Self;
                predetd->etd_AltNextTD = hc->hc_ShortPktEndTD->etd_Self;
            } else {
                eqh->eqh_FirstTD = etd;
                //eqh->eqh_AltNextTD = eqh->eqh_NextTD = etd->etd_Self;
            }

            len = ioreq->iouh_Length - eqh->eqh_Actual;
            if(len > 4*EHCI_PAGE_SIZE)
            {
                len = 4*EHCI_PAGE_SIZE;
            }
            etd->etd_Length = len;
            KPRINTF(1, ("Bulk TD 0x%p len %ld (%ld/%ld) phy=0x%p\n",
                         etd, len, eqh->eqh_Actual, ioreq->iouh_Length, phyaddr));
            WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
            // FIXME need quark scatter gather mechanism here
            WRITEMEM32_LE(&etd->etd_BufferPtr[0], phyaddr);
            WRITEMEM32_LE(&etd->etd_BufferPtr[1], (phyaddr & EHCI_PAGE_MASK) + (1*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[2], (phyaddr & EHCI_PAGE_MASK) + (2*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[3], (phyaddr & EHCI_PAGE_MASK) + (3*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[4], (phyaddr & EHCI_PAGE_MASK) + (4*EHCI_PAGE_SIZE));

            // FIXME Use these on 64-bit-capable hardware
            etd->etd_ExtBufferPtr[0] = 0;
            etd->etd_ExtBufferPtr[1] = 0;
            etd->etd_ExtBufferPtr[2] = 0;
            etd->etd_ExtBufferPtr[3] = 0;
            etd->etd_ExtBufferPtr[4] = 0;

            phyaddr += len;
            eqh->eqh_Actual += len;

            predetd = etd;
        } while((eqh->eqh_Actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (eqh->eqh_Actual == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((eqh->eqh_Actual % ioreq->iouh_MaxPktSize) == 0)));

        if(!etd)
        {
            // not enough etds? try again later
            usbReleaseBuffer(eqh->eqh_Buffer, ioreq->iouh_Data, 0, 0);
            ehciFreeQHandTDs(hc, eqh);
            break;
        }
        ctrlstatus |= ETCF_READYINTEN|(predetd->etd_Length<<ETSS_TRANSLENGTH);
        WRITEMEM32_LE(&predetd->etd_CtrlStatus, ctrlstatus);

        predetd->etd_Succ = NULL;
        CONSTWRITEMEM32_LE(&predetd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&predetd->etd_AltNextTD, EHCI_TERMINATE);

        // due to sillicon bugs, we fill in the first overlay ourselves.
        etd = eqh->eqh_FirstTD;
        eqh->eqh_CurrTD = etd->etd_Self;
        eqh->eqh_NextTD = etd->etd_NextTD;
        eqh->eqh_AltNextTD = etd->etd_AltNextTD;
        eqh->eqh_CtrlStatus = etd->etd_CtrlStatus;
        eqh->eqh_BufferPtr[0] = etd->etd_BufferPtr[0];
        eqh->eqh_BufferPtr[1] = etd->etd_BufferPtr[1];
        eqh->eqh_BufferPtr[2] = etd->etd_BufferPtr[2];
        eqh->eqh_BufferPtr[3] = etd->etd_BufferPtr[3];
        eqh->eqh_BufferPtr[4] = etd->etd_BufferPtr[4];
        eqh->eqh_ExtBufferPtr[0] = etd->etd_ExtBufferPtr[0];
        eqh->eqh_ExtBufferPtr[1] = etd->etd_ExtBufferPtr[1];
        eqh->eqh_ExtBufferPtr[2] = etd->etd_ExtBufferPtr[2];
        eqh->eqh_ExtBufferPtr[3] = etd->etd_ExtBufferPtr[3];
        eqh->eqh_ExtBufferPtr[4] = etd->etd_ExtBufferPtr[4];

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<3) : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the asyncQH)
        eqh->eqh_Succ = hc->hc_EhciAsyncQH->eqh_Succ;
        eqh->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
        SYNC;

        eqh->eqh_Pred = hc->hc_EhciAsyncQH;
        eqh->eqh_Succ->eqh_Pred = eqh;
        hc->hc_EhciAsyncQH->eqh_Succ = eqh;
        hc->hc_EhciAsyncQH->eqh_NextQH = eqh->eqh_Self;
        SYNC;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }
}

void ehciUpdateFrameCounter(struct PCIController *hc) {

    Disable();
    hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xffffc000)|(READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT) & 0x3fff);
    Enable();
}

static AROS_INTH1(ehciCompleteInt, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    KPRINTF(1, ("CompleteInt!\n"));
    ehciUpdateFrameCounter(hc);

    /* **************** PROCESS DONE TRANSFERS **************** */

    if(hc->hc_AsyncAdvanced)
    {
        struct EhciQH *eqh;
        struct EhciTD *etd;
        struct EhciTD *nextetd;

        hc->hc_AsyncAdvanced = FALSE;

        KPRINTF(1, ("AsyncAdvance 0x%p\n", hc->hc_EhciAsyncFreeQH));

        while((eqh = hc->hc_EhciAsyncFreeQH))
        {
            KPRINTF(1, ("FreeQH 0x%p\n", eqh));
            nextetd = eqh->eqh_FirstTD;
            while((etd = nextetd))
            {
                KPRINTF(1, ("FreeTD 0x%p\n", nextetd));
                nextetd = etd->etd_Succ;
                ehciFreeTD(hc, etd);
            }
            hc->hc_EhciAsyncFreeQH = eqh->eqh_Succ;
            ehciFreeQH(hc, eqh);
        }
    }

    ehciHandleFinishedTDs(hc);

    if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        ehciScheduleCtrlTDs(hc);
    }

    if(hc->hc_IntXFerQueue.lh_Head->ln_Succ)
    {
        ehciScheduleIntTDs(hc);
    }

    if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
    {
        ehciScheduleBulkTDs(hc);
    }

    KPRINTF(1, ("CompleteDone\n"));

    return FALSE;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(ehciIntCode, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    struct PCIDevice *base = hc->hc_Device;
    struct PCIUnit *unit = hc->hc_Unit;
    ULONG intr;

    //KPRINTF(1, ("pciEhciInt()\n"));
    intr = READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS);
    if(intr & hc->hc_PCIIntEnMask)
    {
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS, intr);
        //KPRINTF(1, ("INT=%04lx\n", intr));
        if (!(hc->hc_Flags & HCF_ONLINE))
        {
            return FALSE;
        }
        if(intr & EHSF_FRAMECOUNTOVER)
        {
            hc->hc_FrameCounter |= 0x3fff;
            hc->hc_FrameCounter++;
            hc->hc_FrameCounter |= READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT) & 0x3fff;
            KPRINTF(5, ("Frame Counter Rollover %ld\n", hc->hc_FrameCounter));
        }
        if(intr & EHSF_ASYNCADVANCE)
        {
            KPRINTF(1, ("AsyncAdvance\n"));
            hc->hc_AsyncAdvanced = TRUE;
        }
        if(intr & EHSF_HOSTERROR)
        {
            KPRINTF(200, ("Host ERROR!\n"));
        }
        if(intr & EHSF_PORTCHANGED)
        {
            UWORD hciport;
            ULONG oldval;
            UWORD portreg = EHCI_PORTSC1;
            for(hciport = 0; hciport < hc->hc_NumPorts; hciport++, portreg += 4)
            {
                oldval = READREG32_LE(hc->hc_RegBase, portreg);
                // reflect port ownership (shortcut without hc->hc_PortNum20[hciport], as usb 2.0 maps 1:1)
                unit->hu_EhciOwned[hciport] = (oldval & EHPF_NOTPORTOWNER) ? FALSE : TRUE;
                if(oldval & EHPF_ENABLECHANGE)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                }
                if(oldval & EHPF_CONNECTCHANGE)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                }
                if(oldval & EHPF_RESUMEDTX)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                }
                if(oldval & EHPF_OVERCURRENTCHG)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
                }
                WRITEREG32_LE(hc->hc_RegBase, portreg, oldval);
                KPRINTF(20, ("PCI Int Port %ld Change %08lx\n", hciport + 1, oldval));
                if(hc->hc_PortChangeMap[hciport])
                {
                    unit->hu_RootPortChanges |= 1UL<<(hciport + 1);
                }
            }
            uhwCheckRootHubChanges(unit);
        }
        if(intr & (EHSF_TDDONE|EHSF_TDERROR|EHSF_ASYNCADVANCE))
        {
            SureCause(base, &hc->hc_CompleteInt);
        }
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

BOOL ehciInit(struct PCIController *hc, struct PCIUnit *hu) {

    struct PCIDevice *hd = hu->hu_Device;

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

    ULONG cnt;

    struct TagItem pciActivateMem[] =
    {
            { aHidd_PCIDevice_isMEM,    TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciActivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciDeactivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, FALSE },
            { TAG_DONE, 0UL },
    };

    hc->hc_portroute = 0;

    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "EHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)ehciCompleteInt;

    /*
        FIXME: Check the real size from USBCMD Frame List Size field (bits3:2)
        and set the value accordingly if Frame List Flag in the HCCPARAMS indicates RW for the field
        else use default value of EHCI_FRAMELIST_SIZE (1024)
    */
    hc->hc_PCIMemSize = sizeof(ULONG) * EHCI_FRAMELIST_SIZE + EHCI_FRAMELIST_ALIGNMENT + 1;
    hc->hc_PCIMemSize += sizeof(struct EhciQH) * EHCI_QH_POOLSIZE;
    hc->hc_PCIMemSize += sizeof(struct EhciTD) * EHCI_TD_POOLSIZE;

    /*
        FIXME: We should be able to read some EHCI registers before allocating memory
    */
    memptr = HIDD_PCIDriver_AllocPCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMemSize);
    hc->hc_PCIMem = (APTR) memptr;

    if(memptr) {
        // PhysicalAddress - VirtualAdjust = VirtualAddress
        // VirtualAddress  + VirtualAdjust = PhysicalAddress
        hc->hc_PCIVirtualAdjust = pciGetPhysical(hc, memptr) - (APTR)memptr;
        KPRINTF(10, ("VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust));

        // align memory
        memptr = (UBYTE *) ((((IPTR) hc->hc_PCIMem) + EHCI_FRAMELIST_ALIGNMENT) & (~EHCI_FRAMELIST_ALIGNMENT));
        hc->hc_EhciFrameList = (ULONG *) memptr;
        KPRINTF(10, ("FrameListBase 0x%p\n", hc->hc_EhciFrameList));
        memptr += sizeof(APTR) * EHCI_FRAMELIST_SIZE;

        // build up QH pool
        eqh = (struct EhciQH *) memptr;
        hc->hc_EhciQHPool = eqh;
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
        hc->hc_EhciTDPool = etd;
        cnt = EHCI_TD_POOLSIZE - 1;
        do
        {
            etd->etd_Succ = (etd + 1);
            WRITEMEM32_LE(&etd->etd_Self, (IPTR) (&etd->etd_NextTD) + hc->hc_PCIVirtualAdjust);
            etd++;
        } while(--cnt);
        etd->etd_Succ = NULL;
        WRITEMEM32_LE(&etd->etd_Self, (IPTR) (&etd->etd_NextTD) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct EhciTD) * EHCI_TD_POOLSIZE;

        // empty async queue head
        hc->hc_EhciAsyncFreeQH = NULL;
        hc->hc_EhciAsyncQH = eqh = ehciAllocQH(hc);
        eqh->eqh_Succ = eqh;
        eqh->eqh_Pred = eqh;
        CONSTWRITEMEM32_LE(&eqh->eqh_EPCaps, EQEF_RECLAMHEAD);
        eqh->eqh_NextQH = eqh->eqh_Self;

        // empty terminating queue head
        hc->hc_EhciTermQH = eqh = ehciAllocQH(hc);
        eqh->eqh_Succ = NULL;
        CONSTWRITEMEM32_LE(&eqh->eqh_NextQH, EHCI_TERMINATE);
        predeqh = eqh;

        // 1 ms INT QH
        hc->hc_EhciIntQH[0] = eqh = ehciAllocQH(hc);
        eqh->eqh_Succ = predeqh;
        predeqh->eqh_Pred = eqh;
        eqh->eqh_Pred = NULL; // who knows...
        //eqh->eqh_NextQH = predeqh->eqh_Self;
        predeqh = eqh;

        // make 11 levels of QH interrupts
        for(cnt = 1; cnt < 11; cnt++)
        {
            hc->hc_EhciIntQH[cnt] = eqh = ehciAllocQH(hc);
            eqh->eqh_Succ = predeqh;
            eqh->eqh_Pred = NULL; // who knows...
            //eqh->eqh_NextQH = predeqh->eqh_Self; // link to previous int level
            predeqh = eqh;
        }

        ehciUpdateIntTree(hc);

        // fill in framelist with IntQH entry points based on interval
        tabptr = hc->hc_EhciFrameList;
        for(cnt = 0; cnt < EHCI_FRAMELIST_SIZE; cnt++)
        {
            eqh = hc->hc_EhciIntQH[10];
            bitcnt = 0;
            do
            {
                if(cnt & (1UL<<bitcnt))
                {
                    eqh = hc->hc_EhciIntQH[bitcnt];
                    break;
                }
            } while(++bitcnt < 11);
            *tabptr++ = eqh->eqh_Self;
        }

        etd = hc->hc_ShortPktEndTD = ehciAllocTD(hc);
        etd->etd_Succ = NULL;
        CONSTWRITEMEM32_LE(&etd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&etd->etd_AltNextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&etd->etd_CtrlStatus, 0);

        // time to initialize hardware...
        OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (IPTR *) &pciregbase);
        pciregbase = (APTR) (((IPTR) pciregbase) & (~0xf));
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateMem); // activate memory

        extcapoffset = (READREG32_LE(pciregbase, EHCI_HCCPARAMS) & EHCM_EXTCAPOFFSET)>>EHCS_EXTCAPOFFSET;

        while(extcapoffset >= 0x40)
        {
            KPRINTF(10, ("EHCI has extended caps at 0x%08lx\n", extcapoffset));
            legsup = PCIXReadConfigLong(hc, extcapoffset);
            if(((legsup & EHLM_CAP_ID) >> EHLS_CAP_ID) == 0x01)
            {
                if(legsup & EHLF_BIOS_OWNER)
                {
                    KPRINTF(10, ("BIOS still has hands on EHCI, trying to get rid of it\n"));
                    legsup |= EHLF_OS_OWNER;
                    PCIXWriteConfigLong(hc, extcapoffset, legsup);
                    timeout = 100;
                    do
                    {
                        legsup = PCIXReadConfigLong(hc, extcapoffset);
                        if(!(legsup & EHLF_BIOS_OWNER))
                        {
                            KPRINTF(10, ("BIOS gave up on EHCI. Pwned!\n"));
                            break;
                        }
                        uhwDelayMS(10, hu);
                     } while(--timeout);
                     if(!timeout)
                     {
                         KPRINTF(10, ("BIOS didn't release EHCI. Forcing and praying...\n"));
                         legsup |= EHLF_OS_OWNER;
                         legsup &= ~EHLF_BIOS_OWNER;
                         PCIXWriteConfigLong(hc, extcapoffset, legsup);
                     }
                }
                /* disable all SMIs */
                PCIXWriteConfigLong(hc, extcapoffset + 4, 0);
                break;
            }
            extcapoffset = (legsup & EHCM_EXTCAPOFFSET)>>EHCS_EXTCAPOFFSET;
        }

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivateBusmaster); // no busmaster yet

        // we use the operational registers as RegBase.
        hc->hc_RegBase = (APTR) ((IPTR) pciregbase + READREG16_LE(pciregbase, EHCI_CAPLENGTH));
        KPRINTF(10, ("RegBase = 0x%p\n", hc->hc_RegBase));

        KPRINTF(10, ("Resetting EHCI HC\n"));
        KPRINTF(10, ("EHCI CMD: 0x%08x STS: 0x%08x\n", READREG32_LE(hc->hc_RegBase, EHCI_USBCMD), READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS)));
        /* Step 1: Stop the HC */
        tmp = READREG32_LE(hc->hc_RegBase, EHCI_USBCMD);
        tmp &= ~EHUF_RUNSTOP;
        CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, tmp);

        /* Step 2. Wait for the controller to halt */
        cnt = 100;
        do
        {
            uhwDelayMS(10, hu);
            if(READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS) & EHSF_HCHALTED)
            {
                break;
            }
        } while (cnt--);
        if (cnt == 0)
        {
            KPRINTF(200, ("EHCI: Timeout waiting for controller to halt\n"));
        }

        /* Step 3. Reset the controller */
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, tmp | EHUF_HCRESET);

        /* Step 4. Wait for the reset bit to clear */
        cnt = 100;
        do
        {
            uhwDelayMS(10, hu);
            if(!(READREG32_LE(hc->hc_RegBase, EHCI_USBCMD) & EHUF_HCRESET))
            {
                break;
            }
        } while(--cnt);

#ifdef DEBUG
        if(cnt == 0)
        {
            KPRINTF(20, ("Reset Timeout!\n"));
        } else {
            KPRINTF(20, ("Reset finished after %ld ticks\n", 100-cnt));
        }
#endif

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateBusmaster); // enable busmaster

        // Read HCSPARAMS register to obtain number of downstream ports
        hcsparams = READREG32_LE(pciregbase, EHCI_HCSPARAMS);
        hccparams = READREG32_LE(pciregbase, EHCI_HCCPARAMS);

        hc->hc_NumPorts = (hcsparams & EHSM_NUM_PORTS)>>EHSS_NUM_PORTS;

        KPRINTF(20, ("Found EHCI Controller 0x%p with %ld ports (%ld companions with %ld ports each)\n",
                    hc->hc_PCIDeviceObject, hc->hc_NumPorts,
                    (hcsparams & EHSM_NUM_COMPANIONS)>>EHSS_NUM_COMPANIONS,
                    (hcsparams & EHSM_PORTS_PER_COMP)>>EHSS_PORTS_PER_COMP));

        if(hcsparams & EHSF_EXTPORTROUTING)
        {
            hc->hc_complexrouting = TRUE;
            hc->hc_portroute = READREG32_LE(pciregbase, EHCI_HCSPPORTROUTE);
#ifdef DEBUG
            for(cnt = 0; cnt < hc->hc_NumPorts; cnt++) {
                KPRINTF(100, ("Port %ld maps to controller %ld\n", cnt, ((hc->hc_portroute >> (cnt<<2)) & 0xf)));
            }
#endif
        }else{
            hc->hc_complexrouting = FALSE;
        }

        KPRINTF(20, ("HCCParams: 64 Bit=%s, ProgFrameList=%s, AsyncSchedPark=%s\n",
                    (hccparams & EHCF_64BITS) ? "Yes" : "No",
                    (hccparams & EHCF_PROGFRAMELIST) ? "Yes" : "No",
                    (hccparams & EHCF_ASYNCSCHEDPARK) ? "Yes" : "No"));
                    hc->hc_EhciUsbCmd = (1UL<<EHUS_INTTHRESHOLD);

        /* FIXME HERE: Process EHCF_64BITS flag and implement 64-bit addressing */

        if(hccparams & EHCF_ASYNCSCHEDPARK)
        {
            KPRINTF(20, ("Enabling AsyncSchedParkMode with MULTI_3\n"));
            hc->hc_EhciUsbCmd |= EHUF_ASYNCSCHEDPARK|(3<<EHUS_ASYNCPARKCOUNT);
        }

        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, hc->hc_EhciUsbCmd);

        CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT, 0);

        WRITEREG32_LE(hc->hc_RegBase, EHCI_PERIODICLIST, (IPTR)pciGetPhysical(hc, hc->hc_EhciFrameList));
        WRITEREG32_LE(hc->hc_RegBase, EHCI_ASYNCADDR, AROS_LONG2LE(hc->hc_EhciAsyncQH->eqh_Self));
        CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS, EHSF_ALL_INTS);

        // install reset handler
        hc->hc_ResetInt.is_Code = (VOID_FUNC)EhciResetHandler;
        hc->hc_ResetInt.is_Data = hc;
        AddResetCallback(&hc->hc_ResetInt);

        // add interrupt
        hc->hc_PCIIntHandler.is_Node.ln_Name = "EHCI PCI (pciusb.device)";
        hc->hc_PCIIntHandler.is_Node.ln_Pri = 5;
        hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
        hc->hc_PCIIntHandler.is_Code = (VOID_FUNC)ehciIntCode;
        hc->hc_PCIIntHandler.is_Data = hc;
        AddIntServer(INTB_KERNEL + hc->hc_PCIIntLine, &hc->hc_PCIIntHandler);

        hc->hc_PCIIntEnMask = EHSF_ALL_INTS;
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBINTEN, hc->hc_PCIIntEnMask);

        CacheClearE(hc->hc_EhciFrameList, sizeof(ULONG) * EHCI_FRAMELIST_SIZE,      CACRF_ClearD);
        CacheClearE(hc->hc_EhciQHPool,    sizeof(struct EhciQH) * EHCI_QH_POOLSIZE, CACRF_ClearD);
        CacheClearE(hc->hc_EhciTDPool,    sizeof(struct EhciTD) * EHCI_TD_POOLSIZE, CACRF_ClearD);
                    
        CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_CONFIGFLAG, EHCF_CONFIGURED);
        hc->hc_EhciUsbCmd |= EHUF_RUNSTOP|EHUF_PERIODICENABLE|EHUF_ASYNCENABLE;
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, hc->hc_EhciUsbCmd);
        SYNC;

        KPRINTF(20, ("HW Init done\n"));

        KPRINTF(10, ("HW Regs USBCMD=%04lx\n", READREG32_LE(hc->hc_RegBase, EHCI_USBCMD)));
        KPRINTF(10, ("HW Regs USBSTS=%04lx\n", READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS)));
        KPRINTF(10, ("HW Regs FRAMECOUNT=%04lx\n", READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT)));

        KPRINTF(1000, ("ehciInit returns TRUE...\n"));
        return TRUE;
    }

    /*
        FIXME: What would the appropriate debug level be?
    */
    KPRINTF(1000, ("ehciInit returns FALSE...\n"));
    return FALSE;
}

void ehciFree(struct PCIController *hc, struct PCIUnit *hu) {

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        switch(hc->hc_HCIType)
        {
            case HCITYPE_EHCI:
            {
                UWORD portreg;
                UWORD hciport;
                KPRINTF(20, ("Shutting down EHCI 0x%p\n", hc));
                CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBINTEN, 0);
                // disable all ports
                for(hciport = 0; hciport < hc->hc_NumPorts; hciport++)
                {
                    portreg = EHCI_PORTSC1 + (hciport<<2);
                    WRITEREG32_LE(hc->hc_RegBase, portreg, 0);
                }
                CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, 1UL<<EHUS_INTTHRESHOLD);
                uhwDelayMS(10, hu);
                CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_CONFIGFLAG, 0);
                CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, EHUF_HCRESET|(1UL<<EHUS_INTTHRESHOLD));
                SYNC;

                uhwDelayMS(50, hu);
                CONSTWRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, 1UL<<EHUS_INTTHRESHOLD);
                SYNC;

                uhwDelayMS(10, hu);

                KPRINTF(20, ("Shutting down EHCI done.\n"));
                break;
            }
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

}
