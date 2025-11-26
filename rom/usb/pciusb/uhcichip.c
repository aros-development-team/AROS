/*
    Copyright (C) 2010-2025, The AROS Development Team. All rights reserved
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <utility/hooks.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "uhciproto.h"

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

static AROS_INTH1(UhciResetHandler, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    // stop controller and disable all interrupts
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);

    return FALSE;

    AROS_INTFUNC_EXIT
}

void uhciFreeQContext(struct PCIController *hc, struct UhciQH *uqh)
{

    struct UhciTD *utd = NULL;
    struct UhciTD *nextutd;

    pciusbDebug("UHCI", "Unlinking QContext %08lx\n", uqh);
    // unlink from schedule
    uqh->uqh_Pred->uxx_Link = uqh->uqh_Succ->uxx_Self;
    SYNC;

    uqh->uqh_Succ->uxx_Pred = uqh->uqh_Pred;
    uqh->uqh_Pred->uxx_Succ = uqh->uqh_Succ;
    SYNC;

    nextutd = uqh->uqh_FirstTD;
    while(nextutd) {
        pciusbDebug("UHCI", "UHCI: FreeTD %08lx\n", nextutd);
        utd = nextutd;
        nextutd = (struct UhciTD *) utd->utd_Succ;
        uhciFreeTD(hc, utd);
    }
    uhciFreeQH(hc, uqh);
}

void uhciUpdateIntTree(struct PCIController *hc)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    struct UhciXX *uxx;
    struct UhciXX *preduxx;
    struct UhciXX *lastuseduxx;
    UWORD cnt;

    // optimize linkage between queue heads
    preduxx = lastuseduxx = (struct UhciXX *) uhcihcp->uhc_UhciCtrlQH; //uhcihcp->uhc_UhciIsoTD;
    for(cnt = 0; cnt < 9; cnt++) {
        uxx = (struct UhciXX *) uhcihcp->uhc_UhciIntQH[cnt];
        if(uxx->uxx_Succ != preduxx) {
            lastuseduxx = uxx->uxx_Succ;
        }
        uxx->uxx_Link = lastuseduxx->uxx_Self;
        preduxx = uxx;
    }
}

static void uhciInsertIsoPTD(struct PCIController *hc, struct PTDNode *ptd, ULONG slot)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    struct PTDNode *head = uhcihcp->uhc_IsoHead[slot];
    struct PTDNode *tail = uhcihcp->uhc_IsoTail[slot];
    struct UhciTD *utd = (struct UhciTD *)ptd->ptd_Descriptor;

    ptd->ptd_NextPTD = NULL;
    if(!head) {
        ptd->ptd_NextPhys = READMEM32_LE(&uhcihcp->uhc_UhciFrameList[slot]);
        WRITEMEM32_LE(&utd->utd_Link, ptd->ptd_NextPhys);
        uhcihcp->uhc_IsoHead[slot] = uhcihcp->uhc_IsoTail[slot] = ptd;
        WRITEMEM32_LE(&uhcihcp->uhc_UhciFrameList[slot], ptd->ptd_Phys);
        CacheClearE(&uhcihcp->uhc_UhciFrameList[slot], sizeof(ULONG), CACRF_ClearD);
    } else {
        struct UhciTD *tailtd = (struct UhciTD *)tail->ptd_Descriptor;
        ULONG nextphys = READMEM32_LE(&tailtd->utd_Link);

        ptd->ptd_NextPhys = head->ptd_NextPhys;
        WRITEMEM32_LE(&utd->utd_Link, nextphys);
        WRITEMEM32_LE(&tailtd->utd_Link, ptd->ptd_Phys);
        CacheClearE(tailtd, sizeof(*tailtd), CACRF_ClearD);
        tail->ptd_NextPTD = ptd;
        uhcihcp->uhc_IsoTail[slot] = ptd;
    }

    CacheClearE(utd, sizeof(*utd), CACRF_ClearD);
}

static void uhciUnlinkIsoPTD(struct PCIController *hc, struct PTDNode *ptd)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    ULONG slot = ptd->ptd_FrameIdx;
    struct PTDNode *prev = NULL;
    struct PTDNode *head = uhcihcp->uhc_IsoHead[slot];

    while(head && head != ptd) {
        prev = head;
        head = head->ptd_NextPTD;
    }

    if(!head)
        return;

    struct PTDNode *next = head->ptd_NextPTD;

    if(prev) {
        struct UhciTD *prevtd = (struct UhciTD *)prev->ptd_Descriptor;
        WRITEMEM32_LE(&prevtd->utd_Link, next ? next->ptd_Phys : head->ptd_NextPhys);
        CacheClearE(prevtd, sizeof(*prevtd), CACRF_ClearD);
        prev->ptd_NextPTD = next;
    } else {
        if(next)
            next->ptd_NextPhys = head->ptd_NextPhys;

        WRITEMEM32_LE(&uhcihcp->uhc_UhciFrameList[slot], next ? next->ptd_Phys : head->ptd_NextPhys);
        CacheClearE(&uhcihcp->uhc_UhciFrameList[slot], sizeof(ULONG), CACRF_ClearD);
        uhcihcp->uhc_IsoHead[slot] = next;
    }

    if(!next)
        uhcihcp->uhc_IsoTail[slot] = prev;

    head->ptd_NextPTD = NULL;
}

void uhciCheckPortStatusChange(struct PCIController *hc)
{

    struct PCIUnit *unit = hc->hc_Unit;
    UWORD oldval;
    UWORD hciport;

    // check for port status change for UHCI and frame rollovers

    for(hciport = 0; hciport < 2; hciport++) {
        UWORD portreg;
        UWORD idx = hc->hc_PortNum[hciport];
        // don't pay attention to UHCI port changes when pwned by EHCI
        if(unit->hu_PortOwner[idx] == HCITYPE_UHCI) {
            portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
            oldval = READIO16_LE(hc->hc_RegBase, portreg);
            if(oldval & UHPF_ENABLECHANGE) {
                pciusbDebug("UHCI", "Port %ld (%ld) Enable changed\n", idx, hciport);
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
            }
            if(oldval & UHPF_CONNECTCHANGE) {
                pciusbDebug("UHCI", "Port %ld (%ld) Connect changed\n", idx, hciport);
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                if(!(oldval & UHPF_PORTCONNECTED)) {
                    if(unit->hu_PortMap20[idx]) {
                        pciusbDebug("UHCI", "UHCI: Transferring Port %ld back to EHCI\n", idx);
                        unit->hu_PortOwner[idx] = HCITYPE_EHCI;
                    }
                }
            }
            if(oldval & UHPF_RESUMEDTX) {
                pciusbDebug("UHCI", "Port %ld (%ld) Resume changed\n", idx, hciport);
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                oldval &= ~UHPF_RESUMEDTX;
            }
            if(hc->hc_PortChangeMap[hciport]) {
                unit->hu_RootPortChanges |= 1UL<<(idx+1);
                /*KPRINTF(10, ("Port %ld (%ld) contributes %04lx to portmap %04lx\n",
                             idx, hciport, hc->hc_PortChangeMap[hciport], unit->hu_RootPortChanges));*/
            }
            WRITEIO16_LE(hc->hc_RegBase, portreg, oldval);
        }
    }
}

void uhciHandleFinishedTDs(struct PCIController *hc)
{

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

    pciusbDebug("UHCI", "UHCI: Checking for work done...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ)) {
        uqh = (struct UhciQH *) ioreq->iouh_DriverPrivate1;
        if(uqh) {
            pciusbDebug("UHCI", "UHCI: Examining IOReq=%08lx with UQH=%08lx\n", ioreq, uqh);
            linkelem = READMEM32_LE(&uqh->uqh_Element);
            inspect = 0;
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            if(linkelem & UHCI_TERMINATE) {
                pciusbDebug("UHCI", "UHCI: UQH terminated %08lx\n", linkelem);
                inspect = 2;
            } else {
                utd = (struct UhciTD *) ((linkelem & UHCI_PTRMASK) - hc->hc_PCIVirtualAdjust - UHCI_STRUCTURE_OFFSET); // struct UhciTD starts 16/32 bytes before physical TD depending on architecture
                ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                nextutd = (struct UhciTD *)utd->utd_Succ;
                if(!(ctrlstatus & UTCF_ACTIVE) && nextutd) {
                    /* OK, it's not active. Does it look like it's done? Code copied from below.
                       If not done, check the next TD too. */
                    if(ctrlstatus & (UTSF_BABBLE|UTSF_STALLED|UTSF_CRCTIMEOUT|UTSF_DATABUFFERERR|UTSF_BITSTUFFERR)) {
                        /*
                            Babble condition can only occur on the last data packet (or on the first if only one data packet is in the queue)
                            When UHCI encounters a babble condition it will halt immediately,
                            we can therefore just accept the data that has come through and resume as if we got interrupt on completition (IOC).

                            THEORETICAL: Possible fix for VIA babble bug
                            VIA chipset also halt the entire controller and sets the controller on stopped state.
                            We can resume the controller by changing the status bits in the queue so that the queue looks like it has ended with a completition or
                            remove the entire queue and act like it succeeded.
                            As VIA stops the controller we can then write a new frame list current index to point to the next item and then set the run bit back on.
                        */
                        nextutd = 0;
                    } else {
                        token = READMEM32_LE(&utd->utd_Token);
                        len = (ctrlstatus & UTSM_ACTUALLENGTH) >> UTSS_ACTUALLENGTH;
                        if((len != (token & UTTM_TRANSLENGTH) >> UTTS_TRANSLENGTH)) {
                            nextutd = 0;
                        }
                    }
                    if(nextutd) {
                        nextctrlstatus = READMEM32_LE(&nextutd->utd_CtrlStatus);
                    }
                }
                /* Now, did the element link pointer change while we fetched the status for the pointed at TD?
                   If so, disregard the gathered information and assume still active. */
                if(READMEM32_LE(&uqh->uqh_Element) != linkelem) {
                    /* Oh well, probably still active */
                    pciusbDebug("UHCI", "UHCI: Link Element changed, still active.\n");
                } else if(!(ctrlstatus & UTCF_ACTIVE) && (nextutd == 0 || !(nextctrlstatus & UTCF_ACTIVE))) {
                    pciusbDebug("UHCI", "UHCI: CtrlStatus inactive %08lx\n", ctrlstatus);
                    inspect = 1;
                } else if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep])) {
                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                    inspect = 1;
                }
            }
            fixsetupterm = FALSE;
            if(inspect) {
                APTR data = &((UBYTE *)ioreq->iouh_Data)[ioreq->iouh_Actual];
                shortpkt = FALSE;
                if(inspect < 2) { // if all went okay, don't traverse list, assume all bytes successfully transferred
                    utd = uqh->uqh_FirstTD;
                    actual = 0;
                    do {
                        ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                        if(ctrlstatus & UTCF_ACTIVE) {
                            pciusbError("UHCI", "Internal error! Still active?!\n");
                            if(ctrlstatus & UTSF_BABBLE) {
                                pciusbError("UHCI", "HOST CONTROLLER IS DEAD!!!\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                                WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET|UHCF_MAXPACKET64|UHCF_CONFIGURE|UHCF_RUNSTOP);
                                inspect = 0;
                                break;
                            }
                            break;
                        }
                        token = READMEM32_LE(&utd->utd_Token);
                        pciusbDebug("UHCI", "UHCI: TD=%08lx CS=%08lx Token=%08lx\n", utd, ctrlstatus, token);
                        if(ctrlstatus & (UTSF_BABBLE|UTSF_STALLED|UTSF_CRCTIMEOUT|UTSF_DATABUFFERERR|UTSF_BITSTUFFERR)) {
                            if(ctrlstatus & UTSF_BABBLE) {
                                pciusbError("UHCI", "Babble error %08lx/%08lx\n", ctrlstatus, token);
                                ctrlstatus &= ~(UTSF_BABBLE);
                                WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
                                SYNC;
                                inspect = 3;
                                break;
                            } else if(ctrlstatus & UTSF_CRCTIMEOUT) {
                                pciusbError("UHCI", "CRC/Timeout error IOReq=%08lx DIR=%ld\n", ioreq, ioreq->iouh_Dir);
                                if(ctrlstatus & UTSF_STALLED) {
                                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                                } else {
                                    ioreq->iouh_Req.io_Error = (ioreq->iouh_Dir == UHDIR_IN) ? UHIOERR_CRCERROR : UHIOERR_TIMEOUT;
                                }
                            } else if(ctrlstatus & UTSF_STALLED) {
                                pciusbError("UHCI", "STALLED!\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            } else if(ctrlstatus & UTSF_BITSTUFFERR) {
                                pciusbError("UHCI", "Bitstuff error\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                            } else if(ctrlstatus & UTSF_DATABUFFERERR) {
                                pciusbError("UHCI", "Databuffer error\n");
                                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            }
                            inspect = 0;
                            break;
                        }
                        if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]) && (ctrlstatus & UTSF_NAK)) {
                            ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            inspect = 0;
                        }

                        len = (ctrlstatus & UTSM_ACTUALLENGTH)>>UTSS_ACTUALLENGTH;
                        if((len != (token & UTTM_TRANSLENGTH)>>UTTS_TRANSLENGTH)) {
                            shortpkt = TRUE;
                        }
                        len = (len+1) & 0x7ff; // get real length
                        if((token & UTTM_PID)>>UTTS_PID != PID_SETUP) { // don't count setup packet
                            actual += len;
                            // due to the VIA babble bug workaround, actually more bytes can
                            // be received than requested, limit the actual value to the upper limit
                            if(actual > uqh->uqh_Actual) {
                                actual = uqh->uqh_Actual;
                            }
                        }
                        if(shortpkt) {
                            break;
                        }
                    } while((utd = (struct UhciTD *) utd->utd_Succ));
                    if(inspect == 3) {
                        /* bail out from babble */
                        actual = uqh->uqh_Actual;
                    }
                    if((actual < uqh->uqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS))) {
                        pciusbWarn("UHCI", "Short packet: %ld < %ld\n", actual, ioreq->iouh_Length);
                        ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                    }
                } else {
                    pciusbDebug("UHCI", "all %ld bytes transferred\n", uqh->uqh_Actual);
                    actual = uqh->uqh_Actual;
                }
                ioreq->iouh_Actual += actual;
                // due to the short packet, the terminal of a setup packet has not been sent. Please do so.
                if(shortpkt && (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)) {
                    fixsetupterm = TRUE;
                }
                // this is actually no short packet but result of the VIA babble fix
                if(shortpkt && (ioreq->iouh_Actual == ioreq->iouh_Length)) {
                    shortpkt = FALSE;
                }
                unit->hu_DevBusyReq[devadrep] = NULL;
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                if (uqh->uqh_DataBuffer) {
                    UWORD dir;
                    if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                        dir = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT;
                    else
                        dir = ioreq->iouh_Dir;

                    usbReleaseBuffer(uqh->uqh_DataBuffer, data, actual, dir);
                }
                if (uqh->uqh_SetupBuffer)
                    usbReleaseBuffer(uqh->uqh_SetupBuffer, &ioreq->iouh_SetupData, sizeof(ioreq->iouh_SetupData), UHDIR_OUT);
                uhciFreeQContext(hc, uqh);
                if(ioreq->iouh_Req.io_Command == UHCMD_INTXFER) {
                    updatetree = TRUE;
                }
                if(inspect) {
                    if(inspect < 2) { // otherwise, toggle will be right already
                        // use next data toggle bit based on last successful transaction
                        unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? FALSE : TRUE;
                    }
                    if((!shortpkt && (ioreq->iouh_Actual < ioreq->iouh_Length)) || fixsetupterm) {
                        // fragmented, do some more work
                        switch(ioreq->iouh_Req.io_Command) {
                        case UHCMD_CONTROLXFER:
                            pciusbDebug("UHCI", "Rescheduling CtrlTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length);
                            AddHead(&hc->hc_CtrlXFerQueue, (struct Node *) ioreq);
                            break;

                        case UHCMD_INTXFER:
                            pciusbDebug("UHCI", "Rescheduling IntTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length);
                            AddHead(&hc->hc_IntXFerQueue, (struct Node *) ioreq);
                            break;

                        case UHCMD_BULKXFER:
                            pciusbDebug("UHCI", "Rescheduling BulkTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length);
                            AddHead(&hc->hc_BulkXFerQueue, (struct Node *) ioreq);
                            break;

                        default:
                            pciusbError("UHCI", "Uhm, internal error, dunno where to queue this req\n");
                            ReplyMsg(&ioreq->iouh_Req.io_Message);
                        }
                    } else {
                        // check for sucessful clear feature and set address ctrl transfers
                        if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER) {
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
            pciusbDebug("UHCI", "UHCI: IOReq=%08lx has no UQH!\n", ioreq);
        }
        ioreq = nextioreq;
    }
    if(updatetree) {
        pciusbDebug("UHCI", "Updating Tree\n");
        uhciUpdateIntTree(hc);
    }
}

void uhciScheduleCtrlTDs(struct PCIController *hc)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
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
    pciusbDebug("UHCI", "Scheduling new CTRL transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        pciusbDebug("UHCI", "New CTRL transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("UHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        uqh = uhciAllocQH(hc);
        if(!uqh) {
            break;
        }

        setuputd = uhciAllocTD(hc);
        if(!setuputd) {
            uhciFreeQH(hc, uqh);
            break;
        }
        termutd = uhciAllocTD(hc);
        if(!termutd) {
            uhciFreeTD(hc, setuputd);
            uhciFreeQH(hc, uqh);
            break;
        }
        uqh->uqh_IOReq = ioreq;

        //termutd->utd_QueueHead = setuputd->utd_QueueHead = uqh;

        pciusbDebug("UHCI", "SetupTD=%08lx, TermTD=%08lx\n", setuputd, termutd);

        // fill setup td
        ctrlstatus = UTCF_ACTIVE|UTCF_3ERRORSLIMIT;
        if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
            KPRINTF(5, "*** LOW SPEED ***\n");
            ctrlstatus |= UTCF_LOWSPEED;
        }
        token = (ioreq->iouh_DevAddr<<UTTS_DEVADDR)|(ioreq->iouh_Endpoint<<UTTS_ENDPOINT);
        //setuputd->utd_Pred = NULL;
        if(ioreq->iouh_Actual) {
            // this is a continuation of a fragmented ctrl transfer!
            pciusbDebug("UHCI", "Continuing FRAGMENT at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length);
            cont = TRUE;
        } else {
            cont = FALSE;
            uqh->uqh_FirstTD = setuputd;
            uqh->uqh_Element = setuputd->utd_Self; // start of queue
            uqh->uqh_SetupBuffer = usbGetBuffer(&ioreq->iouh_SetupData, sizeof(ioreq->iouh_SetupData), UHDIR_OUT);
            WRITEMEM32_LE(&setuputd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&setuputd->utd_Token, (PID_SETUP<<UTTS_PID)|token|(7<<UTTS_TRANSLENGTH)|UTTF_DATA0);
            WRITEMEM32_LE(&setuputd->utd_BufferPtr, (ULONG) (IPTR) pciGetPhysical(hc, uqh->uqh_SetupBuffer));
        }

        token |= (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? PID_IN : PID_OUT;
        predutd = setuputd;
        actual = ioreq->iouh_Actual;

        if(ioreq->iouh_Length - actual) {
            ctrlstatus |= UTCF_SHORTPACKET;
            if(cont) {
                if(!unit->hu_DevDataToggle[devadrep]) {
                    // continue with data toggle 0
                    token |= UTTF_DATA1;
                }
            } else {
                ioreq->iouh_Actual=0;
            }
            uqh->uqh_DataBuffer = usbGetBuffer(&(((UBYTE *)ioreq->iouh_Data)[ioreq->iouh_Actual]), ioreq->iouh_Length - actual, (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT);
            phyaddr = (ULONG)(IPTR)pciGetPhysical(hc, uqh->uqh_DataBuffer);
            do {
                datautd = uhciAllocTD(hc);
                if(!datautd) {
                    break;
                }
                token ^= UTTF_DATA1; // toggle bit
                predutd->utd_Link = datautd->utd_Self;
                predutd->utd_Succ = (struct UhciXX *) datautd;
                //datautd->utd_Pred = (struct UhciXX *) predutd;
                //datautd->utd_QueueHead = uqh;
                len = ioreq->iouh_Length - actual;
                if(len > ioreq->iouh_MaxPktSize) {
                    len = ioreq->iouh_MaxPktSize;
                }
                WRITEMEM32_LE(&datautd->utd_CtrlStatus, ctrlstatus);
                WRITEMEM32_LE(&datautd->utd_Token, token|((len-1)<<UTTS_TRANSLENGTH)); // no masking need here as len is always >= 1
                WRITEMEM32_LE(&datautd->utd_BufferPtr, phyaddr);
                phyaddr += len;
                actual += len;
                predutd = datautd;
            } while((actual < ioreq->iouh_Length) && (actual - ioreq->iouh_Actual < UHCI_TD_CTRL_LIMIT));
            if(actual == ioreq->iouh_Actual) {
                // not at least one data TD? try again later
                uhciFreeTD(hc, setuputd);
                uhciFreeTD(hc, termutd);
                uhciFreeQH(hc, uqh);
                break;
            }
            if(cont) {
                // free Setup packet
                pciusbDebug("UHCI", "Freeing setup\n");
                uqh->uqh_FirstTD = (struct UhciTD *) setuputd->utd_Succ;
                //uqh->uqh_FirstTD->utd_Pred = NULL;
                uqh->uqh_Element = setuputd->utd_Succ->uxx_Self; // start of queue after setup packet
                uhciFreeTD(hc, setuputd);
                // set toggle for next batch
                unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? FALSE : TRUE;
            }
        } else {
            if(cont) {
                // free Setup packet, assign termination as first packet (no data)
                pciusbDebug("UHCI", "Freeing setup (term only)\n");
                uqh->uqh_FirstTD = (struct UhciTD *) termutd;
                uqh->uqh_Element = termutd->utd_Self; // start of queue after setup packet
                uhciFreeTD(hc, setuputd);
                predutd = NULL;
            }
        }
        uqh->uqh_Actual = actual - ioreq->iouh_Actual;
        ctrlstatus |= UTCF_READYINTEN;
        if(actual == ioreq->iouh_Length) {
            // TERM packet
            pciusbDebug("UHCI", "Activating TERM\n");
            token |= UTTF_DATA1;
            token ^= (PID_IN^PID_OUT)<<UTTS_PID;

            if(predutd) {
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
            pciusbDebug("UHCI", "Setup data phase fragmented\n");
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
        uqh->uqh_Succ = uhcihcp->uhc_UhciCtrlQH->uqh_Succ;
        uqh->uqh_Link = uqh->uqh_Succ->uxx_Self;
        SYNC;

        uqh->uqh_Pred = (struct UhciXX *) uhcihcp->uhc_UhciCtrlQH;
        uqh->uqh_Succ->uxx_Pred = (struct UhciXX *) uqh;
        uhcihcp->uhc_UhciCtrlQH->uqh_Succ = (struct UhciXX *) uqh;
        uhcihcp->uhc_UhciCtrlQH->uqh_Link = uqh->uqh_Self;
        SYNC;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    }
}

void uhciScheduleIntTDs(struct PCIController *hc)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
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
    pciusbDebug("UHCI", "Scheduling new INT transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbDebug("UHCI", "New INT transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("UHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        uqh = uhciAllocQH(hc);
        if(!uqh) {
            break;
        }

        uqh->uqh_IOReq = ioreq;

        ctrlstatus = UTCF_ACTIVE|UTCF_1ERRORLIMIT|UTCF_SHORTPACKET;
        if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
            KPRINTF(5, "*** LOW SPEED ***\n");
            ctrlstatus |= UTCF_LOWSPEED;
        }
        token = (ioreq->iouh_DevAddr<<UTTS_DEVADDR)|(ioreq->iouh_Endpoint<<UTTS_ENDPOINT);
        token |= (ioreq->iouh_Dir == UHDIR_IN) ? PID_IN : PID_OUT;
        predutd = NULL;
        actual = ioreq->iouh_Actual;
        uqh->uqh_DataBuffer = usbGetBuffer(&(((UBYTE *) ioreq->iouh_Data)[ioreq->iouh_Actual]), ioreq->iouh_Length - actual, ioreq->iouh_Dir);
        phyaddr = (ULONG) (IPTR) pciGetPhysical(hc, uqh->uqh_DataBuffer);
        if(unit->hu_DevDataToggle[devadrep]) {
            // continue with data toggle 1
            pciusbDebug("UHCI", "Data1\n");
            token |= UTTF_DATA1;
        } else {
            pciusbDebug("UHCI", "Data0\n");
        }
        do {
            utd = uhciAllocTD(hc);
            if(!utd) {
                break;
            }
            if(predutd) {
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
            if(len > ioreq->iouh_MaxPktSize) {
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

        if(!utd) {
            // not at least one data TD? try again later
            uhciFreeQH(hc, uqh);
            break;
        }

        uqh->uqh_Actual = actual - ioreq->iouh_Actual;
        // set toggle for next batch / succesful transfer
        unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? TRUE : FALSE;
        if(unit->hu_DevDataToggle[devadrep]) {
            // continue with data toggle 1
            pciusbDebug("UHCI", "NewData1\n");
        } else {
            pciusbDebug("UHCI", "NewData0\n");
        }
        ctrlstatus |= UTCF_READYINTEN;
        WRITEMEM32_LE(&predutd->utd_CtrlStatus, ctrlstatus);
        CONSTWRITEMEM32_LE(&utd->utd_Link, UHCI_TERMINATE);
        utd->utd_Succ = NULL;
        //uqh->uqh_LastTD = utd;

        if(ioreq->iouh_Interval >= 255) {
            intuqh = uhcihcp->uhc_UhciIntQH[8]; // 256ms interval
        } else {
            cnt = 0;
            do {
                intuqh = uhcihcp->uhc_UhciIntQH[cnt++];
            } while(ioreq->iouh_Interval >= (1<<cnt));
            pciusbDebug("UHCI", "Scheduled at level %ld\n", cnt);
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

void uhciScheduleBulkTDs(struct PCIController *hc)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
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
    pciusbDebug("UHCI", "Scheduling new BULK transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbDebug("UHCI", "New BULK transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("UHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        uqh = uhciAllocQH(hc);
        if(!uqh) {
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
        if(unit->hu_DevDataToggle[devadrep]) {
            // continue with data toggle 1
            token |= UTTF_DATA1;
        }
        do {
            utd = uhciAllocTD(hc);
            if(!utd) {
                break;
            }
            forcezero = FALSE;
            if(predutd) {
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
            if(len > ioreq->iouh_MaxPktSize) {
                len = ioreq->iouh_MaxPktSize;
            }
            WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&utd->utd_Token, token|(((len-1) & 0x7ff)<<UTTS_TRANSLENGTH));
            WRITEMEM32_LE(&utd->utd_BufferPtr, phyaddr);
            phyaddr += len;
            actual += len;
            predutd = utd;
            token ^= UTTF_DATA1; // toggle bit
            if((actual == ioreq->iouh_Length) && len) {
                if((ioreq->iouh_Flags & UHFF_NOSHORTPKT) || (ioreq->iouh_Dir == UHDIR_IN) || (actual % ioreq->iouh_MaxPktSize)) {
                    // no last zero byte packet
                    break;
                } else {
                    // avoid rare case that the zero byte packet is reached on TD_BULK_LIMIT
                    forcezero = TRUE;
                }
            }
        } while(forcezero || (len && (actual <= ioreq->iouh_Length) && (actual - ioreq->iouh_Actual < UHCI_TD_BULK_LIMIT)));

        if(!utd) {
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
        uqh->uqh_Succ = uhcihcp->uhc_UhciBulkQH->uqh_Succ;
        uqh->uqh_Link = uqh->uqh_Succ->uxx_Self;
        SYNC;

        uqh->uqh_Pred = (struct UhciXX *) uhcihcp->uhc_UhciBulkQH;
        uqh->uqh_Succ->uxx_Pred = (struct UhciXX *) uqh;
        uhcihcp->uhc_UhciBulkQH->uqh_Succ = (struct UhciXX *) uqh;
        uhcihcp->uhc_UhciBulkQH->uqh_Link = uqh->uqh_Self;
        SYNC;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }
}

void uhciUpdateFrameCounter(struct PCIController *hc)
{

    UWORD framecnt;
    Disable();
    framecnt = READIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT) & 0x07ff;
    if(framecnt < (hc->hc_FrameCounter & 0x07ff)) {
        hc->hc_FrameCounter |= 0x07ff;
        hc->hc_FrameCounter++;
        hc->hc_FrameCounter |= framecnt;
        pciusbDebug("UHCI", "Frame Counter Rollover %ld\n", hc->hc_FrameCounter);
    } else {
        hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xfffff800)|framecnt;
    }
    Enable();
}

static AROS_INTH1(uhciCompleteInt, struct PCIController *,hc)
{
    AROS_INTFUNC_INIT

    pciusbDebug("UHCI", "CompleteInt!\n");
    uhciUpdateFrameCounter(hc);

    /* **************** PROCESS DONE TRANSFERS **************** */

    uhciCheckPortStatusChange(hc);
    uhwCheckRootHubChanges(hc->hc_Unit);

    uhciHandleIsochTDs(hc);
    uhciHandleFinishedTDs(hc);

    if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ) {
        uhciScheduleCtrlTDs(hc);
    }

    if(hc->hc_IntXFerQueue.lh_Head->ln_Succ) {
        uhciScheduleIntTDs(hc);
    }

    if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ) {
        uhciScheduleBulkTDs(hc);
    }

    pciusbDebug("UHCI", "CompleteDone\n");

    return FALSE;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(uhciIntCode, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

#ifndef base
    struct PCIDevice *base = hc->hc_Device;
#endif
    UWORD intr;

    pciusbDebug("UHCI", "pciUhciInt()\n");
    intr = READIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS);
    if(intr & (UHSF_USBINT|UHSF_USBERRORINT|UHSF_RESUMEDTX|UHSF_HCSYSERROR|UHSF_HCPROCERROR|UHSF_HCHALTED)) {
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS, intr);
        pciusbDebug("UHCI", "INT=%04lx\n", intr);
        if(intr & (UHSF_HCSYSERROR|UHSF_HCPROCERROR|UHSF_HCHALTED)) {
            pciusbError("UHCI", "Host ERROR!\n");
            WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET|UHCF_GLOBALRESET|UHCF_MAXPACKET64|UHCF_CONFIGURE);
            //WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);
        }
        if (!(hc->hc_Flags & HCF_ONLINE)) {
            return FALSE;
        }
        if(intr & (UHSF_USBINT|UHSF_USBERRORINT)) {
            SureCause(base, &hc->hc_CompleteInt);
        }
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

BOOL uhciInit(struct PCIController *hc, struct PCIUnit *hu)
{
    struct UhciHCPrivate *uhcihcp;
    struct UhciQH *uqh;
    struct UhciQH *preduqh;
    struct UhciTD *utd;
    ULONG *tabptr;
    UBYTE *memptr;
    ULONG bitcnt;

    ULONG cnt;

    struct TagItem pciActivateIO[] = {
        { aHidd_PCIDevice_isIO,     TRUE },
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

    uhcihcp = AllocMem(sizeof(struct UhciHCPrivate), MEMF_CLEAR);
    if (!uhcihcp)
        return FALSE;

    hc->hc_CPrivate = uhcihcp;
    hc->hc_NumPorts = 2; // UHCI always uses 2 ports per controller
    pciusbDebug("UHCI", "Found UHCI Controller %08lx FuncNum=%ld with %ld ports\n", hc->hc_PCIDeviceObject, hc->hc_FunctionNum, hc->hc_NumPorts);
    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "UHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)uhciCompleteInt;

    hc->hc_PCIMem.me_Length = sizeof(ULONG) * UHCI_FRAMELIST_SIZE + UHCI_FRAMELIST_ALIGNMENT + 1;
    hc->hc_PCIMem.me_Length += sizeof(struct UhciQH) * UHCI_QH_POOLSIZE;
    hc->hc_PCIMem.me_Length += sizeof(struct UhciTD) * UHCI_TD_POOLSIZE;

    memptr = ALLOCPCIMEM(hc, hc->hc_PCIDriverObject, hc->hc_PCIMem.me_Length);
    /* memptr will be in the MEMF_31BIT type, therefore
     * we know that it's *physical address* will be 32 bits or
     * less, which is required for UHCI operation
     */
    hc->hc_PCIMem.me_Un.meu_Addr = (APTR) memptr;
    if(memptr) {

        // PhysicalAddress - VirtualAdjust = VirtualAddress
        // VirtualAddress  + VirtualAdjust = PhysicalAddress
        hc->hc_PCIVirtualAdjust = (IPTR)pciGetPhysical(hc, memptr) - (IPTR)memptr;
        pciusbDebug("UHCI", "VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust);

        // align memory
        memptr = (UBYTE *) ((((IPTR) hc->hc_PCIMem.me_Un.meu_Addr) + UHCI_FRAMELIST_ALIGNMENT) & (~UHCI_FRAMELIST_ALIGNMENT));
        uhcihcp->uhc_UhciFrameList = (ULONG *) memptr;
        pciusbDebug("UHCI", "FrameListBase 0x%08lx\n", uhcihcp->uhc_UhciFrameList);
        memptr += sizeof(APTR) * UHCI_FRAMELIST_SIZE;

        uhcihcp->uhc_IsoHead = AllocMem(sizeof(struct PTDNode *) * UHCI_FRAMELIST_SIZE, MEMF_CLEAR);
        uhcihcp->uhc_IsoTail = AllocMem(sizeof(struct PTDNode *) * UHCI_FRAMELIST_SIZE, MEMF_CLEAR);
        if(!uhcihcp->uhc_IsoHead || !uhcihcp->uhc_IsoTail) {
            if(uhcihcp->uhc_IsoHead)
                FreeMem(uhcihcp->uhc_IsoHead, sizeof(struct PTDNode *) * UHCI_FRAMELIST_SIZE);
            if(uhcihcp->uhc_IsoTail)
                FreeMem(uhcihcp->uhc_IsoTail, sizeof(struct PTDNode *) * UHCI_FRAMELIST_SIZE);
            goto init_fail;
        }

        // build up QH pool
        // Again, all the UQHs are in the MEMF_31BIT hc_PCIMem.me_Un.meu_Addr pool,
        // so we can safely treat their physical addresses as 32 bit pointers
        uqh = (struct UhciQH *) memptr;
        uhcihcp->uhc_UhciQHPool = uqh;
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
        // Again, all the UTDs are in the MEMF_31BIT hc_PCIMem.me_Un.meu_Addr pool,
        // so we can safely treat their physical addresses as 32 bit pointers
        utd = (struct UhciTD *) memptr;
        uhcihcp->uhc_UhciTDPool = utd;
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
        uhcihcp->uhc_UhciTermQH = preduqh = uqh = uhciAllocQH(hc);
        uqh->uqh_Succ = NULL;
        CONSTWRITEMEM32_LE(&uqh->uqh_Link, UHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);

        // dummy Bulk QH
        uhcihcp->uhc_UhciBulkQH = uqh = uhciAllocQH(hc);
        uqh->uqh_Succ = (struct UhciXX *) preduqh;
        preduqh->uqh_Pred = (struct UhciXX *) uqh;
        uqh->uqh_Link = preduqh->uqh_Self; // link to terminating QH
        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
        preduqh = uqh;

        // dummy Ctrl QH
        uhcihcp->uhc_UhciCtrlQH = uqh = uhciAllocQH(hc);
        uqh->uqh_Succ = (struct UhciXX *) preduqh;
        preduqh->uqh_Pred = (struct UhciXX *) uqh;
        uqh->uqh_Link = preduqh->uqh_Self; // link to Bulk QH
        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);

        // dummy ISO TD
        uhcihcp->uhc_UhciIsoTD = utd = uhciAllocTD(hc);
        utd->utd_Succ = (struct UhciXX *) uqh;
        //utd->utd_Pred = NULL; // no certain linkage above this level
        uqh->uqh_Pred = (struct UhciXX *) utd;
        utd->utd_Link = uqh->uqh_Self; // link to Ctrl QH

        CONSTWRITEMEM32_LE(&utd->utd_CtrlStatus, 0);

        // 1 ms INT QH
        uhcihcp->uhc_UhciIntQH[0] = uqh = uhciAllocQH(hc);
        uqh->uqh_Succ = (struct UhciXX *) utd;
        uqh->uqh_Pred = NULL; // who knows...
        //uqh->uqh_Link = utd->utd_Self; // link to ISO
        CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
        preduqh = uqh;

        // make 9 levels of QH interrupts
        for(cnt = 1; cnt < 9; cnt++) {
            uhcihcp->uhc_UhciIntQH[cnt] = uqh = uhciAllocQH(hc);
            uqh->uqh_Succ = (struct UhciXX *) preduqh;
            uqh->uqh_Pred = NULL; // who knows...
            //uqh->uqh_Link = preduqh->uqh_Self; // link to previous int level
            CONSTWRITEMEM32_LE(&uqh->uqh_Element, UHCI_TERMINATE);
            preduqh = uqh;
        }

        uhciUpdateIntTree(hc);

        // fill in framelist with IntQH entry points based on interval
        tabptr = uhcihcp->uhc_UhciFrameList;
        for(cnt = 0; cnt < UHCI_FRAMELIST_SIZE; cnt++) {
            uqh = uhcihcp->uhc_UhciIntQH[8];
            bitcnt = 0;
            do {
                if(cnt & (1UL<<bitcnt)) {
                    uqh = uhcihcp->uhc_UhciIntQH[bitcnt];
                    break;
                }
            } while(++bitcnt < 9);
            *tabptr++ = uqh->uqh_Self;
        }

        // this will cause more PCI memory access, but faster USB transfers as well
        //WRITEMEM32_LE(&uhcihcp->uhc_UhciTermQH->uqh_Link, AROS_LONG2LE(uhcihcp->uhc_UhciBulkQH->uqh_Self));

        // time to initialize hardware...
        OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base4, (IPTR *) &hc->hc_RegBase);
        hc->hc_RegBase = (APTR) (((IPTR) hc->hc_RegBase) & (~0xf));
        pciusbDebug("UHCI", "RegBase = 0x%08lx\n", hc->hc_RegBase);
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateIO);

        // disable BIOS legacy support
        pciusbDebug("UHCI", "Turning off BIOS legacy support (old value=%04lx)\n", READCONFIGWORD(hc, hc->hc_PCIDeviceObject,UHCI_USBLEGSUP));
        WRITECONFIGWORD(hc, hc->hc_PCIDeviceObject, UHCI_USBLEGSUP, 0x8f00);

        pciusbDebug("UHCI", "Resetting UHCI HC\n");
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
            pciusbDebug("UHCI", "Reset Timeout!\n");
            WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET);
            uhwDelayMS(15, hu);
        } else {
            pciusbDebug("UHCI", "Reset finished after %ld ticks\n", 100-cnt);
        }

        // stop controller and disable all interrupts first
        pciusbDebug("UHCI", "Stopping controller and enabling busmaster\n");
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateBusmaster); // enable busmaster

        if (hc->hc_Quirks & HCQ_UHCI_VIA_BABBLE) {
            cnt = READCONFIGBYTE(hc, hc->hc_PCIDeviceObject, 0x40);
            if(!(cnt & 0x40)) {
                pciusbDebug("UHCI", "Applying VIA Babble workaround\n");
                WRITECONFIGBYTE(hc, hc->hc_PCIDeviceObject, 0x40, cnt|0x40);
            }
        }

        pciusbDebug("UHCI", "Configuring UHCI HC\n");
        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE);

        WRITEIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT, 0);

        /* uhcihcp->uhc_UhciFrameList points to a portion of hc->hc_PciMem,
         * which we know is 32 bit
         */
        WRITEIO32_LE(hc->hc_RegBase, UHCI_FRAMELISTADDR, (ULONG)(IPTR)pciGetPhysical(hc, uhcihcp->uhc_UhciFrameList));

        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS, UHSF_USBINT | UHSF_USBERRORINT | UHSF_RESUMEDTX | UHSF_HCSYSERROR | UHSF_HCPROCERROR | UHSF_HCHALTED);

        // install reset handler
        hc->hc_ResetInt.is_Node.ln_Name = "UHCI PCI (pciusb.device)";
        hc->hc_ResetInt.is_Code = (VOID_FUNC)UhciResetHandler;
        hc->hc_ResetInt.is_Data = hc;
        AddResetCallback(&hc->hc_ResetInt);

        // add interrupt
        hc->hc_PCIIntHandler.is_Node.ln_Name = hc->hc_ResetInt.is_Node.ln_Name;
        hc->hc_PCIIntHandler.is_Node.ln_Pri = 5;
        hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
        hc->hc_PCIIntHandler.is_Code = (VOID_FUNC)uhciIntCode;
        hc->hc_PCIIntHandler.is_Data = hc;
        PCIXAddInterrupt(hc, &hc->hc_PCIIntHandler);

        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, UHIF_TIMEOUTCRC|UHIF_INTONCOMPLETE|UHIF_SHORTPACKET);

        // clear all port bits (both ports)
        WRITEIO32_LE(hc->hc_RegBase, UHCI_PORT1STSCTRL, 0);

        // enable PIRQ
        pciusbDebug("UHCI", "Enabling PIRQ (old value=%04lx)\n", READCONFIGWORD(hc, hc->hc_PCIDeviceObject,UHCI_USBLEGSUP));
        WRITECONFIGWORD(hc, hc->hc_PCIDeviceObject, UHCI_USBLEGSUP, 0x2000);

        WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE|UHCF_RUNSTOP);
        SYNC;

        pciusbDebug("UHCI", "HW Init done\n");

        pciusbDebug("UHCI", "HW Regs USBCMD=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_USBCMD));
        pciusbDebug("UHCI", "HW Regs USBSTS=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_USBSTATUS));
        pciusbDebug("UHCI", "HW Regs FRAMECOUNT=%04lx\n", READIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT));

        pciusbDebug("UHCI", "uhciInit returns TRUE...\n");
        return TRUE;
    }

init_fail:
    if(uhcihcp) {
        if(uhcihcp->uhc_IsoHead)
            FreeMem(uhcihcp->uhc_IsoHead, sizeof(struct PTDNode *) * UHCI_FRAMELIST_SIZE);
        if(uhcihcp->uhc_IsoTail)
            FreeMem(uhcihcp->uhc_IsoTail, sizeof(struct PTDNode *) * UHCI_FRAMELIST_SIZE);
    }

    if(hc->hc_PCIMem.me_Un.meu_Addr)
        FreeMem(hc->hc_PCIMem.me_Un.meu_Addr, hc->hc_PCIMem.me_Length);

    FreeMem(uhcihcp, sizeof(struct UhciHCPrivate));
    hc->hc_CPrivate = NULL;

    /*
        FIXME: What would the appropriate debug level be?
    */
    KPRINTF(1000, "uhciInit returns FALSE...\n");
    return FALSE;
}

void uhciFree(struct PCIController *hc, struct PCIUnit *hu)
{

    pciusbDebug("UHCI", "Shutting down UHCI %08lx\n", hc);
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);
    // disable PIRQ
    WRITECONFIGWORD(hc, hc->hc_PCIDeviceObject, UHCI_USBLEGSUP, 0);
    // disable all ports
    WRITEIO32_LE(hc->hc_RegBase, UHCI_PORT1STSCTRL, 0);
    uhwDelayMS(50, hu);
    //WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE);
    //uhwDelayMS(50, hu);
    pciusbDebug("UHCI", "Stopping UHCI %08lx\n", hc);
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
    SYNC;

    //KPRINTF(20, ("Reset done UHCI %08lx\n", hc));
    uhwDelayMS(10, hu);

    pciusbDebug("UHCI", "Resetting UHCI %08lx\n", hc);
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET);
    SYNC;

    uhwDelayMS(50, hu);
    WRITEIO16_LE(hc->hc_RegBase, UHCI_USBCMD, 0);
    SYNC;

    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    hc->hc_CPrivate = NULL;
    if (uhcihcp) {
        if(uhcihcp->uhc_IsoHead)
            FreeMem(uhcihcp->uhc_IsoHead, sizeof(struct PTDNode *) * UHCI_FRAMELIST_SIZE);
        if(uhcihcp->uhc_IsoTail)
            FreeMem(uhcihcp->uhc_IsoTail, sizeof(struct PTDNode *) * UHCI_FRAMELIST_SIZE);

        FreeMem(uhcihcp, sizeof(struct UhciHCPrivate));
    }

    pciusbDebug("UHCI", "Shutting down UHCI done.\n");
}

/* ** Root hub support functions ** */

BOOL uhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
    ULONG oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
    ULONG newval = oldval;
    ULONG cnt;
    BOOL cmdgood = FALSE;

    pciusbDebug("UHCI", "%s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval);

    switch(val) {
    /* case UFS_PORT_CONNECTION: not possible */
    case UFS_PORT_ENABLE:
        pciusbDebug("UHCI", "Enabling Port (%s)\n", newval & UHPF_PORTENABLE ? "already" : "ok");
        newval |= UHPF_PORTENABLE;
        cmdgood = TRUE;
        break;

    case UFS_PORT_SUSPEND:
        newval |= UHPF_PORTSUSPEND;
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND; // manually fake suspend change
        cmdgood = TRUE;
        break;

    /* case UFS_PORT_OVER_CURRENT: not possible */
    case UFS_PORT_RESET:
        pciusbDebug("UHCI", "Resetting Port (%s)\n", newval & UHPF_PORTRESET ? "already" : "ok");

        // this is an ugly blocking workaround to the inability of UHCI to clear reset automatically
        newval &= ~(UHPF_PORTSUSPEND|UHPF_PORTENABLE);
        newval |= UHPF_PORTRESET;
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
        uhwDelayMS(25, unit);
        newval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND|UHPF_PORTENABLE);
        pciusbDebug("UHCI", "Reset=%s\n", newval & UHPF_PORTRESET ? "GOOD" : "BAD!");
        // like windows does it
        newval &= ~UHPF_PORTRESET;
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
        uhwDelayMicro(50, unit);
        newval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
        pciusbDebug("UHCI", "Reset=%s\n", newval & UHPF_PORTRESET ? "BAD!" : "GOOD");
        newval &= ~(UHPF_PORTSUSPEND|UHPF_PORTRESET);
        newval |= UHPF_PORTENABLE;
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET|UPSF_PORT_ENABLE; // manually fake reset change

        cnt = 100;
        do {
            uhwDelayMS(1, unit);
            newval = READIO16_LE(hc->hc_RegBase, portreg);
        } while(--cnt && (!(newval & UHPF_PORTENABLE)));
        if(cnt) {
            pciusbDebug("UHCI", "Enabled after %ld ticks\n", 100-cnt);
        } else {
            pciusbWarn("UHCI", "Port refuses to be enabled!\n");
            *retval = UHIOERR_HOSTERROR;
            return TRUE;
        }
        // make enumeration possible
        unit->hu_DevControllers[0] = hc;
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        pciusbDebug("UHCI", "Powering Port\n");
        // ignore for UHCI, is always powered
        cmdgood = TRUE;
        break;

        /* case UFS_PORT_LOW_SPEED: not possible */
        /* case UFS_C_PORT_CONNECTION:
        case UFS_C_PORT_ENABLE:
        case UFS_C_PORT_SUSPEND:
        case UFS_C_PORT_OVER_CURRENT:
        case UFS_C_PORT_RESET: */
    }
    if(cmdgood) {
        pciusbDebug("UHCI", "Port %ld SET_FEATURE %04lx->%04lx\n", idx, oldval, newval);
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
    }
    return cmdgood;
}

BOOL uhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
    ULONG oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
    ULONG newval = oldval;
    BOOL cmdgood = FALSE;

    pciusbDebug("UHCI", "%s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval);

    switch(val) {
    case UFS_PORT_ENABLE:
        pciusbDebug("UHCI", "Disabling Port (%s)\n", newval & UHPF_PORTENABLE ? "ok" : "already");
        newval &= ~UHPF_PORTENABLE;
        cmdgood = TRUE;
        // disable enumeration
        unit->hu_DevControllers[0] = NULL;
        break;

    case UFS_PORT_SUSPEND:
        newval &= ~UHPF_PORTSUSPEND;
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER: // ignore for UHCI, there's no power control here
        pciusbDebug("UHCI", "Disabling Power\n");
        pciusbDebug("UHCI", "Disabling Port (%s)\n", newval & UHPF_PORTENABLE ? "ok" : "already");
        newval &= ~UHPF_PORTENABLE;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_CONNECTION:
        newval |= UHPF_CONNECTCHANGE; // clear-on-write!
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_ENABLE:
        newval |= UHPF_ENABLECHANGE; // clear-on-write!
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_SUSPEND: // ignore for UHCI, there's no bit indicating this
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND; // manually fake suspend change clearing
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_OVER_CURRENT: // ignore for UHCI, there's no bit indicating this
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT; // manually fake over current clearing
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_RESET: // ignore for UHCI, there's no bit indicating this
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET; // manually fake reset change clearing
        cmdgood = TRUE;
        break;
    }
    if(cmdgood) {
        pciusbDebug("UHCI", "Port %ld CLEAR_FEATURE %04lx->%04lx\n", idx, oldval, newval);
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
        if(hc->hc_PortChangeMap[hciport]) {
            unit->hu_RootPortChanges |= 1UL<<idx;
        } else {
            unit->hu_RootPortChanges &= ~(1UL<<idx);
        }
    }
    return cmdgood;
}

BOOL uhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval)
{
    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
    UWORD oldval = READIO16_LE(hc->hc_RegBase, portreg);

    pciusbDebug("UHCI", "%s(0x%p, 0x%p, %04x, %04x, 0x%p)\n", __func__, hc, mptr, hciport, idx, retval);

    *mptr = AROS_WORD2LE(UPSF_PORT_POWER);

    if(oldval & UHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
    if(oldval & UHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
    if(oldval & UHPF_LOWSPEED) *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
    if(oldval & UHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
    if(oldval & UHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

    pciusbDebug("UHCI", "Port %ld is %s\n", idx, oldval & UHPF_LOWSPEED ? "LOWSPEED" : "FULLSPEED");
    pciusbDebug("UHCI", "Port %ld Status %08lx\n", idx, *mptr);

    mptr++;
    if(oldval & UHPF_ENABLECHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
    }
    if(oldval & UHPF_CONNECTCHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
    }
    if(oldval & UHPF_RESUMEDTX) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
    }

    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
    WRITEIO16_LE(hc->hc_RegBase, portreg, oldval);

    pciusbDebug("UHCI", "Port %ld Change %08lx\n", idx, *mptr);

    return TRUE;
}

WORD uhciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;

    pciusbDebug("UHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !ptdcount)
        return UHIOERR_BADPARAMS;

    for(idx = 0; idx < ptdcount; idx++) {
        if(rtn->rtn_PTDs[idx]) {
            if(rtn->rtn_PTDs[idx]->ptd_Descriptor)
                uhciFreeTD(hc, (struct UhciTD *)rtn->rtn_PTDs[idx]->ptd_Descriptor);
            FreeMem(rtn->rtn_PTDs[idx], sizeof(struct PTDNode));
            rtn->rtn_PTDs[idx] = NULL;
        }
    }

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = AllocMem(sizeof(*ptd), MEMF_CLEAR);
        struct UhciTD *utd = ptd ? uhciAllocTD(hc) : NULL;

        if(!ptd || !utd) {
            if(ptd)
                FreeMem(ptd, sizeof(*ptd));

            for(ULONG freeidx = 0; freeidx < idx; freeidx++) {
                struct PTDNode *old = rtn->rtn_PTDs[freeidx];
                if(old->ptd_Descriptor)
                    uhciFreeTD(hc, (struct UhciTD *)old->ptd_Descriptor);
                FreeMem(old, sizeof(*old));
                rtn->rtn_PTDs[freeidx] = NULL;
            }
            return UHIOERR_OUTOFMEMORY;
        }

        ptd->ptd_Descriptor = utd;
        ptd->ptd_Phys = READMEM32_LE(&utd->utd_Self);
        rtn->rtn_PTDs[idx] = ptd;
    }

    rtn->rtn_NextPTD = 0;
    rtn->rtn_BounceBuffer = NULL;

    return RC_OK;
}

WORD uhciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbDebug("UHCI", "%s()\n", __func__);

    rtn->rtn_BufferReq.ubr_Length = rtn->rtn_IOReq.iouh_Length;
    rtn->rtn_BufferReq.ubr_Frame = (READIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT) + 1) & (UHCI_FRAMELIST_SIZE - 1);
    rtn->rtn_BufferReq.ubr_Flags = 0;

    return RC_OK;
}

void uhciHandleIsochTDs(struct PCIController *hc)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    struct RTIsoNode *rtn = (struct RTIsoNode *)hc->hc_RTIsoHandlers.mlh_Head;

    while(rtn->rtn_Node.mln_Succ) {
        struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
        UWORD idx;
        UWORD limit = rtn->rtn_PTDCount;

        if(!rtn->rtn_PTDs || !limit) {
            rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
            continue;
        }

        for(idx = 0; idx < limit; idx++) {
            struct PTDNode *ptd = rtn->rtn_PTDs[idx];
            struct UhciTD *utd;
            ULONG ctrl;
            ULONG actual;
            BOOL error = FALSE;

            if(!ptd || !(ptd->ptd_Flags & PTDF_ACTIVE))
                continue;

            utd = (struct UhciTD *)ptd->ptd_Descriptor;
            CacheClearE(utd, sizeof(*utd), CACRF_InvalidateD);
            ctrl = READMEM32_LE(&utd->utd_CtrlStatus);
            if(ctrl & UTCF_ACTIVE)
                continue;

            if(ctrl & (UTSF_BITSTUFFERR|UTSF_CRCTIMEOUT|UTSF_BABBLE|UTSF_DATABUFFERERR))
                error = TRUE;

            actual = ((ctrl & UTSM_ACTUALLENGTH) >> UTSS_ACTUALLENGTH) + 1;
            rtn->rtn_IOReq.iouh_Actual = actual;
            rtn->rtn_IOReq.iouh_Req.io_Error = error ? UHIOERR_HOSTERROR : UHIOERR_NO_ERROR;
            rtn->rtn_BufferReq.ubr_Length = actual;

            if(rtn->rtn_BounceBuffer &&
                    rtn->rtn_BounceBuffer != rtn->rtn_BufferReq.ubr_Buffer) {
                usbReleaseBuffer(rtn->rtn_BounceBuffer, rtn->rtn_BufferReq.ubr_Buffer,
                                 rtn->rtn_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
                rtn->rtn_BounceBuffer = NULL;
            }

            uhciUnlinkIsoPTD(hc, ptd);

            if(urti) {
                if(rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) {
                    if(urti->urti_InReqHook)
                        CallHookPkt(urti->urti_InReqHook, rtn, &rtn->rtn_BufferReq);
                    if(urti->urti_InDoneHook)
                        CallHookPkt(urti->urti_InDoneHook, rtn, &rtn->rtn_BufferReq);
                } else {
                    if(urti->urti_OutDoneHook)
                        CallHookPkt(urti->urti_OutDoneHook, rtn, &rtn->rtn_BufferReq);
                }
            }

            ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);
            uhciFreeTD(hc, utd);
            ptd->ptd_Descriptor = NULL;
        }

        rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
    }
}

void uhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    ULONG startframe = READIO16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT);
    UWORD idx;
    UWORD limit = rtn->rtn_PTDCount;
    UWORD scheduled = 0;
    APTR dmabuffer;

    pciusbDebug("UHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !limit)
        return;

    dmabuffer = rtn->rtn_BufferReq.ubr_Buffer;
#if __WORDSIZE == 64
    if(dmabuffer) {
        dmabuffer = usbGetBuffer(dmabuffer, rtn->rtn_BufferReq.ubr_Length,
                                 rtn->rtn_IOReq.iouh_Dir);
        if(!dmabuffer)
            return;

        if(dmabuffer != rtn->rtn_BufferReq.ubr_Buffer &&
                rtn->rtn_IOReq.iouh_Dir == UHDIR_OUT) {
            CopyMem(rtn->rtn_BufferReq.ubr_Buffer, dmabuffer, rtn->rtn_BufferReq.ubr_Length);
        }
    }
#endif
    rtn->rtn_BounceBuffer = (dmabuffer != rtn->rtn_BufferReq.ubr_Buffer) ? dmabuffer : NULL;

    for(idx = 0; idx < limit; idx++) {
        UWORD ptdidx = (rtn->rtn_NextPTD + idx) % limit;
        struct PTDNode *ptd = rtn->rtn_PTDs[ptdidx];
        struct UhciTD *utd;
        ULONG ctrlstatus;
        ULONG token;
        ULONG slot;
        ULONG phys;
        ULONG len;

        if(!ptd)
            continue;

        if(!ptd->ptd_Descriptor) {
            ptd->ptd_Descriptor = uhciAllocTD(hc);
            if(!ptd->ptd_Descriptor)
                continue;
            ptd->ptd_Phys = READMEM32_LE(&((struct UhciTD *)ptd->ptd_Descriptor)->utd_Self);
        }

        len = rtn->rtn_IOReq.iouh_Length;
        if(len > UHCI_ISO_MAXPKTSIZE)
            len = UHCI_ISO_MAXPKTSIZE;
        if(!len)
            continue;

        utd = (struct UhciTD *)ptd->ptd_Descriptor;
        slot = (startframe + rtn->rtn_BufferReq.ubr_Frame + scheduled) & (UHCI_FRAMELIST_SIZE - 1);
        phys = (ULONG)(IPTR)pciGetPhysical(hc, dmabuffer);

        ctrlstatus = UTCF_ACTIVE | UTCF_ISOCHRONOUS;
        token = (rtn->rtn_IOReq.iouh_DevAddr << UTTS_DEVADDR) |
                (rtn->rtn_IOReq.iouh_Endpoint << UTTS_ENDPOINT) |
                ((len - 1) << UTTS_TRANSLENGTH) |
                ((rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) ? (PID_IN << UTTS_PID) : (PID_OUT << UTTS_PID));

        WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
        WRITEMEM32_LE(&utd->utd_Token, token);
        WRITEMEM32_LE(&utd->utd_BufferPtr, phys);

        ptd->ptd_FrameIdx = slot;
        ptd->ptd_Length = len;
        ptd->ptd_Flags |= (PTDF_ACTIVE|PTDF_BUFFER_VALID);
        ptd->ptd_NextPTD = NULL;

        uhciInsertIsoPTD(hc, ptd, slot);
        scheduled++;
    }

    if(scheduled)
        rtn->rtn_NextPTD = (rtn->rtn_NextPTD + scheduled) % limit;
}

void uhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    UWORD idx;
    UWORD limit = rtn->rtn_PTDCount;

    pciusbDebug("UHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !limit)
        return;

    for(idx = 0; idx < limit; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd && (ptd->ptd_Flags & PTDF_ACTIVE)) {
            uhciUnlinkIsoPTD(hc, ptd);
            ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);
        }
    }

    if(rtn->rtn_BounceBuffer &&
            rtn->rtn_BounceBuffer != rtn->rtn_BufferReq.ubr_Buffer) {
        usbReleaseBuffer(rtn->rtn_BounceBuffer, rtn->rtn_BufferReq.ubr_Buffer,
                         rtn->rtn_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
        rtn->rtn_BounceBuffer = NULL;
    }
}

void uhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    UWORD idx;
    UWORD limit = rtn->rtn_PTDCount;

    pciusbDebug("UHCI", "%s()\n", __func__);

    uhciStopIsochIO(hc, rtn);

    if(!rtn->rtn_PTDs)
        return;

    for(idx = 0; idx < limit; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd) {
            if(ptd->ptd_Descriptor)
                uhciFreeTD(hc, (struct UhciTD *)ptd->ptd_Descriptor);
            FreeMem(ptd, sizeof(*ptd));
            rtn->rtn_PTDs[idx] = NULL;
        }
    }

    rtn->rtn_BounceBuffer = NULL;
}
