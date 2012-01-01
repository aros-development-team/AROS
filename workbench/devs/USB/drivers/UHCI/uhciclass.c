/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define DEBUG 0

#include <inttypes.h>

#include <aros/symbolsets.h>

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <exec/errors.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/irq.h>

#include <usb/usb.h>

#include <asm/io.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include <dos/dos.h>

#include <string.h>
#include <stdio.h>

#include "uhci.h"

OOP_AttrBase HiddUHCIAttrBase;

static AROS_UFH3(void, HubInterrupt,
                 AROS_UFHA(APTR, interruptData, A1),
                 AROS_UFHA(APTR, interruptCode, A5),
                 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    /* Signal the HUB process about incoming interrupt */
    UHCIData *uhci = interruptData;
    uint8_t sts = 0;
    struct Interrupt *intr;

    /* Remove itself from msg list */
    GetMsg(&uhci->mport);

    if (uhci->timereq->tr_node.io_Error == IOERR_ABORTED)
    {
        D(bug("[UHCI] INTR Aborted\n"));
        return;
    }

    if (inw(uhci->iobase + UHCI_PORTSC1) & (UHCI_PORTSC_CSC|UHCI_PORTSC_OCIC))
        sts |= 1;
    if (inw(uhci->iobase + UHCI_PORTSC2) & (UHCI_PORTSC_CSC|UHCI_PORTSC_OCIC))
        sts |= 2;

    D(bug("[UHCI.%04x] Status on port: 1=%04x, 2=%04x\n", uhci->iobase, inw(uhci->iobase + UHCI_PORTSC1), inw(uhci->iobase + UHCI_PORTSC2)));

    if (sts & 1)
        (bug("[UHCI] Status change on port 1\n"));
    if (sts & 2)
        (bug("[UHCI] Status change on port 2\n"));

    if (sts && uhci->running)
    {
        D(bug("Causing interrupts from list %p\n", &uhci->intList));
        ForeachNode(&uhci->intList, intr)
        {
            Cause(intr);
        }
    }

    uhci->timereq->tr_node.io_Command = TR_ADDREQUEST;
    uhci->timereq->tr_time.tv_secs = 0;
    uhci->timereq->tr_time.tv_micro = 255000;
    SendIO((struct IORequest *)uhci->timereq);

    AROS_USERFUNC_EXIT
}

static void uhci_Handler(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    UHCIData *uhci = (UHCIData *)irq->h_Data;
    UHCI_QueueHeader *qh = NULL;
    UHCI_Pipe *p;

    uint16_t status = inw(uhci->iobase + UHCI_STS);
    uint16_t frame = inw(uhci->iobase + UHCI_FRNUM);
    uint16_t port1 = inw(uhci->iobase + UHCI_PORTSC1);
    uint16_t port2 = inw(uhci->iobase + UHCI_PORTSC2);
    uint16_t cmd = inw(uhci->iobase + UHCI_CMD);
    uint16_t sof = inb(uhci->iobase + UHCI_SOF);

    /* Check if there's really an interrupt for us */
    if((status & UHCI_STS_USBINT) == 0)
    {
        outw(status, uhci->iobase + UHCI_STS);
        return;
    }

    D(bug("[UHCI] INTR Cmd=%04x, SOF=%04x, Status = %04x, Frame=%04x, PortSC1=%04x, PortSC2=%04x\n",
            cmd, sof, status, frame, port1, port2));

    if (!IsListEmpty(&uhci->Interrupts))
    {
        Disable();
        ForeachNode(&uhci->Interrupts, p)
        {
            /* If the Linkptr of queue does not point to the first transfer descriptor, an interrupt
             * has occured. Unfortunately, only the first one, who has added the interrupt to this
             * pipe may receive data */
            if ((p->p_Queue->qh_VLink & 0xfffffff1) != (uint32_t)p->p_FirstTD)
            {
                struct UHCI_Interrupt *intr;

                if (!(p->p_FirstTD->td_Status & UHCI_TD_NAK))
                {
                    D(bug("[UHCI] Interrupt!!! pipe %p, vlink=%p, FirstTD=%p\n", p, p->p_Queue->qh_VLink, p->p_FirstTD));
                    ForeachNode(&p->p_Intr, intr)
                    {
                        D(bug("[UHCI] Issuing interrupt %p\n", intr));
                        Cause(intr->i_intr);
                    }
                }

                /* Reactivate the transfer descriptor */
                p->p_FirstTD->td_Status = UHCI_TD_ZERO_ACTLEN(UHCI_TD_SET_ERRCNT(3)
                                                              | UHCI_TD_ACTIVE
                                                              | UHCI_TD_IOC);

                p->p_FirstTD->td_Token ^= 1 << 19;

                if (p->p_FullSpeed)
                    p->p_FirstTD->td_Status |= UHCI_TD_SPD;
                else
                    p->p_FirstTD->td_Status |= UHCI_TD_LS | UHCI_TD_SPD;

                /* Link the first TD to the Queue header */
                p->p_Queue->qh_VLink = (uint32_t)p->p_FirstTD | UHCI_PTR_TD;
            }
        }
        Enable();
    }

    if (!IsListEmpty(&uhci->ControlLS))
    {
        Disable();
        ForeachNode(&uhci->ControlLS, p)
        {
            UHCI_TransferDesc *td = p->p_FirstTD;
            BOOL changed = FALSE;

            if (p->p_Queue->qh_VLink == UHCI_PTR_T)
                p->p_LastTD = (APTR)UHCI_PTR_T;

            D(
                    UHCI_TransferDesc *t = (UHCI_TransferDesc *)(p->p_Queue->qh_VLink & 0xfffffff1);
                    bug("[UHCI] Control Low:  p->p_Queue->qh_VLink=%p\n", p->p_Queue->qh_VLink);
                    while ((uint32_t)t != UHCI_PTR_T)
                    {
                        bug("[UHCI]     TD=%p (%08x %08x %08x %08x)\n", t,
                                t->td_LinkPtr, t->td_Status, t->td_Token, t->td_Buffer);
                        t = (UHCI_TransferDesc *)(t->td_LinkPtr & 0xfffffff1);
                    }
            );

            while ((uint32_t)td != (p->p_Queue->qh_VLink & 0xfffffff1))
            {
                D(bug("[UHCI] TD=%p\n", td));
                UHCI_TransferDesc *tnext = (UHCI_TransferDesc *)(td->td_LinkPtr & 0xfffffff1);
                uhci_FreeTDQuick(uhci, td);
                td = tnext;
                p->p_FirstTD = tnext;
                changed = TRUE;
            }

            if (changed && p->p_Queue->qh_VLink == UHCI_PTR_T) {
                D(bug("[UHCI] INTR Control pipe %p empty (slow)\n", p));
                p->p_ErrorCode = 0;

                if (p->p_SigTask)
                    Signal(p->p_SigTask, 1 << p->p_Signal);
            }
        }
        Enable();
    }

    if (!IsListEmpty(&uhci->ControlFS))
    {
        Disable();
        ForeachNode(&uhci->ControlFS, p)
        {
            UHCI_TransferDesc *td = p->p_FirstTD;
            BOOL changed = FALSE;

            if (p->p_Queue->qh_VLink & UHCI_PTR_T)
                p->p_LastTD = (APTR)UHCI_PTR_T;

            D(
                    UHCI_TransferDesc *t = (UHCI_TransferDesc *)(p->p_Queue->qh_VLink & 0xfffffff1);
                    bug("[UHCI] Control Fast:  p->p_Queue->qh_VLink=%p\n", p->p_Queue->qh_VLink);
                    while (!((uint32_t)t & UHCI_PTR_T))
                    {
                        bug("[UHCI]     TD=%p (%08x %08x %08x %08x)\n", t,
                                t->td_LinkPtr, t->td_Status, t->td_Token, t->td_Buffer);
                        t = (UHCI_TransferDesc *)(t->td_LinkPtr & 0xfffffff1);
                    }
            );


            while ((uint32_t)td != (p->p_Queue->qh_VLink & 0xfffffff1))
            {
                D(bug("[UHCI] TD=%p\n", td));

                UHCI_TransferDesc *tnext = (UHCI_TransferDesc *)(td->td_LinkPtr & 0xfffffff1);
                uhci_FreeTDQuick(uhci, td);
                td = tnext;
                p->p_FirstTD = tnext;
                changed = TRUE;
            }

            if (changed && p->p_Queue->qh_VLink == UHCI_PTR_T) {
                D(bug("[UHCI] INTR Control pipe %p empty\n", p));
                p->p_ErrorCode = 0;

                if (p->p_SigTask)
                    Signal(p->p_SigTask, 1 << p->p_Signal);
            }
            /* Free the unused TD's */
        }
        Enable();
    }

    if (!IsListEmpty(&uhci->Bulk))
    {
        Disable();
        ForeachNode(&uhci->Bulk, p)
        {
            UHCI_TransferDesc *td = p->p_FirstTD;
            BOOL changed = FALSE;

            if (p->p_Queue->qh_VLink == UHCI_PTR_T)
                p->p_LastTD = (APTR)UHCI_PTR_T;

            D(
                    UHCI_TransferDesc *t = (UHCI_TransferDesc *)(p->p_Queue->qh_VLink & 0xfffffff1);
                    bug("[UHCI] Bulk:  p=%p, p->p_Queue->qh_VLink=%p\n", p, p->p_Queue->qh_VLink);
                    while ((uint32_t)t != UHCI_PTR_T)
                    {
                        bug("[UHCI]     TD=%p (%08x %08x %08x %08x)\n", t,
                                t->td_LinkPtr, t->td_Status, t->td_Token, t->td_Buffer);
                        t = (UHCI_TransferDesc *)(t->td_LinkPtr & 0xfffffff1);
                    }
            );

            while ((uint32_t)td != (p->p_Queue->qh_VLink & 0xfffffff1))
            {
                UHCI_TransferDesc *tnext = (UHCI_TransferDesc *)(td->td_LinkPtr & 0xfffffff1);
                uhci_FreeTDQuick(uhci, td);
                td = tnext;
                p->p_FirstTD = tnext;
                changed = TRUE;
            }

            if (changed && p->p_Queue->qh_VLink == UHCI_PTR_T) {
                D(bug("[UHCI] INTR Bulk pipe %p empty\n", p));
                p->p_ErrorCode = 0;

                if (p->p_SigTask)
                    Signal(p->p_SigTask, 1 << p->p_Signal);
            }
        }
        Enable();
    }

    outw(status, uhci->iobase + UHCI_STS);
}

OOP_Object *METHOD(UHCI, Root, New)
{
    D(bug("[UHCI] UHCI::New()\n"));

    BASE(cl->UserData)->LibNode.lib_OpenCnt++;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        int i;
        UHCIData *uhci = OOP_INST_DATA(cl, o);

        NEWLIST(&uhci->mport.mp_MsgList);
        uhci->mport.mp_Flags = PA_SOFTINT;
        uhci->mport.mp_Node.ln_Type = NT_MSGPORT;
        uhci->mport.mp_SigTask = &uhci->timerint;
        uhci->timerint.is_Code = HubInterrupt;
        uhci->timerint.is_Data = uhci;

        uhci->timereq = CreateIORequest(&uhci->mport, sizeof(struct timerequest));
        OpenDevice((STRPTR)"timer.device", UNIT_VBLANK, (struct IORequest *)uhci->timereq, 0);

        uhci->sd = SD(cl);

        NEWLIST(&uhci->Isochronous);
        NEWLIST(&uhci->Interrupts);
        NEWLIST(&uhci->ControlLS);
        NEWLIST(&uhci->ControlFS);
        NEWLIST(&uhci->Bulk);

        NEWLIST(&uhci->intList);

        if (uhci->tmp)
            AddTail(&uhci->intList, uhci->tmp);

        uhci->tr = (struct timerequest *)CreateIORequest(
                CreateMsgPort(), sizeof(struct timerequest));

        FreeSignal(uhci->tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
        uhci->tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = SIGBREAKB_CTRL_D;
        OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)uhci->tr, 0);

        uhci->iobase = GetTagData(aHidd_UHCI_IOBase, 0, msg->attrList);
        uhci->device = (OOP_Object *)GetTagData(aHidd_UHCI_PCIDevice, 0, msg->attrList);
        uhci->pciDriver = (OOP_Object *)GetTagData(aHidd_UHCI_PCIDriver, 0, msg->attrList);

        ({
            uint32_t base = inw(uhci->iobase + UHCI_FLBASEADDR);
            uint16_t status = inw(uhci->iobase + UHCI_STS);
            uint16_t frame = inw(uhci->iobase + UHCI_FRNUM);
            uint16_t port1 = inw(uhci->iobase + UHCI_PORTSC1);
            uint16_t port2 = inw(uhci->iobase + UHCI_PORTSC2);
            uint16_t cmd = inw(uhci->iobase + UHCI_CMD);
            uint8_t sof = inb(uhci->iobase + UHCI_SOF);
            int i;

            bug("[UHCI] Initial state: Base=%08x Cmd=%04x SOF=%02x Status=%04x Frame=%04x PortSC1=%04x PortSC2=%04x\n",
                    base, cmd, sof, status, frame, port1, port2);

            /* Windows-like BIOS detach procedure */
            bug("[UHCI] Stopping UHCI\n");

            /*
             * Clearing RS bit will stop the controller. Clearing CF bit will tell potential
             * legacy USB BIOS that AROS takes over
             */
            outw(inw(uhci->iobase + UHCI_CMD) & ~(UHCI_CMD_RS | UHCI_CMD_CF), uhci->iobase + UHCI_CMD);

            for (i=0; i < 10; i++)
            {
                uhci_sleep(cl, o, 1);
                if (inw(uhci->iobase + UHCI_STS) & UHCI_STS_HCH)
                {
                    bug("[UHCI] Host controller halted\n");
                    break;
                }
            }

            struct pHidd_PCIDevice_WriteConfigWord wcw;
            struct pHidd_PCIDevice_ReadConfigWord rcw;

            wcw.mID = OOP_GetMethodID((STRPTR)IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
            wcw.reg = PCI_LEGSUP;
            rcw.mID = OOP_GetMethodID((STRPTR)IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
            rcw.reg = PCI_LEGSUP;

            uint16_t reg = OOP_DoMethod(uhci->device, (OOP_Msg)&rcw.mID);
            bug("[UHCI] PCI_LEGSUP=%04x -> ", reg);
            reg &= 0xffef;
            wcw.val = reg;
            bug("%04x\n", reg);
            OOP_DoMethod(uhci->device, (OOP_Msg)&wcw.mID);
            reg = OOP_DoMethod(uhci->device, (OOP_Msg)&rcw.mID);
            bug("[UHCI] PCI_LEGSUP=%04x\n", reg);
            wcw.val = 0x2000;
            OOP_DoMethod(uhci->device, (OOP_Msg)&wcw.mID);
            reg = OOP_DoMethod(uhci->device, (OOP_Msg)&rcw.mID);
            bug("[UHCI] PCI_LEGSUP=%04x\n", reg);
        });



        OOP_GetAttr(uhci->device, aHidd_PCIDevice_INTLine, &uhci->irq);

        D(bug("[UHCI]   New driver with IOBase=%x, IRQ=%d, PCI device=%08x, PCI driver=%08x\n",
                uhci->iobase, uhci->irq, uhci->device, uhci->pciDriver));

        uhci->Frame = HIDD_PCIDriver_AllocPCIMem(uhci->pciDriver, 4096);
        D(bug("[UHCI]   Frame = %08x\n", uhci->Frame));

        outl((uint32_t)uhci->Frame, uhci->iobase + UHCI_FLBASEADDR);

        uhci->irqHandler = AllocPooled(SD(cl)->MemPool, sizeof(HIDDT_IRQ_Handler));

        uhci->irqHandler->h_Node.ln_Name = "UHCI Intr";
        uhci->irqHandler->h_Node.ln_Pri = 127;
        uhci->irqHandler->h_Code = uhci_Handler;
        uhci->irqHandler->h_Data = uhci;

        HIDD_IRQ_AddHandler(SD(cl)->irq, uhci->irqHandler, uhci->irq);
        D(bug("[UHCI]   IRQHandler = %08x\n", uhci->irqHandler));

        struct pHidd_PCIDevice_WriteConfigWord __msg = {
                OOP_GetMethodID((STRPTR)IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord), 0xc0, 0x8f00
        }, *msg = &__msg;

        OOP_DoMethod(uhci->device, (OOP_Msg)msg);

        D(bug("[UHCI]   Preparing initial QH tree\n"));

        uhci->dummy_td = uhci_AllocTD(cl, o);
        uhci->dummy_td->td_Buffer = 0;
        uhci->dummy_td->td_LinkPtr = 0;
        uhci->dummy_td->td_Status = 0;
        uhci->dummy_td->td_Token = 0;

        uhci->qh01 = uhci_AllocQH(cl, o);
        uhci->qh01->qh_VLink = UHCI_PTR_T;
        uhci->qh01->qh_HLink = UHCI_PTR_T;

        for (i=0; i < 2; i++)
        {
            uhci->qh02[i] = uhci_AllocQH(cl, o);
            uhci->qh02[i]->qh_VLink = UHCI_PTR_T;
            uhci->qh02[i]->qh_HLink = UHCI_PTR_QH | (uint32_t)uhci->qh01;
        }

        for (i=0; i < 4; i++)
        {
            uhci->qh04[i] = uhci_AllocQH(cl, o);
            uhci->qh04[i]->qh_VLink = UHCI_PTR_T;
            uhci->qh04[i]->qh_HLink = UHCI_PTR_QH | (uint32_t)uhci->qh02[i & 0x01];
        }

        for (i=0; i < 8; i++)
        {
            uhci->qh08[i] = uhci_AllocQH(cl, o);
            uhci->qh08[i]->qh_VLink = UHCI_PTR_T;
            uhci->qh08[i]->qh_HLink = UHCI_PTR_QH | (uint32_t)uhci->qh04[i & 0x03];
        }

        for (i=0; i < 16; i++)
        {
            uhci->qh16[i] = uhci_AllocQH(cl, o);
            uhci->qh16[i]->qh_VLink = UHCI_PTR_T;
            uhci->qh16[i]->qh_HLink = UHCI_PTR_QH | (uint32_t)uhci->qh08[i & 0x07];
        }

        for (i=0; i < 32; i++)
        {
            uhci->qh32[i] = uhci_AllocQH(cl, o);
            uhci->qh32[i]->qh_VLink = UHCI_PTR_T;
            uhci->qh32[i]->qh_HLink = UHCI_PTR_QH | (uint32_t)uhci->qh16[i & 0x0f];
        }

        for (i=0; i < 1024; i++) {
            uhci->Frame[i] = UHCI_PTR_QH | (uint32_t)uhci->qh32[i % 0x1f];
        }

        D(bug("[UHCI]   Enabling the controller\n"));

        outw(0, uhci->iobase + UHCI_INTR);
        uhci_globalreset(cl, o);
        uhci_reset(cl, o);

        struct pHidd_PCIDevice_WriteConfigWord __msg2 = {
                OOP_GetMethodID((STRPTR)IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord), 0xc0, 0x2000
        }, *msg2 = &__msg2;

        OOP_DoMethod(uhci->device, (OOP_Msg)msg2);

        outb(0x40, uhci->iobase + UHCI_SOF);

        outw(0, uhci->iobase + UHCI_FRNUM);
        outl((uint32_t)uhci->Frame, uhci->iobase + UHCI_FLBASEADDR);

        outw(UHCI_CMD_MAXP | UHCI_CMD_CF, uhci->iobase + UHCI_CMD);

        outw(UHCI_INTR_SPIE | UHCI_INTR_IOCE | UHCI_INTR_RIE | UHCI_INTR_TOCRCIE,
                uhci->iobase + UHCI_INTR );
    }


    D(bug("[UHCI] UHCI::New() = %p\n",o));

    if (!o)
        BASE(cl->UserData)->LibNode.lib_OpenCnt--;

    return o;
}

struct pRoot_Dispose {
    OOP_MethodID	mID;
};

void METHOD(UHCI, Root, Dispose)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    struct uhcibase *base = BASE(cl->UserData);

    D(bug("[UHCI] UHCI::Dispose\n"));

    D(bug("[UHCI]   Stopping USB transfer\n"));
    outw(0, uhci->iobase + UHCI_CMD);

    D(bug("[UHCI]   Releasing USB ring\n"));
    HIDD_PCIDriver_FreePCIMem(uhci->pciDriver, uhci->Frame);

    D(bug("[UHCI]   Removing IRQ handler\n"));
    HIDD_IRQ_RemHandler(SD(cl)->irq, uhci->irqHandler);
    FreePooled(SD(cl)->MemPool, uhci->irqHandler, sizeof(HIDDT_IRQ_Handler));

    AbortIO((struct IORequest *)uhci->timereq);
    DeleteIORequest((struct IORequest *)uhci->timereq);

    struct MsgPort *port = uhci->tr->tr_node.io_Message.mn_ReplyPort;
    port->mp_SigTask = FindTask(NULL);
    CloseDevice((struct IORequest *)uhci->tr);
    DeleteIORequest((struct IORequest *)uhci->tr);
    port->mp_SigBit = AllocSignal(-1);
    DeleteMsgPort(port);

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    base->LibNode.lib_OpenCnt--;
}

void METHOD(UHCI, Root, Get)
{
    uint32_t idx;

    if (IS_USBDEVICE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_USBDevice_Address:
                *msg->storage = 1;
                break;
            case aoHidd_USBDevice_Hub:
                *msg->storage = 0;
                break;
            case aoHidd_USBDevice_Bus:
                *msg->storage = (IPTR)o;
                break;
            default:
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
    else if (IS_USBDRV_ATTR(msg->attrID, idx))
    {

    }
    else
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

void METHOD(UHCI, Root, Set)
{
    uint32_t idx;
    struct TagItem *tag;
    struct TagItem *tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
        if (IS_USBDEVICE_ATTR(tag->ti_Tag, idx))
        {
        }
        else if (IS_USBHUB_ATTR(tag->ti_Tag, idx))
        {

        }
        else if (IS_USBDRV_ATTR(tag->ti_Tag, idx))
        {
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


APTR METHOD(UHCI, Hidd_USBDrv, CreatePipe)
{
    return uhci_CreatePipe(cl, o, msg->type, msg->fullspeed, msg->address, msg->endpoint, msg->period, msg->maxpacket, msg->timeout);
}

void METHOD(UHCI, Hidd_USBDrv, DeletePipe)
{
    return uhci_DeletePipe(cl, o, msg->pipe);
}

void METHOD(UHCI, Hidd_USBDrv, SetTimeout)
{
    UHCI_Pipe	*p = msg->pipe;

    if (p)
    	p->p_TimeoutVal = msg->timeout;
}

BOOL METHOD(UHCI, Hidd_USBDrv, AddInterrupt)
{
    BOOL retval = FALSE;
    UHCI_Pipe *p = msg->pipe;

    if (msg->pipe == (APTR)0xdeadbeef)
    {
        UHCIData *uhci = OOP_INST_DATA(cl, o);
        D(bug("[UHCI] AddInterrupt() local for the UHCI. Intr %p, list %p\n", msg->interrupt, &uhci->intList));

        if (!uhci->iobase)
            uhci->tmp = msg->interrupt;
        else
            AddTail(&uhci->intList, &msg->interrupt->is_Node);

        retval = TRUE;
    }
    else
    {
        D(bug("[UHCI] AddInterrupt()\n"));

        UHCI_Interrupt_t    *intr = AllocVecPooled(SD(cl)->MemPool, sizeof(UHCI_Interrupt_t));

        D(bug("[UHCI::AddInterrupt] intr = %p\n", intr));

        if (intr)
        {
            intr->i_intr = msg->interrupt;

            uhci_QueuedRead(cl, o, p, msg->buffer, msg->length);
            AddTail(&p->p_Intr, &intr->i_node);

            /*
             intr->i_td = uhci_AllocTD(cl, o);

            D(bug("[UHCI::AddInterrupt] intr->i_td = %p\n", intr->i_td));

            if (intr->i_td)
            {
                intr->i_td->td_Buffer = (uint32_t)msg->buffer;
                intr->i_td->td_Status = UHCI_TD_ZERO_ACTLEN(UHCI_TD_SET_ERRCNT(3) | UHCI_TD_ACTIVE);
                intr->i_td->td_Token = UHCI_TD_IN(msg->length, p->p_EndPoint, p->p_DevAddr, p->p_NextToggle);
                intr->i_td->td_Status |= UHCI_TD_SPD;
                intr->i_td->td_LinkPtr = UHCI_PTR_T;

                Disable();

                AddTail(&p->p_Intr, &intr->i_node);

                if (p->p_FirstTD == (APTR)UHCI_PTR_T)
                {
                    p->p_FirstTD = intr->i_td;
                    p->p_LastTD = intr->i_td;
                }
                else
                {
                    p->p_LastTD->td_LinkPtr = (uint32_t)intr->i_td | UHCI_PTR_TD;
                    p->p_LastTD = intr->i_td;
                }

                if (p->p_Queue->qh_VLink == UHCI_PTR_T)
                {
                    p->p_Queue->qh_VLink = (uint32_t)p->p_FirstTD | UHCI_PTR_TD;
                }

                Enable();

                bug("[UHCI::AddInterrupt] p->p_FirstTd = %p, vlink=%p\n", p->p_FirstTD, p->p_Queue->qh_VLink);
                retval = TRUE;
            }
            else
                FreeVecPooled(SD(cl)->MemPool, intr);
            */
        }
    }
    D(bug("[UHCI::AddInterrupt] %s\n", retval ? "success":"failure"));

    return retval;
}

BOOL METHOD(UHCI, Hidd_USBDrv, RemInterrupt)
{
    UHCI_Interrupt_t    *intr;
    UHCI_Pipe           *p = msg->pipe;

    if (p == (UHCI_Pipe *)0xdeadbeef)
    {
        Remove(msg->interrupt);
        return TRUE;
    }
    else
    {
        ForeachNode(&p->p_Intr, intr)
        {
            if (intr->i_intr == msg->interrupt)
                break;
        }

        if (intr)
        {
            Disable();
            Remove(&intr->i_node);
            Enable();

            uhci_FreeTD(cl, o, intr->i_td);
            FreeVecPooled(SD(cl)->MemPool, intr);

            return TRUE;
        }
        return FALSE;
    }
}

BOOL METHOD(UHCI, Hidd_USBDrv, ControlTransfer)
{
    UHCI_Pipe *p = msg->pipe;
    UHCIData *uhci = OOP_INST_DATA(cl, o);

    BOOL retval = TRUE;

    D({
        uint16_t status = inw(uhci->iobase + UHCI_STS);
        uint16_t frame = inw(uhci->iobase + UHCI_FRNUM);
        uint16_t port1 = inw(uhci->iobase + UHCI_PORTSC1);
        uint16_t port2 = inw(uhci->iobase + UHCI_PORTSC2);
        uint16_t cmd = inw(uhci->iobase + UHCI_CMD);
        uint16_t sof = inb(uhci->iobase + UHCI_SOF);

        D(bug("[UHCI] Status check: Cmd=%04x, SOF=%04x, Status = %04x, Frame=%04x, PortSC1=%04x, PortSC2=%04x\n",
                cmd, sof, status, frame, port1, port2));
    });

    int8_t sig = AllocSignal(-1);
    int8_t toutsig = AllocSignal(-1);
    int32_t msec = p->p_TimeoutVal;

    D(bug("[UHCI] ControlTransfer(pipe=%p, request=%p, buffer=%p, length=%d, timeout=%d, signal=%d, toutsig=%d)\n",
            msg->pipe, msg->request, msg->buffer, msg->length, msec, sig, toutsig));

    if (sig >= 0 && toutsig >= 0)
    {
        p->p_Signal = sig;
        p->p_SigTask = FindTask(NULL);

        if (p->p_TimeoutVal != 0)
        {
            p->p_Timeout->tr_node.io_Message.mn_ReplyPort->mp_SigBit = toutsig;
            p->p_Timeout->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

            p->p_Timeout->tr_node.io_Command = TR_ADDREQUEST;
            p->p_Timeout->tr_time.tv_secs = msec / 1000;
            p->p_Timeout->tr_time.tv_micro = 1000 * (msec % 1000);

            SendIO((struct IORequest *)p->p_Timeout);
        }
        uhci_ControlTransfer(cl, o, msg->pipe, msg->request, msg->buffer, msg->length);
        if (Wait((1 << sig) | (1 << toutsig)) & (1 << toutsig))
        {
            bug("[UHCI] !!!TIMEOUT!!!\n");
            p->p_Queue->qh_VLink = UHCI_PTR_T;
            UHCI_TransferDesc *td = p->p_FirstTD;

            GetMsg(p->p_Timeout->tr_node.io_Message.mn_ReplyPort);

            while ((uint32_t)td != UHCI_PTR_T)
            {
                bug("[UHCI]     TD=%p (%08x %08x %08x %08x)\n", td,
                        td->td_LinkPtr, td->td_Status, td->td_Token, td->td_Buffer);
                UHCI_TransferDesc *tnext = (UHCI_TransferDesc *)(td->td_LinkPtr & 0xfffffff1);
                uhci_FreeTD(cl, o, td);
                td = tnext;
                p->p_FirstTD = tnext;
            }
            p->p_LastTD=UHCI_PTR_T;

            retval = FALSE;
        }
        else
        {
            if (!CheckIO((struct IORequest *)p->p_Timeout))
                AbortIO((struct IORequest *)p->p_Timeout);
            WaitIO((struct IORequest *)p->p_Timeout);
            SetSignal(0, 1 << toutsig);

            if (p->p_ErrorCode)
                retval = FALSE;
        }
        FreeSignal(sig);
        FreeSignal(toutsig);
    }
    return retval;
}

BOOL METHOD(UHCI, Hidd_USBDrv, BulkTransfer)
{
    UHCI_Pipe *p = msg->pipe;
    UHCIData *uhci = OOP_INST_DATA(cl, o);

    BOOL retval = TRUE;

    int8_t sig = AllocSignal(-1);
    int8_t toutsig = AllocSignal(-1);
    int32_t msec = p->p_TimeoutVal;

    D(bug("[UHCI] BulkTransfer(pipe=%p, buffer=%p, length=%d, timeout=%d, signal=%d, toutsig=%d)\n",
            msg->pipe, msg->buffer, msg->length, msec, sig, toutsig));

    if (sig >= 0 && toutsig >= 0)
    {
        p->p_Signal = sig;
        p->p_SigTask = FindTask(NULL);

        if (p->p_TimeoutVal != 0)
        {
            p->p_Timeout->tr_node.io_Message.mn_ReplyPort->mp_SigBit = toutsig;
            p->p_Timeout->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

            p->p_Timeout->tr_node.io_Command = TR_ADDREQUEST;
            p->p_Timeout->tr_time.tv_secs = msec / 1000;
            p->p_Timeout->tr_time.tv_micro = 1000 * (msec % 1000);

            SendIO((struct IORequest *)p->p_Timeout);
        }

        uhci_QueuedTransfer(cl, o, p, msg->buffer, msg->length, p->p_EndPoint & 0x80);

        if (Wait((1 << sig) | (1 << toutsig)) & (1 << toutsig))
        {
            bug("[UHCI] !!!TIMEOUT!!!\n");
            p->p_Queue->qh_VLink = UHCI_PTR_T;
            UHCI_TransferDesc *td = p->p_FirstTD;

            GetMsg(p->p_Timeout->tr_node.io_Message.mn_ReplyPort);

            while ((uint32_t)td != UHCI_PTR_T)
            {
                bug("[UHCI]     TD=%p (%08x %08x %08x %08x)\n", td,
                        td->td_LinkPtr, td->td_Status, td->td_Token, td->td_Buffer);
                UHCI_TransferDesc *tnext = (UHCI_TransferDesc *)(td->td_LinkPtr & 0xfffffff1);
                uhci_FreeTD(cl, o, td);
                td = tnext;
                p->p_FirstTD = tnext;
            }
            p->p_LastTD=UHCI_PTR_T;

            retval = FALSE;
        }
        else
        {
            if (!CheckIO((struct IORequest *)p->p_Timeout))
                AbortIO((struct IORequest *)p->p_Timeout);
            WaitIO((struct IORequest *)p->p_Timeout);
            SetSignal(0, 1 << toutsig);

            if (p->p_ErrorCode)
                retval = FALSE;
        }
        FreeSignal(sig);
        FreeSignal(toutsig);
    }

    D({
    if (retval)
    	(bug("[UHCI] Transfer OK\n"));
    else
    	(bug("[UHCI] Transfer FAILED\n"));
    });

    return retval;
}

BOOL METHOD(UHCI, Hidd_USBHub, OnOff)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    BOOL retval;
    D(bug("[UHCI] USBHub::OnOff(%d)\n", msg->on));

    if (!CheckIO((struct IORequest *)uhci->timereq))
        AbortIO((struct IORequest *)uhci->timereq);
    GetMsg(&uhci->mport);
    SetSignal(0, 1 << uhci->mport.mp_SigBit);

    uhci->running = msg->on;

    if (msg->on)
    {
        uhci->timereq->tr_node.io_Command = TR_ADDREQUEST;
        uhci->timereq->tr_time.tv_secs = 1;
        uhci->timereq->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)uhci->timereq);
    }

    OOP_DoSuperMethod(cl,o,(OOP_Msg)msg);

    retval = uhci_run(cl, o, msg->on);
    uhci_sleep(cl, o, 100);

    return retval;
}

BOOL METHOD(UHCI, Hidd_USBHub, PortReset)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    BOOL retval;
    D(bug("[UHCI] USBHub::PortReset(%d)\n", msg->portNummer));
    retval = uhci_PortReset(cl, o, msg->portNummer);
    uhci->reset |= (1 << (msg->portNummer - 1));
    uhci_sleep(cl, o, 100);
    return retval;
}

BOOL METHOD(UHCI, Hidd_USBHub, GetPortStatus)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    uint16_t port, x, status=0, change=0;
    BOOL retval = FALSE;

    if (msg->port == 1)
        port = UHCI_PORTSC1;
    else if (msg->port == 2)
        port = UHCI_PORTSC2;
    else return FALSE;

    x = inw(uhci->iobase + port);

    if (x & UHCI_PORTSC_CCS)
        status |= UPS_CURRENT_CONNECT_STATUS;
    if (x & UHCI_PORTSC_CSC)
        change |= UPS_C_CONNECT_STATUS;
    if (x & UHCI_PORTSC_PE)
        status |= UPS_PORT_ENABLED;
    if (x & UHCI_PORTSC_POEDC)
        change |= UPS_C_PORT_ENABLED;
    if (x & UHCI_PORTSC_OCI)
        status |= UPS_OVERCURRENT_INDICATOR;
    if (x & UHCI_PORTSC_OCIC)
        change |= UPS_C_OVERCURRENT_INDICATOR;
    if (x & UHCI_PORTSC_SUSP)
        status |= UPS_SUSPEND;
    if (x & UHCI_PORTSC_LSDA)
        status |= UPS_LOW_SPEED;

    status |= UPS_PORT_POWER;

    if (uhci->reset & (1 << (msg->port - 1)))
        status |= UPS_C_PORT_RESET;

    msg->status->wPortStatus = AROS_WORD2LE(status);
    msg->status->wPortChange = AROS_WORD2LE(change);

    return TRUE;
}

BOOL METHOD(UHCI, Hidd_USBHub, GetHubStatus)
{
    return TRUE;
}

BOOL METHOD(UHCI, Hidd_USBHub, ClearHubFeature)
{
    return TRUE;
}

BOOL METHOD(UHCI, Hidd_USBHub, SetHubFeature)
{
    return TRUE;
}

BOOL METHOD(UHCI, Hidd_USBHub, ClearPortFeature)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    uint16_t port, x;
    BOOL retval = FALSE;

    if (msg->port == 1)
        port = UHCI_PORTSC1;
    else if (msg->port == 2)
        port = UHCI_PORTSC2;
    else return FALSE;

    switch(msg->feature)
    {
        case UHF_PORT_ENABLE:
            x = URWMASK(inw(uhci->iobase + port));
            outw(x & ~UHCI_PORTSC_PE, uhci->iobase + port);
            retval = TRUE;
            break;

        case UHF_PORT_SUSPEND:
            x = URWMASK(inw(uhci->iobase + port));
            outw(x & ~UHCI_PORTSC_SUSP, uhci->iobase + port);
            retval = TRUE;
            break;

        case UHF_PORT_RESET:
            x = URWMASK(inw(uhci->iobase + port));
            outw(x & ~UHCI_PORTSC_PR, uhci->iobase + port);
            retval = TRUE;
            break;

        case UHF_C_PORT_CONNECTION:
            x = URWMASK(inw(uhci->iobase + port));
            outw(x | UHCI_PORTSC_CSC, uhci->iobase + port);
            retval = TRUE;
            break;

        case UHF_C_PORT_ENABLE:
            x = URWMASK(inw(uhci->iobase + port));
            outw(x | UHCI_PORTSC_POEDC, uhci->iobase + port);
            retval = TRUE;
            break;

        case UHF_C_PORT_OVER_CURRENT:
            x = URWMASK(inw(uhci->iobase + port));
            outw(x | UHCI_PORTSC_OCIC, uhci->iobase + port);
            retval = TRUE;
            break;

        case UHF_C_PORT_RESET:
            uhci->reset &= ~(1 << (msg->port - 1));
            retval = TRUE;
            break;

        default:
            retval = FALSE;
            break;
    }

    return retval;
}

BOOL METHOD(UHCI, Hidd_USBHub, SetPortFeature)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    uint16_t port, x;
    BOOL retval = FALSE;

    if (msg->port == 1)
        port = UHCI_PORTSC1;
    else if (msg->port == 2)
        port = UHCI_PORTSC2;
    else return FALSE;

    switch(msg->feature)
    {
        case UHF_PORT_ENABLE:
            x = URWMASK(inw(uhci->iobase + port));
            outw(x | UHCI_PORTSC_PE, uhci->iobase + port);
            retval = TRUE;
            break;

        case UHF_PORT_SUSPEND:
            x = URWMASK(inw(uhci->iobase + port));
            outw(x | UHCI_PORTSC_SUSP, uhci->iobase + port);
            retval = TRUE;
            break;

        case UHF_PORT_RESET:
            retval = uhci_PortReset(cl, o, msg->port);
            break;

        case UHF_PORT_POWER:
            retval = TRUE;
            break;

        default:
            retval = FALSE;
            break;
    }

    return retval;
}

static const char *string1 = "U\0H\0C\0I\0 \0R\0o\0o\0t\0 \0H\0U\0B";
static const int string1_l = sizeof("U\0H\0C\0I\0 \0R\0o\0o\0t\0 \0H\0U\0B");

static const char *string2 = "T\0h\0e\0 \0A\0R\0O\0S\0 \0D\0e\0v\0e\0l\0o\0p\0m\0e\0n\0t\0 \0T\0e\0a\0m";
static const int string2_l = sizeof("T\0h\0e\0 \0A\0R\0O\0S\0 \0D\0e\0v\0e\0l\0o\0p\0m\0e\0n\0t\0 \0T\0e\0a\0m");

BOOL METHOD(UHCI, Hidd_USBDevice, GetString)
{
    msg->string->bDescriptorType = UDESC_STRING;

    if (msg->id == USB_LANGUAGE_TABLE)
    {
        msg->string->bLength = 4;
        msg->string->bString[0] = 0x0409;
    }
    else if (msg->id == 1)
    {
        msg->string->bLength = 2 + string1_l;
        CopyMem(string1, &msg->string->bString[0], string1_l);
    }
    else if (msg->id == 2)
    {
        msg->string->bLength = 2 + string2_l;
        CopyMem(string2, &msg->string->bString[0], string2_l);
    }
    else if (msg->id == 3)
    {
        char buff[129];
        int i;
        snprintf(buff, 128, "%d.%d", VERSION_NUMBER, REVISION_NUMBER);
        msg->string->bLength = 2 + 2*strlen(buff);
        for (i=0; i < ((msg->string->bLength - 2) >> 1); i++)
            msg->string->bString[i] = AROS_WORD2LE(buff[i]);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

static const usb_device_descriptor_t device_descriptor = {
    bLength:            sizeof(usb_device_descriptor_t),
    bDescriptorType:    UDESC_DEVICE,
    bcdUSB:             0x0101,
    bDeviceClass:       UDCLASS_HUB,
    bDeviceSubClass:    UDSUBCLASS_HUB,
    bDeviceProtocol:    UDPROTO_FSHUB,
    bMaxPacketSize:     64,
    iManufacturer:      2,
    iProduct:           1,
    iSerialNumber:      3,
    bNumConfigurations: 1,
};

static const usb_hub_descriptor_t hub_descriptor = {
    bDescLength:        sizeof(usb_hub_descriptor_t) - 31,
    bDescriptorType:    UDESC_HUB,
    bNbrPorts:          2,
    wHubCharacteristics:0,
    bPwrOn2PwrGood:     50,
    bHubContrCurrent:   0,
    DeviceRemovable:    {0,},
};

static const usb_endpoint_descriptor_t endpoint_descriptor = {
    bLength:            sizeof(usb_endpoint_descriptor_t),
    bDescriptorType:    UDESC_ENDPOINT,
    bEndpointAddress:   0x81,
    bmAttributes:       0x03,
    wMaxPacketSize:     1,
    bInterval:          255
};

usb_endpoint_descriptor_t * METHOD(UHCI, Hidd_USBDevice, GetEndpoint)
{
    if (msg->interface == 0 && msg->endpoint == 0)
        return &endpoint_descriptor;
    else
        return NULL;
}

BOOL METHOD(UHCI, Hidd_USBDevice, GetDeviceDescriptor)
{
    CopyMem(&device_descriptor, msg->descriptor, sizeof(device_descriptor));
    return TRUE;
}

BOOL METHOD(UHCI, Hidd_USBHub, GetHubDescriptor)
{
    CopyMem(&hub_descriptor, msg->descriptor, sizeof(hub_descriptor));
    msg->descriptor->wHubCharacteristics = AROS_WORD2LE(UHD_PWR_NO_SWITCH | UHD_OC_INDIVIDUAL);
    return TRUE;
}

BOOL METHOD(UHCI, Hidd_USBDevice, Configure)
{
    D(bug("[UHCI] ::Configure(%d)", msg->configNr));
    return TRUE;
}

APTR METHOD(UHCI, Hidd_USBDevice, CreatePipe)
{
    if ( (msg->type == PIPE_Interrupt) &&
         (msg->endpoint == 0x81))
        return (APTR)0xdeadbeef;
    else
        return NULL;
}

void METHOD(UHCI, Hidd_USBDevice, DeletePipe)
{
}

void METHOD(UHCI, Hidd_USBDevice, SetTimeout)
{
}

/* Class initialization and destruction */

#undef SD
#define SD(x) (&LIBBASE->sd)

static int UHCI_InitClass(LIBBASETYPEPTR LIBBASE)
{
    int i;
    D(bug("[UHCI] InitClass\n"));

    HiddUHCIAttrBase = OOP_ObtainAttrBase(IID_Drv_USB_UHCI);
    LIBBASE->sd.irq = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);

    if (HiddUHCIAttrBase)
    {
        struct TagItem tags[] = {
                { aHidd_UHCI_IOBase,		0UL },
                { aHidd_UHCI_PCIDevice,		0UL },
                { aHidd_UHCI_PCIDriver, 	0UL },
                { aHidd_USBDevice_Address, 	1UL },
                { aHidd_USBHub_IsRoot,		1UL },
                { aHidd_USBHub_NumPorts,	2UL },
                { TAG_DONE, 				0UL },
        };

        for (i=0; i < LIBBASE->sd.num_devices; i++)
        {
            tags[0].ti_Data = LIBBASE->sd.iobase[i];
            tags[1].ti_Data = (IPTR)LIBBASE->sd.uhciDevice[i];
            tags[2].ti_Data = (IPTR)LIBBASE->sd.uhciPCIDriver[i];
            LIBBASE->sd.uhciDevice[i] = OOP_NewObject(NULL, CLID_Drv_USB_UHCI, tags);
            HIDD_USB_AttachDriver(LIBBASE->sd.usb, LIBBASE->sd.uhciDevice[i]);
        }
    }

    return TRUE;
}

static int UHCI_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[UHCI] ExpungeClass\n"));

    OOP_ReleaseAttrBase(IID_Drv_USB_UHCI);

    return TRUE;
}

ADD2INITLIB(UHCI_InitClass, 0)
ADD2EXPUNGELIB(UHCI_ExpungeClass, 0)
