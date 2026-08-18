/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Broadcom GENETv5 SANA-II driver, the unit: address bookkeeping
          is generic SANA-II ceremony and fully implemented below; the
          ring/interrupt/hardware pieces are left as TODOs for the port
          from OpenBSD's bcmgenet.c.
*/
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

/* == unit lifecycle ======================================================= */

/*
 * TODO: this is the shape of the thing, not a working driver yet.
 * BCMGENET_BeginIO() refuses every request until bgu_InputPort is set
 * (checked there on purpose, so a half-finished unit fails closed
 * instead of PutMsg()-ing onto a NULL port) - still missing:
 *  - bgu_InputPort and bgu_RequestPorts[] (see REQUEST_QUEUE_COUNT):
 *    real AROS ports need a live task or software interrupt behind
 *    mp_SigTask to be woken by PutMsg(), which does not exist yet
 *  - allocate bgu_TX/bgu_RX ring buffers (GENET_TXDESC/RXDESC *
 *    GENET_BUFSIZE each) and hand them to BCMGENET_HWInit()
 *  - register the two IRQ handlers (unit->bgu_HW->irq[0]/[1]) with
 *    KrnAddIRQHandler() into bgu_IRQHandler[0]/[1]
 *  - decide how received frames and completed sends get from IRQ
 *    context to the openers: a bare exec worker task like dwmac's
 *    DWMAC_UnitTask, or an interrupt-driven bottom half like
 *    RPiHDMI's slave process. Left open on purpose - whichever is
 *    chosen is what ends up behind bgu_InputPort/bgu_RequestPorts.
 */
struct BCMGENETUnit *BCMGENET_CreateUnit(struct BCMGENETBase *base)
{
    struct BCMGENETUnit *unit;

    unit = AllocMem(sizeof(struct BCMGENETUnit), MEMF_PUBLIC | MEMF_CLEAR);
    if (!unit)
        return NULL;

    unit->bgu_Base = base;
    unit->bgu_HW = &base->bgm_HW;

    InitSemaphore(&unit->bgu_Lock);
    NEWLIST((struct List *)&unit->bgu_Openers);
    NEWLIST((struct List *)&unit->bgu_MulticastRanges);
    NEWLIST((struct List *)&unit->bgu_TypeTrackers);

    if (unit->bgu_HW->haveMacAddr)
        CopyMem(unit->bgu_HW->macAddr, unit->bgu_DevAddr, ETH_ADDRESSSIZE);
    else
        BCMGENET_GetMACAddress(unit->bgu_HW, unit->bgu_DevAddr);
    CopyMem(unit->bgu_DevAddr, unit->bgu_OrgAddr, ETH_ADDRESSSIZE);

    /* TODO: BCMGENET_HWReset(), ring allocation, BCMGENET_HWInit(),
     * BCMGENET_PHYInit(), IRQ handler registration. Free 'unit' and
     * return NULL on any failure past this point. */

    return unit;
}

void BCMGENET_DeleteUnit(struct BCMGENETBase *base, struct BCMGENETUnit *unit)
{
    if (!unit)
        return;

    /* TODO: tear down in the reverse order of BCMGENET_CreateUnit():
     * remove IRQ handlers, stop RX/TX (BCMGENET_GoOffline), free the
     * ring buffers. */

    FreeMem(unit, sizeof(struct BCMGENETUnit));
}

/* TODO: port the RXEN/TXEN half of genet_init()/genet_stop()
 * (bcmgenet.c:583, 632). */
void BCMGENET_GoOnline(struct BCMGENETBase *base, struct BCMGENETUnit *unit)
{
    unit->bgu_Flags |= IFF_UP;
}

void BCMGENET_GoOffline(struct BCMGENETBase *base, struct BCMGENETUnit *unit)
{
    unit->bgu_Flags &= ~IFF_UP;
}

/*
 * TODO: port genet_qstart()/genet_setup_txbuf() (bcmgenet.c:773, 204):
 * copy the frame into the next bgu_TX slot's buffer, program
 * GENET_TX_DESC_ADDRESS_*(idx)/STATUS(idx) with SOP|EOP|OWN, advance
 * pidx and kick GENET_TX_DMA_PROD_INDEX. Reply the request once the
 * matching TX-done interrupt (or, for now, once safely copied) confirms
 * the slot is free again.
 */
BOOL BCMGENET_SendPacket(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                         struct IOSana2Req *request)
{
    return FALSE;
}
