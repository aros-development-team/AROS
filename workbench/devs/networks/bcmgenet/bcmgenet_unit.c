/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Broadcom GENETv5 SANAII driver, the unit: address bookkeeping
          is generic SANA-II ceremony and fully implemented below; the
          ring/interrupt/hardware pieces are left as TODOs for the port
          from OpenBSD's bcmgenet.c.
*/
#include "exec/tasks.h"
#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <dos/dos.h>
#include <devices/newstyle.h>
#include <devices/sana2specialstats.h>
#include <aros/macros.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include "bcmgenet.h"

#undef UtilityBase

static const UBYTE bcmgenet_broadcast[ETH_ADDRESSSIZE] =
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/* == small helpers ====================================================== */
static void BCMGENET_UnitTask(void);

#define BCMGENET_TASK_STACK 32768

static BOOL bcmgenet_sameaddr(const UBYTE *a, const UBYTE *b)
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
static void bcmgenet_splitaddr(const UBYTE *addr, ULONG *left, UWORD *right)
{
    *left = ((ULONG)addr[0] << 24) | ((ULONG)addr[1] << 16) |
            ((ULONG)addr[2] << 8) | addr[3];
    *right = ((UWORD)addr[4] << 8) | addr[5];
}

struct TypeStats *BCMGENET_FindTypeStats(struct MinList *list, ULONG packet_type)
{
    struct TypeStats *stats;

    ForeachNode(list, stats)
    {
        if (stats->packet_type == packet_type)
            return stats;
    }

    return NULL;
}

void BCMGENET_ReportEvents(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                           ULONG events)
{
    struct IOSana2Req *request, *tail, *next;
    struct List *list;

    list = &unit->bgu_RequestPorts[EVENT_QUEUE]->mp_MsgList;
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

/* == multicast address ranges ============================================ */

static struct AddressRange *bcmgenet_findrange(struct BCMGENETUnit *unit,
    ULONG lower_left, UWORD lower_right, ULONG upper_left, UWORD upper_right)
{
    struct AddressRange *range;

    ForeachNode(&unit->bgu_MulticastRanges, range)
    {
        if (range->lower_bound_left == lower_left &&
            range->lower_bound_right == lower_right &&
            range->upper_bound_left == upper_left &&
            range->upper_bound_right == upper_right)
            return range;
    }

    return NULL;
}

BOOL BCMGENET_AddMulticastRange(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                                const UBYTE *lower, const UBYTE *upper)
{
    struct AddressRange *range;
    ULONG lower_left, upper_left;
    UWORD lower_right, upper_right;

    bcmgenet_splitaddr(lower, &lower_left, &lower_right);
    bcmgenet_splitaddr(upper, &upper_left, &upper_right);

    range = bcmgenet_findrange(unit, lower_left, lower_right,
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
    AddTail((struct List *)&unit->bgu_MulticastRanges, (struct Node *)range);
    Enable();
    unit->bgu_RangeCount++;

    /* TODO: also program a GENET_UMAC_MDF_ADDR0/1(n) hardware filter
     * entry so multicast frames outside promiscuous mode actually
     * reach the ring - see genet_setup_rxfilter_mdf() (bcmgenet.c:380). */

    return TRUE;
}

BOOL BCMGENET_RemMulticastRange(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                                const UBYTE *lower, const UBYTE *upper)
{
    struct AddressRange *range;
    ULONG lower_left, upper_left;
    UWORD lower_right, upper_right;

    bcmgenet_splitaddr(lower, &lower_left, &lower_right);
    bcmgenet_splitaddr(upper, &upper_left, &upper_right);

    range = bcmgenet_findrange(unit, lower_left, lower_right,
                               upper_left, upper_right);
    if (!range)
        return FALSE;

    if (--range->add_count == 0)
    {
        Disable();
        Remove((struct Node *)range);
        Enable();
        FreeMem(range, sizeof(struct AddressRange));
        unit->bgu_RangeCount--;
    }

    return TRUE;
}

BOOL BCMGENET_AddressFilter(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                            UBYTE *address)
{
    struct AddressRange *range;
    ULONG address_left;
    UWORD address_right;

    if (unit->bgu_Flags & IFF_PROMISC)
        return TRUE;

    if ((address[0] & 0x1) == 0)
        return TRUE;

    if (bcmgenet_sameaddr(address, bcmgenet_broadcast))
        return TRUE;

    bcmgenet_splitaddr(address, &address_left, &address_right);

    ForeachNode(&unit->bgu_MulticastRanges, range)
    {
        if ((address_left > range->lower_bound_left ||
             (address_left == range->lower_bound_left &&
              address_right >= range->lower_bound_right)) &&
            (address_left < range->upper_bound_left ||
             (address_left == range->upper_bound_left &&
              address_right <= range->upper_bound_right)))
            return TRUE;
    }

    unit->bgu_SpecialStats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    return FALSE;
}

#define GENET_DMA_ALIGN                 64

static BOOL bcmgenet_alloc_ring(struct bcmgenet_ring *ring)
{
    IPTR base;
    ULONG i;

    ring->bufmemsize = GENET_DMA_DESC_COUNT * GENET_BUFSIZE + GENET_DMA_ALIGN - 1;
    ring->bufmem = AllocMem(ring->bufmemsize, MEMF_PUBLIC | MEMF_CLEAR);
    if (!ring->bufmem)
        return FALSE;

    base = ((IPTR)ring->bufmem + GENET_DMA_ALIGN - 1) & ~(IPTR)(GENET_DMA_ALIGN - 1);

    for (i = 0; i < GENET_DMA_DESC_COUNT; i++)
        ring->buf[i] = (UBYTE *)(base + i * GENET_BUFSIZE);

    return TRUE;
}

static void bcmgenet_free_ring(struct bcmgenet_ring *ring)
{
    if (ring->bufmem)
    {
        FreeMem(ring->bufmem, ring->bufmemsize);
        ring->bufmem = NULL;
    }
}

/* == unit lifecycle ======================================================= */
static AROS_INTH1(BCMGENET_IntHandler, struct BCMGENETUnit *, unit)
{
    struct bcmgenet_hw *hw = unit->bgu_HW;
    ULONG status;

    AROS_INTFUNC_INIT

    status = BCMGENET_Read(hw, GENET_INTRL2_CPU_STAT);
    status &= ~BCMGENET_Read(hw, GENET_INTRL2_CPU_STAT_MASK);

    if ((status & GENET_IRQ_TXDMA_DONE) != 0)
    {
        BCMGENET_Write(hw, GENET_INTRL2_CPU_CLEAR,
                       GENET_IRQ_TXDMA_DONE);

        D(bug("[bcmgenet] TX IRQ\n");)
        unit->bgu_IRQPending |= GENET_IRQ_TXDMA_DONE;
        Signal(unit->bgu_Task, unit->bgu_IRQSignal);
    }

    if ((status & GENET_IRQ_RXDMA_DONE) != 0)
    {
        BCMGENET_Write(hw, GENET_INTRL2_CPU_CLEAR,
                       GENET_IRQ_RXDMA_DONE);

        unit->bgu_IRQPending |= GENET_IRQ_RXDMA_DONE;
        Signal(unit->bgu_Task, unit->bgu_IRQSignal);
    }

    return FALSE;
    AROS_INTFUNC_EXIT
}

struct BCMGENETUnit *BCMGENET_CreateUnit(struct BCMGENETBase *base)
{
    struct BCMGENETUnit *unit;
    struct Task *task;
    struct MemList *ml;
    UBYTE *stack;

    unit = AllocMem(sizeof(struct BCMGENETUnit), MEMF_PUBLIC | MEMF_CLEAR);
    if (!unit)
        return NULL;

    unit->bgu_Base = base;
    unit->bgu_HW = &base->bgm_HW;
    unit->bgu_Sana2Info.SizeAvailable = sizeof(struct Sana2DeviceQuery);
    unit->bgu_Sana2Info.SizeSupplied = sizeof(struct Sana2DeviceQuery);
    unit->bgu_Sana2Info.DevQueryFormat = 0;
    unit->bgu_Sana2Info.DeviceLevel = 0;
    unit->bgu_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;
    unit->bgu_Sana2Info.MTU = ETH_MTU;
    unit->bgu_Sana2Info.HardwareType = S2WireType_Ethernet;

    InitSemaphore(&unit->bgu_Lock);
    NEWLIST((struct List *)&unit->bgu_Openers);
    NEWLIST((struct List *)&unit->bgu_MulticastRanges);
    NEWLIST((struct List *)&unit->bgu_TypeTrackers);

    if (unit->bgu_HW->haveMacAddr)
        CopyMem(unit->bgu_HW->macAddr, unit->bgu_DevAddr, ETH_ADDRESSSIZE);
    else
        BCMGENET_GetMACAddress(unit->bgu_HW, unit->bgu_DevAddr);

    CopyMem(unit->bgu_DevAddr, unit->bgu_OrgAddr, ETH_ADDRESSSIZE);

    unit->bgu_TimerPort = CreateMsgPort();
    if (!unit->bgu_TimerPort)
        goto fail;

    unit->bgu_TimerReq = (struct timerequest *)
        CreateIORequest(unit->bgu_TimerPort, sizeof(*unit->bgu_TimerReq));
    if (!unit->bgu_TimerReq)
        goto fail;

    if (OpenDevice(TIMERNAME, UNIT_MICROHZ,
                   (struct IORequest *)unit->bgu_TimerReq, 0) != 0)
        goto fail;

    bug("[bcmgenet] before hardware reset\n");

    ULONG rev = BCMGENET_Read(unit->bgu_HW, GENET_SYS_REV_CTRL);
    D(bug("[bcmgenet] SYS_REV_CTRL = %08lx\n", rev);)
    /*
     * Read the firmware-programmed station address before reset: UniMAC
     * reset may clear the MAC address registers.
     */
    if (!BCMGENET_HWReset(unit))
    {
        D(bug("[bcmgenet] hardware reset failed\n");)
        FreeMem(unit, sizeof(*unit));
        return NULL;
    }

    bug("[bcmgenet] hardware reset succeeded\n");

    /* Initialise ring buffer elements */
    if (!bcmgenet_alloc_ring(&unit->bgu_RX) ||
        !bcmgenet_alloc_ring(&unit->bgu_TX))
        goto fail;

    BCMGENET_SetMACAddress(unit->bgu_HW, unit->bgu_DevAddr);

    if (!BCMGENET_HWInit(unit))
        goto fail;

    if (!BCMGENET_PHYInit(unit))
        goto fail;

    /* TODO:
     * IRQ handler registration. Free 'unit' and
     * return NULL on any failure past this point. */
    /*
     * irq[0] (INTRL2_0) carries the default queue's TXDMA/RXDMA_DONE -
     * the bits the 0x200 register block exposes. irq[1] (INTRL2_1) is
     * only the per-ring bits for priority queues 0-15, which this
     * driver never enables.
     */
    unit->bgu_IRQHandler[0].is_Node.ln_Type = NT_INTERRUPT;
    unit->bgu_IRQHandler[0].is_Node.ln_Name = "bcmgenet";
    unit->bgu_IRQHandler[0].is_Code = (VOID_FUNC)BCMGENET_IntHandler;
    unit->bgu_IRQHandler[0].is_Data = unit;
    AddIntServer(INTB_KERNEL + unit->bgu_HW->irq[0],
        &unit->bgu_IRQHandler[0]);
    unit->bgu_IRQAdded[0] = TRUE;

    /*
     * The service task. Its stack and control block ride on
     * tc_MemEntry, so task exit frees them without help.
     */
    task = AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
    stack = AllocMem(BCMGENET_TASK_STACK, MEMF_PUBLIC);
    ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry),
                  MEMF_PUBLIC | MEMF_CLEAR);
    if (!task || !stack || !ml)
    {
        if (task)
            FreeMem(task, sizeof(struct Task));
        if (stack)
            FreeMem(stack, BCMGENET_TASK_STACK);
        if (ml)
            FreeMem(ml, sizeof(struct MemList) + sizeof(struct MemEntry));
        goto fail;
    }

    for (ULONG i = 0; i < REQUEST_QUEUE_COUNT; i++)
    {
        struct MsgPort *port;

        port = AllocMem(sizeof(*port), MEMF_PUBLIC | MEMF_CLEAR);
        if (!port)
            goto fail;

        NEWLIST(&port->mp_MsgList);
        port->mp_Flags = PA_IGNORE;

        #if defined(__AROSEXEC_SMP__)
            port->mp_SpinLock = 0;
        #endif

        unit->bgu_RequestPorts[i] = port;
    }

    ml->ml_NumEntries = 2;
    ml->ml_ME[0].me_Addr = task;
    ml->ml_ME[0].me_Length = sizeof(struct Task);
    ml->ml_ME[1].me_Addr = stack;
    ml->ml_ME[1].me_Length = BCMGENET_TASK_STACK;

    task->tc_Node.ln_Type = NT_TASK;
    task->tc_Node.ln_Pri = 0;
    task->tc_Node.ln_Name = "BCMGENET.device";
    task->tc_SPLower = stack;
    task->tc_SPUpper = stack + BCMGENET_TASK_STACK;
    task->tc_SPReg = stack + BCMGENET_TASK_STACK;
    NEWLIST(&task->tc_MemEntry);
    AddTail(&task->tc_MemEntry, (struct Node *)ml);
    task->tc_UserData = unit;

    unit->bgu_Task = task;
    unit->bgu_DeathWatch = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);

    if (AddTask(task, BCMGENET_UnitTask, NULL) == NULL)
    {
        unit->bgu_Task = NULL;
        FreeMem(ml, sizeof(struct MemList) + sizeof(struct MemEntry));
        FreeMem(stack, BCMGENET_TASK_STACK);
        FreeMem(task, sizeof(struct Task));
        goto fail;
    }

    Wait(SIGF_SINGLE);
    if (!unit->bgu_InputPort)
    {
        goto fail;
    }

    return unit;

fail:
    bug("[bcmgenet] unit failed");
    BCMGENET_DeleteUnit(base, unit);
    return 0;
}

void BCMGENET_DeleteUnit(struct BCMGENETBase *base, struct BCMGENETUnit *unit)
{
    BOOL alive;

    if (!unit)
        return;

    /* The IRQ handler keeps a pointer to unit in is_Data. */
    BCMGENET_Write(unit->bgu_HW, GENET_INTRL2_CPU_SET_MASK,
                   GENET_IRQ_TXDMA_DONE);
    BCMGENET_Write(unit->bgu_HW, GENET_INTRL2_CPU_CLEAR,
                   GENET_IRQ_TXDMA_DONE);

    if (unit->bgu_IRQAdded[0])
    {
        RemIntServer(INTB_KERNEL + unit->bgu_HW->irq[0],
                     &unit->bgu_IRQHandler[0]);
        unit->bgu_IRQAdded[0] = FALSE;
    }

    /* The task may still be using the timer, queues, and DMA rings. */
    unit->bgu_DeathWatch = FindTask(NULL);
    Forbid();
    alive = (unit->bgu_Task != NULL);
    if (alive)
        Signal(unit->bgu_Task, SIGBREAKF_CTRL_C);
    Permit();

    if (alive)
        Wait(SIGF_SINGLE);

    /* No task or interrupt handler can access these after this point. */
    for (ULONG i = 0; i < REQUEST_QUEUE_COUNT; i++)
    {
        if (unit->bgu_RequestPorts[i])
        {
            FreeMem(unit->bgu_RequestPorts[i],
                    sizeof(*unit->bgu_RequestPorts[i]));
            unit->bgu_RequestPorts[i] = NULL;
        }
    }

    bcmgenet_free_ring(&unit->bgu_RX);
    bcmgenet_free_ring(&unit->bgu_TX);

    FreeMem(unit, sizeof(struct BCMGENETUnit));
}

/* TODO: port the RXEN/TXEN half of genet_init()/genet_stop()
 * (bcmgenet.c:583, 632). */
void BCMGENET_GoOnline(struct BCMGENETBase *base, struct BCMGENETUnit *unit)
{
    struct bcmgenet_hw *hw = unit->bgu_HW;
    ULONG cmd;

    /* Fjern eventuelle gamle, latchede TX-avbrudd før unmaskering. */
    BCMGENET_Write(hw, GENET_INTRL2_CPU_CLEAR, GENET_IRQ_TXDMA_DONE | GENET_IRQ_RXDMA_DONE);

    /* Aktiver bare TX-completion-avbrudd. */
    BCMGENET_Write(hw, GENET_INTRL2_CPU_CLEAR_MASK,
                   GENET_IRQ_TXDMA_DONE);

    /* TX-MAC må være på for at GENET faktisk skal sende. */
    cmd = BCMGENET_Read(hw, GENET_UMAC_CMD);
    cmd |= GENET_UMAC_CMD_TXEN;
    BCMGENET_Write(hw, GENET_UMAC_CMD, cmd);

    unit->bgu_Flags |= IFF_UP;
}

void BCMGENET_GoOffline(struct BCMGENETBase *base, struct BCMGENETUnit *unit)
{
    unit->bgu_Flags &= ~IFF_UP;
}

/*
 * Port of genet_qstart()/genet_setup_txbuf() (bcmgenet.c:773, 204) for a
 * single request. The frame is copied into the slot's own buffer, so
 * nothing depends on the caller's data once this returns.
 *
 * Returns TRUE when the request is consumed - handed to the engine (the
 * TX-done path replies it) or failed and replied here - and FALSE only
 * when the ring is full and it should be retried later.
 */
BOOL BCMGENET_SendPacket(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                         struct IOSana2Req *request)
{
    struct bcmgenet_ring *tx = &unit->bgu_TX;
    struct bcmgenet_hw *hw = unit->bgu_HW;
    struct Opener *opener = request->ios2_BufferManagement;
    struct TypeStats *tracker;
    UBYTE *buffer;
    ULONG index, queued, data_size, packet_size, len;
    UWORD packet_type;
    UQUAD dma;
    BOOL copied;

    queued = (tx->pidx - tx->cidx) & 0xffff;
    if (queued >= GENET_DMA_DESC_COUNT)
        return FALSE;

    index = tx->pidx & (GENET_DMA_DESC_COUNT - 1);
    buffer = tx->buf[index];
    data_size = packet_size = request->ios2_DataLength;

    if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
    {
        struct eth_frame *frame = (struct eth_frame *)buffer;

        packet_size += ETH_HEADERSIZE;
        CopyMem(request->ios2_DstAddr, frame->eth_packet_dest,
                ETH_ADDRESSSIZE);
        CopyMem(unit->bgu_DevAddr, frame->eth_packet_source,
                ETH_ADDRESSSIZE);
        frame->eth_packet_type =
            AROS_WORD2BE((UWORD)request->ios2_PacketType);

        copied = opener->tx_function(frame->eth_packet_data,
                                     request->ios2_Data, data_size);
    }
    else
        copied = opener->tx_function(buffer, request->ios2_Data, data_size);

    if (!copied)
    {
        request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        request->ios2_WireError = S2WERR_BUFF_ERROR;
        BCMGENET_ReportEvents(base, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE |
                              S2EVENT_BUFF | S2EVENT_TX);
        ReplyMsg(&request->ios2_Req.io_Message);
        return TRUE;
    }

    /* UniMAC appends the CRC but does not pad, so a runt would go out short */
    while (packet_size < ETH_ZLEN)
        buffer[packet_size++] = 0;

    packet_type = (UWORD)request->ios2_PacketType;

    len = packet_size;
    dma = (UQUAD)(IPTR)CachePreDMA(buffer, &len, DMA_ReadFromRAM);

    BCMGENET_Write(hw, GENET_TX_DESC_ADDRESS_LO(index), (ULONG)dma);
    BCMGENET_Write(hw, GENET_TX_DESC_ADDRESS_HI(index), (ULONG)(dma >> 32));

    /*
     * There is no OWN bit to set on the TX side either - advancing the
     * producer index below is what hands the slot over. QTAG goes out
     * with the whole field set, as genet_setup_txbuf() does.
     */
    BCMGENET_Write(hw, GENET_TX_DESC_STATUS(index),
                   GENET_TX_DESC_STATUS_SOP | GENET_TX_DESC_STATUS_EOP |
                   GENET_TX_DESC_STATUS_CRC |
                   GENET_TX_DESC_STATUS_QTAG_MASK |
                   ((packet_size << GENET_TX_DESC_STATUS_BUFLEN_SHIFT) &
                    GENET_TX_DESC_STATUS_BUFLEN_MASK));

    tx->request[index] = request;

    tx->pidx = (tx->pidx + 1) & 0xffff;
    BCMGENET_Write(hw, GENET_TX_DMA_PROD_INDEX(GENET_DMA_DEFAULT_QUEUE),
                   tx->pidx);

    D(bug("[bcmgenet] TX hw prod=%08lx cons=%08lx\n",
          BCMGENET_Read(hw, GENET_TX_DMA_PROD_INDEX(GENET_DMA_DEFAULT_QUEUE)),
          BCMGENET_Read(hw, GENET_TX_DMA_CONS_INDEX(GENET_DMA_DEFAULT_QUEUE)));)
    D(bug("[bcmgenet] TX slot %lu, len %lu, prod %lu\n",
          index, packet_size, tx->pidx);)

    unit->bgu_Stats.PacketsSent++;
    tracker = BCMGENET_FindTypeStats(&unit->bgu_TypeTrackers, packet_type);
    if (tracker)
    {
        tracker->stats.PacketsSent++;
        tracker->stats.BytesSent += packet_size;
    }

    return TRUE;
}

/*
 * Push queued write requests into the ring until it fills or the queue
 * runs dry. Runs only in the unit task; CmdWrite may execute in the
 * caller's context (BeginIO's AttemptSemaphore path) but only ever
 * PutMsg()s here, and the port's PA_SIGNAL wakes the task.
 */
static void bcmgenet_tx_drain(struct BCMGENETBase *base,
                              struct BCMGENETUnit *unit)
{
    struct MsgPort *port = unit->bgu_RequestPorts[WRITE_QUEUE];
    struct IOSana2Req *request;

    while ((request = (struct IOSana2Req *)GetMsg(port)) != NULL)
    {
        ObtainSemaphore(&unit->bgu_Lock);

        if (!BCMGENET_SendPacket(base, unit, request))
        {
            /*
             * Ring full. Back at the *head* - PutMsg would reorder the
             * stream and, with PA_SIGNAL, re-wake the task into a spin.
             * The TX-done interrupt gets the queue moving again.
             */
            Disable();
            AddHead(&port->mp_MsgList,
                    &request->ios2_Req.io_Message.mn_Node);
            Enable();
            ReleaseSemaphore(&unit->bgu_Lock);
            break;
        }

        ReleaseSemaphore(&unit->bgu_Lock);
    }
}

static void bcmgenet_tx_complete(struct BCMGENETBase *base,
                                 struct BCMGENETUnit *unit, ULONG qid)
{
    struct bcmgenet_ring *tx = &unit->bgu_TX;
    struct bcmgenet_hw *hw = unit->bgu_HW;
    ULONG queued, index;
    struct IOSana2Req *request;

    queued = (BCMGENET_Read(hw, GENET_TX_DMA_CONS_INDEX(qid)) -
              tx->cidx) & 0xffff;

    while (queued-- != 0)
    {
        index = tx->next & (GENET_DMA_DESC_COUNT - 1);
        request = tx->request[index];

        if (request)
        {
            tx->request[index] = NULL;
            request->ios2_Req.io_Error = 0;
            ReplyMsg(&request->ios2_Req.io_Message);
        }

        tx->cidx++;
        tx->next = (tx->next + 1) & (GENET_DMA_DESC_COUNT - 1);
    }

    BCMGENET_ReportEvents(base, unit, S2EVENT_TX);
}

/*
 * TODO (step 3): the dwmac-style opener dispatch - packet-type match
 * against each opener's read queue, rx_function copy, SANA2IOF_RAW,
 * orphans to ADOPT_QUEUE, type trackers. See DWMAC_RXPacket()
 * (dwmac_unit.c:228). Until then received frames are only logged and
 * counted, which is enough to see an OFFER arrive in the capture.
 */
static void bcmgenet_rx_packet(struct BCMGENETBase *base,
                               struct BCMGENETUnit *unit,
                               struct eth_frame *frame, ULONG length)
{
    D(bug("[bcmgenet] RX %02x:%02x:%02x:%02x:%02x:%02x -> "
          "%02x:%02x:%02x:%02x:%02x:%02x type %04x len %lu\n",
          frame->eth_packet_source[0], frame->eth_packet_source[1],
          frame->eth_packet_source[2], frame->eth_packet_source[3],
          frame->eth_packet_source[4], frame->eth_packet_source[5],
          frame->eth_packet_dest[0], frame->eth_packet_dest[1],
          frame->eth_packet_dest[2], frame->eth_packet_dest[3],
          frame->eth_packet_dest[4], frame->eth_packet_dest[5],
          AROS_BE2WORD(frame->eth_packet_type), length);)
}

/*
 * Port of genet_rxintr() (bcmgenet.c:682). The engine bumped its
 * producer index past our shadow; every slot in between holds a frame.
 * Advancing the consumer index at the end is what re-arms the slots -
 * the address registers still hold the same buffers, so unlike the
 * OpenBSD original there is no per-slot refill to do.
 */
static void bcmgenet_rx_process(struct BCMGENETBase *base,
                                struct BCMGENETUnit *unit, ULONG qid)
{
    struct bcmgenet_ring *rx = &unit->bgu_RX;
    struct bcmgenet_hw *hw = unit->bgu_HW;
    ULONG pidx, total, index, n;

    pidx = BCMGENET_Read(hw, GENET_RX_DMA_PROD_INDEX(qid)) & 0xffff;
    total = (pidx - rx->pidx) & 0xffff;

    D(bug("[bcmgenet] RX pidx=%04lx total=%lu\n", pidx, total);)

    index = rx->next;
    for (n = 0; n < total; n++)
    {
        ULONG status = BCMGENET_Read(hw, GENET_RX_DESC_STATUS(index));
        ULONG len = (status & GENET_RX_DESC_STATUS_BUFLEN_MASK) >>
                    GENET_RX_DESC_STATUS_BUFLEN_SHIFT;
        UBYTE *buf = rx->buf[index];
        ULONG postlen = GENET_BUFSIZE;

        /* Drop the cached lines so the engine's frame is what gets read */
        CachePostDMA(buf, &postlen, 0);

        /*
         * A frame that does not fit one buffer arrives split across
         * slots with SOP/EOP marking the pieces; at 1536 bytes per
         * buffer and CRC included that cannot happen at our MTU, so
         * anything partial is treated as an error rather than glued.
         * OpenBSD leaves this unchecked ("XXX check for errors").
         */
        if ((status & (GENET_RX_DESC_STATUS_SOP | GENET_RX_DESC_STATUS_EOP))
                != (GENET_RX_DESC_STATUS_SOP | GENET_RX_DESC_STATUS_EOP) ||
            (status & GENET_RX_DESC_STATUS_RX_ERROR) != 0)
        {
            D(bug("[bcmgenet] RX slot %lu bad status %08lx\n", index, status);)
            unit->bgu_Stats.BadData++;
        }
        else if (len > 2 + ETH_HEADERSIZE)
        {
            /*
             * RBUF_ALIGN_2B put two pad bytes ahead of the frame and
             * counted them in BUFLEN; step past them. The tail still
             * carries the 4-byte CRC - the openers do not mind, and
             * OpenBSD keeps it too (it trims only the two pad bytes).
             */
            bcmgenet_rx_packet(base, unit,
                               (struct eth_frame *)(buf + 2), len - 2);
            unit->bgu_Stats.PacketsReceived++;
        }
        else
            unit->bgu_Stats.BadData++;

        index = (index + 1) & (GENET_DMA_DESC_COUNT - 1);
    }

    if (total != 0)
    {
        rx->next = index;
        rx->pidx = pidx;

        /* Handing the consumed slots back is what keeps the ring fed */
        rx->cidx = (rx->cidx + total) & 0xffff;
        BCMGENET_Write(hw, GENET_RX_DMA_CONS_INDEX(qid), rx->cidx);

        BCMGENET_ReportEvents(base, unit, S2EVENT_RX);
    }
}

static void BCMGENET_UnitTask(void)
{

    struct BCMGENETUnit *unit = (struct BCMGENETUnit *)FindTask(NULL)->tc_UserData;
    struct BCMGENETBase *base = unit->bgu_Base;
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

    LONG irqbit = AllocSignal(-1);
    if (irqbit >= 0) {
        unit->bgu_IRQSignal = 1UL << irqbit;
    }

    if (input && timeropen)
    {
        struct MsgPort *writeport = unit->bgu_RequestPorts[WRITE_QUEUE];

        unit->bgu_InputPort = input;
        inputsig = 1UL << input->mp_SigBit;
        timersig = 1UL << timerport->mp_SigBit;

        /*
         * CmdWrite queues here from whatever context BeginIO ran in.
         * Sharing the input port's signal means one Wait() mask covers
         * both; the drain below runs on either.
         */
        writeport->mp_SigTask = FindTask(NULL);
        writeport->mp_SigBit = input->mp_SigBit;
        writeport->mp_Flags = PA_SIGNAL;

        running = TRUE;

        timerreq->tr_node.io_Command = TR_ADDREQUEST;
        timerreq->tr_time.tv_secs = 1;
        timerreq->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)timerreq);
        timerpending = TRUE;

        Signal(unit->bgu_DeathWatch, SIGF_SINGLE);
    }

    while (running)
    {
        sigs = Wait(inputsig | timersig | unit->bgu_IRQSignal | SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C)
            running = FALSE;

        if (sigs & inputsig)
        {
            while ((request = (struct IOSana2Req *)GetMsg(input)) != NULL)
            {
                ObtainSemaphore(&unit->bgu_Lock);
                BCMGENET_HandleRequest(base, request);
            }

            bcmgenet_tx_drain(base, unit);
        }

        if (sigs & timersig)
        {
            if (GetMsg(timerport))
            {
                timerpending = FALSE;

                ObtainSemaphore(&unit->bgu_Lock);
                BCMGENET_CheckLink(base, unit);
                ReleaseSemaphore(&unit->bgu_Lock);

                if (running)
                {
                    timerreq->tr_node.io_Command = TR_ADDREQUEST;
                    timerreq->tr_time.tv_secs = 1;
                    timerreq->tr_time.tv_micro = 0;
                    SendIO((struct IORequest *)timerreq);
                    timerpending = TRUE;

                    D(bug("[bcmgenet] TX hw prod=%08lx cons=%08lx\n",
                          BCMGENET_Read(unit->bgu_HW, GENET_TX_DMA_PROD_INDEX(GENET_DMA_DEFAULT_QUEUE)),
                          BCMGENET_Read(unit->bgu_HW, GENET_TX_DMA_CONS_INDEX(GENET_DMA_DEFAULT_QUEUE)));)
                }
            }
        }

        if (sigs & unit->bgu_IRQSignal)
        {
            ULONG pending;

            Disable();
            pending = unit->bgu_IRQPending;
            unit->bgu_IRQPending = 0;
            Enable();

            if (pending & GENET_IRQ_TXDMA_DONE)
            {
                bcmgenet_tx_complete(base, unit, GENET_DMA_DEFAULT_QUEUE);
                /* Freed slots may unblock requests parked on a full ring */
                bcmgenet_tx_drain(base, unit);
            }

            if (pending & GENET_IRQ_RXDMA_DONE)
                bcmgenet_rx_process(base, unit, GENET_DMA_DEFAULT_QUEUE);
        }
    }

    unit->bgu_InputPort = NULL;

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
    if (irqbit >= 0)
        FreeSignal(irqbit);

    /*
     * The stack and task structure are on tc_MemEntry, so returning -
     * which removes the task - also frees them. Nothing of the unit may
     * be touched after the signal.
     */
    Forbid();
    unit->bgu_Task = NULL;
    Signal(unit->bgu_DeathWatch, SIGF_SINGLE);

}

/*
 * Called once a second from the unit task, under bgu_Lock. Reports what
 * the PHY negotiated; BCMGENET_PHYGetLink() has already pushed the speed
 * into UniMAC, so all that is left here is the bookkeeping the rest of
 * the driver reads (CmdDeviceQuery derives BPS from bgu_SpeedMbps).
 */
BOOL BCMGENET_CheckLink(struct BCMGENETBase *base, struct BCMGENETUnit *unit)
{
    ULONG mbps = unit->bgu_SpeedMbps;
    BOOL fullduplex = unit->bgu_FullDuplex;
    BOOL up;

    up = BCMGENET_PHYGetLink(unit->bgu_HW, &mbps, &fullduplex);

    /* Nothing to say unless the link or the negotiated mode moved */
    if (up == unit->bgu_LinkUp &&
        (!up || (mbps == unit->bgu_SpeedMbps &&
                 fullduplex == unit->bgu_FullDuplex)))
        return up;

    unit->bgu_LinkUp = up;

    if (up)
    {
        unit->bgu_SpeedMbps = mbps;
        unit->bgu_FullDuplex = fullduplex;

        unit->bgu_Sana2Info.BPS = mbps * 1000000UL;
    }
    else
    {
        unit->bgu_Sana2Info.BPS = 0;
    }
    D(bug("[bcmgenet] link %s, %lu Mbit %s duplex\n", up ? "up" : "down",
          mbps, fullduplex ? "full" : "half");)

    return up;
}
