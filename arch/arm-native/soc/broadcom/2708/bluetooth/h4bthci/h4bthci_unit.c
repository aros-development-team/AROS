/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: H4 framing and the unit task.
*/

#include <aros/debug.h>
#include <exec/exec.h>
#include <exec/errors.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/pl011bt.h>
#include <bluetooth/hci.h>

#include <string.h>

#include "h4bthci_intern.h"

/* proto/pl011bt.h declares this as APTR for a resource, not a library base --
 * a resource is not opened with OpenLibrary and has no library node. The unit
 * task fills it in; it is the only thing here that touches the hardware. */
APTR PL011BTBase;

/*
 * A UART hands over bytes; HCI is packets. Everything in this file exists to
 * bridge that, and the H4 encoding is the whole of the bridge: one type byte,
 * then a header whose length field says how much more to wait for.
 *
 * The lengths differ per type and there is no generic answer:
 *   EVENT  1 byte type,  1 byte length
 *   ACL    2 byte handle, 2 byte length (little endian on the wire)
 *   SCO    2 byte handle, 1 byte length
 * so the reassembler has to know all three to know when a packet is whole.
 */
static ULONG h4bthci_PacketLength(const UBYTE *buf, ULONG have)
{
    switch (buf[0])
    {
    case H4BTHCI_EVENT:
        if (have < 3)
            return 0;
        return 3 + buf[2];

    case H4BTHCI_ACL:
        if (have < 5)
            return 0;
        return 5 + buf[3] + (((ULONG)buf[4]) << 8);

    case H4BTHCI_SCO:
        if (have < 4)
            return 0;
        return 4 + buf[3];

    default:
        /*
         * Not a type this transport knows. Resynchronising by guessing where
         * the next packet starts would invent data; dropping the byte and
         * saying so is the only honest recovery, and a controller that does
         * this is broken in a way worth seeing in the log.
         */
        bug("[h4bthci] unknown H4 packet type 0x%02x, dropping a byte\n",
            buf[0]);
        return (ULONG)-1;
    }
}

/* Hand a complete event to whoever is waiting, and to every listener. */
static void h4bthci_DeliverEvent(struct H4BTHCIUnit *unit,
                                   const UBYTE *packet, ULONG length)
{
    struct IOBTHCIReq *ioreq = NULL;
    struct MinNode *mn;

    ObtainSemaphore(&unit->hu_QueueLock);
    if (unit->hu_EventQueue.mlh_Head->mln_Succ)
        ioreq = (struct IOBTHCIReq *)RemHead((struct List *)&unit->hu_EventQueue);
    ReleaseSemaphore(&unit->hu_QueueLock);

    if (ioreq)
    {
        ULONG payload = length - 1;     /* without the H4 type byte */

        if (payload > ioreq->iobt_Length)
        {
            ioreq->iobt_Actual = ioreq->iobt_Length;
            ioreq->iobt_Req.io_Error = BTIOERR_OVERFLOW;
        }
        else
        {
            ioreq->iobt_Actual = payload;
            ioreq->iobt_Req.io_Error = 0;
        }
        if (ioreq->iobt_Data && ioreq->iobt_Actual)
            CopyMem((APTR)(packet + 1), ioreq->iobt_Data, ioreq->iobt_Actual);

        ReplyMsg(&ioreq->iobt_Req.io_Message);
    }

    /*
     * Listeners registered through BTCMD_ADDMSGPORT get a copy. They are not
     * consumers in the request sense -- the stack uses them to watch events it
     * did not ask for -- so a missing listener is not an error and a listener
     * that cannot be allocated for is dropped rather than blocking the wire.
     */
    ObtainSemaphore(&unit->hu_QueueLock);
    for (mn = unit->hu_Listeners.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
        struct H4BTHCIListener *listener = (struct H4BTHCIListener *)mn;
        struct BTHCIEventMsg *msg;

        msg = AllocVec(sizeof(struct BTHCIEventMsg), MEMF_PUBLIC | MEMF_CLEAR);
        if (!msg)
            continue;

        msg->bem_Msg.mn_Node.ln_Type = NT_MESSAGE;
        msg->bem_Msg.mn_Length = sizeof(struct BTHCIEventMsg);
        CopyMem((APTR)(packet + 1), &msg->bem_Event,
                (length - 1) > sizeof(struct BTHCIEvent)
                    ? sizeof(struct BTHCIEvent) : (length - 1));
        PutMsg(listener->hl_Port, &msg->bem_Msg);
    }
    ReleaseSemaphore(&unit->hu_QueueLock);
}

static void h4bthci_DeliverACL(struct H4BTHCIUnit *unit,
                                 const UBYTE *packet, ULONG length)
{
    struct IOBTHCIReq *ioreq = NULL;

    ObtainSemaphore(&unit->hu_QueueLock);
    if (unit->hu_ACLQueue.mlh_Head->mln_Succ)
        ioreq = (struct IOBTHCIReq *)RemHead((struct List *)&unit->hu_ACLQueue);
    ReleaseSemaphore(&unit->hu_QueueLock);

    if (!ioreq)
        return;         /* nothing waiting: the packet is dropped, as USB does */

    {
        ULONG payload = length - 1;

        if (payload > ioreq->iobt_Length)
        {
            ioreq->iobt_Actual = ioreq->iobt_Length;
            ioreq->iobt_Req.io_Error = BTIOERR_OVERFLOW;
        }
        else
        {
            ioreq->iobt_Actual = payload;
            ioreq->iobt_Req.io_Error = 0;
        }
        if (ioreq->iobt_Data && ioreq->iobt_Actual)
            CopyMem((APTR)(packet + 1), ioreq->iobt_Data, ioreq->iobt_Actual);
    }

    ReplyMsg(&ioreq->iobt_Req.io_Message);
}

static void h4bthci_Reassemble(struct H4BTHCIUnit *unit)
{
    while (unit->hu_RXLen > 0)
    {
        ULONG want = h4bthci_PacketLength(unit->hu_RX, unit->hu_RXLen);

        if (want == (ULONG)-1)
        {
            memmove(unit->hu_RX, unit->hu_RX + 1, --unit->hu_RXLen);
            continue;
        }
        if (want == 0 || want > unit->hu_RXLen)
            break;                      /* incomplete: wait for more bytes */

        switch (unit->hu_RX[0])
        {
        case H4BTHCI_EVENT:
            h4bthci_DeliverEvent(unit, unit->hu_RX, want);
            break;
        case H4BTHCI_ACL:
            h4bthci_DeliverACL(unit, unit->hu_RX, want);
            break;
        default:
            break;                      /* SCO is accepted and discarded */
        }

        unit->hu_RXLen -= want;
        if (unit->hu_RXLen)
            memmove(unit->hu_RX, unit->hu_RX + want, unit->hu_RXLen);
    }
}

/*
 * Send one packet, type byte first.
 *
 * PL011BTWrite() returns the number of bytes it accepted, not a status: a
 * successful one-byte write returns 1. This compared that against PL011BT_OK,
 * which is 0, so every successful write was read as a failure and every HCI
 * command came back as "HCI transmission failed, host error (3)" -- the
 * transport being up made that look like the radio not answering.
 *
 * It is also a non-blocking write that stops at TXFF, so a full FIFO means a
 * short write rather than an error. The caller has to push the remainder,
 * which nothing did.
 */
static LONG h4bthci_WriteAll(struct H4BTHCIUnit *unit, const UBYTE *data,
                               ULONG length)
{
    ULONG done = 0;
    ULONG spins = 0;

    while (done < length)
    {
        LONG n = PL011BTWrite(unit, data + done, length - done);

        if (n < 0)
        {
            bug("[h4bthci] PL011BTWrite failed, rc=%ld\n", (LONG)n);
            return BTIOERR_HOSTERROR;
        }
        if (n == 0)
        {
            /* FIFO full: give it room rather than spinning forever. */
            if (++spins > 100000)
            {
                bug("[h4bthci] transmit FIFO stayed full, %lu of %lu sent\n",
                    done, length);
                return BTIOERR_HOSTERROR;
            }
            continue;
        }
        spins = 0;
        done += (ULONG)n;
    }
    return 0;
}

static LONG h4bthci_Send(struct H4BTHCIUnit *unit, UBYTE type,
                           const UBYTE *data, ULONG length)
{
    UBYTE hdr = type;
    LONG err = h4bthci_WriteAll(unit, &hdr, 1);

    if (err)
        return err;
    if (length)
        return h4bthci_WriteAll(unit, data, length);

    return 0;
}

LONG h4bthci_QueueRequest(struct H4BTHCIUnit *unit, struct IOBTHCIReq *ioreq)
{
    switch (ioreq->iobt_Req.io_Command)
    {
    case BTCMD_QUERYDEVICE:
    {
        struct TagItem *tags = (struct TagItem *)ioreq->iobt_Data;
        struct TagItem *tag;

        while (tags && (tag = NextTagItem(&tags)))
        {
            switch (tag->ti_Tag)
            {
            case BTA_Author:
                *((STRPTR *)tag->ti_Data) = "The AROS Development Team";
                break;
            case BTA_ProductName:
                *((STRPTR *)tag->ti_Data) = "Raspberry Pi onboard Bluetooth";
                break;
            case BTA_Description:
                *((STRPTR *)tag->ti_Data) =
                    "H4 HCI transport over the BCM283x PL011";
                break;
            case BTA_Copyright:
                *((STRPTR *)tag->ti_Data) = "(C) 2026 The AROS Development Team";
                break;
            case BTA_Version:
                *((ULONG *)tag->ti_Data) = 45;
                break;
            case BTA_Revision:
                *((ULONG *)tag->ti_Data) = 1;
                break;
            case BTA_DriverVersion:
                *((ULONG *)tag->ti_Data) = 1;
                break;
            }
        }
        ioreq->iobt_Req.io_Error = 0;
        return 0;
    }

    case BTCMD_WRITEHCI:
        ioreq->iobt_Req.io_Error =
            h4bthci_Send(unit, H4BTHCI_COMMAND,
                           ioreq->iobt_Data, ioreq->iobt_Length);
        ioreq->iobt_Actual = ioreq->iobt_Req.io_Error ? 0 : ioreq->iobt_Length;
        return 0;

    case BTCMD_WRITEACL:
        ioreq->iobt_Req.io_Error =
            h4bthci_Send(unit, H4BTHCI_ACL,
                           ioreq->iobt_Data, ioreq->iobt_Length);
        ioreq->iobt_Actual = ioreq->iobt_Req.io_Error ? 0 : ioreq->iobt_Length;
        return 0;

    case BTCMD_READEVENT:
        ObtainSemaphore(&unit->hu_QueueLock);
        AddTail((struct List *)&unit->hu_EventQueue,
                (struct Node *)&ioreq->iobt_Req.io_Message.mn_Node);
        ReleaseSemaphore(&unit->hu_QueueLock);
        return -1;                      /* queued; the unit task replies */

    case BTCMD_READACL:
        ObtainSemaphore(&unit->hu_QueueLock);
        AddTail((struct List *)&unit->hu_ACLQueue,
                (struct Node *)&ioreq->iobt_Req.io_Message.mn_Node);
        ReleaseSemaphore(&unit->hu_QueueLock);
        return -1;

    case BTCMD_ADDMSGPORT:
    {
        struct H4BTHCIListener *listener =
            AllocVec(sizeof(struct H4BTHCIListener), MEMF_PUBLIC | MEMF_CLEAR);

        if (!listener)
        {
            ioreq->iobt_Req.io_Error = BTIOERR_OUTOFMEMORY;
            return 0;
        }
        listener->hl_Port = (struct MsgPort *)ioreq->iobt_Data;
        ObtainSemaphore(&unit->hu_QueueLock);
        AddTail((struct List *)&unit->hu_Listeners, (struct Node *)listener);
        ReleaseSemaphore(&unit->hu_QueueLock);
        ioreq->iobt_Req.io_Error = 0;
        return 0;
    }

    case BTCMD_REMMSGPORT:
    {
        struct MinNode *mn;

        ObtainSemaphore(&unit->hu_QueueLock);
        for (mn = unit->hu_Listeners.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
        {
            struct H4BTHCIListener *listener = (struct H4BTHCIListener *)mn;

            if (listener->hl_Port == (struct MsgPort *)ioreq->iobt_Data)
            {
                Remove((struct Node *)mn);
                FreeVec(listener);
                break;
            }
        }
        ReleaseSemaphore(&unit->hu_QueueLock);
        ioreq->iobt_Req.io_Error = 0;
        return 0;
    }

    /*
     * SCO is not wired. The onboard controller supports it, but routing audio
     * needs a PCM path this port does not have, and answering the request as
     * though it worked would be worse than refusing it.
     */
    case BTCMD_SETUPSCO:
    case BTCMD_READSCO:
    case BTCMD_WRITESCO:
        ioreq->iobt_Req.io_Error = IOERR_NOCMD;
        return 0;

    default:
        ioreq->iobt_Req.io_Error = IOERR_NOCMD;
        return 0;
    }
}

void h4bthci_UnitTask(void)
{
    struct Task *self = FindTask(NULL);
    struct H4BTHCIUnit *unit = (struct H4BTHCIUnit *)
        ((struct Process *)self)->pr_Task.tc_UserData;
    struct Task *waiter;

    /*
     * Say which step failed.
     *
     * There are four of them and none of them said anything, so a failure
     * arrived at AddBTHardware as the single word "failed" -- with the
     * resource itself reporting "[PL011BT] ready" a moment earlier, which
     * makes the two impossible to tell apart from a log. Each step now names
     * itself and its return code.
     */
    PL011BTBase = OpenResource("pl011bt.resource");
    if (!PL011BTBase)
        bug("[h4bthci] pl011bt.resource not available\n");
    else
    {
        LONG rc = PL011BTClaim(unit);

        if (rc != PL011BT_OK)
            bug("[h4bthci] PL011BTClaim failed, rc=%ld\n", (LONG)rc);
        else
        {
            rc = PL011BTSetPower(unit, 1);
            if (rc != PL011BT_OK)
                bug("[h4bthci] PL011BTSetPower failed, rc=%ld\n", (LONG)rc);
            else
            {
                rc = PL011BTConfigure(unit, H4BTHCI_BAUD,
                        PL011BT_CONFIG_RTS_CTS);
                if (rc != PL011BT_OK)
                    bug("[h4bthci] PL011BTConfigure(%lu baud) failed,"
                        " rc=%ld\n", (ULONG)H4BTHCI_BAUD, (LONG)rc);
                else
                {
                    bug("[h4bthci] transport up at %lu baud\n",
                        (ULONG)H4BTHCI_BAUD);
                    unit->hu_Task = self;
                }
            }
            if (!unit->hu_Task)
                PL011BTRelease(unit);
        }
    }

    /* Tell the opener whether there is a controller, either way. */
    Forbid();
    waiter = unit->hu_ReadySigTask;
    if (waiter)
        Signal(waiter, 1L << unit->hu_ReadySignal);
    Permit();

    if (!unit->hu_Task)
        return;

    /* %lu, not %u: kprintf fetches a %u as a 16-bit UWORD, and 115200
     * does not fit in one -- it would report 49664 baud. */
    bug("[h4bthci] claimed the PL011 at %lu baud\n",
        (ULONG)H4BTHCI_BAUD);

    /*
     * Polled, and that is a known cost rather than a design.
     * pl011bt.resource's capability bits say whether RX and TX interrupts are
     * available (PL011BT_CAP_RX_INTERRUPT), and they are zero until that path
     * is proven -- so until then the honest thing is a poll with a delay
     * rather than a wait that would never be signalled.
     */
    for (;;)
    {
        LONG got;

        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
            break;

        got = PL011BTRead(unit, unit->hu_RX + unit->hu_RXLen,
                         H4BTHCI_RXBUF - unit->hu_RXLen);
        if (got > 0)
        {
            unit->hu_RXLen += got;
            h4bthci_Reassemble(unit);
        }
        else
            Delay(1);
    }

    PL011BTSetPower(unit, 0);
    PL011BTRelease(unit);

    Forbid();
    unit->hu_Task = NULL;
    waiter = unit->hu_ReadySigTask;
    if (waiter)
        Signal(waiter, 1L << unit->hu_ReadySignal);
    Permit();
}
