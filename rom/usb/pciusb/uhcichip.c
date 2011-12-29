/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved
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

static AROS_UFH3(void, UhciResetHandler,
                 AROS_UFHA(struct PCIController *, hc, A1),
                 AROS_UFHA(APTR, unused, A5),
                 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    // stop controller and disable all interrupts
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);

    AROS_USERFUNC_EXIT
}

void uhciFreeQContext(struct PCIController *hc, struct UhciQH *uqh) {

    struct UhciTD *utd = NULL;
    struct UhciTD *nextutd;

    KPRINTF(5, ("Unlinking QContext %08lx\n", uqh));
    // unlink from schedule
    uqh->uqh_Pred->uxx_Link = uqh->uqh_Succ->uxx_Self;
    SYNC;

    uqh->uqh_Succ->uxx_Pred = uqh->uqh_Pred;
    uqh->uqh_Pred->uxx_Succ = uqh->uqh_Succ;
    SYNC;

    nextutd = uqh->uqh_FirstTD;
    while(nextutd)
    {
        KPRINTF(1, ("FreeTD %08lx\n", nextutd));
        utd = nextutd;
        nextutd = (struct UhciTD *) utd->utd_Succ;
        uhciFreeTD(hc, utd);
    }
    uhciFreeQH(hc, uqh);
}

void uhciUpdateIntTree(struct PCIController *hc) {

    struct UhciXX *uxx;
    struct UhciXX *preduxx;
    struct UhciXX *lastuseduxx;
    UWORD cnt;

    // optimize linkage between queue heads
    preduxx = lastuseduxx = (struct UhciXX *) hc->hc_UhciCtrlQH; //hc->hc_UhciIsoTD;
    for(cnt = 0; cnt < 9; cnt++)
    {
        uxx = (struct UhciXX *) hc->hc_UhciIntQH[cnt];
        if(uxx->uxx_Succ != preduxx)
        {
            lastuseduxx = uxx->uxx_Succ;
        }
        uxx->uxx_Link = lastuseduxx->uxx_Self;
        preduxx = uxx;
    }
}

void uhciCheckPortStatusChange(struct PCIController *hc) {

    struct PCIUnit *unit = hc->hc_Unit;
    UWORD oldval;
    UWORD hciport;

    // check for port status change for UHCI and frame rollovers

    for(hciport = 0; hciport < 2; hciport++)
    {
        UWORD portreg;
        UWORD idx = hc->hc_PortNum20[hciport];
        // don't pay attention to UHCI port changes when pwned by EHCI
        if(!unit->hu_EhciOwned[idx])
        {
            portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
            oldval = READIO16_LE(hc->hc_RegBase, portreg);
            if(oldval & UHPF_ENABLECHANGE)
            {
                KPRINTF(10, ("Port %ld (%ld) Enable changed\n", idx, hciport));
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
            }
            if(oldval & UHPF_CONNECTCHANGE)
            {
                KPRINTF(10, ("Port %ld (%ld) Connect changed\n", idx, hciport));
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                if(!(oldval & UHPF_PORTCONNECTED))
                {
                    if(unit->hu_PortMap20[idx])
                    {
                        KPRINTF(20, ("Transferring Port %ld back to EHCI\n", idx));
                        unit->hu_EhciOwned[idx] = TRUE;
                    }
                }
            }
            if(oldval & UHPF_RESUMEDTX)
            {
                KPRINTF(10, ("Port %ld (%ld) Resume changed\n", idx, hciport));
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                oldval &= ~UHPF_RESUMEDTX;
            }
            if(hc->hc_PortChangeMap[hciport])
            {
                unit->hu_RootPortChanges |= 1UL<<(idx+1);
                /*KPRINTF(10, ("Port %ld (%ld) contributes %04lx to portmap %04lx\n",
                             idx, hciport, hc->hc_PortChangeMap[hciport], unit->hu_RootPortChanges));*/
            }
            WRITEIO16_LE(hc->hc_RegBase, portreg, oldval);
        }
    }
}

void uhciHandleFinishedTDs(struct PCIController *hc) {

    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    struct IOUsbHWReq *nextioreq;
    struct UhciQH *uqh;
    struct UhciTD *utd;
    struct UhciTD *nextutd;
    UWORD devadrep;
    ULONG len;
    ULONG linkelem;
    UWORD inspect;
    BOOL shortpkt;
    ULONG ctrlstatus;
    ULONG nextctrlstatus = 0;
    ULONG token = 0;
    ULONG actual;
    BOOL updatetree = FALSE;
    BOOL fixsetupterm = FALSE;

    KPRINTF(1, ("Checking for work done...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        uqh = (struct UhciQH *) ioreq->iouh_DriverPrivate1;
        if(uqh)
        {
            KPRINTF(1, ("Examining IOReq=%08lx with UQH=%08lx\n", ioreq, uqh));
            linkelem = READMEM32_LE(&uqh->uqh_Element);
            inspect = 0;
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            if(linkelem & UHCI_TERMINATE)
            {
                KPRINTF(1, ("UQH terminated %08lx\n", linkelem));
                inspect = 2;
            } else {
                utd = (struct UhciTD *) ((linkelem & UHCI_PTRMASK) - hc->hc_PCIVirtualAdjust - 16); // struct UhciTD starts 16 bytes before physical TD
                ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                nextutd = (struct UhciTD *)utd->utd_Succ;
                if(!(ctrlstatus & UTCF_ACTIVE) && nextutd)
                {
                    /* OK, it's not active. Does it look like it's done? Code copied from below.
                       If not done, check the next TD too. */
                    if(ctrlstatus & (UTSF_BABBLE|UTSF_STALLED|UTSF_CRCTIMEOUT|UTSF_DATABUFFERERR|UTSF_BITSTUFFERR))
                    {
                        nextutd = 0;
                    }
                    else
                    {
                        token = READMEM32_LE(&utd->utd_Token);
                        len = (ctrlstatus & UTSM_ACTUALLENGTH) >> UTSS_ACTUALLENGTH;
                        if((len != (token & UTTM_TRANSLENGTH) >> UTTS_TRANSLENGTH))
                        {
                           nextutd = 0;
                        }
                    }
                    if(nextutd)
                    {
                        nextctrlstatus = READMEM32_LE(&nextutd->utd_CtrlStatus);
                    }
                }
                /* Now, did the element link pointer change while we fetched the status for the pointed at TD?
                   If so, disregard the gathered information and assume still active. */
                if(READMEM32_LE(&uqh->uqh_Element) != linkelem)
                {
                    /* Oh well, probably still active */
                    KPRINTF(1, ("Link Element changed, still active.\n"));
                }
                else if(!(ctrlstatus & UTCF_ACTIVE) && (nextutd == 0 || !(nextctrlstatus & UTCF_ACTIVE)))
                {
                    KPRINTF(1, ("CtrlStatus inactive %08lx\n", ctrlstatus));
                    inspect = 1;
                }
                else if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
                {
                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                    inspect = 1;
                }
            }
            fixsetupterm = FALSE;
            if(inspect)
            {
                APTR data = &((UBYTE *)ioreq->iouh_Data)[ioreq->iouh_Actual];
                shortpkt = FALSE;
                if(inspect < 2) // if all went okay, don't traverse list, assume all bytes successfully transferred
                {
                    utd = uqh->uqh_FirstTD;
                    actual = 0;
                    do
                    {
                        ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                        if(ctrlstatus & UTCF_ACTIVE)
                        {
                            KPRINTF(20, ("Internal error! Still active?!\n"));
                            if(ctrlstatus & UTSF_BABBLE)
                            {
                                KPRINTF(200, ("HOST CONTROLLER IS DEAD!!!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET|UHCF_MAXPACKET64|UHCF_CONFIGURE|UHCF_RUNSTOP);
                                inspect = 0;
                                break;
                            }
                            break;
                        }
                        token = READMEM32_LE(&utd->utd_Token);
                        KPRINTF(1, ("TD=%08lx CS=%08lx Token=%08lx\n", utd, ctrlstatus, token));
                        if(ctrlstatus & (UTSF_BABBLE|UTSF_STALLED|UTSF_CRCTIMEOUT|UTSF_DATABUFFERERR|UTSF_BITSTUFFERR))
                        {
                            if(ctrlstatus & UTSF_BABBLE)
                            {
                                KPRINTF(20, ("Babble error %08lx/%08lx\n", ctrlstatus, token));
                                ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
#if 0
                                // VIA chipset seems to die on babble!?!
                                KPRINTF(10, ("HW Regs USBCMD=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_USBCMD)));
                                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE|UHCF_RUNSTOP);
                                SYNC;
#endif
                                //retry
                                //ctrlstatus &= ~(UTSF_BABBLE|UTSF_STALLED|UTSF_CRCTIMEOUT|UTSF_DATABUFFERERR|UTSF_BITSTUFFERR|UTSF_NAK);
                                ctrlstatus |= UTCF_ACTIVE;
                                WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
                                SYNC;
                                inspect = 3;
                                break;
                            }
                            else if(ctrlstatus & UTSF_CRCTIMEOUT)
                            {
                                KPRINTF(20, ("CRC/Timeout error IOReq=%08lx DIR=%ld\n", ioreq, ioreq->iouh_Dir));
                                if(ctrlstatus & UTSF_STALLED)
                                {
                                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                                } else {
                                    ioreq->iouh_Req.io_Error = (ioreq->iouh_Dir == UHDIR_IN) ? UHIOERR_CRCERROR : UHIOERR_TIMEOUT;
                                }
                            }
                            else if(ctrlstatus & UTSF_STALLED)
                            {
                                KPRINTF(20, ("STALLED!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            }
                            else if(ctrlstatus & UTSF_BITSTUFFERR)
                            {
                                KPRINTF(20, ("Bitstuff error\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                            }
                            else if(ctrlstatus & UTSF_DATABUFFERERR)
                            {
                                KPRINTF(20, ("Databuffer error\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            }
                            inspect = 0;
                            break;
                        }
                        if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]) && (ctrlstatus & UTSF_NAK))
                        {
                            ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            inspect = 0;
                        }

                        len = (ctrlstatus & UTSM_ACTUALLENGTH)>>UTSS_ACTUALLENGTH;
                        if((len != (token & UTTM_TRANSLENGTH)>>UTTS_TRANSLENGTH))
                        {
                            shortpkt = TRUE;
                        }
                        len = (len+1) & 0x7ff; // get real length
                        if((token & UTTM_PID)>>UTTS_PID != PID_SETUP) // don't count setup packet
                        {
                            actual += len;
                            // due to the VIA babble bug workaround, actually more bytes can
                            // be received than requested, limit the actual value to the upper limit
                            if(actual > uqh->uqh_Actual)
                            {
                                actual = uqh->uqh_Actual;
                            }
                        }
                        if(shortpkt)
                        {
                            break;
                        }
                    } while((utd = (struct UhciTD *) utd->utd_Succ));
                    if(inspect == 3)
                    {
                        // bail out from babble
                        ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
                        continue;
                    }
                    if((actual < uqh->uqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                    {
                        KPRINTF(10, ("Short packet: %ld < %ld\n", actual, ioreq->iouh_Length));
                        ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                    }
                } else {
                    KPRINTF(10, ("all %ld bytes transferred\n", uqh->uqh_Actual));
                    actual = uqh->uqh_Actual;
                }
                ioreq->iouh_Actual += actual;
                // due to the short packet, the terminal of a setup packet has not been sent. Please do so.
                if(shortpkt && (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER))
                {
                    fixsetupterm = TRUE;
                }
                // this is actually no short packet but result of the VIA babble fix
                if(shortpkt && (ioreq->iouh_Actual == ioreq->iouh_Length))
                {
                    shortpkt = FALSE;
                }
                unit->hu_DevBusyReq[devadrep] = NULL;
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                if (uqh->uqh_DataBuffer)
                    usbReleaseBuffer(uqh->uqh_DataBuffer, data, actual, ioreq->iouh_Dir);
                if (uqh->uqh_SetupBuffer)
                    usbReleaseBuffer(uqh->uqh_SetupBuffer, &ioreq->iouh_SetupData, sizeof(ioreq->iouh_SetupData), (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT);
                uhciFreeQContext(hc, uqh);
                if(ioreq->iouh_Req.io_Command == UHCMD_INTXFER)
                {
                    updatetree = TRUE;
                }
                if(inspect)
                {
                    if(inspect < 2) // otherwise, toggle will be right already
                    {
                        // use next data toggle bit based on last successful transaction
                        unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? FALSE : TRUE;
                    }
                    if((!shortpkt && (ioreq->iouh_Actual < ioreq->iouh_Length)) || fixsetupterm)
                    {
                        // fragmented, do some more work
                        switch(ioreq->iouh_Req.io_Command)
                        {
                            case UHCMD_CONTROLXFER:
                                KPRINTF(10, ("Rescheduling CtrlTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                AddHead(&hc->hc_CtrlXFerQueue, (struct Node *) ioreq);
                                break;

                            case UHCMD_INTXFER:
                                KPRINTF(10, ("Rescheduling IntTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                AddHead(&hc->hc_IntXFerQueue, (struct Node *) ioreq);
                                break;

                            case UHCMD_BULKXFER:
                                KPRINTF(10, ("Rescheduling BulkTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                AddHead(&hc->hc_BulkXFerQueue, (struct Node *) ioreq);
                                break;

                            default:
                                KPRINTF(10, ("Uhm, internal error, dunno where to queue this req\n"));
                                ReplyMsg(&ioreq->iouh_Req.io_Message);
                        }
                    } else {
                        // check for sucessful clear feature and set address ctrl transfers
                        if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                        {
                            uhwCheckSpecialCtrlTransfers(hc, ioreq);
                        }
                        ReplyMsg(&ioreq->iouh_Req.io_Message);
                    }
                } else {
                    // be sure to save the data toggle bit where the error occurred
                    unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? TRUE : FALSE;
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                }
            }
        } else {
            KPRINTF(20, ("IOReq=%08lx has no UQH!\n", ioreq));
        }
        ioreq = nextioreq;
    }
    if(updatetree)
    {
        KPRINTF(10, ("Updating Tree\n"));
        uhciUpdateIntTree(hc);
    }
}

void uhciScheduleCtrlTDs(struct PCIController *hc) {

    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct UhciQH *uqh;
    struct UhciTD *setuputd;
    struct UhciTD *datautd;
    struct UhciTD *termutd;
    struct UhciTD *predutd;
    ULONG actual;
    ULONG ctrlstatus;
    ULONG token;
    ULONG len;
    ULONG phyaddr;
    BOOL cont;

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

        uqh = uhciAllocQH(hc);
        if(!uqh)
        {
            break;
        }

        setuputd = uhciAllocTD(hc);
        if(!setuputd)
        {
            uhciFreeQH(hc, uqh);
            break;
        }
        termutd = uhciAllocTD(hc);
        if(!termutd)
        {
            uhciFreeTD(hc, setuputd);
            uhciFreeQH(hc, uqh);
            break;
        }
        uqh->uqh_IOReq = ioreq;

        //termutd->utd_QueueHead = setuputd->utd_QueueHead = uqh;

        KPRINTF(1, ("SetupTD=%08lx, TermTD=%08lx\n", setuputd, termutd));

        // fill setup td
        ctrlstatus = UTCF_ACTIVE|UTCF_3ERRORSLIMIT;
        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            ctrlstatus |= UTCF_LOWSPEED;
        }
        token = (ioreq->iouh_DevAddr<<UTTS_DEVADDR)|(ioreq->iouh_Endpoint<<UTTS_ENDPOINT);
        //setuputd->utd_Pred = NULL;
        if(ioreq->iouh_Actual)
        {
            // this is a continuation of a fragmented ctrl transfer!
            KPRINTF(1, ("Continuing FRAGMENT at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
            cont = TRUE;
        } else {
            cont = FALSE;
            uqh->uqh_FirstTD = setuputd;
            uqh->uqh_Element = setuputd->utd_Self; // start of queue
            uqh->uqh_SetupBuffer = usbGetBuffer(&ioreq->iouh_SetupData, sizeof(ioreq->iouh_SetupData), (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT);
            WRITEMEM32_LE(&setuputd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&setuputd->utd_Token, (PID_SETUP<<UTTS_PID)|token|(7<<UTTS_TRANSLENGTH)|UTTF_DATA0);
            WRITEMEM32_LE(&setuputd->utd_BufferPtr, (ULONG) (IPTR) pciGetPhysical(hc, uqh->uqh_SetupBuffer));
        }

        token |= (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? PID_IN : PID_OUT;
        predutd = setuputd;
        actual = ioreq->iouh_Actual;

        if(ioreq->iouh_Length - actual)
        {
            ctrlstatus |= UTCF_SHORTPACKET;
            if(cont)
            {
                if(!unit->hu_DevDataToggle[devadrep])
                {
                    // continue with data toggle 0
                    token |= UTTF_DATA1;
                }
            } else {
                ioreq->iouh_Actual=0;
            }
            uqh->uqh_DataBuffer = usbGetBuffer(&(((UBYTE *) ioreq->iouh_Data)[ioreq->iouh_Actual]), ioreq->iouh_Length - actual, ioreq->iouh_Dir);
            phyaddr = (ULONG)(IPTR)pciGetPhysical(hc, uqh->uqh_DataBuffer);
            do
            {
                datautd = uhciAllocTD(hc);
                if(!datautd)
                {
                    break;
                }
                token ^= UTTF_DATA1; // toggle bit
                predutd->utd_Link = datautd->utd_Self;
                predutd->utd_Succ = (struct UhciXX *) datautd;
                //datautd->utd_Pred = (struct UhciXX *) predutd;
                //datautd->utd_QueueHead = uqh;
                len = ioreq->iouh_Length - actual;
                if(len > ioreq->iouh_MaxPktSize)
                {
                    len = ioreq->iouh_MaxPktSize;
                }
                WRITEMEM32_LE(&datautd->utd_CtrlStatus, ctrlstatus);
#if 1
/* FIXME: This workaround for a VIA babble bug will potentially overwrite innocent memory (very rarely), but will avoid the host controller dropping dead completely. */
                if((len < ioreq->iouh_MaxPktSize) && (ioreq->iouh_SetupData.bmRequestType & URTF_IN))
                {
                    WRITEMEM32_LE(&datautd->utd_Token, token|((ioreq->iouh_MaxPktSize-1)<<UTTS_TRANSLENGTH)); // no masking need here as len is always >= 1
                } else {
                    WRITEMEM32_LE(&datautd->utd_Token, token|((len-1)<<UTTS_TRANSLENGTH)); // no masking need here as len is always >= 1
                }
#else
                WRITEMEM32_LE(&datautd->utd_Token, token|((len-1)<<UTTS_TRANSLENGTH)); // no masking need here as len is always >= 1
#endif
                WRITEMEM32_LE(&datautd->utd_BufferPtr, phyaddr);
                phyaddr += len;
                actual += len;
                predutd = datautd;
            } while((actual < ioreq->iouh_Length) && (actual - ioreq->iouh_Actual < UHCI_TD_CTRL_LIMIT));
            if(actual == ioreq->iouh_Actual)
            {
                // not at least one data TD? try again later
                uhciFreeTD(hc, setuputd);
                uhciFreeTD(hc, termutd);
                uhciFreeQH(hc, uqh);
                break;
            }
            if(cont)
            {
                // free Setup packet
                KPRINTF(1, ("Freeing setup\n"));
                uqh->uqh_FirstTD = (struct UhciTD *) setuputd->utd_Succ;
                //uqh->uqh_FirstTD->utd_Pred = NULL;
                uqh->uqh_Element = setuputd->utd_Succ->uxx_Self; // start of queue after setup packet
                uhciFreeTD(hc, setuputd);
                // set toggle for next batch
                unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? FALSE : TRUE;
            }
        } else {
            if(cont)
            {
                // free Setup packet, assign termination as first packet (no data)
                KPRINTF(1, ("Freeing setup (term only)\n"));
                uqh->uqh_FirstTD = (struct UhciTD *) termutd;
                uqh->uqh_Element = termutd->utd_Self; // start of queue after setup packet
                uhciFreeTD(hc, setuputd);
                predutd = NULL;
            }
        }
        uqh->uqh_Actual = actual - ioreq->iouh_Actual;
        ctrlstatus |= UTCF_READYINTEN;
        if(actual == ioreq->iouh_Length)
        {
            // TERM packet
            KPRINTF(1, ("Activating TERM\n"));
            token |= UTTF_DATA1;
            token ^= (PID_IN^PID_OUT)<<UTTS_PID;

            if(predutd)
            {
                predutd->utd_Link = termutd->utd_Self;
                predutd->utd_Succ = (struct UhciXX *) termutd;
            }
            //termutd->utd_Pred = (struct UhciXX *) predutd;
            WRITEMEM32_LE(&termutd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&termutd->utd_Token, token|(0x7ff<<UTTS_TRANSLENGTH));
            CONSTWRITEMEM32_LE(&termutd->utd_Link, UHCI_TERMINATE);
            termutd->utd_Succ = NULL;
            //uqh->uqh_LastTD = termutd;
        } else {
            KPRINTF(1, ("Setup data phase fragmented\n"));
            // don't create TERM, we don't know the final data toggle bit
            // but mark the last data TD for interrupt generation
            WRITEMEM32_LE(&predutd->utd_CtrlStatus, ctrlstatus);
            uhciFreeTD(hc, termutd);
            CONSTWRITEMEM32_LE(&predutd->utd_Link, UHCI_TERMINATE);
            predutd->utd_Succ = NULL;
            //uqh->uqh_LastTD = predutd;
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = uqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the CtrlQH)
        uqh->uqh_Succ = hc->hc_UhciCtrlQH->uqh_Succ;
        uqh->uqh_Link = uqh->uqh_Succ->uxx_Self;
        SYNC;

        uqh->uqh_Pred = (struct UhciXX *) hc->hc_UhciCtrlQH;
        uqh->uqh_Succ->uxx_Pred = (struct UhciXX *) uqh;
        hc->hc_UhciCtrlQH->uqh_Succ = (struct UhciXX *) uqh;
        hc->hc_UhciCtrlQH->uqh_Link = uqh->uqh_Self;
        SYNC;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    }
}

void uhciScheduleIntTDs(struct PCIController *hc) {

    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD devadrep;
    struct UhciQH *uqh;
    struct UhciQH *intuqh;
    struct UhciTD *utd;
    struct UhciTD *predutd;
    ULONG actual;
    ULONG ctrlstatus;
    ULONG token;
    ULONG len;
    ULONG phyaddr;

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

        uqh = uhciAllocQH(hc);
        if(!uqh)
        {
            break;
        }

        uqh->uqh_IOReq = ioreq;

        ctrlstatus = UTCF_ACTIVE|UTCF_1ERRORLIMIT|UTCF_SHORTPACKET;
        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            ctrlstatus |= UTCF_LOWSPEED;
        }
        token = (ioreq->iouh_DevAddr<<UTTS_DEVADDR)|(ioreq->iouh_Endpoint<<UTTS_ENDPOINT);
        token |= (ioreq->iouh_Dir == UHDIR_IN) ? PID_IN : PID_OUT;
        predutd = NULL;
        actual = ioreq->iouh_Actual;
        uqh->uqh_DataBuffer = usbGetBuffer(&(((UBYTE *) ioreq->iouh_Data)[ioreq->iouh_Actual]), ioreq->iouh_Length - actual, ioreq->iouh_Dir);
        phyaddr = (ULONG) (IPTR) pciGetPhysical(hc, uqh->uqh_DataBuffer);
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 1
            KPRINTF(1, ("Data1\n"));
            token |= UTTF_DATA1;
        } else {
            KPRINTF(1, ("Data0\n"));
        }
        do
        {
            utd = uhciAllocTD(hc);
            if(!utd)
            {
                break;
            }
            if(predutd)
            {
                WRITEMEM32_LE(&predutd->utd_Link, READMEM32_LE(&utd->utd_Self)|UHCI_DFS);
                predutd->utd_Succ = (struct UhciXX *) utd;
                //utd->utd_Pred = (struct UhciXX *) predutd;
            } else {
                uqh->uqh_FirstTD = utd;
                uqh->uqh_Element = utd->utd_Self;
                //utd->utd_Pred = NULL;
            }
            //utd->utd_QueueHead = uqh;
            len = ioreq->iouh_Length - actual;
            if(len > ioreq->iouh_MaxPktSize)
            {
                len = ioreq->iouh_MaxPktSize;
            }

            WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&utd->utd_Token, token|(((len-1) & 0x7ff)<<UTTS_TRANSLENGTH));
            WRITEMEM32_LE(&utd->utd_BufferPtr, phyaddr);
            phyaddr += len;
            actual += len;
            predutd = utd;
            token ^= UTTF_DATA1; // toggle bit
        } while((actual < ioreq->iouh_Length) && (actual - ioreq->iouh_Actual < UHCI_TD_INT_LIMIT));

        if(!utd)
        {
            // not at least one data TD? try again later
            uhciFreeQH(hc, uqh);
            break;
        }

        uqh->uqh_Actual = actual - ioreq->iouh_Actual;
        // set toggle for next batch / succesful transfer
        unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? TRUE : FALSE;
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 1
            KPRINTF(1, ("NewData1\n"));
        } else {
            KPRINTF(1, ("NewData0\n"));
        }
        ctrlstatus |= UTCF_READYINTEN;
        WRITEMEM32_LE(&predutd->utd_CtrlStatus, ctrlstatus);
        CONSTWRITEMEM32_LE(&utd->utd_Link, UHCI_TERMINATE);
        utd->utd_Succ = NULL;
        //uqh->uqh_LastTD = utd;

        if(ioreq->iouh_Interval >= 255)
        {
            intuqh = hc->hc_UhciIntQH[8]; // 256ms interval
        } else {
            cnt = 0;
            do
            {
                intuqh = hc->hc_UhciIntQH[cnt++];
            } while(ioreq->iouh_Interval >= (1<<cnt));
            KPRINTF(1, ("Scheduled at level %ld\n", cnt));
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = uqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the right IntQH)
        uqh->uqh_Succ = intuqh->uqh_Succ;
        uqh->uqh_Link = uqh->uqh_Succ->uxx_Self;
        SYNC;

        uqh->uqh_Pred = (struct UhciXX *) intuqh;
        uqh->uqh_Succ->uxx_Pred = (struct UhciXX *) uqh;
        intuqh->uqh_Succ = (struct UhciXX *) uqh;
        intuqh->uqh_Link = uqh->uqh_Self;
        SYNC;
        Enable();

        uhciUpdateIntTree(hc);

        ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    }
}

void uhciScheduleBulkTDs(struct PCIController *hc) {

    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct UhciQH *uqh;
    struct UhciTD *utd;
    struct UhciTD *predutd;
    ULONG actual;
    ULONG ctrlstatus;
    ULONG token;
    ULONG len;
    ULONG phyaddr;
    BOOL forcezero;

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

        uqh = uhciAllocQH(hc);
        if(!uqh)
        {
            break;
        }

        uqh->uqh_IOReq = ioreq;

        // fill setup td
        ctrlstatus = UTCF_ACTIVE|UTCF_1ERRORLIMIT|UTCF_SHORTPACKET;
        token = (ioreq->iouh_DevAddr<<UTTS_DEVADDR)|(ioreq->iouh_Endpoint<<UTTS_ENDPOINT);
        token |= (ioreq->iouh_Dir == UHDIR_IN) ? PID_IN : PID_OUT;
        predutd = NULL;
        actual = ioreq->iouh_Actual;

        // Get a MEMF_31BIT bounce buffer
        uqh->uqh_DataBuffer = usbGetBuffer(&(((UBYTE *) ioreq->iouh_Data)[ioreq->iouh_Actual]), ioreq->iouh_Length - actual, ioreq->iouh_Dir);
        phyaddr = (IPTR)pciGetPhysical(hc, uqh->uqh_DataBuffer);
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 1
            token |= UTTF_DATA1;
        }
        do
        {
            utd = uhciAllocTD(hc);
            if(!utd)
            {
                break;
            }
            forcezero = FALSE;
            if(predutd)
            {
                WRITEMEM32_LE(&predutd->utd_Link, READMEM32_LE(&utd->utd_Self)|UHCI_DFS);
                predutd->utd_Succ = (struct UhciXX *) utd;
                //utd->utd_Pred = (struct UhciXX *) predutd;
            } else {
                uqh->uqh_FirstTD = utd;
                uqh->uqh_Element = utd->utd_Self;
                //utd->utd_Pred = NULL;
            }
            //utd->utd_QueueHead = uqh;
            len    = ioreq->iouh_Length - actual;
            if(len > ioreq->iouh_MaxPktSize)
            {
                len = ioreq->iouh_MaxPktSize;
            }
            WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&utd->utd_Token, token|(((len-1) & 0x7ff)<<UTTS_TRANSLENGTH));
            WRITEMEM32_LE(&utd->utd_BufferPtr, phyaddr);
            phyaddr += len;
            actual += len;
            predutd = utd;
            token ^= UTTF_DATA1; // toggle bit
            if((actual == ioreq->iouh_Length) && len)
            {
                if((ioreq->iouh_Flags & UHFF_NOSHORTPKT) || (ioreq->iouh_Dir == UHDIR_IN) || (actual % ioreq->iouh_MaxPktSize))
                {
                    // no last zero byte packet
                    break;
                } else {
                    // avoid rare case that the zero byte packet is reached on TD_BULK_LIMIT
                    forcezero = TRUE;
                }
            }
        } while(forcezero || (len && (actual <= ioreq->iouh_Length) && (actual - ioreq->iouh_Actual < UHCI_TD_BULK_LIMIT)));

        if(!utd)
        {
            // not at least one data TD? try again later
            uhciFreeQH(hc, uqh);
            break;
        }
        uqh->uqh_Actual = actual - ioreq->iouh_Actual;
        // set toggle for next batch / succesful transfer
        unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? TRUE : FALSE;

        ctrlstatus |= UTCF_READYINTEN;
        WRITEMEM32_LE(&predutd->utd_CtrlStatus, ctrlstatus);
        CONSTWRITEMEM32_LE(&utd->utd_Link, UHCI_TERMINATE);
        utd->utd_Succ = NULL;
        //uqh->uqh_LastTD = utd;

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = uqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the BulkQH)
        uqh->uqh_Succ = hc->hc_UhciBulkQH->uqh_Succ;
        uqh->uqh_Link = uqh->uqh_Succ->uxx_Self;
        SYNC;

        uqh->uqh_Pred = (struct UhciXX *) hc->hc_UhciBulkQH;
        uqh->uqh_Succ->uxx_Pred = (struct UhciXX *) uqh;
        hc->hc_UhciBulkQH->uqh_Succ = (struct UhciXX *) uqh;
        hc->hc_UhciBulkQH->uqh_Link = uqh->uqh_Self;
        SYNC;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }
}

void uhciUpdateFrameCounter(struct PCIController *hc) {

    UWORD framecnt;
    Disable();
    framecnt = READIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT) & 0x07ff;
    if(framecnt < (hc->hc_FrameCounter & 0x07ff))
    {
        hc->hc_FrameCounter |= 0x07ff;
        hc->hc_FrameCounter++;
        hc->hc_FrameCounter |= framecnt;
        KPRINTF(10, ("Frame Counter Rollover %ld\n", hc->hc_FrameCounter));
    } else {
        hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xfffff800)|framecnt;
    }
    Enable();
}

void uhciCompleteInt(struct PCIController *hc) {

    KPRINTF(1, ("CompleteInt!\n"));
    uhciUpdateFrameCounter(hc);

    /* **************** PROCESS DONE TRANSFERS **************** */

    uhciCheckPortStatusChange(hc);
    uhwCheckRootHubChanges(hc->hc_Unit);

    uhciHandleFinishedTDs(hc);

    if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        uhciScheduleCtrlTDs(hc);
    }

    if(hc->hc_IntXFerQueue.lh_Head->ln_Succ)
    {
        uhciScheduleIntTDs(hc);
    }

    if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
    {
        uhciScheduleBulkTDs(hc);
    }

    KPRINTF(1, ("CompleteDone\n"));
}

void uhciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw) {

    struct PCIController *hc = (struct PCIController *) irq->h_Data;
    struct PCIDevice *base = hc->hc_Device;
    UWORD intr;

    //KPRINTF(10, ("pciUhciInt()\n"));
    intr = READIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS);
    if(intr & (UHSF_USBINT|UHSF_USBERRORINT|UHSF_RESUMEDTX|UHSF_HCSYSERROR|UHSF_HCPROCERROR|UHSF_HCHALTED))
    {
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS, intr);
        //KPRINTF(1, ("INT=%04lx\n", intr));
        if(intr & (UHSF_HCSYSERROR|UHSF_HCPROCERROR|UHSF_HCHALTED))
        {
            KPRINTF(200, ("Host ERROR!\n"));
            WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET|UHCF_GLOBALRESET|UHCF_MAXPACKET64|UHCF_CONFIGURE);
            //WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);
        }
        if (!(hc->hc_Flags & HCF_ONLINE))
        {
            return;
        }
        if(intr & (UHSF_USBINT|UHSF_USBERRORINT))
        {
            SureCause(base, &hc->hc_CompleteInt);
        }
    }
}

BOOL uhciInit(struct PCIController *hc, struct PCIUnit *hu) {

    struct PCIDevice *hd = hu->hu_Device;

    struct UhciQH *uqh;
    struct UhciQH *preduqh;
    struct UhciTD *utd;
    ULONG *tabptr;
    UBYTE *memptr;
    ULONG bitcnt;

    ULONG cnt;

    struct TagItem pciActivateIO[] =
    {
            { aHidd_PCIDevice_isIO,     TRUE },
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

    hc->hc_NumPorts = 2; // UHCI always uses 2 ports per controller
    KPRINTF(20, ("Found UHCI Controller %08lx FuncNum=%ld with %ld ports\n", hc->hc_PCIDeviceObject, hc->hc_FunctionNum, hc->hc_NumPorts));
    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "UHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (void (*)(void)) &uhciCompleteInt;

    hc->hc_PCIMemSize = sizeof(ULONG) * UHCI_FRAMELIST_SIZE + UHCI_FRAMELIST_ALIGNMENT + 1;
    hc->hc_PCIMemSize += sizeof(struct UhciQH) * UHCI_QH_POOLSIZE;
    hc->hc_PCIMemSize += sizeof(struct UhciTD) * UHCI_TD_POOLSIZE;

    memptr = HIDD_PCIDriver_AllocPCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMemSize);
    /* memptr will be in the MEMF_31BIT type, therefore
     * we know that it's *physical address* will be 32 bits or
     * less, which is required for UHCI operation
     */
    hc->hc_PCIMem = (APTR) memptr;
    if(memptr) {

        // PhysicalAddress - VirtualAdjust = VirtualAddress
        // VirtualAddress  + VirtualAdjust = PhysicalAddress
        hc->hc_PCIVirtualAdjust = (IPTR)pciGetPhysical(hc, memptr) - (IPTR)memptr;
        KPRINTF(10, ("VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust));

        // align memory
        memptr = (UBYTE *) ((((IPTR) hc->hc_PCIMem) + UHCI_FRAMELIST_ALIGNMENT) & (~UHCI_FRAMELIST_ALIGNMENT));
        hc->hc_UhciFrameList = (ULONG *) memptr;
        KPRINTF(10, ("FrameListBase 0x%08lx\n", hc->hc_UhciFrameList));
        memptr += sizeof(APTR) * UHCI_FRAMELIST_SIZE;

        // build up QH pool
        // Again, all the UQHs are in the MEMF_31BIT hc_PCIMem pool,
        // so we can safely treat their physical addresses as 32 bit pointers
        uqh = (struct UhciQH *) memptr;
        hc->hc_UhciQHPool = uqh;
        cnt = UHCI_QH_POOLSIZE - 1;
        do {
            // minimal initalization
            uqh->uqh_Succ = (struct UhciXX *) (uqh + 1);
            WRITEMEM32_LE(&uqh->uqh_Self, (ULONG) ((IPTR)(&uqh->uqh_Link) + hc->hc_PCIVirtualAdjust + UHCI_QHSELECT));
            uqh++;
        } while(--cnt);
        uqh->uqh_Succ = NULL;
        WRITEMEM32_LE(&uqh->uqh_Self, (ULONG) ((IPTR)(&uqh->uqh_Link) + hc->hc_PCIVirtualAdjust + UHCI_QHSELECT));
        memptr += sizeof(struct UhciQH) * UHCI_QH_POOLSIZE;

        // build up TD pool
        // Again, all the UTDs are in the MEMF_31BIT hc_PCIMem pool,
        // so we can safely treat their physical addresses as 32 bit pointers
        utd = (struct UhciTD *) memptr;
        hc->hc_UhciTDPool = utd;
        cnt = UHCI_TD_POOLSIZE - 1;
        do {
            utd->utd_Succ = (struct UhciXX *) (utd + 1);
            WRITEMEM32_LE(&utd->utd_Self, (ULONG) ((IPTR)(&utd->utd_Link) + hc->hc_PCIVirtualAdjust + UHCI_TDSELECT));
            utd++;
        } while(--cnt);
        utd->utd_Succ = NULL;
        WRITEMEM32_LE(&utd->utd_Self, (ULONG) ((IPTR)(&utd->utd_Link) + hc->hc_PCIVirtualAdjust + UHCI_TDSELECT));
        memptr += sizeof(struct UhciTD) * UHCI_TD_POOLSIZE;

        // terminating QH
        hc->hc_UhciTermQH = preduqh = uqh = uhciAllocQH(hc);
        uqh->uqh_Succ = NULL;
        CONSTWRITEMEM32_LE(&uqh->uqh_Link, UHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);

        // dummy Bulk QH
        hc->hc_UhciBulkQH = uqh = uhciAllocQH(hc);
        uqh->uqh_Succ = (struct UhciXX *) preduqh;
        preduqh->uqh_Pred = (struct UhciXX *) uqh;
        uqh->uqh_Link = preduqh->uqh_Self; // link to terminating QH
        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
        preduqh = uqh;

        // dummy Ctrl QH
        hc->hc_UhciCtrlQH = uqh = uhciAllocQH(hc);
        uqh->uqh_Succ = (struct UhciXX *) preduqh;
        preduqh->uqh_Pred = (struct UhciXX *) uqh;
        uqh->uqh_Link = preduqh->uqh_Self; // link to Bulk QH
        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);

        // dummy ISO TD
        hc->hc_UhciIsoTD = utd = uhciAllocTD(hc);
        utd->utd_Succ = (struct UhciXX *) uqh;
        //utd->utd_Pred = NULL; // no certain linkage above this level
        uqh->uqh_Pred = (struct UhciXX *) utd;
        utd->utd_Link = uqh->uqh_Self; // link to Ctrl QH

        CONSTWRITEMEM32_LE(&utd->utd_CtrlStatus, 0);

        // 1 ms INT QH
        hc->hc_UhciIntQH[0] = uqh = uhciAllocQH(hc);
        uqh->uqh_Succ = (struct UhciXX *) utd;
        uqh->uqh_Pred = NULL; // who knows...
        //uqh->uqh_Link = utd->utd_Self; // link to ISO
        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
        preduqh = uqh;

        // make 9 levels of QH interrupts
        for(cnt = 1; cnt < 9; cnt++) {
            hc->hc_UhciIntQH[cnt] = uqh = uhciAllocQH(hc);
            uqh->uqh_Succ = (struct UhciXX *) preduqh;
            uqh->uqh_Pred = NULL; // who knows...
            //uqh->uqh_Link = preduqh->uqh_Self; // link to previous int level
            CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
            preduqh = uqh;
        }

        uhciUpdateIntTree(hc);

        // fill in framelist with IntQH entry points based on interval
        tabptr = hc->hc_UhciFrameList;
        for(cnt = 0; cnt < UHCI_FRAMELIST_SIZE; cnt++) {
            uqh = hc->hc_UhciIntQH[8];
            bitcnt = 0;
            do {
                if(cnt & (1UL<<bitcnt)) {
                    uqh = hc->hc_UhciIntQH[bitcnt];
                    break;
                }
            } while(++bitcnt < 9);
            *tabptr++ = uqh->uqh_Self;
        }

        // this will cause more PCI memory access, but faster USB transfers as well
        //WRITEMEM32_LE(&hc->hc_UhciTermQH->uqh_Link, AROS_LONG2LE(hc->hc_UhciBulkQH->uqh_Self));

        // time to initialize hardware...
        OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base4, (IPTR *) &hc->hc_RegBase);
        hc->hc_RegBase = (APTR) (((IPTR) hc->hc_RegBase) & (~0xf));
        KPRINTF(10, ("RegBase = 0x%08lx\n", hc->hc_RegBase));
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateIO);

        // disable BIOS legacy support
        KPRINTF(10, ("Turning off BIOS legacy support (old value=%04lx)\n", PCIXReadConfigWord(hc, UHCI_USBLEGSUP)));
        PCIXWriteConfigWord(hc, UHCI_USBLEGSUP, 0x8f00);

        KPRINTF(10, ("Resetting UHCI HC\n"));
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_GLOBALRESET);
        uhwDelayMS(15, hu);

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivateBusmaster); // no busmaster yet

        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET);
        cnt = 100;
        do {
            uhwDelayMS(10, hu);
            if(!(READIO16_LE(hc->hc_RegBase, UHCI_USBCMD) & UHCF_HCRESET)) {
                break;
            }
        } while(--cnt);

        if(cnt == 0) {
            KPRINTF(20, ("Reset Timeout!\n"));
            WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET);
            uhwDelayMS(15, hu);
        } else {
            KPRINTF(20, ("Reset finished after %ld ticks\n", 100-cnt));
        }

        // stop controller and disable all interrupts first
        KPRINTF(10, ("Stopping controller and enabling busmaster\n"));
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateBusmaster); // enable busmaster

        // Fix for VIA Babble problem
        cnt = PCIXReadConfigByte(hc, 0x40);
        if(!(cnt & 0x40)) {
            KPRINTF(20, ("Applying VIA Babble workaround\n"));
            PCIXWriteConfigByte(hc, 0x40, cnt|0x40);
        }

        KPRINTF(10, ("Configuring UHCI HC\n"));
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE);

        WRITEIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT, 0);

        /* hc->hc_UhciFrameList points to a portion of hc->hc_PciMem,
         * which we know is 32 bit
         */
        WRITEIO32_LE(hc->hc_RegBase, UHCI_FRAMELISTADDR, (ULONG)(IPTR)pciGetPhysical(hc, hc->hc_UhciFrameList));

        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS, UHIF_TIMEOUTCRC|UHIF_INTONCOMPLETE|UHIF_SHORTPACKET);

        // install reset handler
        hc->hc_ResetInt.is_Code = UhciResetHandler;
        hc->hc_ResetInt.is_Data = hc;
        AddResetCallback(&hc->hc_ResetInt);

        // add interrupt
        hc->hc_PCIIntHandler.h_Node.ln_Name = "UHCI PCI (pciusb.device)";
        hc->hc_PCIIntHandler.h_Node.ln_Pri = 5;
        hc->hc_PCIIntHandler.h_Code = uhciIntCode;
        hc->hc_PCIIntHandler.h_Data = hc;
        HIDD_IRQ_AddHandler(hd->hd_IRQHidd, &hc->hc_PCIIntHandler, hc->hc_PCIIntLine);

        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, UHIF_TIMEOUTCRC|UHIF_INTONCOMPLETE|UHIF_SHORTPACKET);

        // clear all port bits (both ports)
        WRITEIO32_LE(hc->hc_RegBase, UHCI_PORT1STSCTRL, 0);

        // enable PIRQ
        KPRINTF(10, ("Enabling PIRQ (old value=%04lx)\n", PCIXReadConfigWord(hc, UHCI_USBLEGSUP)));
        PCIXWriteConfigWord(hc, UHCI_USBLEGSUP, 0x2000);

        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE|UHCF_RUNSTOP);
        SYNC;

        KPRINTF(20, ("HW Init done\n"));

        KPRINTF(10, ("HW Regs USBCMD=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_USBCMD)));
        KPRINTF(10, ("HW Regs USBSTS=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS)));
        KPRINTF(10, ("HW Regs FRAMECOUNT=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT)));

        KPRINTF(20, ("uhciInit returns TRUE...\n"));
        return TRUE;
    }

    /*
        FIXME: What would the appropriate debug level be?
    */
    KPRINTF(1000, ("uhciInit returns FALSE...\n"));
    return FALSE;
}

void uhciFree(struct PCIController *hc, struct PCIUnit *hu) {

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        switch(hc->hc_HCIType)
        {
            case HCITYPE_UHCI:
            {
                KPRINTF(20, ("Shutting down UHCI %08lx\n", hc));
                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);
                // disable PIRQ
                PCIXWriteConfigWord(hc, UHCI_USBLEGSUP, 0);
                // disable all ports
                WRITEIO32_LE(hc->hc_RegBase, UHCI_PORT1STSCTRL, 0);
                uhwDelayMS(50, hu);
                //WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE);
                //uhwDelayMS(50, hu);
                KPRINTF(20, ("Stopping UHCI %08lx\n", hc));
                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
                SYNC;

                //KPRINTF(20, ("Reset done UHCI %08lx\n", hc));
                uhwDelayMS(10, hu);

                KPRINTF(20, ("Resetting UHCI %08lx\n", hc));
                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET);
                SYNC;

                uhwDelayMS(50, hu);
                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
                SYNC;

                KPRINTF(20, ("Shutting down UHCI done.\n"));
                break;
            }
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

}

