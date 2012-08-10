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
#include <oop/oop.h>

#include <devices/timer.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include <usb/usb.h>

#include <asm/io.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include "uhci.h"

//#define ENABLE_RECLAIM_BANDWIDTH    1

void uhci_sleep(OOP_Class *cl, OOP_Object *o, uint32_t msec)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);

    uhci->tr->tr_node.io_Command = TR_ADDREQUEST;
    uhci->tr->tr_time.tv_secs = msec / 1000;
    uhci->tr->tr_time.tv_micro = 1000 * (msec % 1000);

    uhci->tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

    DoIO((struct IORequest *)uhci->tr);
}

/*
 * This function allocates a new 16-byte Transfer Descriptor from the
 * pool of 4K-aligned PCI-accessible memory regions. Within each 4K page,
 * a bitmap is used to determine, which of the TD elements are available
 * for use.
 *
 * This function returns NULL if no free TD's are found and no more memory
 * is available.
 */
UHCI_TransferDesc *uhci_AllocTD(OOP_Class *cl, OOP_Object *o)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    struct TDNode *n;
    uint32_t node_num = 32;
    uint32_t bmp_pos = 8;

    Disable();

    /* Walk through the list of already available 4K pages */
    ForeachNode(&SD(cl)->td_list, n)
    {
        /*
         * For each 4K page, search the first free node (cleared bit) and alloc it.
         */
        for (bmp_pos=0; bmp_pos < 8; bmp_pos++)
        {
            if (n->td_Bitmap[bmp_pos] != 0xffffffff)
            {
                for (node_num = 0; node_num < 32; node_num++)
                {
                    if (!(n->td_Bitmap[bmp_pos] & (1 << node_num)))
                    {
                        UHCI_TransferDesc * td = (UHCI_TransferDesc *)&n->td_Page[(bmp_pos*32 + node_num) * 16];
                        /* Mark the TD as used and return a pointer to it */
                        n->td_Bitmap[bmp_pos] |= 1 << node_num;

                        Enable();

                        return td;
                    }
                }
            }
        }
    }

    /*
     * No free TDs have been found on the list of 4K pages. Create new Page node
     * and alloc 4K PCI accessible memory region for it
     */
    if ((n = AllocPooled(SD(cl)->MemPool, sizeof(struct TDNode))))
    {
        if ((n->td_Page = HIDD_PCIDriver_AllocPCIMem(uhci->pciDriver, 4096)))
        {
            /* Make 4K node available for future allocations */
            AddHead(&SD(cl)->td_list, (struct Node*)n);
            UHCI_TransferDesc * td = (UHCI_TransferDesc *)(UHCI_TransferDesc *)&n->td_Page[0];

            /* Mark first TD as used and return a pointer to it */
            n->td_Bitmap[0] |= 1;

            Enable();

            return td;
        }
        FreePooled(SD(cl)->MemPool, n, sizeof(struct TDNode));
    }

    Enable();

    /* Everything failed? Out of memory, most likely */
    return NULL;
}

/*
 * This function allocates a new 8-bit Queue Header aligned at the 16-byte boundary.
 * See uhci_AllocTD for more details.
 */
UHCI_QueueHeader *uhci_AllocQH(OOP_Class *cl, OOP_Object *o)
{
    /*
     * Since the Queue Headers have to be aligned at the 16-byte boundary, they may
     * be allocated from the same pool TD's do
     */
    return (UHCI_QueueHeader *)uhci_AllocTD(cl, o);
}

/*
 * Mark the Transfer Descriptor free, so that it may be allocated by another one.
 * A quick version which may be called from interrupts.
 */
void uhci_FreeTDQuick(UHCIData *uhci, UHCI_TransferDesc *td)
{
    struct TDNode *t;
    /* We're inside interrupt probably. Don't ObtainSemaphore. Just lock interrupts instead */
    Disable();

    /* traverse through the list of 4K pages */
    ForeachNode(&uhci->sd->td_list, t)
    {
        /* Address match? */
        if ((IPTR)t->td_Page == ((IPTR)td & ~0xfff))
        {
            /* extract the correct location of the TD within the bitmap */
            int bmp = (((IPTR)td & 0xe00) >> 9);
            int node = (((IPTR)td & 0x1f0) >> 4);

            /* Free the node */
            t->td_Bitmap[bmp] &= ~(1 << node);

            break;
        }
    }

    Enable();
}

void uhci_FreeQHQuick(UHCIData *uhci, UHCI_QueueHeader *qh)
{
    uhci_FreeTDQuick(uhci, (UHCI_TransferDesc *)qh);
}

/*
 * Mark the Transfer Descriptor free, so that it may be allocated by another one.
 * If the 4K page contains no used descriptors, the page will be freed
 */
void uhci_FreeTD(OOP_Class *cl, OOP_Object *o, UHCI_TransferDesc *td)
{
    struct TDNode *t, *next;
    UHCIData *uhci = OOP_INST_DATA(cl, o);

    Disable();

    /* traverse through the list of 4K pages */
    ForeachNodeSafe(&SD(cl)->td_list, t, next)
    {
        /* Address match? */
        if ((IPTR)t->td_Page == ((IPTR)td & ~0xfff))
        {
            /* extract the correct location of the TD within the bitmap */
            int bmp = (((IPTR)td & 0xe00) >> 9);
            int node = (((IPTR)td & 0x1f0) >> 4);

            /* Free the node */
            t->td_Bitmap[bmp] &= ~(1 << node);

            /* Check if all TD nodes are free within the 4K page */
            int i;
            for (i=0; i < 8; i++)
                if (t->td_Bitmap[i] != 0)
                    break;

            /* So it is. Free the 4K page */
            if (i==8)
            {
                Remove((struct Node*)t);
                HIDD_PCIDriver_FreePCIMem(uhci->pciDriver, t->td_Page);
                FreePooled(SD(cl)->MemPool, t, sizeof(struct TDNode));
            }

            break;
        }
    }
    Enable();
}

void uhci_FreeQH(OOP_Class *cl, OOP_Object *o, UHCI_QueueHeader *qh)
{
    uhci_FreeTD(cl, o, (UHCI_TransferDesc *)qh);
}

void uhci_globalreset(OOP_Class *cl, OOP_Object *o)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);

    outw(UHCI_CMD_GRESET, uhci->iobase + UHCI_CMD);
    uhci_sleep(cl, o, USB_BUS_RESET_DELAY);
    outw(0, uhci->iobase + UHCI_CMD);
}

void uhci_reset(OOP_Class *cl, OOP_Object *o)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    int n;

    outw(UHCI_CMD_HCRESET, uhci->iobase + UHCI_CMD);
    for (n=0; n < UHCI_RESET_TIMEOUT && (inw(uhci->iobase + UHCI_CMD) & UHCI_CMD_HCRESET); n++)
        uhci_sleep(cl, o, 10);

    if (n >= UHCI_RESET_TIMEOUT)
        D(bug("[UHCI] Device did not reset properly\n"));
}

BOOL uhci_run(OOP_Class *cl, OOP_Object *o, BOOL run)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    BOOL running;
    uint16_t cmd;
    int n;

    cmd = inw(uhci->iobase + UHCI_CMD);
    if (run)
        cmd |= UHCI_CMD_RS;
    else
        cmd &= ~UHCI_CMD_RS;

    outw(cmd, uhci->iobase + UHCI_CMD);

    for (n=0; n < 10; n++)
    {
        running = !(inw(uhci->iobase + UHCI_STS) & UHCI_STS_HCH);

        if (run == running)
        {
            D(bug("[UHCI] %s done. cmd=%04x, sts=%04x\n",
                    run ? "start":"stop", inw(uhci->iobase + UHCI_CMD),
                            inw(uhci->iobase + UHCI_STS)));

            return TRUE;
        }

        uhci_sleep(cl, o, 10);
    }

    D(bug("[UHCI] Failed to change the running state\n"));
    return FALSE;
}

void uhci_RebuildList(OOP_Class *cl, OOP_Object *o)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    UHCI_Pipe *p;
    UHCI_QueueHeader *qh, *first_fast = NULL;

    qh = uhci->qh01;

    /*
     * If low speed control list is not empty, iterate through all its elements
     * and link them linear using the qh pointer
     */
    if (!IsListEmpty(&uhci->ControlLS))
    {
        ForeachNode(&uhci->ControlLS, p)
        {

            /* The successor QH's horizontal link should point to end of the list
             * as long as the node is not linked properly. This way we guarantee,
             * that the list of QH's may be always executed */

            p->p_Queue->qh_HLink = UHCI_PTR_T;

            qh->qh_HLink = (uint32_t)p->p_Queue | UHCI_PTR_QH;
            qh = (UHCI_QueueHeader *)p->p_Queue;
        }
    }

    if (!IsListEmpty(&uhci->ControlFS))
    {
        first_fast = (UHCI_QueueHeader *)((UHCI_Pipe *)(GetHead(&uhci->ControlFS)))->p_Queue;

        ForeachNode(&uhci->ControlFS, p)
        {
            p->p_Queue->qh_HLink = UHCI_PTR_T;
            qh->qh_HLink = (uint32_t)p->p_Queue | UHCI_PTR_QH;
            qh = (UHCI_QueueHeader *)p->p_Queue;
        }
    }

    if (!IsListEmpty(&uhci->Bulk))
    {
        if (!first_fast)
            first_fast = (UHCI_QueueHeader *)((UHCI_Pipe *)(GetHead(&uhci->Bulk)))->p_Queue;

            ForeachNode(&uhci->Bulk, p)
            {
                p->p_Queue->qh_HLink = UHCI_PTR_T;
                qh->qh_HLink = (uint32_t)p->p_Queue | UHCI_PTR_QH;
                qh = (UHCI_QueueHeader *)p->p_Queue;
            }
    }

    /*
     * If the first fast node exists, make a loop of FullSpeed control and bulk queue
     * headers, in order to reclaim the bandwidth (since all bulk QH's are executed by
     * bredth first).
     */
#ifdef ENABLE_RECLAIM_BANDWIDTH
    if (first_fast)
    {
        qh->qh_HLink = (uint32_t)first_fast | UHCI_PTR_QH;
    }
    else
    {
        qh->qh_HLink = UHCI_PTR_T;
    }
#else
    qh->qh_HLink = UHCI_PTR_T;
#endif
}

void uhci_DeletePipe(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe)
{
    D(bug("[UHCI] DeletePipe(%p)\n", pipe));

    if (pipe)
    {
        UHCI_TransferDesc *td = (UHCI_TransferDesc *)pipe->p_FirstTD;

        /* Lock interrupts */
        Disable();
        /* Remove the pipe from transfer list and rebuild the lists */
        Remove((struct Node*)pipe);
        uhci_RebuildList(cl, o);
        Enable();

        /* FIXME: Don't delay. Just wait for the incomplete TD's */
        //Delay(1);

        /* At this stage the transfer nodes may be safely deleted from the queue */
        while ((uint32_t)td != UHCI_PTR_T)
        {
            UHCI_TransferDesc *tnext = (UHCI_TransferDesc *)(td->td_LinkPtr & 0xfffffff1);
            uhci_FreeTD(cl, o, td);
            td = tnext;
        }

        pipe->p_Timeout->tr_node.io_Message.mn_ReplyPort->mp_SigBit = AllocSignal(-1);
        CloseDevice((struct IORequest *)pipe->p_Timeout);
        DeleteMsgPort(pipe->p_Timeout->tr_node.io_Message.mn_ReplyPort);
        DeleteIORequest((struct IORequest *)pipe->p_Timeout);

        FreePooled(SD(cl)->MemPool, pipe, sizeof(UHCI_Pipe));
    }
}

void uhci_InsertIntr(UHCIData *uhci, UHCI_Pipe *pipe)
{
    UHCI_QueueHeader **q;
    uint8_t          count, i, minimum, bestat;

    (bug("[UHCI] Inserting Interrupt queue %p with period %d\n", pipe, pipe->p_Interval));

    if (pipe->p_Interval < 2)
    {
        q = &uhci->qh01;
        count = 1;
    }
    else if (pipe->p_Interval < 4)
    {
        q = uhci->qh02;
        count = 2;
    }
    else if (pipe->p_Interval < 8)
    {
        q = uhci->qh04;
        count = 4;
    }
    else if (pipe->p_Interval < 16)
    {
        q = uhci->qh08;
        count = 8;
    }
    else if (pipe->p_Interval < 32)
    {
        q = uhci->qh16;
        count = 16;
    }
    else
    {
        q = uhci->qh32;
        count = 32;
    }

    i=0;
    minimum = q[0]->qh_Data2;
    bestat = 0;
    do {
        if (minimum > q[i]->qh_Data2)
        {
            minimum = q[i]->qh_Data2;
            bestat = i;
        }
    } while (++i < count);

    (bug("[UHCI] Best node (%d uses) %d from %d\n", minimum, bestat, count));

    pipe->p_QHNode = bestat;
    pipe->p_QHLocation = count;

    q[bestat]->qh_Data2++;

    pipe->p_Queue->qh_VLink = q[bestat]->qh_VLink;
    pipe->p_Queue->qh_HLink = q[bestat]->qh_HLink;
    q[bestat]->qh_VLink = ((uint32_t)pipe->p_Queue) | UHCI_PTR_QH;
}

UHCI_Pipe *uhci_CreatePipe(OOP_Class *cl, OOP_Object *o, enum USB_PipeType type, BOOL fullspeed,
        uint8_t addr, uint8_t endp, uint8_t period, uint32_t maxp, uint32_t timeout)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    UHCI_Pipe *pipe;

    pipe = AllocPooled(SD(cl)->MemPool, sizeof(UHCI_Pipe));

    (bug("[UHCI] CreatePipe(%d, %d, %d, %d, %d)\n",
            type, fullspeed, addr, endp, maxp));

    if (pipe)
    {
        pipe->p_Queue = uhci_AllocQH(cl, o);

        pipe->p_Type = type;
        pipe->p_FullSpeed = fullspeed;
        pipe->p_NextToggle = 0;
        pipe->p_DevAddr = addr;
        pipe->p_EndPoint = endp;
        pipe->p_MaxTransfer = maxp;
        pipe->p_Interval = period;
        pipe->p_FirstTD = (APTR)UHCI_PTR_T;
        pipe->p_LastTD = (APTR)UHCI_PTR_T;
        pipe->p_Queue->qh_VLink = UHCI_PTR_T;
        pipe->p_Queue->qh_Data1 = (IPTR)pipe;
        pipe->p_Queue->qh_Data2 = 0;

        NEWLIST(&pipe->p_Intr);

        pipe->p_TimeoutVal = timeout;
        pipe->p_Timeout = (struct timerequest *)CreateIORequest(
                CreateMsgPort(), sizeof(struct timerequest));

        FreeSignal(pipe->p_Timeout->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
        OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)pipe->p_Timeout, 0);

        /* According to the pipe's type, add it to proper list */
        switch (type)
        {
            case PIPE_Bulk:
                AddTail((struct List*)&uhci->Bulk, (struct Node*)pipe);
                break;

            case PIPE_Control:
                if (fullspeed)
                    AddTail((struct List*)&uhci->ControlFS, (struct Node*)pipe);
                else
                    AddTail((struct List*)&uhci->ControlLS, (struct Node*)pipe);
                break;

            case PIPE_Isochronous:
                AddTail((struct List*)&uhci->Isochronous, (struct Node*)pipe);
                break;

            case PIPE_Interrupt:
                uhci_InsertIntr(uhci, pipe);
                AddTail((struct List*)&uhci->Interrupts, (struct Node*)pipe);
                break;
        }

        uhci_RebuildList(cl, o);
    }

    return pipe;
}

void uhci_QueuedTransfer(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe, VOID *buffer, uint32_t length, BOOL in)
{
    uint8_t *ptr = (uint8_t*)buffer;
    UHCI_TransferDesc *td=NULL, *t=NULL;

    uint8_t dt = pipe->p_NextToggle;
    uint32_t ntd = (length + pipe->p_MaxTransfer - 1) / pipe->p_MaxTransfer;

    if (ntd % 2 == 0)
        dt ^= 1;

    pipe->p_NextToggle = dt ^ 1;

    while (ntd--)
    {
        uint32_t len = (length > pipe->p_MaxTransfer) ? pipe->p_MaxTransfer : length;

        if (td)
        {
            UHCI_TransferDesc *tmp = uhci_AllocTD(cl, o);
            td->td_LinkPtr = (uint32_t)tmp | UHCI_PTR_TD;
            td = tmp;
        }
        else
        {
            td = t = uhci_AllocTD(cl, o);
        }

        td->td_Buffer = (uint32_t)ptr;
        td->td_LinkPtr = UHCI_PTR_T;
        td->td_Status = UHCI_TD_ZERO_ACTLEN(UHCI_TD_SET_ERRCNT(3) | UHCI_TD_ACTIVE);
        if (in)
            td->td_Token = UHCI_TD_IN(len, pipe->p_EndPoint, pipe->p_DevAddr, dt);
        else
            td->td_Token = UHCI_TD_OUT(len, pipe->p_EndPoint, pipe->p_DevAddr, dt);

        if (pipe->p_FullSpeed)
            td->td_Status |= UHCI_TD_SPD;
        else
            td->td_Status |= UHCI_TD_LS | UHCI_TD_SPD;

        dt ^= 1;
        ptr += len;

		D(bug("[UHCI]     TD=%p (%08x %08x %08x %08x)\n", td,
				td->td_LinkPtr, td->td_Status, td->td_Token, td->td_Buffer));
    }

    td->td_Status |= UHCI_TD_IOC;

    Disable();
    if (pipe->p_FirstTD == (APTR)UHCI_PTR_T)
    {
        pipe->p_FirstTD = t;
        pipe->p_LastTD = td;
    }
    else
    {
        pipe->p_LastTD->td_LinkPtr = (uint32_t)t | UHCI_PTR_TD;
        pipe->p_LastTD = td;
    }

    if (pipe->p_Queue->qh_VLink == UHCI_PTR_T)
    {
        pipe->p_Queue->qh_VLink = (uint32_t)t | UHCI_PTR_TD;
    }
    Enable();
}

void uhci_QueuedWrite(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe, VOID *buffer, uint32_t length)
{
    uhci_QueuedTransfer(cl, o, pipe, buffer, length, FALSE);
}

void uhci_QueuedRead(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe, VOID *buffer, uint32_t length)
{
    uhci_QueuedTransfer(cl, o, pipe, buffer, length, TRUE);
}

void uhci_ControlTransfer(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe,
        USBDevice_Request *request, VOID *buffer, uint32_t length)
{
    UHCI_TransferDesc *req, *td=NULL, *stat;
    BOOL isread = request->bmRequestType & UT_READ;
    uint32_t len = request->wLength;

    D(bug("[UHCI] ControlTransfer %08x(%02x %02x %04x %04x %04x) %08x %d\n",
            request, request->bmRequestType, request->bRequest, request->wValue, request->wIndex,
            request->wLength, buffer, length));

    if (length <= 1023)
    {
        req = uhci_AllocTD(cl, o);
        req->td_Token = UHCI_TD_SETUP(sizeof(USBDevice_Request), pipe->p_EndPoint, pipe->p_DevAddr);
        req->td_Buffer = (uint32_t)request;
        req->td_Status = UHCI_TD_ZERO_ACTLEN(UHCI_TD_SET_ERRCNT(3) | UHCI_TD_ACTIVE);

        if (!pipe->p_FullSpeed)
            req->td_Status |= UHCI_TD_LS;

        stat = uhci_AllocTD(cl, o);
        stat->td_LinkPtr = UHCI_PTR_T;
        stat->td_Status = UHCI_TD_ZERO_ACTLEN(UHCI_TD_SET_ERRCNT(3) | UHCI_TD_ACTIVE | UHCI_TD_IOC);
        stat->td_Buffer = 0;
        stat->td_Token = isread ? UHCI_TD_OUT(0, pipe->p_EndPoint, pipe->p_DevAddr, 1)
                : UHCI_TD_IN (0, pipe->p_EndPoint, pipe->p_DevAddr, 1);

        if (!pipe->p_FullSpeed)
            stat->td_Status |= UHCI_TD_LS;

        if (buffer)
        {
            uint8_t d = 1;
            UHCI_TransferDesc *prev = req;
            uint8_t *buff = buffer;
            length = len;

            while (length > 0)
            {
                len = (length > pipe->p_MaxTransfer) ? pipe->p_MaxTransfer : length;

                td = uhci_AllocTD(cl, o);

                td->td_Token = isread ? UHCI_TD_IN (len, pipe->p_EndPoint, pipe->p_DevAddr, d)
                    : UHCI_TD_OUT(len, pipe->p_EndPoint, pipe->p_DevAddr, d);
                td->td_Status = UHCI_TD_ZERO_ACTLEN(UHCI_TD_SET_ERRCNT(3) | UHCI_TD_ACTIVE  | UHCI_TD_SPD);
                td->td_Buffer = (uint32_t)buff;

                if (!pipe->p_FullSpeed)
                    td->td_Status |= UHCI_TD_LS;

                td->td_Status |= UHCI_TD_SPD;
                prev->td_LinkPtr = (uint32_t)td | UHCI_PTR_TD | UHCI_PTR_VF;

                d ^= 1;
                prev = td;
                buff += len;
                length -= len;
            }

            td->td_LinkPtr = (uint32_t)stat | UHCI_PTR_TD | UHCI_PTR_VF;
        }
        else
        {
            req->td_LinkPtr = (uint32_t)stat | UHCI_PTR_TD | UHCI_PTR_VF;
        }

        D(
                UHCI_TransferDesc *t = req;

                while ((uint32_t)t != UHCI_PTR_T)
                {
                    bug("[UHCI]     TD=%p (%08x %08x %08x %08x)\n", t,
                            t->td_LinkPtr, t->td_Status, t->td_Token, t->td_Buffer);
                    t = (UHCI_TransferDesc *)(t->td_LinkPtr & 0xfffffff1);
                }
        );

        Disable();
        if (pipe->p_FirstTD == (APTR)UHCI_PTR_T)
        {
            pipe->p_FirstTD = req;
            pipe->p_LastTD = stat;
        }
        else
        {
            pipe->p_LastTD->td_LinkPtr = (uint32_t)req | UHCI_PTR_TD;
            pipe->p_LastTD = stat;
        }

        if (pipe->p_Queue->qh_VLink == UHCI_PTR_T)
        {
            pipe->p_Queue->qh_VLink = (uint32_t)req | UHCI_PTR_TD;
        }
        Enable();
    }
}

BOOL uhci_PortReset(OOP_Class *cl, OOP_Object *o, uint8_t p)
{
    UHCIData *uhci = OOP_INST_DATA(cl, o);
    int lim, port, x;

    if (p == 1)
        port = UHCI_PORTSC1;
    else if (p == 2)
        port = UHCI_PORTSC2;
    else
        return FALSE;

    x = URWMASK(inw(uhci->iobase + port));
    outw(x | UHCI_PORTSC_PR, uhci->iobase + port);

    uhci_sleep(cl, o, 100);

    x = URWMASK(inw(uhci->iobase + port));
    outw(x & ~UHCI_PORTSC_PR, uhci->iobase + port);

    uhci_sleep(cl, o, 100);

    x = URWMASK(inw(uhci->iobase + port));
    outw(x | UHCI_PORTSC_PE, uhci->iobase + port);

    for (lim = 10; --lim > 0;)
    {
        uhci_sleep(cl, o, 10);

        x = inw(uhci->iobase + port);

        if (!(x & UHCI_PORTSC_CCS))
            break;

        if (x & (UHCI_PORTSC_POEDC | UHCI_PORTSC_CSC))
        {
            outw(URWMASK(x) | (x & (UHCI_PORTSC_POEDC | UHCI_PORTSC_CSC)), uhci->iobase + port);
            continue;
        }

        if (x & UHCI_PORTSC_PE)
            break;

        outw(URWMASK(x) | UHCI_PORTSC_PE, uhci->iobase + port);

    }

    if (lim <= 0)
    {
        D(bug("[UHCI] Port reset timeout\n"));
        return FALSE;
    }

    uhci->reset |= 1 << (p-1);

    return TRUE;
}
