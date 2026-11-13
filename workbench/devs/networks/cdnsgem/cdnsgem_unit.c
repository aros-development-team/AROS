/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Cadence GEM SANA-II driver, the unit: address bookkeeping, the
          receive filter, the descriptor rings and the service task that
          drives them. The SANA-II half is bcmgenet's; the ring handling
          follows cad_encap()/cad_rxeof()/cad_txeof() in OpenBSD if_cad.c.
*/

/* Bring-up diagnostics: unit lifecycle and link changes, nothing per frame. */
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

#include "cdnsgem.h"

#undef UtilityBase

static const UBYTE cdnsgem_broadcast[ETH_ADDRESSSIZE] =
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static void CDNSGEM_UnitTask(void);

#define CDNSGEM_TASK_STACK 32768

static inline void __dsb(void) { asm volatile("dsb sy" ::: "memory"); }
static inline void __dmb(void) { asm volatile("dmb sy" ::: "memory"); }

static BOOL cdnsgem_sameaddr(const UBYTE *a, const UBYTE *b)
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
static void cdnsgem_splitaddr(const UBYTE *addr, ULONG *left, UWORD *right)
{
    *left = ((ULONG)addr[0] << 24) | ((ULONG)addr[1] << 16) |
            ((ULONG)addr[2] << 8) | addr[3];
    *right = ((UWORD)addr[4] << 8) | addr[5];
}

struct TypeStats *CDNSGEM_FindTypeStats(struct MinList *list, ULONG packet_type)
{
    struct TypeStats *stats;

    ForeachNode(list, stats)
    {
        if (stats->packet_type == packet_type)
            return stats;
    }

    return NULL;
}

void CDNSGEM_ReportEvents(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                          ULONG events)
{
    struct IOSana2Req *request, *tail, *next;
    struct List *list;

    list = &unit->cgu_RequestPorts[EVENT_QUEUE]->mp_MsgList;
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

static struct AddressRange *cdnsgem_findrange(struct CDNSGEMUnit *unit,
    ULONG lower_left, UWORD lower_right, ULONG upper_left, UWORD upper_right)
{
    struct AddressRange *range;

    ForeachNode(&unit->cgu_MulticastRanges, range)
    {
        if (range->lower_bound_left == lower_left &&
            range->lower_bound_right == lower_right &&
            range->upper_bound_left == upper_left &&
            range->upper_bound_right == upper_right)
            return range;
    }

    return NULL;
}

BOOL CDNSGEM_AddMulticastRange(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                               const UBYTE *lower, const UBYTE *upper)
{
    struct AddressRange *range;
    ULONG lower_left, upper_left;
    UWORD lower_right, upper_right;

    cdnsgem_splitaddr(lower, &lower_left, &lower_right);
    cdnsgem_splitaddr(upper, &upper_left, &upper_right);

    range = cdnsgem_findrange(unit, lower_left, lower_right,
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
    AddTail((struct List *)&unit->cgu_MulticastRanges, (struct Node *)range);
    Enable();
    unit->cgu_RangeCount++;

    if (unit->cgu_Flags & IFF_UP)
        CDNSGEM_SetRXFilter(base, unit);

    return TRUE;
}

BOOL CDNSGEM_RemMulticastRange(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                               const UBYTE *lower, const UBYTE *upper)
{
    struct AddressRange *range;
    ULONG lower_left, upper_left;
    UWORD lower_right, upper_right;

    cdnsgem_splitaddr(lower, &lower_left, &lower_right);
    cdnsgem_splitaddr(upper, &upper_left, &upper_right);

    range = cdnsgem_findrange(unit, lower_left, lower_right,
                              upper_left, upper_right);
    if (!range)
        return FALSE;

    if (--range->add_count == 0)
    {
        Disable();
        Remove((struct Node *)range);
        Enable();
        FreeMem(range, sizeof(struct AddressRange));
        unit->cgu_RangeCount--;

        if (unit->cgu_Flags & IFF_UP)
            CDNSGEM_SetRXFilter(base, unit);
    }

    return TRUE;
}

BOOL CDNSGEM_AddressFilter(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                           UBYTE *address)
{
    struct AddressRange *range;
    ULONG address_left;
    UWORD address_right;

    if (unit->cgu_Flags & IFF_PROMISC)
        return TRUE;

    if ((address[0] & 0x1) == 0)
        return TRUE;

    if (cdnsgem_sameaddr(address, cdnsgem_broadcast))
        return TRUE;

    cdnsgem_splitaddr(address, &address_left, &address_right);

    ForeachNode(&unit->cgu_MulticastRanges, range)
    {
        if ((address_left > range->lower_bound_left ||
             (address_left == range->lower_bound_left &&
              address_right >= range->lower_bound_right)) &&
            (address_left < range->upper_bound_left ||
             (address_left == range->upper_bound_left &&
              address_right <= range->upper_bound_right)))
            return TRUE;
    }

    unit->cgu_SpecialStats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    return FALSE;
}

/* == rings ================================================================ */

#define GEM_DMA_ALIGN                   64

static BOOL cdnsgem_alloc_ring(struct CDNSGEMBase *base, struct cdnsgem_ring *ring)
{
    IPTR addr;
    ULONG i;

    ring->bufmemsize = CDNSGEM_NDESC * GEM_BUFSIZE + GEM_DMA_ALIGN - 1;
    ring->bufmem = AllocMem(ring->bufmemsize, MEMF_PUBLIC | MEMF_CLEAR);
    if (!ring->bufmem)
        return FALSE;

    addr = ((IPTR)ring->bufmem + GEM_DMA_ALIGN - 1) & ~(IPTR)(GEM_DMA_ALIGN - 1);

    for (i = 0; i < CDNSGEM_NDESC; i++)
    {
        ring->buf[i] = (UBYTE *)(addr + i * GEM_BUFSIZE);
        ring->dma[i] = CDNSGEM_BusAddress(base, ring->buf[i]);
    }

    return TRUE;
}

static void cdnsgem_free_ring(struct cdnsgem_ring *ring)
{
    if (ring->bufmem)
    {
        FreeMem(ring->bufmem, ring->bufmemsize);
        ring->bufmem = NULL;
    }
}

/* Give the engine every RX buffer before RXEN, so nothing waits for one */
static void cdnsgem_rx_arm(struct CDNSGEMUnit *unit)
{
    struct cdnsgem_ring *rx = &unit->cgu_RX;
    ULONG i, len;

    for (i = 0; i < CDNSGEM_NDESC; i++)
    {
        len = GEM_BUFSIZE;
        CachePreDMA(rx->buf[i], &len, 0);
    }
}

/* Replies whatever the TX ring still holds; the engine must be stopped */
static void cdnsgem_tx_abort(struct CDNSGEMUnit *unit)
{
    struct cdnsgem_ring *tx = &unit->cgu_TX;
    struct IOSana2Req *request;
    ULONG index;

    while (tx->cidx != tx->pidx)
    {
        index = tx->cidx % CDNSGEM_NDESC;
        request = tx->request[index];

        if (request)
        {
            tx->request[index] = NULL;
            request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
            request->ios2_WireError = S2WERR_UNIT_OFFLINE;
            ReplyMsg(&request->ios2_Req.io_Message);
        }

        tx->cidx++;
    }
}

/* == unit lifecycle ======================================================= */

/*
 * The ISR clears on read (written back too, for the clear-on-write
 * variants). The bits go to the task to act on. RP1 forwards the level
 * line as an MSI on a rising edge, so the line has to have dropped before
 * returning - hence the loop until nothing we enabled is pending.
 */
static AROS_INTH1(CDNSGEM_IntHandler, struct CDNSGEMUnit *, unit)
{
    AROS_INTFUNC_INIT

    struct cdnsgem_hw *hw = unit->cgu_HW;
    ULONG isr, pending = 0, n;

    for (n = 0; n < 16; n++)
    {
        isr = CDNSGEM_Read(hw, GEM_ISR) & GEM_IXR_USED_MASK;
        if (isr == 0)
            break;

        CDNSGEM_Write(hw, GEM_ISR, isr);
        pending |= isr;
    }

    /* Atomic: on SMP the task may be taking the bits on another core */
    if (pending)
    {
        __atomic_or_fetch(&unit->cgu_IRQPending, pending, __ATOMIC_RELEASE);
        Signal(unit->cgu_Task, unit->cgu_IRQSignal);
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

struct CDNSGEMUnit *CDNSGEM_CreateUnit(struct CDNSGEMBase *base)
{
    struct CDNSGEMUnit *unit;
    struct Task *task;
    struct MemList *ml;
    UBYTE *stack;

    unit = AllocMem(sizeof(struct CDNSGEMUnit), MEMF_PUBLIC | MEMF_CLEAR);
    if (!unit)
        return NULL;

    unit->cgu_Base = base;
    unit->cgu_HW = &base->cgm_HW;
    unit->cgu_Sana2Info.SizeAvailable = sizeof(struct Sana2DeviceQuery);
    unit->cgu_Sana2Info.SizeSupplied = sizeof(struct Sana2DeviceQuery);
    unit->cgu_Sana2Info.DevQueryFormat = 0;
    unit->cgu_Sana2Info.DeviceLevel = 0;
    unit->cgu_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;
    unit->cgu_Sana2Info.MTU = ETH_MTU;
    unit->cgu_Sana2Info.HardwareType = S2WireType_Ethernet;

    InitSemaphore(&unit->cgu_Lock);
    NEWLIST((struct List *)&unit->cgu_Openers);
    NEWLIST((struct List *)&unit->cgu_MulticastRanges);
    NEWLIST((struct List *)&unit->cgu_TypeTrackers);

    if (unit->cgu_HW->haveMacAddr)
        CopyMem(unit->cgu_HW->macAddr, unit->cgu_DevAddr, ETH_ADDRESSSIZE);
    else
        CDNSGEM_GetMACAddress(unit->cgu_HW, unit->cgu_DevAddr);

    CopyMem(unit->cgu_DevAddr, unit->cgu_OrgAddr, ETH_ADDRESSSIZE);

    D(bug("[cdnsgem] station address %02x:%02x:%02x:%02x:%02x:%02x from %s\n",
          unit->cgu_DevAddr[0], unit->cgu_DevAddr[1], unit->cgu_DevAddr[2],
          unit->cgu_DevAddr[3], unit->cgu_DevAddr[4], unit->cgu_DevAddr[5],
          unit->cgu_HW->haveMacAddr ? "device tree" : "LADDR");)

    unit->cgu_TimerPort = CreateMsgPort();
    if (!unit->cgu_TimerPort)
        goto fail;

    unit->cgu_TimerReq = (struct timerequest *)
        CreateIORequest(unit->cgu_TimerPort, sizeof(*unit->cgu_TimerReq));
    if (!unit->cgu_TimerReq)
        goto fail;

    if (OpenDevice(TIMERNAME, UNIT_MICROHZ,
                   (struct IORequest *)unit->cgu_TimerReq, 0) != 0)
        goto fail;

    unit->cgu_DescMem = CDNSGEM_AllocDMA(base, CDNSGEM_DESCMEM_SIZE);
    if (!unit->cgu_DescMem)
        goto fail;
    unit->cgu_DescDMA = CDNSGEM_BusAddress(base, unit->cgu_DescMem);

    /* One upper half per direction covers every queue */
    if ((unit->cgu_DescDMA >> 32) !=
        ((unit->cgu_DescDMA + CDNSGEM_DESCMEM_SIZE - 1) >> 32))
    {
        D(bug("[cdnsgem] descriptor block straddles 4 GB\n");)
        goto fail;
    }

    unit->cgu_TX.desc = unit->cgu_DescMem;
    unit->cgu_RX.desc = unit->cgu_TX.desc + CDNSGEM_NDESC + 1;

    if (!cdnsgem_alloc_ring(base, &unit->cgu_RX) ||
        !cdnsgem_alloc_ring(base, &unit->cgu_TX))
        goto fail;

    D(bug("[cdnsgem] descriptors at %p (bus %08x%08x), buffers bus %08x%08x\n",
          unit->cgu_DescMem, (ULONG)(unit->cgu_DescDMA >> 32),
          (ULONG)unit->cgu_DescDMA, (ULONG)(unit->cgu_RX.dma[0] >> 32),
          (ULONG)unit->cgu_RX.dma[0]);)

    CDNSGEM_HWReset(unit->cgu_HW);
    CDNSGEM_SetMACAddress(unit->cgu_HW, unit->cgu_DevAddr);
    CDNSGEM_HWInit(unit->cgu_HW);

    if (!CDNSGEM_PHYInit(unit))
        goto fail;

    /*
     * The service task. Its stack and control block ride on
     * tc_MemEntry, so task exit frees them without help.
     */
    task = AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
    stack = AllocMem(CDNSGEM_TASK_STACK, MEMF_PUBLIC);
    ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry),
                  MEMF_PUBLIC | MEMF_CLEAR);
    if (!task || !stack || !ml)
    {
        if (task)
            FreeMem(task, sizeof(struct Task));
        if (stack)
            FreeMem(stack, CDNSGEM_TASK_STACK);
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

        unit->cgu_RequestPorts[i] = port;
    }

    ml->ml_NumEntries = 2;
    ml->ml_ME[0].me_Addr = task;
    ml->ml_ME[0].me_Length = sizeof(struct Task);
    ml->ml_ME[1].me_Addr = stack;
    ml->ml_ME[1].me_Length = CDNSGEM_TASK_STACK;

    task->tc_Node.ln_Type = NT_TASK;
    task->tc_Node.ln_Pri = 0;
    task->tc_Node.ln_Name = "cdnsgem.device";
    task->tc_SPLower = stack;
    task->tc_SPUpper = stack + CDNSGEM_TASK_STACK;
    task->tc_SPReg = stack + CDNSGEM_TASK_STACK;
    NEWLIST(&task->tc_MemEntry);
    AddTail(&task->tc_MemEntry, (struct Node *)ml);
    task->tc_UserData = unit;

    unit->cgu_Task = task;
    unit->cgu_DeathWatch = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);

    if (AddTask(task, CDNSGEM_UnitTask, NULL) == NULL)
    {
        unit->cgu_Task = NULL;
        FreeMem(ml, sizeof(struct MemList) + sizeof(struct MemEntry));
        FreeMem(stack, CDNSGEM_TASK_STACK);
        FreeMem(task, sizeof(struct Task));
        goto fail;
    }

    Wait(SIGF_SINGLE);
    if (!unit->cgu_InputPort)
        goto fail;

    /* The handler signals the task, so only once the task exists */
    unit->cgu_IRQHandler.is_Node.ln_Type = NT_INTERRUPT;
    unit->cgu_IRQHandler.is_Node.ln_Name = "cdnsgem";
    unit->cgu_IRQHandler.is_Code = (VOID_FUNC)CDNSGEM_IntHandler;
    unit->cgu_IRQHandler.is_Data = unit;
    if (!CDNSGEM_AddInterrupt(base, &unit->cgu_IRQHandler))
        goto fail;
    unit->cgu_IRQAdded = TRUE;

    return unit;

fail:
    D(bug("[cdnsgem] unit creation failed\n");)
    CDNSGEM_DeleteUnit(base, unit);
    return NULL;
}

void CDNSGEM_DeleteUnit(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit)
{
    BOOL alive;

    if (!unit)
        return;

    /* Stop the engine before anything it reads or writes goes away */
    CDNSGEM_HWReset(unit->cgu_HW);

    if (unit->cgu_IRQAdded)
    {
        CDNSGEM_RemInterrupt(base, &unit->cgu_IRQHandler);
        unit->cgu_IRQAdded = FALSE;
    }

    /* The task may still be using the timer, queues, and rings. */
    unit->cgu_DeathWatch = FindTask(NULL);
    Forbid();
    alive = (unit->cgu_Task != NULL);
    if (alive)
        Signal(unit->cgu_Task, SIGBREAKF_CTRL_C);
    Permit();

    if (alive)
        Wait(SIGF_SINGLE);

    /* A task that failed to start signalled twice */
    SetSignal(0, SIGF_SINGLE);

    cdnsgem_tx_abort(unit);

    for (ULONG i = 0; i < REQUEST_QUEUE_COUNT; i++)
    {
        if (unit->cgu_RequestPorts[i])
        {
            FreeMem(unit->cgu_RequestPorts[i],
                    sizeof(*unit->cgu_RequestPorts[i]));
            unit->cgu_RequestPorts[i] = NULL;
        }
    }

    cdnsgem_free_ring(&unit->cgu_RX);
    cdnsgem_free_ring(&unit->cgu_TX);

    if (unit->cgu_DescMem)
        CDNSGEM_FreeDMA(base, unit->cgu_DescMem);

    if (unit->cgu_TimerReq)
    {
        if (unit->cgu_TimerReq->tr_node.io_Device)
            CloseDevice((struct IORequest *)unit->cgu_TimerReq);
        DeleteIORequest((struct IORequest *)unit->cgu_TimerReq);
    }
    if (unit->cgu_TimerPort)
        DeleteMsgPort(unit->cgu_TimerPort);

    FreeMem(unit, sizeof(struct CDNSGEMUnit));
}

/*
 * Our own address sits in LADDR 0. Multicast goes through the 64-bit hash;
 * a range wider than one address can only be had by opening the hash up
 * entirely. CDNSGEM_AddressFilter() enforces the exact ranges afterwards.
 */
void CDNSGEM_SetRXFilter(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit)
{
    struct cdnsgem_hw *hw = unit->cgu_HW;
    struct AddressRange *range;
    UQUAD hash = 0;
    ULONG netcfg;

    CDNSGEM_SetMACAddress(hw, unit->cgu_DevAddr);

    netcfg = CDNSGEM_Read(hw, GEM_NETCFG);
    netcfg &= ~GEM_NETCFG_UCASTHASHEN;

    if (unit->cgu_Flags & IFF_PROMISC)
    {
        netcfg |= GEM_NETCFG_COPYALL;
        netcfg &= ~GEM_NETCFG_MCASTHASHEN;
    }
    else
    {
        netcfg &= ~GEM_NETCFG_COPYALL;
        netcfg |= GEM_NETCFG_MCASTHASHEN;

        ForeachNode(&unit->cgu_MulticastRanges, range)
        {
            UBYTE addr[ETH_ADDRESSSIZE];

            if (range->lower_bound_left != range->upper_bound_left ||
                range->lower_bound_right != range->upper_bound_right)
            {
                hash = ~0ULL;
                break;
            }

            addr[0] = (range->lower_bound_left >> 24) & 0xff;
            addr[1] = (range->lower_bound_left >> 16) & 0xff;
            addr[2] = (range->lower_bound_left >> 8) & 0xff;
            addr[3] = range->lower_bound_left & 0xff;
            addr[4] = (range->lower_bound_right >> 8) & 0xff;
            addr[5] = range->lower_bound_right & 0xff;

            hash |= 1ULL << CDNSGEM_HashMAC(addr);
        }

        CDNSGEM_Write(hw, GEM_HASHL, (ULONG)hash);
        CDNSGEM_Write(hw, GEM_HASHH, (ULONG)(hash >> 32));
    }

    CDNSGEM_Write(hw, GEM_NETCFG, netcfg);
}

void CDNSGEM_GoOnline(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit)
{
    struct cdnsgem_hw *hw = unit->cgu_HW;

    CDNSGEM_InitRings(unit);
    cdnsgem_rx_arm(unit);
    CDNSGEM_SetRXFilter(base, unit);

    /* Speed and duplex into NETCFG before the MAC is let go */
    CDNSGEM_CheckLink(base, unit);

    CDNSGEM_Write(hw, GEM_ISR, CDNSGEM_Read(hw, GEM_ISR));
    CDNSGEM_Write(hw, GEM_IER, GEM_IXR_USED_MASK);

    hw->netctl |= GEM_NETCTL_RXEN | GEM_NETCTL_TXEN;
    CDNSGEM_Write(hw, GEM_NETCTL, hw->netctl);

    unit->cgu_Flags |= IFF_UP;
}

void CDNSGEM_GoOffline(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit)
{
    struct cdnsgem_hw *hw = unit->cgu_HW;
    ULONG spins;

    hw->netctl &= ~(GEM_NETCTL_RXEN | GEM_NETCTL_TXEN);
    CDNSGEM_Write(hw, GEM_NETCTL, hw->netctl);
    CDNSGEM_Write(hw, GEM_IDR, ~0U);

    for (spins = 10000; spins > 0; spins--)
    {
        if ((CDNSGEM_Read(hw, GEM_TXSR) & GEM_TXSR_TXGO) == 0)
            break;
    }

    /* Clearing TXEN rewound the engine; GoOnline() starts the rings over */
    cdnsgem_tx_abort(unit);

    unit->cgu_Flags &= ~IFF_UP;
}

/*
 * One request into the next TX slot. The frame is copied into the slot's
 * own buffer, so nothing depends on the caller's data once this returns.
 *
 * Returns TRUE when the request is consumed - handed to the engine (the
 * TX-done path replies it) or failed and replied here - and FALSE only
 * when the ring is full and it should be retried later.
 */
BOOL CDNSGEM_SendPacket(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                        struct IOSana2Req *request)
{
    struct cdnsgem_ring *tx = &unit->cgu_TX;
    struct cdnsgem_hw *hw = unit->cgu_HW;
    struct Opener *opener = request->ios2_BufferManagement;
    struct TypeStats *tracker;
    UBYTE *buffer;
    ULONG index, data_size, packet_size, len, status;
    UWORD packet_type;
    BOOL copied;

    /* One slot stays free, so a full ring never looks empty */
    if (tx->pidx - tx->cidx >= CDNSGEM_NDESC - 1)
        return FALSE;

    index = tx->pidx % CDNSGEM_NDESC;
    buffer = tx->buf[index];
    data_size = packet_size = request->ios2_DataLength;

    if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
    {
        struct eth_frame *frame = (struct eth_frame *)buffer;

        packet_size += ETH_HEADERSIZE;
        CopyMem(request->ios2_DstAddr, frame->eth_packet_dest,
                ETH_ADDRESSSIZE);
        CopyMem(unit->cgu_DevAddr, frame->eth_packet_source,
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
        CDNSGEM_ReportEvents(base, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE |
                             S2EVENT_BUFF | S2EVENT_TX);
        ReplyMsg(&request->ios2_Req.io_Message);
        return TRUE;
    }

    while (packet_size < ETH_ZLEN)
        buffer[packet_size++] = 0;

    packet_type = (UWORD)request->ios2_PacketType;

    len = packet_size;
    CachePreDMA(buffer, &len, DMA_ReadFromRAM);

    tx->request[index] = request;

    /* The address words were set by CDNSGEM_InitRings(); clearing USED
       is the hand-over */
    status = (packet_size & GEM_TXD_LEN_MASK) | GEM_TXD_LAST;
    if (index == CDNSGEM_NDESC - 1)
        status |= GEM_TXD_WRAP;
    tx->desc[index].status = status;
    __dsb();

    tx->pidx++;
    CDNSGEM_Write(hw, GEM_NETCTL, hw->netctl | GEM_NETCTL_STARTTX);

    unit->cgu_Stats.PacketsSent++;
    tracker = CDNSGEM_FindTypeStats(&unit->cgu_TypeTrackers, packet_type);
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
static void cdnsgem_tx_drain(struct CDNSGEMBase *base,
                             struct CDNSGEMUnit *unit)
{
    struct MsgPort *port = unit->cgu_RequestPorts[WRITE_QUEUE];
    struct IOSana2Req *request;

    while ((request = (struct IOSana2Req *)GetMsg(port)) != NULL)
    {
        ObtainSemaphore(&unit->cgu_Lock);

        if ((unit->cgu_Flags & IFF_UP) == 0)
        {
            request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
            request->ios2_WireError = S2WERR_UNIT_OFFLINE;
            ReplyMsg(&request->ios2_Req.io_Message);
        }
        else if (!CDNSGEM_SendPacket(base, unit, request))
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
            ReleaseSemaphore(&unit->cgu_Lock);
            break;
        }

        ReleaseSemaphore(&unit->cgu_Lock);
    }
}

/*
 * The engine sets USED in a descriptor once its frame is out, and stops
 * at the first one still set. A STARTTX that lands while it is stopping
 * is lost, so a ring with work left but the transmitter idle is restarted.
 */
static void cdnsgem_tx_complete(struct CDNSGEMBase *base,
                                struct CDNSGEMUnit *unit)
{
    struct cdnsgem_ring *tx = &unit->cgu_TX;
    struct cdnsgem_hw *hw = unit->cgu_HW;
    struct IOSana2Req *request;
    ULONG index, status, done = 0;

    while (tx->cidx != tx->pidx)
    {
        index = tx->cidx % CDNSGEM_NDESC;
        status = tx->desc[index].status;
        if ((status & GEM_TXD_USED) == 0)
            break;

        request = tx->request[index];
        if (request)
        {
            tx->request[index] = NULL;
            request->ios2_Req.io_Error = 0;

            if (status & (GEM_TXD_RLIMIT | GEM_TXD_CORRUPT | GEM_TXD_LCOLL))
            {
                request->ios2_Req.io_Error = S2ERR_TX_FAILURE;
                request->ios2_WireError = S2WERR_GENERIC_ERROR;
                if (status & GEM_TXD_RLIMIT)
                    unit->cgu_SpecialStats[S2SS_ETHERNET_RETRIES & 0xffff]++;
            }

            ReplyMsg(&request->ios2_Req.io_Message);
        }

        tx->cidx++;
        done++;
    }

    if (tx->cidx != tx->pidx &&
        (CDNSGEM_Read(hw, GEM_TXSR) & GEM_TXSR_TXGO) == 0)
        CDNSGEM_Write(hw, GEM_NETCTL, hw->netctl | GEM_NETCTL_STARTTX);

    if (done)
        CDNSGEM_ReportEvents(base, unit, S2EVENT_TX);
}

static void cdnsgem_copy_packet(struct CDNSGEMBase *base,
                                struct CDNSGEMUnit *unit,
                                struct IOSana2Req *request,
                                ULONG packet_size, UWORD packet_type,
                                struct eth_frame *frame)
{
    struct Library *UtilityBase = base->cgm_UtilityBase;
    struct Opener *opener;
    UBYTE *ptr;
    BOOL filtered = FALSE;

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if (cdnsgem_sameaddr(frame->eth_packet_dest, cdnsgem_broadcast))
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
        CDNSGEM_ReportEvents(base, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE |
                             S2EVENT_BUFF | S2EVENT_RX);
    }

    Disable();
    Remove((APTR)request);
    Enable();
    ReplyMsg((APTR)request);
}

static void cdnsgem_rx_packet(struct CDNSGEMBase *base,
                              struct CDNSGEMUnit *unit,
                              struct eth_frame *frame, ULONG length)
{
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    struct TypeStats *tracker;
    BOOL accepted, is_orphan = TRUE;
    UWORD packet_type;

    if (!CDNSGEM_AddressFilter(base, unit, frame->eth_packet_dest))
        return;

    /*
     * Kept unsigned: an ethertype with the high bit set would otherwise
     * sign-extend and never match an opener's ULONG packet type.
     */
    packet_type = (UWORD)AROS_BE2WORD(frame->eth_packet_type);

    unit->cgu_Stats.PacketsReceived++;

    tracker = CDNSGEM_FindTypeStats(&unit->cgu_TypeTrackers, packet_type);
    if (tracker)
    {
        tracker->stats.PacketsReceived++;
        tracker->stats.BytesReceived += length;
    }

    opener = (APTR)unit->cgu_Openers.mlh_Head;
    opener_tail = (APTR)&unit->cgu_Openers.mlh_Tail;

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
                cdnsgem_copy_packet(base, unit, request, length,
                                    packet_type, frame);
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
        unit->cgu_Stats.UnknownTypesReceived++;

        if (!IsListEmpty(&unit->cgu_RequestPorts[ADOPT_QUEUE]->mp_MsgList))
        {
            cdnsgem_copy_packet(base, unit,
                (APTR)unit->cgu_RequestPorts[ADOPT_QUEUE]->mp_MsgList.lh_Head,
                length, packet_type, frame);
        }
    }
}

/*
 * Every descriptor the engine has filled carries USED. The buffer stays
 * with its slot, so handing the slot back is clearing USED again.
 */
static void cdnsgem_rx_process(struct CDNSGEMBase *base,
                               struct CDNSGEMUnit *unit)
{
    struct cdnsgem_ring *rx = &unit->cgu_RX;
    ULONG index, n, status, len, postlen, addr;

    for (n = 0; n < CDNSGEM_NDESC; n++)
    {
        index = rx->cidx % CDNSGEM_NDESC;
        if ((rx->desc[index].addrlo & GEM_RXD_ADDR_USED) == 0)
            break;

        /* status only after USED has been seen */
        __dmb();
        status = rx->desc[index].status;
        len = status & GEM_RXD_LEN_MASK;

        /* Drop the cached lines so the engine's frame is what gets read */
        postlen = GEM_BUFSIZE;
        CachePostDMA(rx->buf[index], &postlen, 0);

        if ((status & (GEM_RXD_SOF | GEM_RXD_EOF)) !=
            (GEM_RXD_SOF | GEM_RXD_EOF) || len < ETH_HEADERSIZE)
            unit->cgu_Stats.BadData++;
        else
            cdnsgem_rx_packet(base, unit,
                (struct eth_frame *)(rx->buf[index] + GEM_RX_OFFSET), len);

        addr = (ULONG)rx->dma[index];
        if (index == CDNSGEM_NDESC - 1)
            addr |= GEM_RXD_ADDR_WRAP;
        rx->desc[index].status = 0;
        __dmb();
        rx->desc[index].addrlo = addr;

        rx->cidx++;
    }

    if (n != 0)
        CDNSGEM_ReportEvents(base, unit, S2EVENT_RX);
}

static void cdnsgem_irq_work(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                             ULONG pending)
{
    struct cdnsgem_hw *hw = unit->cgu_HW;

    ObtainSemaphore(&unit->cgu_Lock);

    if ((unit->cgu_Flags & IFF_UP) == 0)
    {
        ReleaseSemaphore(&unit->cgu_Lock);
        return;
    }

    if (pending & (GEM_IXR_TXDONE | GEM_IXR_TXUSED | GEM_IXR_RETRY | GEM_IXR_TXURUN))
        cdnsgem_tx_complete(base, unit);

    if (pending & GEM_IXR_TXURUN)
        unit->cgu_SpecialStats[S2SS_ETHERNET_FIFO_UNDERRUNS & 0xffff]++;

    if (pending & GEM_IXR_RXOVR)
        unit->cgu_Stats.Overruns++;

    if (pending & GEM_IXR_RXDONE)
        cdnsgem_rx_process(base, unit);

    /* A DMA bus error: stop rather than let the engine scribble on */
    if (pending & GEM_IXR_HRESP)
    {
        D(bug("[cdnsgem] DMA bus error, interface stopped\n");)
        hw->netctl &= ~(GEM_NETCTL_RXEN | GEM_NETCTL_TXEN);
        CDNSGEM_Write(hw, GEM_NETCTL, hw->netctl);
        CDNSGEM_Write(hw, GEM_IDR, ~0U);
        CDNSGEM_ReportEvents(base, unit, S2EVENT_ERROR | S2EVENT_HARDWARE);
    }

    ReleaseSemaphore(&unit->cgu_Lock);
}

static void CDNSGEM_UnitTask(void)
{
    struct CDNSGEMUnit *unit = (struct CDNSGEMUnit *)FindTask(NULL)->tc_UserData;
    struct CDNSGEMBase *base = unit->cgu_Base;
    struct MsgPort *input, *timerport = NULL;
    struct timerequest *timerreq = NULL;
    struct IOSana2Req *request;
    BOOL timeropen = FALSE, timerpending = FALSE, running = FALSE;
    ULONG inputsig = 0, timersig = 0, sigs;
    LONG irqbit;

    input = CreateMsgPort();
    timerport = CreateMsgPort();
    if (timerport)
        timerreq = (struct timerequest *)
            CreateIORequest(timerport, sizeof(struct timerequest));
    if (timerreq &&
        OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK,
                   (struct IORequest *)timerreq, 0) == 0)
        timeropen = TRUE;

    irqbit = AllocSignal(-1);
    if (irqbit >= 0)
        unit->cgu_IRQSignal = 1UL << irqbit;

    if (input && timeropen && irqbit >= 0)
    {
        struct MsgPort *writeport = unit->cgu_RequestPorts[WRITE_QUEUE];

        unit->cgu_InputPort = input;
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
    }

    Signal(unit->cgu_DeathWatch, SIGF_SINGLE);

    while (running)
    {
        sigs = Wait(inputsig | timersig | unit->cgu_IRQSignal | SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C)
            running = FALSE;

        if (sigs & inputsig)
        {
            while ((request = (struct IOSana2Req *)GetMsg(input)) != NULL)
            {
                ObtainSemaphore(&unit->cgu_Lock);
                CDNSGEM_HandleRequest(base, request);
            }

            cdnsgem_tx_drain(base, unit);
        }

        if (sigs & timersig)
        {
            if (GetMsg(timerport))
            {
                timerpending = FALSE;

                ObtainSemaphore(&unit->cgu_Lock);
                CDNSGEM_CheckLink(base, unit);
                ReleaseSemaphore(&unit->cgu_Lock);

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

        if (sigs & unit->cgu_IRQSignal)
        {
            ULONG pending = __atomic_exchange_n(&unit->cgu_IRQPending, 0,
                                                __ATOMIC_ACQUIRE);

            if (pending)
                cdnsgem_irq_work(base, unit, pending);

            /* Freed slots may unblock requests parked on a full ring;
               CmdWrite also raises this signal to get its frame sent */
            cdnsgem_tx_drain(base, unit);
        }
    }

    unit->cgu_InputPort = NULL;

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
    unit->cgu_Task = NULL;
    Signal(unit->cgu_DeathWatch, SIGF_SINGLE);
}

/*
 * Called once a second from the unit task, under cgu_Lock, and from
 * GoOnline(). CDNSGEM_PHYGetLink() has already put the speed into NETCFG;
 * this is the bookkeeping the rest of the driver reads.
 */
BOOL CDNSGEM_CheckLink(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit)
{
    ULONG mbps = unit->cgu_SpeedMbps;
    BOOL fullduplex = unit->cgu_FullDuplex;
    BOOL up;

    up = CDNSGEM_PHYGetLink(unit->cgu_HW, &mbps, &fullduplex);

    /* Nothing to say unless the link or the negotiated mode moved */
    if (up == unit->cgu_LinkUp &&
        (!up || (mbps == unit->cgu_SpeedMbps &&
                 fullduplex == unit->cgu_FullDuplex)))
        return up;

    unit->cgu_LinkUp = up;

    if (up)
    {
        unit->cgu_SpeedMbps = mbps;
        unit->cgu_FullDuplex = fullduplex;

        unit->cgu_Sana2Info.BPS = mbps * 1000000UL;
    }
    else
    {
        unit->cgu_Sana2Info.BPS = 0;
    }
    D(bug("[cdnsgem] link %s, %u Mbit %s duplex\n", up ? "up" : "down",
          mbps, fullduplex ? "full" : "half");)

    return up;
}
