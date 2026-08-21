/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: DesignWare MAC SANA-II driver, the unit: rings, interrupts and
          the process that serves deferred requests.
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <hardware/intbits.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <devices/newstyle.h>
#include <aros/macros.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include "dwmac.h"

#undef UtilityBase

static const UBYTE dwmac_broadcast[ETH_ADDRESSSIZE] =
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/*
 * Everything the engine reads or writes goes through RAM, and the port
 * is cache coherent - but the descriptor handover still needs ordering,
 * or a descriptor could be owned before its contents are visible.
 * CacheClearE is the exec spelling of that fence.
 */
#define DWMAC_SYNC(addr, len) CacheClearE((APTR)(addr), (len), CACRF_ClearD)

static void DWMAC_RXProcess(struct DWMACUnit *unit);
static void DWMAC_TXComplete(struct DWMACUnit *unit);
static void DWMAC_UnitTask(void);

/* The service task makes no DOS calls, so it can be a bare exec task */
#define DWMAC_TASK_STACK 32768

/* == small helpers ====================================================== */

static BOOL dwmac_sameaddr(const UBYTE *a, const UBYTE *b)
{
    ULONG i;

    for (i = 0; i < ETH_ADDRESSSIZE; i++)
    {
        if (a[i] != b[i])
            return FALSE;
    }
    return TRUE;
}

/*
 * A 48-bit address split for range comparisons: the first four bytes as
 * one long, the last two as a word.
 */
static void dwmac_splitaddr(const UBYTE *addr, ULONG *left, UWORD *right)
{
    *left = ((ULONG)addr[0] << 24) | ((ULONG)addr[1] << 16) |
            ((ULONG)addr[2] << 8) | addr[3];
    *right = ((UWORD)addr[4] << 8) | addr[5];
}

struct TypeStats *DWMAC_FindTypeStats(struct MinList *list, ULONG packet_type)
{
    struct TypeStats *stats;

    ForeachNode(list, stats)
    {
        if (stats->packet_type == packet_type)
            return stats;
    }

    return NULL;
}

void DWMAC_ReportEvents(struct DWMACBase *base, struct DWMACUnit *unit,
                        ULONG events)
{
    struct IOSana2Req *request, *tail, *next;
    struct List *list;

    list = &unit->dwu_RequestPorts[EVENT_QUEUE]->mp_MsgList;
    next = (APTR)list->lh_Head;
    tail = (APTR)&list->lh_Tail;

    Disable();
    while (next != tail)
    {
        request = next;
        next = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

        if ((request->ios2_WireError & events) != 0)
        {
            request->ios2_WireError = events;
            Remove((APTR)request);
            ReplyMsg((APTR)request);
        }
    }
    Enable();
}

/* == address filtering ================================================== */

static struct AddressRange *dwmac_findrange(struct DWMACUnit *unit,
    ULONG lower_left, UWORD lower_right, ULONG upper_left, UWORD upper_right)
{
    struct AddressRange *range;

    ForeachNode(&unit->dwu_MulticastRanges, range)
    {
        if (range->lower_bound_left == lower_left &&
            range->lower_bound_right == lower_right &&
            range->upper_bound_left == upper_left &&
            range->upper_bound_right == upper_right)
            return range;
    }

    return NULL;
}

BOOL DWMAC_AddMulticastRange(struct DWMACBase *base, struct DWMACUnit *unit,
                             const UBYTE *lower, const UBYTE *upper)
{
    struct AddressRange *range;
    ULONG lower_left, upper_left;
    UWORD lower_right, upper_right;

    dwmac_splitaddr(lower, &lower_left, &lower_right);
    dwmac_splitaddr(upper, &upper_left, &upper_right);

    range = dwmac_findrange(unit, lower_left, lower_right,
                            upper_left, upper_right);
    if (range)
    {
        range->add_count++;
        return TRUE;
    }

    range = AllocMem(sizeof(struct AddressRange), MEMF_PUBLIC | MEMF_CLEAR);
    if (!range)
        return FALSE;

    range->add_count = 1;
    range->lower_bound_left = lower_left;
    range->lower_bound_right = lower_right;
    range->upper_bound_left = upper_left;
    range->upper_bound_right = upper_right;

    Disable();
    AddTail((struct List *)&unit->dwu_MulticastRanges, (struct Node *)range);
    Enable();
    unit->dwu_RangeCount++;

    return TRUE;
}

BOOL DWMAC_RemMulticastRange(struct DWMACBase *base, struct DWMACUnit *unit,
                             const UBYTE *lower, const UBYTE *upper)
{
    struct AddressRange *range;
    ULONG lower_left, upper_left;
    UWORD lower_right, upper_right;

    dwmac_splitaddr(lower, &lower_left, &lower_right);
    dwmac_splitaddr(upper, &upper_left, &upper_right);

    range = dwmac_findrange(unit, lower_left, lower_right,
                            upper_left, upper_right);
    if (!range)
        return FALSE;

    if (--range->add_count == 0)
    {
        Disable();
        Remove((struct Node *)range);
        Enable();
        FreeMem(range, sizeof(struct AddressRange));
        unit->dwu_RangeCount--;
    }

    return TRUE;
}

BOOL DWMAC_AddressFilter(struct DWMACBase *base, struct DWMACUnit *unit,
                         UBYTE *address)
{
    struct AddressRange *range;
    ULONG address_left;
    UWORD address_right;

    if (unit->dwu_Flags & IFF_PROMISC)
        return TRUE;

    /* The hardware's perfect filter already threw out foreign unicast */
    if ((address[0] & 0x1) == 0)
        return TRUE;

    if (dwmac_sameaddr(address, dwmac_broadcast))
        return TRUE;

    dwmac_splitaddr(address, &address_left, &address_right);

    ForeachNode(&unit->dwu_MulticastRanges, range)
    {
        if ((address_left > range->lower_bound_left ||
             (address_left == range->lower_bound_left &&
              address_right >= range->lower_bound_right)) &&
            (address_left < range->upper_bound_left ||
             (address_left == range->upper_bound_left &&
              address_right <= range->upper_bound_right)))
            return TRUE;
    }

    unit->dwu_SpecialStats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    return FALSE;
}

/* == receive delivery =================================================== */

void DWMAC_CopyPacket(struct DWMACBase *base, struct DWMACUnit *unit,
                      struct IOSana2Req *request, ULONG packet_size,
                      UWORD packet_type, struct eth_frame *frame)
{
    struct Library *UtilityBase = base->dwm_UtilityBase;
    struct Opener *opener;
    UBYTE *ptr;
    BOOL filtered = FALSE;

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if (dwmac_sameaddr(frame->eth_packet_dest, dwmac_broadcast))
        request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
    else if ((frame->eth_packet_dest[0] & 0x1) != 0)
        request->ios2_Req.io_Flags |= SANA2IOF_MCAST;

    CopyMem(frame->eth_packet_source, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(frame->eth_packet_dest, request->ios2_DstAddr, ETH_ADDRESSSIZE);
    request->ios2_PacketType = packet_type;

    if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
    {
        packet_size -= ETH_HEADERSIZE;
        ptr = frame->eth_packet_data;
    }
    else
        ptr = (UBYTE *)frame;

    request->ios2_DataLength = packet_size;

    opener = request->ios2_BufferManagement;
    if (request->ios2_Req.io_Command == CMD_READ &&
        opener->filter_hook != NULL)
    {
        if (!CallHookPkt(opener->filter_hook, request, ptr))
            filtered = TRUE;
    }

    /* A rejected packet leaves the request queued for the next one */
    if (filtered)
        return;

    if (!opener->rx_function(request->ios2_Data, ptr, packet_size))
    {
        request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        request->ios2_WireError = S2WERR_BUFF_ERROR;
        DWMAC_ReportEvents(base, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE |
                           S2EVENT_BUFF | S2EVENT_RX);
    }

    Disable();
    Remove((APTR)request);
    Enable();
    ReplyMsg((APTR)request);
}

static void DWMAC_RXPacket(struct DWMACBase *base, struct DWMACUnit *unit,
                           struct eth_frame *frame, ULONG length)
{
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    struct TypeStats *tracker;
    BOOL accepted, is_orphan = TRUE;
    UWORD packet_type;

    if (!DWMAC_AddressFilter(base, unit, frame->eth_packet_dest))
        return;

    /*
     * Kept unsigned: an ethertype with the high bit set would otherwise
     * sign-extend and never match an opener's ULONG packet type.
     */
    packet_type = (UWORD)AROS_BE2WORD(frame->eth_packet_type);

    unit->dwu_Stats.PacketsReceived++;

    tracker = DWMAC_FindTypeStats(&unit->dwu_TypeTrackers, packet_type);
    if (tracker)
    {
        tracker->stats.PacketsReceived++;
        tracker->stats.BytesReceived += length;
    }

    opener = (APTR)unit->dwu_Openers.mlh_Head;
    opener_tail = (APTR)&unit->dwu_Openers.mlh_Tail;

    while (opener != opener_tail)
    {
        request = (APTR)opener->read_port.mp_MsgList.lh_Head;
        request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
        accepted = FALSE;

        while (request != request_tail && !accepted)
        {
            struct IOSana2Req *next =
                (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

            /* An 802.3 length field matches any small-type request */
            if (request->ios2_PacketType == packet_type ||
                (request->ios2_PacketType <= ETH_MTU &&
                 packet_type <= ETH_MTU))
            {
                DWMAC_CopyPacket(base, unit, request, length, packet_type,
                                 frame);
                accepted = TRUE;
            }
            request = next;
        }

        if (accepted)
            is_orphan = FALSE;

        opener = (APTR)opener->node.mln_Succ;
    }

    if (is_orphan)
    {
        unit->dwu_Stats.UnknownTypesReceived++;

        if (!IsListEmpty(&unit->dwu_RequestPorts[ADOPT_QUEUE]->mp_MsgList))
        {
            DWMAC_CopyPacket(base, unit,
                (APTR)unit->dwu_RequestPorts[ADOPT_QUEUE]->mp_MsgList.lh_Head,
                length, packet_type, frame);
        }
    }
}

/* == the receive ring =================================================== */

static void DWMAC_RXProcess(struct DWMACUnit *unit)
{
    struct DWMACBase *base = unit->dwu_Base;
    struct dwmac_hw *hw = unit->dwu_HW;

    for (;;)
    {
        ULONG slot = unit->dwu_RXCurrent;
        volatile struct dwmac_desc *desc =
            (volatile struct dwmac_desc *)&unit->dwu_RXDesc[slot];
        UBYTE *buf = unit->dwu_RXBuf + slot * unit->dwu_BufSize;
        ULONG des3 = desc->des3;

        if (des3 & DWMAC_RDES3_OWN)
            break;

        /* The ownership read must land before the frame is looked at */
        DWMAC_SYNC(buf, unit->dwu_BufSize);

        if ((des3 & (DWMAC_RDES3_FD | DWMAC_RDES3_LD)) ==
            (DWMAC_RDES3_FD | DWMAC_RDES3_LD) && !(des3 & DWMAC_RDES3_ES))
        {
            ULONG length = des3 & DWMAC_RDES3_LEN_MASK;

            if (length > unit->dwu_BufSize)
                length = unit->dwu_BufSize;

            if (length >= ETH_HEADERSIZE)
                DWMAC_RXPacket(base, unit, (struct eth_frame *)buf, length);
        }
        else
            unit->dwu_Stats.BadData++;

        desc->des0 = (ULONG)(IPTR)buf;
        desc->des1 = (ULONG)((UQUAD)(IPTR)buf >> 32);
        desc->des2 = 0;
        DWMAC_SYNC(desc, sizeof(*desc));
        desc->des3 = DWMAC_RDES3_OWN | DWMAC_RDES3_IOC | DWMAC_RDES3_BUF1V;
        DWMAC_SYNC(desc, sizeof(*desc));

        /*
         * The tail is the freshest refilled slot; the engine stops just
         * short of it, so the ring can never be overrun.
         */
        DWMAC_Write(hw, DWMAC_DMA_CH0_RXDESC_TAIL, (ULONG)(IPTR)desc);

        unit->dwu_RXCurrent = (slot + 1) % DWMAC_RXDESC;
    }
}

/* == the transmit ring ================================================== */

static AROS_INTH1(DWMAC_TXIntF, struct DWMACUnit *, unit)
{
    AROS_INTFUNC_INIT

    struct DWMACBase *base = unit->dwu_Base;
    struct dwmac_hw *hw = unit->dwu_HW;
    struct MsgPort *port = unit->dwu_RequestPorts[WRITE_QUEUE];
    BOOL proceed = TRUE;

    while (proceed && !IsListEmpty(&port->mp_MsgList))
    {
        ULONG slot = unit->dwu_TXHead;
        ULONG next = (slot + 1) % DWMAC_TXDESC;
        volatile struct dwmac_desc *desc =
            (volatile struct dwmac_desc *)&unit->dwu_TXDesc[slot];
        UBYTE *buffer = unit->dwu_TXBuf + slot * unit->dwu_BufSize;
        struct IOSana2Req *request;
        struct Opener *opener;
        ULONG data_size, packet_size, wire_error = 0;
        BYTE error = 0;

        if (next == unit->dwu_TXTail)
        {
            proceed = FALSE;
            break;
        }

        request = (APTR)port->mp_MsgList.lh_Head;
        data_size = packet_size = request->ios2_DataLength;
        opener = request->ios2_BufferManagement;

        if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
        {
            struct eth_frame *frame = (struct eth_frame *)buffer;

            packet_size += ETH_HEADERSIZE;
            CopyMem(request->ios2_DstAddr, frame->eth_packet_dest,
                    ETH_ADDRESSSIZE);
            CopyMem(unit->dwu_DevAddr, frame->eth_packet_source,
                    ETH_ADDRESSSIZE);
            frame->eth_packet_type =
                AROS_WORD2BE((UWORD)request->ios2_PacketType);

            if (!opener->tx_function(frame->eth_packet_data,
                                     request->ios2_Data, data_size))
                error = S2ERR_NO_RESOURCES;
        }
        else
        {
            if (!opener->tx_function(buffer, request->ios2_Data, data_size))
                error = S2ERR_NO_RESOURCES;
        }

        if (error != 0)
        {
            wire_error = S2WERR_BUFF_ERROR;
            DWMAC_ReportEvents(base, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE |
                               S2EVENT_BUFF | S2EVENT_TX);
        }
        else
        {
            struct TypeStats *tracker;
            UWORD packet_type;

            while (packet_size < ETH_ZLEN)
                buffer[packet_size++] = 0;

            packet_type = ((UWORD)buffer[12] << 8) | buffer[13];

            desc->des0 = (ULONG)(IPTR)buffer;
            desc->des1 = (ULONG)((UQUAD)(IPTR)buffer >> 32);
            desc->des2 = DWMAC_TDES2_IOC |
                         (packet_size & DWMAC_TDES2_B1L_MASK);
            DWMAC_SYNC(desc, sizeof(*desc));
            desc->des3 = DWMAC_TDES3_OWN | DWMAC_TDES3_FD | DWMAC_TDES3_LD |
                         (packet_size & DWMAC_TDES3_LEN_MASK);
            DWMAC_SYNC(desc, sizeof(*desc));

            unit->dwu_TXHead = next;
            DWMAC_Write(hw, DWMAC_DMA_CH0_TXDESC_TAIL,
                        (ULONG)(IPTR)&unit->dwu_TXDesc[next]);

            unit->dwu_Stats.PacketsSent++;
            tracker = DWMAC_FindTypeStats(&unit->dwu_TypeTrackers,
                                          packet_type);
            if (tracker)
            {
                tracker->stats.PacketsSent++;
                tracker->stats.BytesSent += packet_size;
            }
        }

        request->ios2_Req.io_Error = error;
        request->ios2_WireError = wire_error;
        Disable();
        Remove((APTR)request);
        Enable();
        ReplyMsg((APTR)request);
    }

    /* A full ring stalls the queue; the completion interrupt reopens it */
    port->mp_Flags = proceed ? PA_SOFTINT : PA_IGNORE;

    return FALSE;

    AROS_INTFUNC_EXIT
}

static void DWMAC_TXComplete(struct DWMACUnit *unit)
{
    struct MsgPort *port = unit->dwu_RequestPorts[WRITE_QUEUE];

    while (unit->dwu_TXTail != unit->dwu_TXHead)
    {
        ULONG slot = unit->dwu_TXTail;
        volatile struct dwmac_desc *desc =
            (volatile struct dwmac_desc *)&unit->dwu_TXDesc[slot];
        ULONG des3 = desc->des3;

        if (des3 & DWMAC_TDES3_OWN)
            break;

        if (des3 & DWMAC_TDES3_ES)
            unit->dwu_SpecialStats[S2SS_ETHERNET_RETRIES & 0xffff]++;

        unit->dwu_TXTail = (slot + 1) % DWMAC_TXDESC;
    }

    if (port->mp_Flags == PA_IGNORE &&
        (unit->dwu_TXHead + 1) % DWMAC_TXDESC != unit->dwu_TXTail)
    {
        port->mp_Flags = PA_SOFTINT;
        Cause(&unit->dwu_TXInt);
    }
}

/* == the interrupt ====================================================== */

static AROS_INTH1(DWMAC_IntHandlerF, struct DWMACUnit *, unit)
{
    AROS_INTFUNC_INIT

    struct DWMACBase *base = unit->dwu_Base;
    struct dwmac_hw *hw = unit->dwu_HW;
    ULONG status = DWMAC_Read(hw, DWMAC_DMA_CH0_STATUS);

    if ((status & (DWMAC_DMA_STAT_NIS | DWMAC_DMA_STAT_AIS)) == 0)
        return FALSE;

    /* Every bit is write-one-to-clear */
    DWMAC_Write(hw, DWMAC_DMA_CH0_STATUS, status);

    if (status & (DWMAC_DMA_STAT_RI | DWMAC_DMA_STAT_RBU))
    {
        if (status & DWMAC_DMA_STAT_RBU)
            unit->dwu_Stats.Overruns++;
        DWMAC_RXProcess(unit);
    }

    if (status & (DWMAC_DMA_STAT_TI | DWMAC_DMA_STAT_TBU |
                  DWMAC_DMA_STAT_TPS))
        DWMAC_TXComplete(unit);

    if (status & DWMAC_DMA_STAT_FBE)
        DWMAC_ReportEvents(base, unit, S2EVENT_ERROR | S2EVENT_HARDWARE);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/* == bringing the interface up and down ================================= */

static void dwmac_setspeed(struct dwmac_hw *hw, ULONG mbps, BOOL fullduplex)
{
    ULONG cfg = DWMAC_Read(hw, DWMAC_MAC_CONFIG);

    cfg &= ~(DWMAC_CONFIG_PS | DWMAC_CONFIG_FES | DWMAC_CONFIG_DM);
    if (mbps != 1000)
    {
        cfg |= DWMAC_CONFIG_PS;
        if (mbps == 100)
            cfg |= DWMAC_CONFIG_FES;
    }
    if (fullduplex)
        cfg |= DWMAC_CONFIG_DM;

    DWMAC_Write(hw, DWMAC_MAC_CONFIG, cfg);
}

void DWMAC_GoOnline(struct DWMACBase *base, struct DWMACUnit *unit)
{
    struct dwmac_hw *hw = unit->dwu_HW;
    ULONG filter;

    unit->dwu_Stats.PacketsSent = 0;
    unit->dwu_Stats.PacketsReceived = 0;
    unit->dwu_Stats.BadData = 0;
    unit->dwu_Stats.Overruns = 0;
    unit->dwu_Stats.UnknownTypesReceived = 0;

    DWMAC_SetMACAddress(hw, unit->dwu_DevAddr);

    filter = DWMAC_FILTER_PM;
    if (unit->dwu_Flags & IFF_PROMISC)
        filter |= DWMAC_FILTER_PR;
    DWMAC_Write(hw, DWMAC_MAC_PKT_FILTER, filter);

    dwmac_setspeed(hw, unit->dwu_SpeedMbps, unit->dwu_FullDuplex);
    {
        ULONG cfg = DWMAC_Read(hw, DWMAC_MAC_CONFIG) | DWMAC_CONFIG_ACS |
                    DWMAC_CONFIG_CST | DWMAC_CONFIG_TE | DWMAC_CONFIG_RE;
        /*
         * For a jumbo MTU, JE lifts the giant-packet limit to 9018 bytes and
         * JD/WD stop the transmit-jabber and receive-watchdog timers from
         * cutting an oversized frame short.  Cleared for a standard MTU.
         */
        if (unit->dwu_MTU > ETH_MTU)
            cfg |= DWMAC_CONFIG_JE | DWMAC_CONFIG_JD | DWMAC_CONFIG_WD;
        else
            cfg &= ~(DWMAC_CONFIG_JE | DWMAC_CONFIG_JD | DWMAC_CONFIG_WD);
        DWMAC_Write(hw, DWMAC_MAC_CONFIG, cfg);
    }

    DWMAC_Write(hw, DWMAC_DMA_CH0_INT_ENABLE,
                DWMAC_DMA_INT_NIE | DWMAC_DMA_INT_AIE | DWMAC_DMA_INT_RIE |
                DWMAC_DMA_INT_TIE | DWMAC_DMA_INT_RBUE | DWMAC_DMA_INT_FBEE);

    DWMAC_Write(hw, DWMAC_DMA_CH0_TX_CONTROL,
                DWMAC_Read(hw, DWMAC_DMA_CH0_TX_CONTROL) | DWMAC_DMA_TX_ST);
    DWMAC_Write(hw, DWMAC_DMA_CH0_RX_CONTROL,
                DWMAC_Read(hw, DWMAC_DMA_CH0_RX_CONTROL) | DWMAC_DMA_RX_SR);

    /* Nudge receive into its ring */
    DWMAC_Write(hw, DWMAC_DMA_CH0_RXDESC_TAIL,
                (ULONG)(IPTR)&unit->dwu_RXDesc[DWMAC_RXDESC - 1]);

    unit->dwu_Flags |= IFF_UP;

    D(bug("[dwmac] online, %u Mbit %s duplex%s\n", unit->dwu_SpeedMbps,
          unit->dwu_FullDuplex ? "full" : "half",
          unit->dwu_LinkUp ? "" : " (link still negotiating)");)

    DWMAC_ReportEvents(base, unit, S2EVENT_ONLINE);
}

void DWMAC_GoOffline(struct DWMACBase *base, struct DWMACUnit *unit)
{
    struct dwmac_hw *hw = unit->dwu_HW;
    struct IOSana2Req *request;

    unit->dwu_Flags &= ~IFF_UP;

    DWMAC_Write(hw, DWMAC_DMA_CH0_INT_ENABLE, 0);
    DWMAC_Write(hw, DWMAC_DMA_CH0_TX_CONTROL,
                DWMAC_Read(hw, DWMAC_DMA_CH0_TX_CONTROL) & ~DWMAC_DMA_TX_ST);
    DWMAC_Write(hw, DWMAC_DMA_CH0_RX_CONTROL,
                DWMAC_Read(hw, DWMAC_DMA_CH0_RX_CONTROL) & ~DWMAC_DMA_RX_SR);
    DWMAC_Write(hw, DWMAC_MAC_CONFIG,
                DWMAC_Read(hw, DWMAC_MAC_CONFIG) &
                ~(DWMAC_CONFIG_TE | DWMAC_CONFIG_RE));

    while ((request = (struct IOSana2Req *)
                GetMsg(unit->dwu_RequestPorts[WRITE_QUEUE])) != NULL)
    {
        request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        request->ios2_WireError = S2WERR_UNIT_OFFLINE;
        ReplyMsg((APTR)request);
    }

    DWMAC_ReportEvents(base, unit, S2EVENT_OFFLINE);
}

void DWMAC_FlushUnit(struct DWMACBase *base, struct DWMACUnit *unit,
                     UBYTE last_queue, BYTE error)
{
    struct IOSana2Req *request;
    struct Opener *opener;
    UBYTE i;

    for (i = 0; i <= last_queue; i++)
    {
        while ((request = (struct IOSana2Req *)
                    GetMsg(unit->dwu_RequestPorts[i])) != NULL)
        {
            request->ios2_Req.io_Error = error;
            ReplyMsg((APTR)request);
        }
    }

    ForeachNode(&unit->dwu_Openers, opener)
    {
        while ((request = (struct IOSana2Req *)
                    GetMsg(&opener->read_port)) != NULL)
        {
            request->ios2_Req.io_Error = error;
            ReplyMsg((APTR)request);
        }
    }
}

/* == the rings ========================================================== */

static void DWMAC_InitRings(struct DWMACUnit *unit)
{
    struct dwmac_hw *hw = unit->dwu_HW;
    ULONG i;

    for (i = 0; i < DWMAC_TXDESC; i++)
    {
        unit->dwu_TXDesc[i].des0 = 0;
        unit->dwu_TXDesc[i].des1 = 0;
        unit->dwu_TXDesc[i].des2 = 0;
        unit->dwu_TXDesc[i].des3 = 0;
    }

    for (i = 0; i < DWMAC_RXDESC; i++)
    {
        IPTR buf = (IPTR)(unit->dwu_RXBuf + i * unit->dwu_BufSize);

        unit->dwu_RXDesc[i].des0 = (ULONG)buf;
        unit->dwu_RXDesc[i].des1 = (ULONG)((UQUAD)buf >> 32);
        unit->dwu_RXDesc[i].des2 = 0;
        unit->dwu_RXDesc[i].des3 = DWMAC_RDES3_OWN | DWMAC_RDES3_IOC |
                                   DWMAC_RDES3_BUF1V;
    }

    unit->dwu_TXHead = 0;
    unit->dwu_TXTail = 0;
    unit->dwu_RXCurrent = 0;

    DWMAC_SYNC(unit->dwu_TXDesc,
               (DWMAC_TXDESC + DWMAC_RXDESC) * sizeof(struct dwmac_desc));

    DWMAC_Write(hw, DWMAC_DMA_CH0_CONTROL, 0);
    DWMAC_Write(hw, DWMAC_DMA_CH0_TX_CONTROL,
                (8 << DWMAC_DMA_TX_PBL_SHIFT) | DWMAC_DMA_TX_OSF);
    DWMAC_Write(hw, DWMAC_DMA_CH0_RX_CONTROL,
                (8 << DWMAC_DMA_RX_PBL_SHIFT) |
                (unit->dwu_BufSize << DWMAC_DMA_RX_RBSZ_SHIFT));

    DWMAC_Write(hw, DWMAC_DMA_CH0_TXDESC_HI,
                (ULONG)((UQUAD)(IPTR)unit->dwu_TXDesc >> 32));
    DWMAC_Write(hw, DWMAC_DMA_CH0_TXDESC_LO, (ULONG)(IPTR)unit->dwu_TXDesc);
    DWMAC_Write(hw, DWMAC_DMA_CH0_RXDESC_HI,
                (ULONG)((UQUAD)(IPTR)unit->dwu_RXDesc >> 32));
    DWMAC_Write(hw, DWMAC_DMA_CH0_RXDESC_LO, (ULONG)(IPTR)unit->dwu_RXDesc);

    DWMAC_Write(hw, DWMAC_DMA_CH0_TXDESC_RINGLEN, DWMAC_TXDESC - 1);
    DWMAC_Write(hw, DWMAC_DMA_CH0_RXDESC_RINGLEN, DWMAC_RXDESC - 1);

    DWMAC_Write(hw, DWMAC_DMA_CH0_TXDESC_TAIL,
                (ULONG)(IPTR)&unit->dwu_TXDesc[0]);
    DWMAC_Write(hw, DWMAC_DMA_CH0_RXDESC_TAIL,
                (ULONG)(IPTR)&unit->dwu_RXDesc[DWMAC_RXDESC - 1]);
}

/* == the unit process =================================================== */

static void DWMAC_CheckLink(struct DWMACBase *base, struct DWMACUnit *unit)
{
    ULONG mbps = unit->dwu_SpeedMbps;
    BOOL fullduplex = unit->dwu_FullDuplex;
    BOOL up = DWMAC_PHYGetLink(unit->dwu_HW, &mbps, &fullduplex);

    if (up == unit->dwu_LinkUp &&
        (!up || (mbps == unit->dwu_SpeedMbps &&
                 fullduplex == unit->dwu_FullDuplex)))
        return;

    unit->dwu_LinkUp = up;

    if (up)
    {
        unit->dwu_SpeedMbps = mbps;
        unit->dwu_FullDuplex = fullduplex;
        unit->dwu_Sana2Info.BPS = mbps * 1000000UL;
        dwmac_setspeed(unit->dwu_HW, mbps, fullduplex);
    }

    D(bug("[dwmac] link %s, %u Mbit %s duplex\n", up ? "up" : "down",
          mbps, fullduplex ? "full" : "half");)
}

static void DWMAC_UnitTask(void)
{
    struct DWMACUnit *unit =
        (struct DWMACUnit *)FindTask(NULL)->tc_UserData;
    struct DWMACBase *base = unit->dwu_Base;
    struct MsgPort *input, *timerport = NULL;
    struct timerequest *timerreq = NULL;
    struct IOSana2Req *request;
    BOOL timeropen = FALSE, timerpending = FALSE, running = FALSE;
    ULONG inputsig = 0, timersig = 0, sigs;

    input = CreateMsgPort();
    timerport = CreateMsgPort();
    if (timerport)
        timerreq = (struct timerequest *)
            CreateIORequest(timerport, sizeof(struct timerequest));
    if (timerreq &&
        OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK,
                   (struct IORequest *)timerreq, 0) == 0)
        timeropen = TRUE;

    if (input && timeropen)
    {
        unit->dwu_InputPort = input;
        inputsig = 1UL << input->mp_SigBit;
        timersig = 1UL << timerport->mp_SigBit;
        running = TRUE;

        timerreq->tr_node.io_Command = TR_ADDREQUEST;
        timerreq->tr_time.tv_secs = 1;
        timerreq->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)timerreq);
        timerpending = TRUE;

        Signal(unit->dwu_DeathWatch, SIGF_SINGLE);
    }

    while (running)
    {
        sigs = Wait(inputsig | timersig | SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C)
            running = FALSE;

        if (sigs & inputsig)
        {
            while ((request = (struct IOSana2Req *)GetMsg(input)) != NULL)
            {
                ObtainSemaphore(&unit->dwu_Lock);
                DWMAC_HandleRequest(base, request);
            }
        }

        if (sigs & timersig)
        {
            if (GetMsg(timerport))
            {
                timerpending = FALSE;

                ObtainSemaphore(&unit->dwu_Lock);
                DWMAC_CheckLink(base, unit);
                ReleaseSemaphore(&unit->dwu_Lock);

                if (running)
                {
                    timerreq->tr_node.io_Command = TR_ADDREQUEST;
                    timerreq->tr_time.tv_secs = 1;
                    timerreq->tr_time.tv_micro = 0;
                    SendIO((struct IORequest *)timerreq);
                    timerpending = TRUE;
                }
            }
        }
    }

    unit->dwu_InputPort = NULL;

    if (timeropen)
    {
        if (timerpending)
        {
            AbortIO((struct IORequest *)timerreq);
            WaitIO((struct IORequest *)timerreq);
        }
        CloseDevice((struct IORequest *)timerreq);
    }
    if (timerreq)
        DeleteIORequest((struct IORequest *)timerreq);
    if (timerport)
        DeleteMsgPort(timerport);
    if (input)
    {
        while ((request = (struct IOSana2Req *)GetMsg(input)) != NULL)
        {
            request->ios2_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((APTR)request);
        }
        DeleteMsgPort(input);
    }

    /*
     * The stack and task structure are on tc_MemEntry, so returning -
     * which removes the task - also frees them. Nothing of the unit may
     * be touched after the signal.
     */
    Forbid();
    unit->dwu_Task = NULL;
    Signal(unit->dwu_DeathWatch, SIGF_SINGLE);
}

/* == unit lifetime ====================================================== */

/* A station address must have bits and must not be a group address */
static BOOL dwmac_validmac(const UBYTE *addr)
{
    ULONG i, bits = 0;

    for (i = 0; i < ETH_ADDRESSSIZE; i++)
        bits |= addr[i];

    return (bits != 0) && ((addr[0] & 0x1) == 0);
}

/*
 * Read the interface MTU once, at unit creation, from
 * ENV:SYS/Net/dwmac/unit0/MTU (there is only ever unit 0 per controller).
 * Absent or out-of-range values leave the standard 1500-byte MTU in place.
 * The value drives the advertised MTU, the transmit size limit, and the
 * ring buffer size, so it must be settled before the buffers are allocated.
 */
static void DWMAC_ConfigMTU(struct DWMACUnit *unit)
{
    ULONG mtu = ETH_MTU;
    struct Library *DOSBase;

    if ((DOSBase = OpenLibrary((CONST_STRPTR)"dos.library", 36)) != NULL)
    {
        char value[16];

        if (GetVar((CONST_STRPTR)DWMAC_ENV_MTU_PATH, value, sizeof(value),
                   LV_VAR) > 0)
        {
            LONG v = 0;
            if (StrToLong(value, &v) > 0 && v >= 576)
                mtu = (ULONG)v;
            else
            {
                D(bug("[dwmac] ignoring out-of-range %s='%s'\n",
                      DWMAC_ENV_MTU_PATH, value);)
            }
        }
        CloseLibrary(DOSBase);
    }

    if (mtu > DWMAC_MAX_MTU)
    {
        D(bug("[dwmac] MTU %lu exceeds maximum %lu - clamping\n",
              (unsigned long)mtu, (unsigned long)DWMAC_MAX_MTU);)
        mtu = DWMAC_MAX_MTU;
    }

    /*
     * Store-and-forward (§DWMAC_HWInit) needs the whole frame to fit the MTL
     * FIFO, so a jumbo MTU is only honoured up to what the smaller of the
     * TX/RX FIFOs (from HW_FEATURE1) can buffer.  The standard MTU always
     * fits and is never clamped below.
     */
    {
        ULONG feat1 = DWMAC_Read(unit->dwu_HW, DWMAC_MAC_HW_FEATURE1);
        ULONG txf   = 128UL << DWMAC_HWFEAT1_TXFIFO(feat1);
        ULONG rxf   = 128UL << DWMAC_HWFEAT1_RXFIFO(feat1);
        ULONG fifo  = (txf < rxf) ? txf : rxf;

        if (mtu + ETH_HEADERSIZE + ETH_CRCSIZE > fifo)
        {
            ULONG cap = (fifo > ETH_HEADERSIZE + ETH_CRCSIZE)
                        ? fifo - ETH_HEADERSIZE - ETH_CRCSIZE : ETH_MTU;
            if (cap < ETH_MTU)
                cap = ETH_MTU;
            if (mtu > cap)
            {
                D(bug("[dwmac] MTU %lu exceeds FIFO capacity %lu - "
                      "clamping to %lu\n", (unsigned long)mtu,
                      (unsigned long)fifo, (unsigned long)cap);)
                mtu = cap;
            }
        }
    }

    unit->dwu_MTU      = mtu;
    unit->dwu_FrameMax = mtu + ETH_HEADERSIZE + ETH_CRCSIZE;
    /* One whole frame per buffer, rounded up to a 64-byte cache line (also
     * satisfies the RX DMA buffer-size alignment). */
    unit->dwu_BufSize  = (unit->dwu_FrameMax + 63) & ~63UL;

    D(bug("[dwmac] MTU %lu, frame_max %lu, buffer %lu\n",
          (unsigned long)unit->dwu_MTU, (unsigned long)unit->dwu_FrameMax,
          (unsigned long)unit->dwu_BufSize);)
}

struct DWMACUnit *DWMAC_CreateUnit(struct DWMACBase *base)
{
    struct DWMACUnit *unit;
    struct dwmac_hw *hw = &base->dwm_HW;
    struct Task *task;
    struct MemList *ml;
    UBYTE *stack;
    ULONG i;

    unit = AllocMem(sizeof(struct DWMACUnit), MEMF_PUBLIC | MEMF_CLEAR);
    if (!unit)
        return NULL;

    unit->dwu_Base = base;
    unit->dwu_HW = hw;

    InitSemaphore(&unit->dwu_Lock);
    NEWLIST(&unit->dwu_Openers);
    NEWLIST(&unit->dwu_MulticastRanges);
    NEWLIST(&unit->dwu_TypeTrackers);

    /* Until the PHY reports in, assume the fastest the part can do */
    unit->dwu_SpeedMbps = 1000;
    unit->dwu_FullDuplex = TRUE;

    unit->dwu_TXInt.is_Node.ln_Type = NT_INTERRUPT;
    unit->dwu_TXInt.is_Node.ln_Name = "dwmac tx";
    unit->dwu_TXInt.is_Code = (VOID_FUNC)DWMAC_TXIntF;
    unit->dwu_TXInt.is_Data = unit;

    for (i = 0; i < REQUEST_QUEUE_COUNT; i++)
    {
        struct MsgPort *port =
            AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);

        unit->dwu_RequestPorts[i] = port;
        if (!port)
        {
            DWMAC_DeleteUnit(base, unit);
            return NULL;
        }
        NEWLIST(&port->mp_MsgList);
        port->mp_Flags = PA_IGNORE;
        port->mp_SigTask = &unit->dwu_TXInt;
    }

    DWMAC_ConfigMTU(unit);

    unit->dwu_DescMemSize =
        (DWMAC_TXDESC + DWMAC_RXDESC) * sizeof(struct dwmac_desc) + 63;
    unit->dwu_DescMem = AllocMem(unit->dwu_DescMemSize,
                                 MEMF_PUBLIC | MEMF_CLEAR);
    unit->dwu_BufMemSize =
        (DWMAC_TXDESC + DWMAC_RXDESC) * unit->dwu_BufSize + 63;
    unit->dwu_BufMem = AllocMem(unit->dwu_BufMemSize,
                                MEMF_PUBLIC | MEMF_CLEAR);
    if (!unit->dwu_DescMem || !unit->dwu_BufMem)
    {
        DWMAC_DeleteUnit(base, unit);
        return NULL;
    }

    unit->dwu_TXDesc = (struct dwmac_desc *)
        (((IPTR)unit->dwu_DescMem + 63) & ~63);
    unit->dwu_RXDesc = unit->dwu_TXDesc + DWMAC_TXDESC;
    unit->dwu_TXBuf = (UBYTE *)(((IPTR)unit->dwu_BufMem + 63) & ~63);
    unit->dwu_RXBuf = unit->dwu_TXBuf + DWMAC_TXDESC * unit->dwu_BufSize;

    /*
     * The tree's word beats the filter registers, which beat nothing -
     * but firmware leaves placeholders (all ones, all zeroes) in both,
     * so each source has to earn its keep.
     */
    if (hw->haveMacAddr && dwmac_validmac(hw->macAddr))
        CopyMem(hw->macAddr, unit->dwu_OrgAddr, ETH_ADDRESSSIZE);
    else if (!(DWMAC_GetMACAddress(hw, unit->dwu_OrgAddr) &&
               dwmac_validmac(unit->dwu_OrgAddr)))
    {
        static const UBYTE fallback[ETH_ADDRESSSIZE] =
            { 0x02, 'A', 'R', 'O', 'S', 0x00 };

        D(bug("[dwmac] no usable MAC address anywhere, making one up\n");)
        CopyMem((APTR)fallback, unit->dwu_OrgAddr, ETH_ADDRESSSIZE);
    }
    CopyMem(unit->dwu_OrgAddr, unit->dwu_DevAddr, ETH_ADDRESSSIZE);

    D(bug("[dwmac] station address %02x:%02x:%02x:%02x:%02x:%02x\n",
          unit->dwu_OrgAddr[0], unit->dwu_OrgAddr[1], unit->dwu_OrgAddr[2],
          unit->dwu_OrgAddr[3], unit->dwu_OrgAddr[4], unit->dwu_OrgAddr[5]);)

    if (!DWMAC_HWInit(hw))
    {
        DWMAC_DeleteUnit(base, unit);
        return NULL;
    }

    DWMAC_SetMACAddress(hw, unit->dwu_DevAddr);
    DWMAC_InitRings(unit);
    D(bug("[dwmac] rings ready, tx %p rx %p\n",
          unit->dwu_TXDesc, unit->dwu_RXDesc);)
    DWMAC_PHYInit(hw);

    unit->dwu_Sana2Info.SizeAvailable = sizeof(struct Sana2DeviceQuery);
    unit->dwu_Sana2Info.SizeSupplied = sizeof(struct Sana2DeviceQuery);
    unit->dwu_Sana2Info.DevQueryFormat = 0;
    unit->dwu_Sana2Info.DeviceLevel = 0;
    unit->dwu_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;
    unit->dwu_Sana2Info.MTU = unit->dwu_MTU;
    unit->dwu_Sana2Info.BPS = unit->dwu_SpeedMbps * 1000000UL;
    unit->dwu_Sana2Info.HardwareType = S2WireType_Ethernet;

    if (!hw->irq)
    {
        D(bug("[dwmac] the tree names no interrupt source\n");)
        DWMAC_DeleteUnit(base, unit);
        return NULL;
    }

    unit->dwu_IRQHandler.is_Node.ln_Type = NT_INTERRUPT;
    unit->dwu_IRQHandler.is_Node.ln_Name = "dwmac";
    unit->dwu_IRQHandler.is_Code = (VOID_FUNC)DWMAC_IntHandlerF;
    unit->dwu_IRQHandler.is_Data = unit;
    AddIntServer(INTB_KERNEL + hw->irq, &unit->dwu_IRQHandler);
    unit->dwu_IRQAdded = TRUE;
    D(bug("[dwmac] interrupt server on source %u\n", hw->irq);)

    unit->dwu_RequestPorts[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

    /*
     * The service task. Its stack and control block ride on
     * tc_MemEntry, so task exit frees them without help.
     */
    task = AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
    stack = AllocMem(DWMAC_TASK_STACK, MEMF_PUBLIC);
    ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry),
                  MEMF_PUBLIC | MEMF_CLEAR);
    if (!task || !stack || !ml)
    {
        if (task)
            FreeMem(task, sizeof(struct Task));
        if (stack)
            FreeMem(stack, DWMAC_TASK_STACK);
        if (ml)
            FreeMem(ml, sizeof(struct MemList) + sizeof(struct MemEntry));
        DWMAC_DeleteUnit(base, unit);
        return NULL;
    }

    ml->ml_NumEntries = 2;
    ml->ml_ME[0].me_Addr = task;
    ml->ml_ME[0].me_Length = sizeof(struct Task);
    ml->ml_ME[1].me_Addr = stack;
    ml->ml_ME[1].me_Length = DWMAC_TASK_STACK;

    task->tc_Node.ln_Type = NT_TASK;
    task->tc_Node.ln_Pri = 0;
    task->tc_Node.ln_Name = "dwmac.device";
    task->tc_SPLower = stack;
    task->tc_SPUpper = stack + DWMAC_TASK_STACK;
    task->tc_SPReg = stack + DWMAC_TASK_STACK;
    NEWLIST(&task->tc_MemEntry);
    AddTail(&task->tc_MemEntry, (struct Node *)ml);
    task->tc_UserData = unit;

    unit->dwu_Task = task;
    unit->dwu_DeathWatch = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);

    if (AddTask(task, DWMAC_UnitTask, NULL) == NULL)
    {
        unit->dwu_Task = NULL;
        FreeMem(ml, sizeof(struct MemList) + sizeof(struct MemEntry));
        FreeMem(stack, DWMAC_TASK_STACK);
        FreeMem(task, sizeof(struct Task));
        DWMAC_DeleteUnit(base, unit);
        return NULL;
    }

    Wait(SIGF_SINGLE);
    if (!unit->dwu_InputPort)
    {
        DWMAC_DeleteUnit(base, unit);
        return NULL;
    }

    D(bug("[dwmac] unit ready\n");)

    return unit;
}

void DWMAC_DeleteUnit(struct DWMACBase *base, struct DWMACUnit *unit)
{
    BOOL alive;
    ULONG i;

    if (!unit)
        return;

    if (unit->dwu_Flags & IFF_UP)
        DWMAC_GoOffline(base, unit);

    unit->dwu_DeathWatch = FindTask(NULL);
    Forbid();
    alive = (unit->dwu_Task != NULL);
    if (alive)
        Signal(unit->dwu_Task, SIGBREAKF_CTRL_C);
    Permit();
    if (alive)
        Wait(SIGF_SINGLE);

    if (unit->dwu_IRQAdded)
        RemIntServer(INTB_KERNEL + unit->dwu_HW->irq, &unit->dwu_IRQHandler);

    DWMAC_FlushUnit(base, unit, GENERAL_QUEUE, IOERR_ABORTED);

    for (i = 0; i < REQUEST_QUEUE_COUNT; i++)
    {
        if (unit->dwu_RequestPorts[i])
            FreeMem(unit->dwu_RequestPorts[i], sizeof(struct MsgPort));
    }

    if (unit->dwu_DescMem)
        FreeMem(unit->dwu_DescMem, unit->dwu_DescMemSize);
    if (unit->dwu_BufMem)
        FreeMem(unit->dwu_BufMem, unit->dwu_BufMemSize);

    FreeMem(unit, sizeof(struct DWMACUnit));
}
