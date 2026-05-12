#ifndef USB2OTG_INTERN_H
#define USB2OTG_INTERN_H
/*
    Copyright � 2013-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include LC_LIBDEFS_FILE

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <aros/types/spinlock_s.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <dos/dos.h>

#include <devices/timer.h>
#include <utility/utility.h>

#include <devices/usbhardware.h>
#include <devices/newstyle.h>

#include <oop/oop.h>

#include <proto/kernel.h>

extern IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase
#include <hardware/bcm2708.h>
#include <hardware/usb2otg.h>

/*
    Force the USB chipset to run in Host mode
    AFAIK Poseidon doesnt support device mode? - TODO
*/
//#define OTG_FORCEHOSTMODE
//#define OTG_FORCEDEVICEMODE

#include <asm/cpu.h>

/* Reply the iorequest with success */
#define RC_OK	      0

/* Magic cookie, don't set error fields & don't reply the ioreq */
#define RC_DONTREPLY  -1

#define MAX_ROOT_PORTS	 16

#define VCMB_PROPCHAN   8
#define VCPOWER_USBHCD  3
#define VCPOWER_STATE_ON    1
#define VCPOWER_STATE_WAIT  2

static inline ULONG rd32le(IPTR iobase) {
    ULONG val;
    dmb();
    val = AROS_LE2LONG(*(volatile ULONG *)(iobase));
    dsb();
    return val;
}

static inline UWORD rd16le(IPTR iobase) {
    UWORD val;
    dmb();
    val = AROS_LE2WORD(*(volatile UWORD *)(iobase));
    dsb();
    return val;
}

static inline UBYTE rd8(IPTR iobase) {
    UBYTE val;
    dmb();
    val = *(volatile UBYTE *)(iobase);
    dsb();
    return val;
}

static inline void wr32le(IPTR iobase, ULONG value) {
    dsb();
    *(volatile ULONG *)(iobase) = AROS_LONG2LE(value);
    dmb();
}

static inline void wr16le(IPTR iobase, UWORD value) {
    dsb();
    *(volatile UWORD *)(iobase) = AROS_WORD2LE(value);
    dmb();
}

static inline void wr8be(IPTR iobase, UBYTE value) {
    dsb();
    *(volatile UBYTE *)(iobase) = value;
    dmb();
}

/*
 * Spin-wait for AHB master IDLE (RESET[31]). Required by DWC2 spec
 * before any soft reset or FIFO flush.
 */
static inline BOOL usb2otg_wait_ahb_idle(int timeout, const char *where)
{
    while (!(rd32le(USB2OTG_RESET) & USB2OTG_RESET_AHBIDLE) && --timeout > 0)
        asm volatile("mov r0, r0\n");
    if (timeout <= 0)
    {
        bug("[USB2OTG] AHB IDLE wait timed out (%s)\n", where);
        return FALSE;
    }
    return TRUE;
}

/* Spin-wait for a self-clearing RESET bit (CORESOFT/TXFIFOFLUSH/RXFIFOFLUSH). */
static inline BOOL usb2otg_wait_reset_bit_clear(ULONG bit, int timeout,
    const char *what)
{
    while ((rd32le(USB2OTG_RESET) & bit) && --timeout > 0)
        asm volatile("mov r0, r0\n");
    if (timeout <= 0)
    {
        bug("[USB2OTG] %s timed out\n", what);
        return FALSE;
    }
    return TRUE;
}

/*
 * RMW HOSTPORT with W1C status-change bits masked out, so toggling a
 * feature doesn't accidentally acknowledge port-change events.
 */
static inline void usb2otg_hostport_rmw(ULONG set, ULONG clear)
{
    ULONG v = rd32le(USB2OTG_HOSTPORT);
    v &= ~USB2OTG_HOSTPORT_SC_BITS;
    v &= ~clear;
    v |= set;
    wr32le(USB2OTG_HOSTPORT, v);
}

/* Clear stale retry state in DriverPrivate slots at xfer-entry. */
static inline void usb2otg_reset_retry_state(struct IOUsbHWReq *ioreq)
{
    ioreq->iouh_DriverPrivate1 = 0;
    ioreq->iouh_DriverPrivate2 = 0;
}

struct USBNSDeviceQueryResult
{
    ULONG               DevQueryFormat;
    ULONG               SizeAvailable;
    UWORD               DeviceType;
    UWORD               DeviceSubType;
    const UWORD         *SupportedCommands;     /* 0 terminated list of cmd's   */
};

struct USB2OTGUnit
{
    struct Unit         hu_Unit;

    struct List         hu_IOPendingQueue;	/* Root Hub Pending IO Requests */

    struct List         hu_TDQueue;
    struct List         hu_PeriodicTDQueue;
    struct List         hu_CtrlXFerQueue;
    struct List         hu_IntXFerQueue;
    struct List         hu_IntXFerScheduled;
    struct List         hu_IsoXFerQueue;
    struct List         hu_BulkXFerQueue;
    struct List         hu_FinishedXfers;
    /*
     * Single lock for all USB internal state. DWC2 IRQ is pinned to
     * CPU 0; this only provides cross-CPU exclusion for task-context
     * submission. Single lock minimizes barriers on the hot IRQ path.
     */
    spinlock_t          hu_Lock;


    struct USB2OTGChannel {
        struct IOUsbHWReq * hc_Request;
        ULONG               hc_XferSize;
        APTR                hc_OrigBuffer;  /* Non-NULL when bounce buffer is active (stores original unaligned ptr) */
        ULONG               hc_BounceLen;   /* Bytes in bounce buffer for copy-back */
        UBYTE               hc_BounceDir;   /* Direction: 0=OUT, 1=IN */
        UBYTE               hc_WatchdogCount; /* Incremented each NakTimeout period while channel active */
        UBYTE               hc_SplitCSplitPending; /* SSPLIT was issued and request must not be resubmitted yet */
        UBYTE               hc_DeferCount;     /* Consecutive watchdog defers; caps total defer time */
        struct IOUsbHWReq * hc_DiagReq;     /* Last bulk request tracked on this channel */
        ULONG               hc_DiagStartFrame;
        ULONG               hc_DiagLastProgressFrame;
        ULONG               hc_DiagLastActual;
        UWORD               hc_DiagLastIntr;
        UBYTE               hc_DiagRequeueCount;
        UBYTE               hc_DiagNoProgressCount;
        UWORD               hc_BareChhltdTotal; /* cumulative bare-CHHLTD per request; absolute give-up cap */
        ULONG               hc_StartHfnum;    /* HFNUM at last StartChannel arm; for bulk diag timing */
        UBYTE               hc_NakParked;     /* 1 = bulk-IN parked waiting for NAK gate; SOF re-arms with quick=1 */
        UBYTE               hc_PingPending;   /* 1 = OR in HOSTTSIZE_PING on next arm */
        UBYTE               hc_PingState;     /* PING state machine: 0=none, 1=needs PING arm, 2=PING armed */
        UWORD               hc_NyetCount;     /* NYET-handler hits per request; resets on new request */
        ULONG               hc_LastSampleHfnum; /* sample snapshot for WAIT-PING/WAIT-QUIET liveness check */
        ULONG               hc_LastSampleTsize;
        ULONG               hc_LastSampleActual;
        UWORD               hc_LastSampleIntr;
    }                   hu_Channel[8];

/*
 * Per-channel DMA bounce buffers for unaligned transfers. DWC2 DMA
 * on BCM2835/2837 requires DWORD-aligned addresses. Sized to fit a
 * typical AROS 16 KB SCSI WRITE cluster (8 chans x 16 KB = 128 KB).
 */
#define DMA_BOUNCE_SIZE (16 * 1024)

//    struct IOUsbHWReq * hu_InProgressXFer[8];
//    ULONG               hu_InProgressXFerSize[8];

    struct List         hu_AbortQueue;

    APTR                hu_GlobalIRQHandle;
    struct Interrupt	hu_PendingInt;
    struct Interrupt    hu_NakTimeoutInt;
    struct timerequest  hu_NakTimeoutReq;
    struct MsgPort      hu_NakTimeoutMsgPort;
    struct Task         *hu_WorkerTask;
    struct MsgPort      *hu_WorkerPort;
    cpumask_t           hu_WorkerAffinity;
    ULONG               hu_WorkFlags;

    UBYTE               hu_OperatingMode;       /* HOST/DEVICE mode */
    UBYTE               hu_HubAddr;
    UBYTE               hu_HostChans;
    UBYTE               hu_DevEPs;
    UBYTE               hu_DevInEPs;

    BOOL                hu_UnitAllocated;       /* unit opened */
    BOOL                hu_HubPortChanged;      /* Root port state change */
    APTR                hu_USB2OTGBase;

    ULONG               hu_XferSizeWidth;
    ULONG               hu_PktSizeWidth;

    ULONG               hu_PIDBits[128];        /* PID 2-bit pairs, one ULONG per device, each ULONG contains 2-bits for every endpoint */

    /*
     * Per-(device, endpoint) PING-pending state (USB 2.0 §8.5.1).
     * Channel-local hc_PingPending doesn't survive requeue, so PING
     * state must live on the (dev, ep) pair. 1 bit per ep, 1 ULONG
     * per dev addr.
     */
    ULONG               hu_PingBits[128];

    /*
     * Per-device bulk NAK rate gate. Stores the earliest microframe
     * (11-bit HOSTFRAMENO) at which this device's next bulk retry may
     * arm; 0xFFFF = no gate. Mirrors FreeBSD dwc_otg's did_nak
     * rate-check (dwc_otg.c:1215-1250). Stops NAK-storm CPU0 burn.
     */
    UWORD               hu_NakGate[128];

    /*
     * Per-device bulk channel binding: [0]→CHAN_BULK, [1]→CHAN_BULK2.
     * 0 = free. Partitions two concurrent bulk devices onto a channel
     * each so a wedged/NAK-storming device cannot block the other.
     */
    UBYTE               hu_BulkOwnerDev[2];

/*
 * DMA buffers — must be in heap memory so the 0xC0000000 VC bus
 * alias maps correctly. Kernel static arrays in 0xf8XXXXXX VA do not.
 * ULONG arrays guarantee 4-byte alignment for DMA.
 */
    ULONG               hu_StatusDmaBuf[4];
    /* Cache-line aligned: per-buffer flush won't drag in adjacent data. */
    ULONG               hu_BounceBuf[8][DMA_BOUNCE_SIZE / sizeof(ULONG)]
                        __attribute__((aligned(64)));
};

/* PRIVATE device node */
struct USB2OTGDevice
{
    struct Library	hd_Library;	        /* standard */
    UWORD		hd_Flags;	        /* various flags */

    APTR		hd_KernelBase;		/* kernel.resource base */
    APTR                hd_UtilityBase;	        /* for tags etc */

    APTR		hd_MemPool;	        /* memory pool */

    struct USB2OTGUnit  *hd_Unit;	        /* we only currently support a single unit.. */

    struct MsgPort	*hd_MsgPort;
    struct timerequest	*hd_TimerReq;	        /* Timer I/O Requests */

};

#define FNAME_DEV(x)            USB2OTG__Dev__ ## x
#define FNAME_ROOTHUB(x)        USB2OTG__RootHub__ ## x

#define USB2OTG_WORK_PENDING    (1U << 0)
#define USB2OTG_WORK_NAKTIMEOUT (1U << 1)

#ifdef UtilityBase
#undef UtilityBase
#endif

#ifdef KernelBase
#undef KernelBase
#endif

#define	UtilityBase     USB2OTGBase->hd_UtilityBase

#define KernelBase      USB2OTGBase->hd_KernelBase

#define CHAN_CTRL       0
#define CHAN_BULK       1
#define CHAN_INT1       2
#define CHAN_INT2       3
#define CHAN_INT3       4
#define CHAN_INT4       5
#define CHAN_INT5       6
#define CHAN_INT_LAST   CHAN_INT5
#define CHAN_BULK2      7

/*
 * Bulk-OUT per-packet throttle (busy-wait iterations before arming).
 * Some flash devices with tiny SRAM buffers go silent (no NYET/NAK)
 * when overrun. Value 0 disables; ~1M iterations ≈ 1 ms per packet
 * on Cortex-A7 @ 1.2 GHz.
 */
#define USB2OTG_BULK_OUT_THROTTLE_COUNT 0

/*
 * Per-transfer-type watchdog thresholds in 150 ms ticks. Channel
 * must be active this many ticks before force-fail. Inspired by
 * FreeBSD dwc_otg_timer_start().
 */
#define USB2OTG_WD_TICKS_INT_DIRECT   3   /* HS INT direct: 450 ms */
#define USB2OTG_WD_TICKS_INT_SPLIT    6   /* LS/FS INT through TT: 900 ms */
#define USB2OTG_WD_TICKS_CTRL         6   /* Control transfers: 900 ms */
#define USB2OTG_WD_TICKS_BULK_DIRECT 20   /* HS bulk direct: 3 s */
#define USB2OTG_WD_TICKS_BULK_SPLIT  10   /* LS/FS bulk through TT: 1.5 s */
#define USB2OTG_WD_TICKS_DEFAULT      3   /* anything else */

/* Bulk NAK rate gate: 8 microframes = 1 ms at HS. */
#define USB2OTG_NAK_GATE_NONE          0xFFFFU
#define USB2OTG_BULK_NAK_GATE_UFRAMES  8

/*
 * Flash-busy defer cap (WAIT-PING / WAIT-QUIET). Each cycle is
 * 20 ticks × 150 ms = 3 s. Cap × 3 s = total tolerated NAK+PING-storm
 * time. The halt path triggered when the cap hits can leave Pi 3B+
 * in an unrecoverable CHENA+CHDIS state (neither NPTXFIFO flush nor
 * HCLKSOFT clears it), so set high (200 × 3 s = 10 min) to tolerate
 * cheap-stick FTL GC pauses.
 */
#define USB2OTG_FLASH_BUSY_DEFER_CAP   200

/*
 * Per-request retry budget shared across transient errors (XactErr,
 * DataTglErr, BNA — USB 2.0 §8.4.6). Counter in iouh_DriverPrivate2;
 * resets on XFERCOMPL/NAK/NYET. Matches FreeBSD dwc_otg errcnt.
 */
#define USB2OTG_TRANSIENT_RETRY_LIMIT  3

static inline ULONG usb2otg_current_uframe(void)
{
    /* HFNUM[13:0] FRNUM: 14-bit microframe counter, wraps every 2.048 s. */
    return rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff;
}

static inline BOOL usb2otg_nak_gated(struct USB2OTGUnit *u, UBYTE dev)
{
    UWORD gate = u->hu_NakGate[dev & 0x7f];
    ULONG now;
    ULONG delta;

    if (gate == USB2OTG_NAK_GATE_NONE)
        return FALSE;

    now = usb2otg_current_uframe();
    /* gate in future iff delta is within half the 14-bit wrap (~1 s). */
    delta = (gate - now) & 0x3fff;
    if (delta == 0 || delta >= 0x2000)
    {
        /* Gate reached/past — clear. */
        D(
            {
                static ULONG cleared = 0;
                if (++cleared <= 3 || (cleared & 0x3f) == 0)
                    bug("[USB2OTG:GATE] cleared dev=%d gate=%04x now=%04lx delta=%lu (#%lu)\n",
                        (int)dev, (unsigned)gate, (unsigned long)now,
                        (unsigned long)delta, (unsigned long)cleared);
            }
        )
        u->hu_NakGate[dev & 0x7f] = USB2OTG_NAK_GATE_NONE;
        return FALSE;
    }
    return TRUE;
}

static inline void usb2otg_nak_gate_set(struct USB2OTGUnit *u, UBYTE dev,
    ULONG uframes)
{
    UWORD new_gate = (UWORD)((usb2otg_current_uframe() + uframes) & 0x3fff);
    u->hu_NakGate[dev & 0x7f] = new_gate;
    D(
        if (uframes >= 100)
        {
            bug("[USB2OTG:GATE] set dev=%d gate=%04x uframes=%lu now=%04lx\n",
                (int)dev, (unsigned)new_gate, (unsigned long)uframes,
                (unsigned long)usb2otg_current_uframe());
        }
    )
}

static inline UBYTE usb2otg_watchdog_ticks(struct IOUsbHWReq *req)
{
    BOOL split;

    /* ISO is intentionally absent: cmdIsoXFer queues but nothing drains. */
    if (req == NULL)
        return USB2OTG_WD_TICKS_DEFAULT;

    split = (req->iouh_Flags & UHFF_SPLITTRANS) != 0;

    switch (req->iouh_Req.io_Command)
    {
        case UHCMD_INTXFER:
            return split ? USB2OTG_WD_TICKS_INT_SPLIT
                         : USB2OTG_WD_TICKS_INT_DIRECT;
        case UHCMD_CONTROLXFER:
            return USB2OTG_WD_TICKS_CTRL;
        case UHCMD_BULKXFER:
            return split ? USB2OTG_WD_TICKS_BULK_SPLIT
                         : USB2OTG_WD_TICKS_BULK_DIRECT;
        default:
            return USB2OTG_WD_TICKS_DEFAULT;
    }
}

struct Unit             *FNAME_DEV(OpenUnit)(struct IOUsbHWReq *, LONG, struct USB2OTGDevice *);
void                    FNAME_DEV(CloseUnit)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);

void                    FNAME_DEV(TermIO)(struct IOUsbHWReq *, struct USB2OTGDevice *);

WORD                    FNAME_DEV(cmdNSDeviceQuery)(struct IOStdReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdQueryDevice)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);

WORD                    FNAME_DEV(cmdReset)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdFlush)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);

WORD                    FNAME_DEV(cmdUsbReset)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdUsbResume)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdUsbSuspend)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdUsbOper)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdControlXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdBulkXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdIntXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdIsoXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);

void                    FNAME_DEV(Cause)(struct USB2OTGDevice *, struct Interrupt *);

static inline BOOL usb2otg_diag_track_bulk_req(struct IOUsbHWReq *req)
{
    return req != NULL &&
           req->iouh_Req.io_Command == UHCMD_BULKXFER &&
           req->iouh_Length != 0;
}

/* Enable to trace all bulk I/O (mass storage, ethernet, etc.). */
#define USB2OTG_DEBUG_MASS_STORAGE 1

static inline BOOL usb2otg_trace_bulk(int chan, struct IOUsbHWReq *req)
{
#if USB2OTG_DEBUG_MASS_STORAGE
    return req != NULL &&
           req->iouh_Req.io_Command == UHCMD_BULKXFER;
#else
    (void)chan; (void)req;
    return FALSE;
#endif
}

static inline ULONG usb2otg_diag_frame(void)
{
    return (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;
}

/*
 * Move req from channel slot to follow-up queue under hu_Lock, with
 * a TOCTOU check vs the watchdog. Returns FALSE if the watchdog
 * already took the request (caller must not touch req).
 */
static inline BOOL usb2otg_irq_finish_or_requeue(struct USB2OTGUnit *unit,
    int chan, struct IOUsbHWReq *req, struct List *queue, BOOL head)
{
#if defined(__AROSEXEC_SMP__)
    /* KernelBase macro resolves through USB2OTGBase; needs local visibility. */
    struct USB2OTGDevice *USB2OTGBase = unit->hu_USB2OTGBase;
    KrnSpinLock(&unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
    if (unit->hu_Channel[chan].hc_Request != req)
    {
        KrnSpinUnLock(&unit->hu_Lock);
        return FALSE;
    }
#endif
    if (head)
        ADDHEAD(queue, (struct Node *)req);
    else
        ADDTAIL(queue, (struct Node *)req);
    unit->hu_Channel[chan].hc_Request = NULL;
    unit->hu_Channel[chan].hc_NakParked = 0;
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&unit->hu_Lock);
#endif
    return TRUE;
}

/* Standard diag log cadence: first 5 events, then every 64th. */
static inline BOOL usb2otg_diag_log_rate(ULONG count)
{
    return count <= 5 || (count & 0x3f) == 0;
}

static inline void usb2otg_diag_bulk_assign(struct USB2OTGUnit *otg_Unit, int chan,
    struct IOUsbHWReq *req)
{
    struct USB2OTGChannel *hc = &otg_Unit->hu_Channel[chan];

    if (!usb2otg_diag_track_bulk_req(req))
        return;

    if (hc->hc_DiagReq != req)
    {
        ULONG frame = usb2otg_diag_frame();

        hc->hc_DiagReq = req;
        hc->hc_DiagStartFrame = frame;
        hc->hc_DiagLastProgressFrame = frame;
        hc->hc_DiagLastActual = req->iouh_Actual;
        hc->hc_DiagLastIntr = 0;
        hc->hc_DiagRequeueCount = 0;
        hc->hc_DiagNoProgressCount = 0;
        hc->hc_BareChhltdTotal = 0;
        hc->hc_NyetCount = 0;
        hc->hc_LastSampleHfnum = 0;
        hc->hc_LastSampleTsize = 0;
        hc->hc_LastSampleActual = 0;
        hc->hc_LastSampleIntr = 0;
    }
}

static inline void usb2otg_diag_bulk_progress(struct USB2OTGUnit *otg_Unit, int chan,
    struct IOUsbHWReq *req, ULONG intr)
{
    struct USB2OTGChannel *hc = &otg_Unit->hu_Channel[chan];

    if (!usb2otg_diag_track_bulk_req(req))
        return;

    usb2otg_diag_bulk_assign(otg_Unit, chan, req);
    hc->hc_DiagLastIntr = (UWORD)intr;

    if (req->iouh_Actual != hc->hc_DiagLastActual)
    {
        D(ULONG prev = hc->hc_DiagLastActual;)

        hc->hc_DiagLastActual = req->iouh_Actual;
        hc->hc_DiagLastProgressFrame = usb2otg_diag_frame();

        D(
            if (hc->hc_DiagRequeueCount >= 4 || hc->hc_DiagNoProgressCount >= 4)
            {
                bug("[USB2OTG:DIAG] bulk-progress chan=%d dev=%d ep=%d dir=%s %lu->%lu/%lu intr=%04x rq=%u np=%u split=%u\n",
                    chan,
                    (int)req->iouh_DevAddr,
                    (int)req->iouh_Endpoint,
                    req->iouh_Dir == UHDIR_IN ? "IN" : "OUT",
                    (unsigned long)prev,
                    (unsigned long)req->iouh_Actual,
                    (unsigned long)req->iouh_Length,
                    (unsigned int)intr,
                    (unsigned int)hc->hc_DiagRequeueCount,
                    (unsigned int)hc->hc_DiagNoProgressCount,
                    (unsigned int)otg_Unit->hu_Channel[chan].hc_SplitCSplitPending);
            }
        )

        hc->hc_DiagRequeueCount = 0;
        hc->hc_DiagNoProgressCount = 0;
        hc->hc_BareChhltdTotal = 0;
    }
}

static inline void usb2otg_diag_bulk_requeue(struct USB2OTGUnit *otg_Unit, int chan,
    struct IOUsbHWReq *req, ULONG intr, const char *why)
{
    struct USB2OTGChannel *hc = &otg_Unit->hu_Channel[chan];

    if (!usb2otg_diag_track_bulk_req(req))
        return;

    usb2otg_diag_bulk_assign(otg_Unit, chan, req);
    hc->hc_DiagLastIntr = (UWORD)intr;
    hc->hc_DiagRequeueCount++;

    if (req->iouh_Actual == hc->hc_DiagLastActual)
        hc->hc_DiagNoProgressCount++;
    else
    {
        hc->hc_DiagLastActual = req->iouh_Actual;
        hc->hc_DiagLastProgressFrame = usb2otg_diag_frame();
        hc->hc_DiagNoProgressCount = 0;
    }

    D(
        if (hc->hc_DiagRequeueCount == 4 ||
            hc->hc_DiagRequeueCount == 8 ||
            hc->hc_DiagRequeueCount == 16)
        {
            ULONG frame = usb2otg_diag_frame();
            int int_busy = 0;
            int scan;

            for (scan = CHAN_INT1; scan <= CHAN_INT_LAST; scan++)
            {
                if (otg_Unit->hu_Channel[scan].hc_Request != NULL)
                    int_busy++;
            }

            bug("[USB2OTG:DIAG] bulk-requeue chan=%d dev=%d ep=%d dir=%s act=%lu/%lu intr=%04x why=%s rq=%u np=%u age=%lu bulkq=%d intbusy=%d split=%08x char=%08x tsize=%08x\n",
                chan,
                (int)req->iouh_DevAddr,
                (int)req->iouh_Endpoint,
                req->iouh_Dir == UHDIR_IN ? "IN" : "OUT",
                (unsigned long)req->iouh_Actual,
                (unsigned long)req->iouh_Length,
                (unsigned int)intr,
                why,
                (unsigned int)hc->hc_DiagRequeueCount,
                (unsigned int)hc->hc_DiagNoProgressCount,
                (unsigned long)((frame - hc->hc_DiagStartFrame) & 0x7ff),
                (int)IsListEmpty(&otg_Unit->hu_BulkXFerQueue) ? 0 : 1,
                int_busy,
                rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)),
                rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
                rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE)));
        }
    )
}

static inline void usb2otg_diag_bulk_finish(struct USB2OTGUnit *otg_Unit, int chan,
    struct IOUsbHWReq *req)
{
    struct USB2OTGChannel *hc = &otg_Unit->hu_Channel[chan];

    if (!usb2otg_diag_track_bulk_req(req))
        return;

    usb2otg_diag_bulk_assign(otg_Unit, chan, req);

    D(
        if (req->iouh_Req.io_Error != 0 ||
            hc->hc_DiagRequeueCount >= 4 ||
            hc->hc_DiagNoProgressCount >= 4)
        {
            ULONG frame = usb2otg_diag_frame();

            bug("[USB2OTG:DIAG] bulk-finish chan=%d dev=%d ep=%d dir=%s act=%lu/%lu err=%d rq=%u np=%u nyet=%u age=%lu last_intr=%04x split=%u\n",
                chan,
                (int)req->iouh_DevAddr,
                (int)req->iouh_Endpoint,
                req->iouh_Dir == UHDIR_IN ? "IN" : "OUT",
                (unsigned long)req->iouh_Actual,
                (unsigned long)req->iouh_Length,
                (int)req->iouh_Req.io_Error,
                (unsigned int)hc->hc_DiagRequeueCount,
                (unsigned int)hc->hc_DiagNoProgressCount,
                (unsigned int)hc->hc_NyetCount,
                (unsigned long)((frame - hc->hc_DiagStartFrame) & 0x7ff),
                (unsigned int)hc->hc_DiagLastIntr,
                (unsigned int)otg_Unit->hu_Channel[chan].hc_SplitCSplitPending);
        }
    )

    hc->hc_DiagReq = NULL;
    hc->hc_DiagStartFrame = 0;
    hc->hc_DiagLastProgressFrame = 0;
    hc->hc_DiagLastActual = 0;
    hc->hc_DiagLastIntr = 0;
    hc->hc_DiagRequeueCount = 0;
    hc->hc_DiagNoProgressCount = 0;
    hc->hc_NyetCount = 0;
    hc->hc_LastSampleHfnum = 0;
    hc->hc_LastSampleTsize = 0;
    hc->hc_LastSampleActual = 0;
    hc->hc_LastSampleIntr = 0;
}
void                    FNAME_DEV(WorkerTask)(struct USB2OTGUnit *);

WORD                    FNAME_ROOTHUB(cmdControlXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_ROOTHUB(cmdIntXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
void                    FNAME_ROOTHUB(PendingIO)(struct USB2OTGUnit *);

void                    FNAME_DEV(GlobalIRQHandler)(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase);
void                    FNAME_DEV(ScheduleCtrlTDs)(struct USB2OTGUnit *);
void                    FNAME_DEV(ScheduleBulkTDs)(struct USB2OTGUnit *);
void                    FNAME_DEV(ScheduleIntTDs)(struct USB2OTGUnit *);
BOOL                    FNAME_DEV(SetupChannel)(struct USB2OTGUnit *, int chan);
void                    FNAME_DEV(StartChannel)(struct USB2OTGUnit *, int chan, int quick);
int                     FNAME_DEV(AdvanceChannel)(struct USB2OTGUnit *, int chan);
void                    FNAME_DEV(FinalizeChannel)(struct USB2OTGUnit *, int chan);

#endif /* USB2OTG_INTERN_H */
