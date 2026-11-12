/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: USB2OTG isochronous transfers and realtime iso handlers
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "usb2otg_intern.h"

/*
 * Lent to iso while bound: the last INT channel first, as INT polls free
 * it between intervals; a waiting bulk IN can hold the second bulk one.
 */
static const UBYTE usb2otg_iso_hwchan[USB2OTG_ISO_CHANS] = { CHAN_INT5, CHAN_BULK2 };

/* (Micro)frames an endpoint keeps its channel with nothing queued */
#define USB2OTG_ISO_LINGER  64
/* SOFs an armed packet may go without its halt: then halt it, then drop it */
#define USB2OTG_ISO_STUCK   32
/* Payload of one split transaction */
#define USB2OTG_ISO_SPLITMAX    188

#define USB2OTG_HFNUM_MASK  0x3fff

/* is_Phase: where a split packet is */
#define ISO_IDLE        0
#define ISO_OUT_SS      1       /* start-splits carrying the OUT data */
#define ISO_IN_SS       2       /* the IN start-split */
#define ISO_IN_CS       3       /* complete-splits fetching the IN data */

/* HCSPLT.XactPos for iso OUT payload pieces */
#define XACTPOS_MID     0
#define XACTPOS_END     1
#define XACTPOS_BEGIN   2
#define XACTPOS_ALL     3

#define ISO_CALLHOOK(h, o, m) \
    AROS_UFC3(IPTR, (h)->h_Entry, \
              AROS_UFCA(struct Hook *, (h), A0), \
              AROS_UFCA(APTR, (o), A2), \
              AROS_UFCA(APTR, (m), A1))

#if defined(__AROSEXEC_SMP__)
#define ISO_LOCK(u)     do { Disable(); KrnSpinLock(&(u)->hu_Lock, NULL, SPINLOCK_MODE_WRITE); } while (0)
#define ISO_UNLOCK(u)   do { KrnSpinUnLock(&(u)->hu_Lock); Enable(); } while (0)
#else
#define ISO_LOCK(u)     Disable()
#define ISO_UNLOCK(u)   Enable()
#endif

/* frame1 at or before frame2, modulo the 14-bit HFNUM */
static inline BOOL iso_frame_le(UWORD frame1, UWORD frame2)
{
    return ((frame2 - frame1) & USB2OTG_HFNUM_MASK) < (USB2OTG_HFNUM_MASK >> 1);
}

/*
 * In HFNUM units: microframes behind a high-speed root port, which a
 * split endpoint always is, else frames.
 */
static UWORD iso_interval(struct USB2OTGIso *is, struct IOUsbHWReq *req)
{
    UWORD ival = req->iouh_Interval ? req->iouh_Interval : 1;

    if (req->iouh_Flags & UHFF_HIGHSPEED)
        return ival;
    /* Full-speed iso: Poseidon passes the descriptor's exponent */
    ival = 1 << ((ival > 11 ? 11 : ival) - 1);
    return is->is_Split ? ival << 3 : ival;
}

static ULONG iso_mult(struct IOUsbHWReq *req)
{
    ULONG mult = (req->iouh_Flags & UHFF_MULTI_3) / UHFF_MULTI_1;

    return mult ? mult : 1;
}

BOOL usb2otg_iso_init(struct USB2OTGUnit *unit)
{
    UBYTE *buf;
    int i;

    NEWLIST(&unit->hu_IsoDone);
    InitSemaphore(&unit->hu_IsoSem);

    unit->hu_IsoBufRaw = AllocMem(USB2OTG_ISO_CHANS * USB2OTG_ISO_BUFSIZE + 63, MEMF_PUBLIC | MEMF_CLEAR);
    if (unit->hu_IsoBufRaw == NULL)
        return FALSE;
    buf = (UBYTE *)(((IPTR)unit->hu_IsoBufRaw + 63) & ~(IPTR)63);

    for (i = 0; i < USB2OTG_ISO_CHANS; i++)
    {
        NEWLIST(&unit->hu_Iso[i].is_Queue);
        unit->hu_Iso[i].is_Chan = usb2otg_iso_hwchan[i];
        unit->hu_Iso[i].is_Buf = buf + i * USB2OTG_ISO_BUFSIZE;
    }
    return TRUE;
}

/* Queue a transfer on the iso channel bound to its endpoint, binding a free one if needed */
static WORD iso_submit(struct USB2OTGUnit *unit, struct IOUsbHWReq *req)
{
#if defined(__AROSEXEC_SMP__)
    struct USB2OTGDevice *USB2OTGBase = unit->hu_USB2OTGBase;
#endif
    struct USB2OTGIso *is = NULL, *free_is = NULL;
    UWORD now;
    int i;

    if (req->iouh_Flags & UHFF_LOWSPEED)
        return UHIOERR_BADPARAMS;
    if ((req->iouh_MaxPktSize & 0x7ff) * iso_mult(req) > USB2OTG_ISO_BUFSIZE)
        return UHIOERR_BADPARAMS;

    req->iouh_Req.io_Flags &= ~IOF_QUICK;
    req->iouh_Req.io_Error = 0;
    req->iouh_Actual = 0;

    ISO_LOCK(unit);
    for (i = 0; i < USB2OTG_ISO_CHANS; i++)
    {
        struct USB2OTGIso *c = &unit->hu_Iso[i];

        if (!c->is_Bound)
        {
            if (free_is == NULL)
                free_is = c;
        }
        else if (c->is_Dev == req->iouh_DevAddr && c->is_Ep == req->iouh_Endpoint &&
                 c->is_Dir == req->iouh_Dir)
            is = c;
    }
    if (is == NULL && free_is != NULL)
    {
        is = free_is;
        is->is_Bound = TRUE;
        is->is_Fresh = TRUE;
        is->is_Dev = req->iouh_DevAddr;
        is->is_Ep = req->iouh_Endpoint;
        is->is_Dir = req->iouh_Dir;
        is->is_Split = (req->iouh_Flags & UHFF_SPLITTRANS) ? TRUE : FALSE;
        is->is_HubAddr = req->iouh_SplitHubAddr;
        is->is_HubPort = req->iouh_SplitHubPort;
        is->is_Phase = ISO_IDLE;
        now = rd32le(USB2OTG_HOSTFRAMENO) & USB2OTG_HFNUM_MASK;
        /* Splits start on a frame boundary: the second one from now */
        is->is_Next = is->is_Split ? ((now + 16) & ~7 & USB2OTG_HFNUM_MASK)
                                   : ((now + 2) & USB2OTG_HFNUM_MASK);
        /* The bulk/INT schedulers leave the channel alone from now on */
        unit->hu_IsoChanMask |= 1 << is->is_Chan;
    }
    if (is != NULL)
        ADDTAIL(&is->is_Queue, req);
    usb2otg_sof_gate_wake(unit);
    ISO_UNLOCK(unit);

    return (is != NULL) ? RC_DONTREPLY : UHIOERR_HOSTERROR;
}

WORD FNAME_DEV(cmdIsoXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG:ISO] dev=%d ep=%d dir=%d len=%lu ival=%d split=%d\n",
        ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Dir,
        (unsigned long)ioreq->iouh_Length, ioreq->iouh_Interval,
        (ioreq->iouh_Flags & UHFF_SPLITTRANS) ? 1 : 0));

    return iso_submit(otg_Unit, ioreq);
}

/* Write the channel and enable it; ODDFRM as given */
static void iso_start(struct USB2OTGIso *is, ULONG split,
                      ULONG size, ULONG pktcnt, ULONG pid, ULONG dmaoff, BOOL odd)
{
    struct IOUsbHWReq *req = is->is_Req;
    int chan = is->is_Chan;
    ULONG tmp;

    wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), split);
    wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE),
        USB2OTG_HOSTTSIZE_SIZE(size) | USB2OTG_HOSTTSIZE_PKTCNT(pktcnt) | USB2OTG_HOSTTSIZE_PID(pid));
    wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), 0xc0000000 | (ULONG)(IPTR)(is->is_Buf + dmaoff));

    /* In DMA mode the halt reports every outcome */
    wr32le(USB2OTG_CHANNEL_REG(chan, INTRMASK), USB2OTG_INTRCHAN_HALT);
    wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);
    tmp = rd32le(USB2OTG_HOSTINTRMASK);
    wr32le(USB2OTG_HOSTINTRMASK, tmp | (1 << chan));
    tmp = rd32le(USB2OTG_INTRMASK);
    wr32le(USB2OTG_INTRMASK, tmp | USB2OTG_INTRCORE_HOSTCHANNEL);

    is->is_Size = size;
    is->is_Pending = 0;
    is->is_Age = 0;
    is->is_Armed = TRUE;

    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE),
        USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize & 0x7ff) |
        USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
        USB2OTG_HOSTCHAR_EPDIR(req->iouh_Dir == UHDIR_IN ? 1 : 0) |
        USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_ISO) |
        USB2OTG_HOSTCHAR_EC(split ? 1 : iso_mult(req)) |
        USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
        (odd ? USB2OTG_HOSTCHAR_ODDFRAME : 0) |
        USB2OTG_HOSTCHAR_ENABLE);
}

/* Fetch the next request's packet into the bounce buffer: its length */
static void iso_load(struct USB2OTGIso *is)
{
    struct IOUsbHWReq *req = is->is_Req;
    ULONG pkt = (req->iouh_MaxPktSize & 0x7ff) * iso_mult(req);
    ULONG len = req->iouh_Length - is->is_Offset;

    if (len > pkt)
        len = pkt;
    is->is_Len = len;
    is->is_POff = 0;
    if (req->iouh_Dir == UHDIR_IN)
    {
        is->is_PLen = pkt;
        CacheClearE(is->is_Buf, pkt ? pkt : 1, CACRF_ClearD);
    }
    else
    {
        is->is_PLen = len;
        CopyMem((UBYTE *)req->iouh_Data + is->is_Offset, is->is_Buf, len);
        CacheClearE(is->is_Buf, len ? len : 1, CACRF_ClearD);
    }
}

/* One packet without splits: the channel does it all in its (micro)frame */
static void iso_arm_direct(struct USB2OTGIso *is, UWORD target)
{
    struct IOUsbHWReq *req = is->is_Req;
    ULONG mps = req->iouh_MaxPktSize & 0x7ff;
    ULONG mult = iso_mult(req);

    iso_load(is);
    if (req->iouh_Dir == UHDIR_IN)
        iso_start(is, 0, is->is_PLen, mult,
            (mult == 3) ? USB2OTG_PID_DATA2 : (mult == 2) ? USB2OTG_PID_DATA1 : USB2OTG_PID_DATA0,
            0, (target & 1) ^ is->is_OddFlip);
    else
    {
        ULONG pktcnt = is->is_Len ? (is->is_Len + mps - 1) / mps : 1;

        iso_start(is, 0, is->is_Len, pktcnt,
            (pktcnt > 1) ? USB2OTG_PID_MDATA : USB2OTG_PID_DATA0, 0, (target & 1) ^ is->is_OddFlip);
    }
}

/*
 * The next split transaction of the packet in progress. For splits the
 * core compares ODDFRM with the frame, not the microframe.
 */
static void iso_arm_split(struct USB2OTGIso *is)
{
    ULONG hub = USB2OTG_HCSPLT_SPLITEN | ((is->is_HubAddr & 0x7f) << 7) | (is->is_HubPort & 0x7f);
    ULONG left = is->is_PLen - is->is_POff;
    BOOL odd = is->is_Frame & 1;
    ULONG pos;

    switch (is->is_Phase)
    {
    case ISO_OUT_SS:
        /* 188 bytes a microframe, in order, the TT forwarding as it goes */
        if (is->is_POff == 0)
            pos = (left <= USB2OTG_ISO_SPLITMAX) ? XACTPOS_ALL : XACTPOS_BEGIN;
        else
            pos = (left <= USB2OTG_ISO_SPLITMAX) ? XACTPOS_END : XACTPOS_MID;
        is->is_Chunk = (left > USB2OTG_ISO_SPLITMAX) ? USB2OTG_ISO_SPLITMAX : left;
        iso_start(is, hub | (pos << 14), is->is_Chunk, 1, USB2OTG_PID_DATA0, is->is_POff, odd);
        break;

    case ISO_IN_SS:
        iso_start(is, hub | (XACTPOS_ALL << 14), is->is_PLen, 1, USB2OTG_PID_DATA0, 0, odd);
        break;

    case ISO_IN_CS:
        iso_start(is, hub | USB2OTG_HCSPLT_COMPLSPLT | (XACTPOS_ALL << 14),
            left, 1, USB2OTG_PID_DATA0, is->is_POff, odd);
        break;
    }
}

/*
 * The packet is over (lost if !ok): account it to its request and, when
 * that is done, move it to the done list. hu_Lock held. TRUE if a request
 * finished.
 */
static BOOL iso_packet_done(struct USB2OTGUnit *unit, struct USB2OTGIso *is, BOOL ok, ULONG got)
{
    struct IOUsbHWReq *req = is->is_Req;
    BOOL done = FALSE;

    is->is_Phase = ISO_IDLE;
    if (req == NULL)
        return FALSE;

    is->is_Pkts++;
    if (!ok)
        is->is_Lost++;
    /* A lost packet is skipped: iso has no retries */
    if (ok)
    {
        if (req->iouh_Dir == UHDIR_IN)
        {
            if (got > req->iouh_Length - req->iouh_Actual)
                got = req->iouh_Length - req->iouh_Actual;
            if (got > 0)
            {
                CacheClearE(is->is_Buf, got, CACRF_InvalidateD);
                CopyMem(is->is_Buf, (UBYTE *)req->iouh_Data + req->iouh_Actual, got);
                req->iouh_Actual += got;
            }
        }
        else
            req->iouh_Actual += is->is_Len;
    }
    is->is_Offset += is->is_Len ? is->is_Len : 1;

    if (req == is->is_AbortReq)
    {
        req->iouh_Req.io_Error = IOERR_ABORTED;
        done = TRUE;
    }
    else if (is->is_Offset >= req->iouh_Length)
        done = TRUE;

    if (done)
    {
        is->is_Req = NULL;
        is->is_AbortReq = NULL;
        ADDTAIL(&unit->hu_IsoDone, req);
    }
    return done;
}

/* A dropped split sequence leaves TT state the channel must not keep */
static BOOL iso_split_drop(struct USB2OTGUnit *unit, struct USB2OTGIso *is)
{
    usb2otg_exorcise_channel(is->is_Chan);
    return iso_packet_done(unit, is, FALSE, 0);
}

/*
 * The armed transaction halted. Direct: the packet is done. Split: move
 * the sequence on; the next transaction goes from the next SOF, one a
 * microframe. hu_Lock held. TRUE if a request finished.
 */
static BOOL iso_halted(struct USB2OTGUnit *unit, struct USB2OTGIso *is, ULONG intr, UWORD now)
{
    ULONG left = USB2OTG_HOSTTSIZE_SIZE(rd32le(USB2OTG_CHANNEL_REG(is->is_Chan, TRANSSIZE)));
    BOOL ok = (intr & (USB2OTG_INTRCHAN_TRANSFERCOMPLETE | USB2OTG_INTRCHAN_ACKNOWLEDGE)) != 0;

    is->is_Armed = FALSE;

    if (!is->is_Split)
    {
        /* Halt with nothing else: the core skipped it, ODDFRM is the other way */
        if (!(intr & ~USB2OTG_INTRCHAN_HALT))
            is->is_OddFlip ^= 1;
        return iso_packet_done(unit, is, (intr & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) != 0,
            (left < is->is_Size) ? is->is_Size - left : 0);
    }

    switch (is->is_Phase)
    {
    case ISO_OUT_SS:
        if (!ok)
        {
            is->is_DropOut++;
            return iso_split_drop(unit, is);
        }
        is->is_POff += is->is_Chunk;
        if (is->is_POff >= is->is_PLen)
            return iso_packet_done(unit, is, TRUE, 0);
        is->is_At = (now + 1) & USB2OTG_HFNUM_MASK;
        return FALSE;

    case ISO_IN_SS:
        if (!ok)
        {
            is->is_DropSS++;
            return iso_split_drop(unit, is);
        }
        /* The TT runs it next microframe: ask for the result the one after */
        is->is_Phase = ISO_IN_CS;
        is->is_At = (now + 2) & USB2OTG_HFNUM_MASK;
        return FALSE;

    case ISO_IN_CS:
        if (intr & USB2OTG_INTRCHAN_NOTREADY)
        {
            /* NYET: not there yet, try again next microframe */
            is->is_At = (now + 1) & USB2OTG_HFNUM_MASK;
            return FALSE;
        }
        if (!(intr & USB2OTG_INTRCHAN_TRANSFERCOMPLETE))
        {
            is->is_DropCS++;
            return iso_split_drop(unit, is);
        }
        if (left < is->is_PLen - is->is_POff)
            is->is_POff += (is->is_PLen - is->is_POff) - left;
        /* MDATA: more follows; DATA0 ends the packet */
        if (((rd32le(USB2OTG_CHANNEL_REG(is->is_Chan, TRANSSIZE)) >> 29) & 3) == USB2OTG_PID_MDATA &&
            is->is_POff < is->is_PLen)
        {
            is->is_At = (now + 1) & USB2OTG_HFNUM_MASK;
            return FALSE;
        }
        return iso_packet_done(unit, is, TRUE, is->is_POff);
    }
    return FALSE;
}

/*
 * Move the bound endpoint on, from the SOF or the previous halt: start a
 * packet that is due, or (split) issue the sequence's next transaction.
 * is_Next keeps it to one packet an interval; ODDFRM holds the channel to
 * the target. hu_Lock held. FALSE if nothing is queued.
 */
static BOOL iso_kick(struct USB2OTGUnit *unit, struct USB2OTGIso *is, UWORD now)
{
    UWORD target = (now + 1) & USB2OTG_HFNUM_MASK;

    if (!is->is_Bound || is->is_Armed || unit->hu_Channel[is->is_Chan].hc_Request != NULL)
        return TRUE;
    /* Bulk/INT may let go before their halt has landed */
    if (rd32le(USB2OTG_CHANNEL_REG(is->is_Chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE)
        return TRUE;

    /* Coming from bulk/INT: clear their split engine state first */
    if (is->is_Fresh)
    {
        usb2otg_exorcise_channel(is->is_Chan);
        is->is_Fresh = FALSE;
    }

    if (is->is_Phase != ISO_IDLE)
    {
        /* Split sequence: one transaction a microframe, within its frame */
        if (!iso_frame_le(is->is_At, now))
            return TRUE;
        if ((now >> 3) != is->is_Frame)
        {
            is->is_DropLate++;
            if (iso_split_drop(unit, is))
                FNAME_DEV(Cause)(unit->hu_USB2OTGBase, &unit->hu_PendingInt);
            return TRUE;
        }
        iso_arm_split(is);
        return TRUE;
    }

    if (is->is_Req == NULL)
    {
        is->is_Req = (struct IOUsbHWReq *)REMHEAD(&is->is_Queue);
        is->is_Offset = 0;
        if (is->is_Req == NULL)
            return FALSE;
    }
    is->is_Idle = 0;

    if (!iso_frame_le(is->is_Next, target))
        return TRUE;

    if (!is->is_Split)
    {
        /* Behind (a slow start or a stall): restart from here, no burst */
        if (!iso_frame_le(target, is->is_Next))
            is->is_Next = target;
        iso_arm_direct(is, target);
        is->is_Next = (target + iso_interval(is, is->is_Req)) & USB2OTG_HFNUM_MASK;
        return TRUE;
    }

    /*
     * A split packet starts in the last microframe before its frame, so
     * the first start-split goes out at the frame's start. Missed that by
     * more than a microframe or two: wait for the next slot.
     */
    if (((target - is->is_Next) & USB2OTG_HFNUM_MASK) > 2)
    {
        UWORD ival = iso_interval(is, is->is_Req);

        while (iso_frame_le(is->is_Next, (target - 3) & USB2OTG_HFNUM_MASK))
            is->is_Next = (is->is_Next + ival) & USB2OTG_HFNUM_MASK;
        return TRUE;
    }
    is->is_Frame = (is->is_Next >> 3) & 0x7ff;
    is->is_Next = (is->is_Next + iso_interval(is, is->is_Req)) & USB2OTG_HFNUM_MASK;
    iso_load(is);
    is->is_Phase = (is->is_Req->iouh_Dir == UHDIR_IN) ? ISO_IN_SS : ISO_OUT_SS;
    iso_arm_split(is);
    return TRUE;
}

/*
 * From the SOF interrupt. A channel still finishing bulk/INT work it had
 * before binding is waited for; one idle a while is handed back.
 */
void usb2otg_iso_sof(struct USB2OTGUnit *unit, ULONG hfnum)
{
#if defined(__AROSEXEC_SMP__)
    struct USB2OTGDevice *USB2OTGBase = unit->hu_USB2OTGBase;
#endif
    UWORD now = hfnum & USB2OTG_HFNUM_MASK;
    BOOL wake = FALSE;
    int i;

    if (unit->hu_IsoChanMask == 0)
        return;

#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    for (i = 0; i < USB2OTG_ISO_CHANS; i++)
    {
        struct USB2OTGIso *is = &unit->hu_Iso[i];

        /* A transaction whose halt never came must not stop the stream */
        if (is->is_Armed && ++is->is_Age >= USB2OTG_ISO_STUCK)
        {
            if (is->is_Age == USB2OTG_ISO_STUCK)
                wr32le(USB2OTG_CHANNEL_REG(is->is_Chan, CHARBASE),
                    rd32le(USB2OTG_CHANNEL_REG(is->is_Chan, CHARBASE)) |
                    USB2OTG_HOSTCHAR_ENABLE | USB2OTG_HOSTCHAR_DISABLE);
            else if (is->is_Age >= 2 * USB2OTG_ISO_STUCK)
            {
                bug("[USB2OTG:ISO] chan %d never halted, packet dropped\n", (int)is->is_Chan);
                is->is_Armed = FALSE;
                if (iso_split_drop(unit, is))
                    wake = TRUE;
            }
            continue;
        }

        if (!iso_kick(unit, is, now) && ++is->is_Idle > USB2OTG_ISO_LINGER)
        {
            /* Not just a slow requeue: hand the channel back, split state cleared */
            bug("[USB2OTG:ISO] dev %d ep %d %s%s: %u packets, %u lost (split: out %u, in-ss %u, in-cs %u, late %u)\n",
                is->is_Dev, is->is_Ep, is->is_Dir == UHDIR_IN ? "in" : "out", is->is_Split ? " split" : "",
                is->is_Pkts, is->is_Lost, is->is_DropOut, is->is_DropSS, is->is_DropCS, is->is_DropLate);
            is->is_Pkts = is->is_Lost = 0;
            is->is_DropOut = is->is_DropSS = is->is_DropCS = is->is_DropLate = 0;
            usb2otg_exorcise_channel(is->is_Chan);
            is->is_Bound = FALSE;
            unit->hu_IsoChanMask &= ~(1 << is->is_Chan);
            wake = TRUE;
        }
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&unit->hu_Lock);
#endif

    /* A returned channel for bulk/INT, or a request to reply */
    if (wake)
        FNAME_DEV(Cause)(unit->hu_USB2OTGBase, &unit->hu_PendingInt);
}

/* From the channel interrupt of a lent channel; the dispatcher acked intr */
void usb2otg_iso_chan_irq(struct USB2OTGUnit *unit, int chan, ULONG intr)
{
#if defined(__AROSEXEC_SMP__)
    struct USB2OTGDevice *USB2OTGBase = unit->hu_USB2OTGBase;
#endif
    struct USB2OTGIso *is = NULL;
    UWORD now;
    BOOL done;
    int i;

    for (i = 0; i < USB2OTG_ISO_CHANS; i++)
        if (unit->hu_Iso[i].is_Chan == chan)
            is = &unit->hu_Iso[i];
    if (is == NULL || !is->is_Armed)
        return;

    is->is_Pending |= intr;
    if (!(intr & USB2OTG_INTRCHAN_HALT))
        return;

#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    now = rd32le(USB2OTG_HOSTFRAMENO) & USB2OTG_HFNUM_MASK;
    done = iso_halted(unit, is, is->is_Pending, now);
    /* Direct: the next packet now, in case the SOF that would arm it is late */
    if (!is->is_Split)
        iso_kick(unit, is, now);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&unit->hu_Lock);
#endif

    if (done)
        FNAME_DEV(Cause)(unit->hu_USB2OTGBase, &unit->hu_PendingInt);
}

/* After a core reset: whatever was armed is gone, the stream goes on */
void usb2otg_iso_reset(struct USB2OTGUnit *unit)
{
    int i;

    /* The packet in progress goes again from its start */
    for (i = 0; i < USB2OTG_ISO_CHANS; i++)
    {
        unit->hu_Iso[i].is_Armed = FALSE;
        unit->hu_Iso[i].is_Phase = ISO_IDLE;
    }
}

/* AbortIO: TRUE if the request was found; it is then replied aborted */
BOOL usb2otg_iso_abort(struct USB2OTGUnit *unit, struct IOUsbHWReq *ioreq)
{
#if defined(__AROSEXEC_SMP__)
    struct USB2OTGDevice *USB2OTGBase = unit->hu_USB2OTGBase;
#endif
    struct IOUsbHWReq *cmp;
    BOOL found = FALSE;
    int i;

    ISO_LOCK(unit);
    for (i = 0; i < USB2OTG_ISO_CHANS && !found; i++)
    {
        struct USB2OTGIso *is = &unit->hu_Iso[i];

        if (is->is_Req == ioreq)
        {
            /* On the channel or mid split sequence: ended with it, within a frame */
            if (is->is_Armed || is->is_Phase != ISO_IDLE)
                is->is_AbortReq = ioreq;
            else
            {
                is->is_Req = NULL;
                ioreq->iouh_Req.io_Error = IOERR_ABORTED;
                ADDTAIL(&unit->hu_IsoDone, ioreq);
            }
            found = TRUE;
            break;
        }
        ForeachNode(&is->is_Queue, cmp)
        {
            if (cmp == ioreq)
            {
                REMOVE(ioreq);
                ioreq->iouh_Req.io_Error = IOERR_ABORTED;
                ADDTAIL(&unit->hu_IsoDone, ioreq);
                found = TRUE;
                break;
            }
        }
    }
    ISO_UNLOCK(unit);

    if (found)
        FNAME_DEV(Cause)(unit->hu_USB2OTGBase, &unit->hu_PendingInt);
    return found;
}

/* ---- realtime iso, after pcixhci's RT iso contract ---- */

/* Fill one packet through the client's request hook and queue it */
static void rt_queue(struct USB2OTGUnit *unit, struct USB2OTGRTSlot *slot)
{
    struct USB2OTGRTIso *rt = slot->rs_RT;
    struct IOUsbHWRTIso *urti = rt->rt_URTI;
    struct IOUsbHWBufferReq cur = rt->rt_BufReq;
    struct IOUsbHWReq *req = &slot->rs_Req;
    ULONG len = 0, n;

    cur.ubr_Frame = 0;
    cur.ubr_Flags = 0;
    if (req->iouh_Dir == UHDIR_IN)
    {
        cur.ubr_Length = rt->rt_PktSize;
        if (urti->urti_InReqHook)
            ISO_CALLHOOK(urti->urti_InReqHook, urti, &cur);
        slot->rs_BufReq = cur;
        len = rt->rt_PktSize;
        cur.ubr_Buffer += cur.ubr_Length;
    }
    else
    {
        /* The hook may scatter one packet over several buffers */
        do
        {
            cur.ubr_Flags &= ~UBFF_CONTBUFFER;
            cur.ubr_Length = rt->rt_PktSize - len;
            if (urti->urti_OutReqHook)
                ISO_CALLHOOK(urti->urti_OutReqHook, urti, &cur);
            if (len == 0)
                slot->rs_BufReq = cur;
            n = cur.ubr_Length;
            if (n > rt->rt_PktSize - len)
                n = rt->rt_PktSize - len;
            if (n > 0 && cur.ubr_Buffer != NULL)
                CopyMem(cur.ubr_Buffer, slot->rs_Buf + len, n);
            cur.ubr_Buffer += n;
            len += n;
        } while ((cur.ubr_Flags & UBFF_CONTBUFFER) && len < rt->rt_PktSize);
    }
    rt->rt_BufReq = cur;

    req->iouh_Data = slot->rs_Buf;
    req->iouh_Length = len;
    slot->rs_Busy = (iso_submit(unit, req) == RC_DONTREPLY);
}

static void rt_free(struct USB2OTGRTIso *rt)
{
    int i;

    for (i = 0; i < USB2OTG_RT_SLOTS; i++)
        FreeVec(rt->rt_Slots[i].rs_Buf);
    FreeVec(rt);
}

/* hu_IsoSem held */
static void rt_done(struct USB2OTGUnit *unit, struct USB2OTGRTSlot *slot)
{
    struct USB2OTGRTIso *rt = slot->rs_RT;
    struct IOUsbHWRTIso *urti = rt->rt_URTI;
    struct IOUsbHWBufferReq *ubr = &slot->rs_BufReq;
    int i;

    slot->rs_Busy = FALSE;
    if (rt->rt_Dying)
    {
        for (i = 0; i < USB2OTG_RT_SLOTS; i++)
            if (rt->rt_Slots[i].rs_Busy)
                return;
        rt_free(rt);
        return;
    }
    if (!rt->rt_Running)
        return;

    /* A lost packet reaches the hook as a short one */
    if (slot->rs_Req.iouh_Req.io_Error == 0)
    {
        ubr->ubr_Length = slot->rs_Req.iouh_Actual;
        if (slot->rs_Req.iouh_Dir == UHDIR_IN)
        {
            if (ubr->ubr_Length > 0 && ubr->ubr_Buffer != NULL)
                CopyMem(slot->rs_Buf, ubr->ubr_Buffer, ubr->ubr_Length);
            if (urti->urti_InDoneHook)
                ISO_CALLHOOK(urti->urti_InDoneHook, urti, ubr);
        }
        else if (urti->urti_OutDoneHook)
            ISO_CALLHOOK(urti->urti_OutDoneHook, urti, ubr);
    }

    rt_queue(unit, slot);
}

/*
 * hu_IsoSem held: abort the slots. Each comes back through the done list
 * (one on a channel at its halt), and only rt_done clears rs_Busy.
 */
static void rt_stop(struct USB2OTGUnit *unit, struct USB2OTGRTIso *rt)
{
    int i;

    rt->rt_Running = FALSE;
    for (i = 0; i < USB2OTG_RT_SLOTS; i++)
        if (rt->rt_Slots[i].rs_Busy)
            usb2otg_iso_abort(unit, &rt->rt_Slots[i].rs_Req);
}

/*
 * Poseidon sends these with DoIO and no reply port, so they complete
 * here, in BeginIO. hu_IsoSem keeps the completions out meanwhile.
 */
WORD FNAME_DEV(cmdRTIso)(struct IOUsbHWReq *ioreq,
                     struct USB2OTGUnit *otg_Unit,
                     LIBBASETYPEPTR USB2OTGBase)
{
    struct IOUsbHWRTIso *urti = ioreq->iouh_Data;
    struct USB2OTGRTIso *rt;
    WORD ret = 0;
    ULONG mult;
    int i;

    if (urti == NULL)
        return UHIOERR_BADPARAMS;

    ObtainSemaphore(&otg_Unit->hu_IsoSem);
    rt = urti->urti_DriverPrivate1;

    switch (ioreq->iouh_Req.io_Command)
    {
    case UHCMD_ADDISOHANDLER:
        if (ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            ret = UHIOERR_BADPARAMS;
            break;
        }
        rt = AllocVec(sizeof(*rt), MEMF_ANY | MEMF_CLEAR);
        if (rt == NULL)
        {
            ret = UHIOERR_OUTOFMEMORY;
            break;
        }
        mult = (ioreq->iouh_Flags & UHFF_MULTI_3) / UHFF_MULTI_1;
        rt->rt_PktSize = (ioreq->iouh_MaxPktSize & 0x7ff) * (mult ? mult : 1);
        rt->rt_URTI = urti;
        rt->rt_BufReq.ubr_Length = rt->rt_PktSize;
        for (i = 0; i < USB2OTG_RT_SLOTS; i++)
        {
            struct USB2OTGRTSlot *slot = &rt->rt_Slots[i];

            slot->rs_RT = rt;
            slot->rs_Req = *ioreq;
            slot->rs_Req.iouh_Req.io_Command = UHCMD_ISOXFER;
            /* No reply port marks a slot on the done list */
            slot->rs_Req.iouh_Req.io_Message.mn_ReplyPort = NULL;
            slot->rs_Buf = AllocVec(rt->rt_PktSize ? rt->rt_PktSize : 1, MEMF_ANY | MEMF_CLEAR);
            if (slot->rs_Buf == NULL)
            {
                rt_free(rt);
                rt = NULL;
                ret = UHIOERR_OUTOFMEMORY;
                break;
            }
        }
        urti->urti_DriverPrivate1 = rt;
        break;

    case UHCMD_STARTRTISO:
        if (rt == NULL)
            ret = UHIOERR_BADPARAMS;
        else if (!rt->rt_Running)
        {
            rt->rt_Running = TRUE;
            for (i = 0; i < USB2OTG_RT_SLOTS; i++)
                if (!rt->rt_Slots[i].rs_Busy)
                    rt_queue(otg_Unit, &rt->rt_Slots[i]);
        }
        break;

    case UHCMD_STOPRTISO:
        if (rt == NULL)
            ret = UHIOERR_BADPARAMS;
        else
            rt_stop(otg_Unit, rt);
        break;

    case UHCMD_REMISOHANDLER:
        if (rt == NULL)
        {
            ret = UHIOERR_BADPARAMS;
            break;
        }
        rt_stop(otg_Unit, rt);
        urti->urti_DriverPrivate1 = NULL;
        rt->rt_Dying = TRUE;
        for (i = 0; i < USB2OTG_RT_SLOTS; i++)
            if (rt->rt_Slots[i].rs_Busy)
                break;
        /* A slot still on a channel frees it when it comes back */
        if (i == USB2OTG_RT_SLOTS)
            rt_free(rt);
        break;

    default:
        ret = IOERR_NOCMD;
        break;
    }

    ReleaseSemaphore(&otg_Unit->hu_IsoSem);
    /* Completions deferred while we held the semaphore */
    FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);
    return ret;
}

/* Worker (or softint): reply finished transfers, run realtime streams on */
void usb2otg_iso_pending(struct USB2OTGUnit *unit)
{
#if defined(__AROSEXEC_SMP__)
    struct USB2OTGDevice *USB2OTGBase = unit->hu_USB2OTGBase;
#endif
    struct IOUsbHWReq *req;

    if (IsListEmpty(&unit->hu_IsoDone))
        return;

    /* A softint cannot wait: if a handler command holds it, it Causes us again */
    if (!AttemptSemaphore(&unit->hu_IsoSem))
        return;

    for (;;)
    {
        ISO_LOCK(unit);
        req = (struct IOUsbHWReq *)REMHEAD(&unit->hu_IsoDone);
        ISO_UNLOCK(unit);
        if (req == NULL)
            break;

        if (req->iouh_Req.io_Message.mn_ReplyPort == NULL)
            rt_done(unit, (struct USB2OTGRTSlot *)req);
        else
            FNAME_DEV(TermIO)(req, unit->hu_USB2OTGBase);
    }

    ReleaseSemaphore(&unit->hu_IsoSem);
}
