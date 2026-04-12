/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA.
*/

/*
 * LPe32002 Unit Management
 *
 * Implements unit creation/deletion, the unit scheduler task,
 * TX/RX interrupt handlers, and utility functions for the SANA2
 * framework (event reporting, packet copying, address filtering,
 * multicast range management).
 */

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>

#include <aros/io.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>
#include <devices/timer.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/timer.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lpe32002.h"
#include "lpe32002_hw.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/* ============================================================
 * Event Reporting
 * ============================================================ */

VOID ReportEvents(struct LPe32002Base *LPeBase, struct LPe32002Unit *unit, ULONG events)
{
    struct IOSana2Req *request, *tail, *next_request;
    struct List *list;

    list = &unit->lpeu_request_ports[EVENT_QUEUE]->mp_MsgList;
    next_request = (APTR)list->lh_Head;
    tail = (APTR)&list->lh_Tail;

    Disable();
    while(next_request != tail)
    {
        request = next_request;
        next_request = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

        if((request->ios2_WireError & events) != 0)
        {
            request->ios2_WireError = events;
            Remove((APTR)request);
            ReplyMsg((APTR)request);
        }
    }
    Enable();
}

/* ============================================================
 * Type Statistics Lookup
 * ============================================================ */

struct TypeStats *FindTypeStats(struct LPe32002Base *LPeBase, struct LPe32002Unit *unit,
                    struct MinList *list, ULONG packet_type)
{
    struct TypeStats *stats, *tail;
    BOOL found = FALSE;

    stats = (APTR)list->mlh_Head;
    tail = (APTR)&list->mlh_Tail;

    while(stats != tail && !found)
    {
        if(stats->packet_type == packet_type)
            found = TRUE;
        else
            stats = (APTR)stats->node.mln_Succ;
    }

    if(!found)
        stats = NULL;

    return stats;
}

/* ============================================================
 * Flush Unit
 * ============================================================ */

void FlushUnit(LIBBASETYPEPTR LIBBASE, struct LPe32002Unit *unit, UBYTE last_queue, BYTE error)
{
    struct IORequest *request;
    UBYTE i;
    struct Opener *opener, *tail;

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    for (i = 0; i <= last_queue; i++)
    {
        while ((request = (APTR)GetMsg(unit->lpeu_request_ports[i])) != NULL)
        {
            request->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *)request);
        }
    }

    opener = (APTR)unit->lpeu_Openers.mlh_Head;
    tail = (APTR)&unit->lpeu_Openers.mlh_Tail;

    while(opener != tail)
    {
        while ((request = (APTR)GetMsg(&opener->read_port)) != NULL)
        {
            request->io_Error = error;
            ReplyMsg((struct Message *)request);
        }
        opener = (struct Opener *)opener->node.mln_Succ;
    }
}

/* ============================================================
 * TX Interrupt (Cause'd softint to push packets to HW)
 * ============================================================ */

static AROS_INTH1(lpe_TX_Int, struct LPe32002Unit *, unit)
{
    AROS_INTFUNC_INIT

    struct LPe32002Base *LPeBase = unit->lpeu_device;
    LIBBASETYPEPTR LIBBASE = LPeBase;
    struct fc_eth_frame *frame;
    struct IOSana2Req *request;
    struct Opener *opener;
    struct MsgPort *port;
    struct TypeStats *tracker;
    UWORD packet_size, data_size;
    UBYTE *buffer;
    BYTE error;
    ULONG wire_error = 0;
    BOOL proceed = TRUE;

    D(bug("[%s] ## %s()\n", unit->lpeu_name, __func__));

    port = unit->lpeu_request_ports[WRITE_QUEUE];

    while(proceed && (!IsMsgPortEmpty(port)))
    {
        error = 0;
        request = (APTR)port->mp_MsgList.lh_Head;
        data_size = packet_size = request->ios2_DataLength;

        opener = (APTR)request->ios2_BufferManagement;

        frame = AllocMem(FC_RXTX_ALLOC_BUFSIZE, MEMF_PUBLIC | MEMF_CLEAR);
        if (!frame) {
            error = S2ERR_NO_RESOURCES;
            wire_error = S2WERR_BUFF_ERROR;
        }

        if (error == 0)
        {
            if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
            {
                packet_size += FC_PACKET_DATA;
                CopyMem(request->ios2_DstAddr, frame->fc_packet_dest, FC_ADDR_SIZE);
                CopyMem(unit->lpeu_dev_addr, frame->fc_packet_source, FC_ADDR_SIZE);
                frame->fc_packet_type = AROS_WORD2BE(request->ios2_PacketType);

                buffer = frame->fc_packet_data;
            }
            else
            {
                buffer = (UBYTE *)frame;
            }

            if (!opener->tx_function(buffer, request->ios2_Data, data_size))
            {
                error = S2ERR_NO_RESOURCES;
                wire_error = S2WERR_BUFF_ERROR;
                ReportEvents(LIBBASE, unit,
                    S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_TX);
            }

            if (error == 0)
            {
                D(bug("[%s] ## %s: Submitting frame (%ld bytes)\n",
                    unit->lpeu_name, __func__, (ULONG)packet_size));

                if (lpe_tx_frame(unit, frame, packet_size) != 0)
                {
                    error = S2ERR_NO_RESOURCES;
                    wire_error = S2WERR_GENERIC_ERROR;
                }
            }

            FreeMem(frame, FC_RXTX_ALLOC_BUFSIZE);
        }

        /* Reply to the request */
        request->ios2_Req.io_Error = error;
        request->ios2_WireError = wire_error;
        Disable();
        Remove((APTR)request);
        Enable();
        ReplyMsg((APTR)request);

        if (error == 0)
        {
            tracker = FindTypeStats(LIBBASE, unit, &unit->lpeu_type_trackers,
                                    request->ios2_PacketType);
            if (tracker != NULL)
            {
                tracker->stats.PacketsSent++;
                tracker->stats.BytesSent += packet_size;
            }
        }
    }

    return 0;

    AROS_INTFUNC_EXIT
}

/* ============================================================
 * Watchdog Timer Handler
 * ============================================================ */

static AROS_INTH1(lpe_WatchdogHandler, struct LPe32002Unit *, unit)
{
    AROS_INTFUNC_INIT

    struct Device *TimerBase = unit->lpeu_TimerSlowReq->tr_node.io_Device;
    struct timeval time;

    GetSysTime(&time);

    if (unit->lpeu_toutNEED && (CmpTime(&time, &unit->lpeu_toutPOLL) < 0))
    {
        unit->lpeu_toutNEED = FALSE;
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/* ============================================================
 * Main Interrupt Handler
 * ============================================================ */

static AROS_INTH1(lpe_IntHandler, struct LPe32002Unit *, unit)
{
    AROS_INTFUNC_INIT

    struct sli4_eq_entry *eqe;
    struct sli4_queue *eq = &unit->lpeu_eq;
    int processed = 0;

    /* Check Event Queue for pending events */
    while (1)
    {
        eqe = (struct sli4_eq_entry *)((UBYTE *)eq->base + (eq->index * eq->entry_size));

        if (!(eqe->valid))
            break;

        /* Clear valid bit */
        eqe->valid = 0;

        eq->index = (eq->index + 1) % eq->size;
        eq->num_proc++;
        processed++;

        /* Process completions on associated CQs */
        lpe_process_cq(unit, &unit->lpeu_cq_wq);
        lpe_process_cq(unit, &unit->lpeu_cq_rq);
        lpe_process_cq(unit, &unit->lpeu_cq_mq);
    }

    if (processed == 0)
        return FALSE;   /* Not our interrupt */

    /* Re-arm the EQ */
    lpe_eq_arm(unit, eq, TRUE);

    /* Re-arm all CQs */
    lpe_cq_arm(unit, &unit->lpeu_cq_wq, TRUE);
    lpe_cq_arm(unit, &unit->lpeu_cq_rq, TRUE);
    lpe_cq_arm(unit, &unit->lpeu_cq_mq, TRUE);

    return TRUE;

    AROS_INTFUNC_EXIT
}

/* ============================================================
 * Packet Copy and Address Filter
 * ============================================================ */

VOID CopyPacket(struct LPe32002Base *LPeBase, struct LPe32002Unit *unit,
    struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
    struct fc_eth_frame *buffer)
{
    LIBBASETYPEPTR LIBBASE = LPeBase;
    struct Opener *opener;
    BOOL filtered = FALSE;
    UBYTE *ptr;
    const UBYTE broadcast[FC_ADDR_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    D(bug("[%s] %s(packet @ %p, len = %d)\n", unit->lpeu_name, __func__, buffer, packet_size));

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if(memcmp(buffer->fc_packet_dest, broadcast, FC_ADDR_SIZE) == 0)
    {
        request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
    }
    else if((buffer->fc_packet_dest[0] & 0x1) != 0)
    {
        request->ios2_Req.io_Flags |= SANA2IOF_MCAST;
    }

    CopyMem(buffer->fc_packet_source, request->ios2_SrcAddr, FC_ADDR_SIZE);
    CopyMem(buffer->fc_packet_dest, request->ios2_DstAddr, FC_ADDR_SIZE);
    request->ios2_PacketType = packet_type;

    if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
    {
        packet_size -= FC_PACKET_DATA;
        ptr = (UBYTE *)&buffer->fc_packet_data[0];
    }
    else
    {
        ptr = (UBYTE *)buffer;
    }

    request->ios2_DataLength = packet_size;

    opener = request->ios2_BufferManagement;
    if((request->ios2_Req.io_Command == CMD_READ) &&
        (opener->filter_hook != NULL))
        if(!CallHookPkt(opener->filter_hook, request, ptr))
            filtered = TRUE;

    if(!filtered)
    {
        if(!opener->rx_function(request->ios2_Data, ptr, packet_size))
        {
            request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
            request->ios2_WireError = S2WERR_BUFF_ERROR;
            ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX);
        }
        Disable();
        Remove((APTR)request);
        Enable();
        ReplyMsg((APTR)request);
    }
}

BOOL AddressFilter(struct LPe32002Base *LPeBase, struct LPe32002Unit *unit, UBYTE *address)
{
    struct AddressRange *range, *tail;
    BOOL accept = TRUE;
    const UBYTE broadcast[FC_ADDR_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    if((address[0] & 0x01) != 0 && memcmp(address, broadcast, FC_ADDR_SIZE) != 0)
    {
        range = (APTR)unit->lpeu_multicast_ranges.mlh_Head;
        tail = (APTR)&unit->lpeu_multicast_ranges.mlh_Tail;
        accept = FALSE;

        while((range != tail) && !accept)
        {
            if ((memcmp(address, range->lower_bound, FC_ADDR_SIZE) >= 0) &&
                (memcmp(address, range->upper_bound, FC_ADDR_SIZE) <= 0))
                accept = TRUE;
            range = (APTR)range->node.mln_Succ;
        }

        if(!accept)
            unit->lpeu_special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    }
    return accept;
}

/* ============================================================
 * Multicast Range Management
 * ============================================================ */

BOOL AddMulticastRange(LIBBASETYPEPTR LIBBASE, struct LPe32002Unit *unit,
                       const UBYTE *lower_bound, const UBYTE *upper_bound)
{
    struct AddressRange *range;
    const UBYTE broadcast[FC_ADDR_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    /* Allow broadcast as a special case */
    if (memcmp(lower_bound, broadcast, FC_ADDR_SIZE) == 0 &&
        memcmp(upper_bound, broadcast, FC_ADDR_SIZE) == 0)
    {
        return TRUE;
    }

    range = AllocMem(sizeof(struct AddressRange), MEMF_PUBLIC | MEMF_CLEAR);
    if (range != NULL)
    {
        CopyMem(lower_bound, range->lower_bound, FC_ADDR_SIZE);
        CopyMem(upper_bound, range->upper_bound, FC_ADDR_SIZE);
        range->add_count = 1;

        Disable();
        AddTail((APTR)&unit->lpeu_multicast_ranges, (APTR)range);
        Enable();

        unit->lpeu_range_count++;
        return TRUE;
    }
    return FALSE;
}

BOOL RemMulticastRange(LIBBASETYPEPTR LIBBASE, struct LPe32002Unit *unit,
                       const UBYTE *lower_bound, const UBYTE *upper_bound)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->lpeu_multicast_ranges.mlh_Head;
    tail = (APTR)&unit->lpeu_multicast_ranges.mlh_Tail;

    while ((range != tail) && !found)
    {
        if ((memcmp(lower_bound, range->lower_bound, FC_ADDR_SIZE) == 0) &&
            (memcmp(upper_bound, range->upper_bound, FC_ADDR_SIZE) == 0))
        {
            if ((--range->add_count) == 0)
            {
                Disable();
                Remove((APTR)range);
                Enable();
                FreeMem(range, sizeof(struct AddressRange));
                unit->lpeu_range_count--;
            }
            found = TRUE;
        }
        else
        {
            range = (APTR)range->node.mln_Succ;
        }
    }

    return found;
}

/* ============================================================
 * Unit Scheduler Task
 * ============================================================ */

AROS_UFH3(void, lpe_Scheduler,
    AROS_UFHA(STRPTR,              argPtr, A0),
    AROS_UFHA(ULONG,               argSize, D0),
    AROS_UFHA(struct ExecBase *,    SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct LPeStartup *sm_UD = FindTask(NULL)->tc_UserData;
    struct LPe32002Unit *unit = sm_UD->lpesm_Unit;

    LIBBASETYPEPTR LIBBASE = unit->lpeu_device;
    struct MsgPort *reply_port, *input;

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));
    D(bug("[%s] %s: Setting device up\n", unit->lpeu_name, __func__));

    reply_port = CreateMsgPort();
    input = CreateMsgPort();

    unit->lpeu_input_port = input;

    unit->lpeu_TimerSlowPort = CreateMsgPort();

    if (unit->lpeu_TimerSlowPort)
    {
        unit->lpeu_TimerSlowReq = (struct timerequest *)
            CreateIORequest((struct MsgPort *)unit->lpeu_TimerSlowPort, sizeof(struct timerequest));

        if (unit->lpeu_TimerSlowReq)
        {
            if (!OpenDevice("timer.device", UNIT_VBLANK,
                (struct IORequest *)unit->lpeu_TimerSlowReq, 0))
            {
                struct Message *msg = AllocVec(sizeof(struct Message), MEMF_PUBLIC | MEMF_CLEAR);
                ULONG sigset;

                D(bug("[%s] %s: Got VBLANK timer\n", unit->lpeu_name, __func__));

                msg->mn_ReplyPort = reply_port;
                msg->mn_Length = sizeof(struct Message);

                D(bug("[%s] %s: Sending startup handshake\n", unit->lpeu_name, __func__));
                PutMsg(sm_UD->lpesm_SyncPort, msg);
                WaitPort(reply_port);
                GetMsg(reply_port);

                FreeVec(msg);

                D(bug("[%s] %s: Entering main loop\n", unit->lpeu_name, __func__));

                unit->lpeu_signal_0 = AllocSignal(-1);
                unit->lpeu_signal_1 = AllocSignal(-1);
                unit->lpeu_signal_2 = AllocSignal(-1);
                unit->lpeu_signal_3 = AllocSignal(-1);

                sigset = 1 << input->mp_SigBit  |
                         1 << unit->lpeu_signal_0  |
                         1 << unit->lpeu_signal_1  |
                         1 << unit->lpeu_signal_2  |
                         1 << unit->lpeu_signal_3;

                for(;;)
                {
                    ULONG recvd = Wait(sigset);
                    if (recvd & (1 << unit->lpeu_signal_0))
                    {
                        /* Shutdown signal */
                        FreeSignal(unit->lpeu_signal_0);
                        FreeSignal(unit->lpeu_signal_1);
                        FreeSignal(unit->lpeu_signal_2);
                        FreeSignal(unit->lpeu_signal_3);

                        CloseDevice((struct IORequest *)unit->lpeu_TimerSlowReq);
                        DeleteIORequest((struct IORequest *)unit->lpeu_TimerSlowReq);
                        DeleteMsgPort(unit->lpeu_TimerSlowPort);
                        DeleteMsgPort(input);
                        DeleteMsgPort(reply_port);

                        D(bug("[%s] %s: Process shutdown\n", unit->lpeu_name, __func__));
                        return;
                    }
                    else if (recvd & (1 << input->mp_SigBit))
                    {
                        struct IOSana2Req *io;

                        while ((io = (struct IOSana2Req *)GetMsg(input)) != NULL)
                        {
                            D(bug("[%s] %s: Handling incoming request\n", unit->lpeu_name, __func__));
                            ObtainSemaphore(&unit->lpeu_unit_lock);
                            handle_request(LIBBASE, io);
                        }
                    }
                    else
                    {
                        D(bug("[%s] %s: Signal received\n", unit->lpeu_name, __func__));
                    }
                }
            }
        }
    }

    AROS_USERFUNC_EXIT
}

/* ============================================================
 * Create Unit
 * ============================================================ */

struct LPe32002Unit *CreateUnit(struct LPe32002Base *LPeBase, OOP_Object *pciDevice)
{
    LIBBASETYPEPTR LIBBASE = LPeBase;
    struct LPe32002Unit *unit;
    BOOL success = TRUE;
    int i;

    D(bug("[lpe32002] %s()\n", __func__));

    if ((unit = AllocMem(sizeof(struct LPe32002Unit), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
    {
        IPTR DeviceID, RevisionID;
        IPTR Bar0Addr, Bar0Size, Bar1Addr, Bar1Size;
        OOP_Object *driver;

        unit->lpeu_UnitNum = LPeBase->lpeb_UnitCount++;

        unit->lpeu_Sana2Info.HardwareType = S2WireType_Ethernet;
        unit->lpeu_Sana2Info.MTU = FC_MTU;
        unit->lpeu_Sana2Info.AddrFieldSize = 8 * FC_ADDR_SIZE;

        if ((unit->lpeu_name = AllocVec(12 + (unit->lpeu_UnitNum / 10) + 2, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        {
            FreeMem(unit, sizeof(struct LPe32002Unit));
            return NULL;
        }

        sprintf((char *)unit->lpeu_name, "lpe32002.%d", (int)unit->lpeu_UnitNum);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_RevisionID, &RevisionID);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);

        unit->lpeu_device     = LPeBase;
        unit->lpeu_PCIDevice  = pciDevice;
        unit->lpeu_PCIDriver  = driver;

        unit->lpeu_mtu        = unit->lpeu_Sana2Info.MTU;

        InitSemaphore(&unit->lpeu_unit_lock);
        NEWLIST(&unit->lpeu_Openers);
        NEWLIST(&unit->lpeu_multicast_ranges);
        NEWLIST(&unit->lpeu_type_trackers);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->lpeu_IRQ);
        D(bug("[%s] %s: IRQ #%lu\n", unit->lpeu_name, __func__, (ULONG)unit->lpeu_IRQ));

        /* Map BAR0 (SLI-4 Configuration Registers) */
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &Bar0Addr);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, &Bar0Size);
        unit->lpeu_bar0 = HIDD_PCIDriver_MapPCI(driver, (APTR)Bar0Addr, Bar0Size);
        unit->lpeu_bar0_size = Bar0Size;
        D(bug("[%s] %s: BAR0 mapped @ %p (%ld bytes)\n",
            unit->lpeu_name, __func__, unit->lpeu_bar0, (ULONG)Bar0Size));

        /* Map BAR1 (SLI-4 Control Registers / Doorbells) */
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base3, &Bar1Addr);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size3, &Bar1Size);
        unit->lpeu_bar1 = HIDD_PCIDriver_MapPCI(driver, (APTR)Bar1Addr, Bar1Size);
        unit->lpeu_bar1_size = Bar1Size;
        D(bug("[%s] %s: BAR1 mapped @ %p (%ld bytes)\n",
            unit->lpeu_name, __func__, unit->lpeu_bar1, (ULONG)Bar1Size));

        if (unit->lpeu_bar0 && unit->lpeu_bar1)
        {
            struct TagItem attrs[] = {
                { aHidd_PCIDevice_isIO,     TRUE },
                { aHidd_PCIDevice_isMEM,    TRUE },
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE,                 0    },
            };
            OOP_SetAttrs(pciDevice, (struct TagItem *)&attrs);

            /* Set up delay timer for hw functions */
            unit->lpeu_DelayPort.mp_SigBit = SIGB_SINGLE;
            unit->lpeu_DelayPort.mp_Flags = PA_SIGNAL;
            unit->lpeu_DelayPort.mp_SigTask = FindTask(NULL);
            unit->lpeu_DelayPort.mp_Node.ln_Type = NT_MSGPORT;
            NEWLIST(&unit->lpeu_DelayPort.mp_MsgList);

            unit->lpeu_DelayReq.tr_node.io_Message.mn_ReplyPort = &unit->lpeu_DelayPort;
            unit->lpeu_DelayReq.tr_node.io_Message.mn_Length = sizeof(struct timerequest);
            OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ,
                       (struct IORequest *)&unit->lpeu_DelayReq, 0);

            /* Allocate DMA-able bootstrap mailbox */
            unit->lpeu_bmbx = AllocMem(sizeof(struct sli4_mbox_cmd), MEMF_PUBLIC | MEMF_CLEAR);
            if (unit->lpeu_bmbx)
                unit->lpeu_bmbx_dma = HIDD_PCIDriver_CPUtoPCI(driver, unit->lpeu_bmbx);

            if (!unit->lpeu_bmbx) {
                D(bug("[%s] %s: Failed to allocate bootstrap mailbox\n",
                    unit->lpeu_name, __func__));
                goto fail;
            }

            /* Initialize hardware (firmware, queues, etc.) */
            if (lpe_hw_init(unit) != 0) {
                D(bug("[%s] %s: Hardware initialization failed\n",
                    unit->lpeu_name, __func__));
                goto fail;
            }

            /* Copy WWPN as the device address */
            CopyMem(unit->lpeu_sli_params.wwpn, unit->lpeu_org_addr, FC_ADDR_SIZE);
            CopyMem(unit->lpeu_org_addr, unit->lpeu_dev_addr, FC_ADDR_SIZE);

            D(bug("[%s] %s: WWPN %02lx:%02lx:%02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
                unit->lpeu_name, __func__,
                (ULONG)unit->lpeu_dev_addr[0], (ULONG)unit->lpeu_dev_addr[1],
                (ULONG)unit->lpeu_dev_addr[2], (ULONG)unit->lpeu_dev_addr[3],
                (ULONG)unit->lpeu_dev_addr[4], (ULONG)unit->lpeu_dev_addr[5],
                (ULONG)unit->lpeu_dev_addr[6], (ULONG)unit->lpeu_dev_addr[7]));

            /* Allocate TX/RX descriptor arrays */
            unit->lpeu_tx_count = LPE_WQ_DEPTH;
            unit->lpeu_tx_descs = AllocMem(sizeof(struct lpe_tx_desc) * unit->lpeu_tx_count,
                                            MEMF_PUBLIC | MEMF_CLEAR);
            unit->lpeu_tx_prod = 0;
            unit->lpeu_tx_cons = 0;

            unit->lpeu_rx_count = LPE_RQ_DEPTH;
            unit->lpeu_rx_descs = AllocMem(sizeof(struct lpe_rx_desc) * unit->lpeu_rx_count,
                                            MEMF_PUBLIC | MEMF_CLEAR);
            unit->lpeu_rx_prod = 0;
            unit->lpeu_rx_cons = 0;

            if (!unit->lpeu_tx_descs || !unit->lpeu_rx_descs) {
                D(bug("[%s] %s: Failed to allocate TX/RX descriptors\n",
                    unit->lpeu_name, __func__));
                goto fail;
            }

            /* Disable IRQs until we go online */
            lpe_irq_disable(unit);

            /* Set up interrupt handlers */
            unit->lpeu_irqhandler.is_Node.ln_Type = NT_INTERRUPT;
            unit->lpeu_irqhandler.is_Node.ln_Pri = 100;
            unit->lpeu_irqhandler.is_Node.ln_Name = LIBBASE->lpeb_Device.dd_Library.lib_Node.ln_Name;
            unit->lpeu_irqhandler.is_Code = (VOID_FUNC)lpe_IntHandler;
            unit->lpeu_irqhandler.is_Data = unit;

            unit->lpeu_touthandler.is_Node.ln_Type = NT_INTERRUPT;
            unit->lpeu_touthandler.is_Node.ln_Pri = 100;
            unit->lpeu_touthandler.is_Node.ln_Name = LIBBASE->lpeb_Device.dd_Library.lib_Node.ln_Name;
            unit->lpeu_touthandler.is_Code = (VOID_FUNC)lpe_WatchdogHandler;
            unit->lpeu_touthandler.is_Data = unit;

            unit->lpeu_tx_int.is_Node.ln_Type = NT_INTERRUPT;
            unit->lpeu_tx_int.is_Node.ln_Name = unit->lpeu_name;
            unit->lpeu_tx_int.is_Code = (VOID_FUNC)lpe_TX_Int;
            unit->lpeu_tx_int.is_Data = unit;

            /* Create request ports */
            for (i = 0; i < REQUEST_QUEUE_COUNT; i++)
            {
                struct MsgPort *port;

                if ((port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
                    success = FALSE;

                if (success)
                {
                    unit->lpeu_request_ports[i] = port;
                    NEWLIST(&port->mp_MsgList);
                    port->mp_Flags = PA_IGNORE;
                    port->mp_SigTask = &unit->lpeu_tx_int;
                }
            }

            unit->lpeu_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

            if (success)
            {
                struct LPeStartup *sm_UD;
                UBYTE tmpbuff[100];

                if ((sm_UD = AllocMem(sizeof(struct LPeStartup), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
                {
                    sprintf((char *)tmpbuff, LPE_TASK_NAME, unit->lpeu_name);

                    sm_UD->lpesm_SyncPort = CreateMsgPort();
                    sm_UD->lpesm_Unit = unit;

                    unit->lpeu_Process = CreateNewProcTags(
                                            NP_Entry, (IPTR)lpe_Scheduler,
                                            NP_Name, tmpbuff,
                                            NP_Synchronous, FALSE,
                                            NP_Priority, 0,
                                            NP_UserData, (IPTR)sm_UD,
                                            NP_StackSize, 140960,
                                            TAG_DONE);

                    {
                        struct Message *msg;
                        WaitPort(sm_UD->lpesm_SyncPort);
                        msg = GetMsg(sm_UD->lpesm_SyncPort);
                        ReplyMsg(msg);
                        DeleteMsgPort(sm_UD->lpesm_SyncPort);
                    }
                    FreeMem(sm_UD, sizeof(struct LPeStartup));

                    D(bug("[%s] %s: Unit %ld initialised @ %p\n",
                        unit->lpeu_name, __func__, (ULONG)unit->lpeu_UnitNum, unit));
                    return unit;
                }
            }
        }
        else
        {
            D(bug("[%s] %s: Failed to map PCI BARs\n", unit->lpeu_name, __func__));
        }

fail:
        /* Cleanup on failure */
        DeleteUnit(LPeBase, unit);
        return NULL;
    }
    return NULL;
}

/* ============================================================
 * Delete Unit
 * ============================================================ */

void DeleteUnit(struct LPe32002Base *LPeBase, struct LPe32002Unit *unit)
{
    LIBBASETYPEPTR LIBBASE = LPeBase;
    int i;
    if (unit)
    {
        D(bug("[lpe32002] %s(unit @ %p)\n", __func__, unit));

        /* Signal the scheduler task to shut down */
        if (unit->lpeu_Process)
        {
            Signal(&unit->lpeu_Process->pr_Task, 1 << unit->lpeu_signal_0);
        }

        /* Free interrupt resources */
        lpe_free_irq(unit);

        /* Tear down SLI-4 queues */
        lpe_teardown_queues(unit);

        /* Free bootstrap mailbox */
        if (unit->lpeu_bmbx)
            FreeMem(unit->lpeu_bmbx, sizeof(struct sli4_mbox_cmd));

        /* Free TX descriptors and their buffers */
        if (unit->lpeu_tx_descs) {
            for (i = 0; i < (int)unit->lpeu_tx_count; i++) {
                if (unit->lpeu_tx_descs[i].buf.virt)
                    FreeMem(unit->lpeu_tx_descs[i].buf.virt, unit->lpeu_tx_descs[i].buf.size);
            }
            FreeMem(unit->lpeu_tx_descs, sizeof(struct lpe_tx_desc) * unit->lpeu_tx_count);
        }

        /* Free RX descriptors and their buffers */
        if (unit->lpeu_rx_descs) {
            for (i = 0; i < (int)unit->lpeu_rx_count; i++) {
                if (unit->lpeu_rx_descs[i].hdr_buf.virt)
                    FreeMem(unit->lpeu_rx_descs[i].hdr_buf.virt, unit->lpeu_rx_descs[i].hdr_buf.size);
                if (unit->lpeu_rx_descs[i].data_buf.virt)
                    FreeMem(unit->lpeu_rx_descs[i].data_buf.virt, unit->lpeu_rx_descs[i].data_buf.size);
            }
            FreeMem(unit->lpeu_rx_descs, sizeof(struct lpe_rx_desc) * unit->lpeu_rx_count);
        }

        /* Free request ports */
        for (i = 0; i < REQUEST_QUEUE_COUNT; i++) {
            if (unit->lpeu_request_ports[i])
                FreeMem(unit->lpeu_request_ports[i], sizeof(struct MsgPort));
        }

        /* Unmap PCI BARs */
        if (unit->lpeu_bar0)
            HIDD_PCIDriver_UnmapPCI(unit->lpeu_PCIDriver, unit->lpeu_bar0, unit->lpeu_bar0_size);
        if (unit->lpeu_bar1)
            HIDD_PCIDriver_UnmapPCI(unit->lpeu_PCIDriver, unit->lpeu_bar1, unit->lpeu_bar1_size);

        /* Close delay timer */
        if (unit->lpeu_DelayReq.tr_node.io_Device)
            CloseDevice((struct IORequest *)&unit->lpeu_DelayReq);

        if (unit->lpeu_name)
            FreeVec(unit->lpeu_name);

        FreeMem(unit, sizeof(struct LPe32002Unit));
    }
}
