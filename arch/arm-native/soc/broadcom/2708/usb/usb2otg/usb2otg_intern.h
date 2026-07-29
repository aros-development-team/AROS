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
        asm volatile("yield\n");
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
        asm volatile("yield\n");
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
/*
 * Core-state snapshot for diffing the boot baseline against the
 * post-recovery state (transfers work after the former, die after the
 * latter — the difference must be visible somewhere below).
 */
static inline void usb2otg_dump_core_state(const char *tag)
{
    D(bug("[USB2OTG:CORE %s] GOTG=%08x GUSB=%08x GAHB=%08x GINTMSK=%08x GRST=%08x\n",
        tag,
        rd32le(USB2OTG_OTGCTRL),
        rd32le(USB2OTG_USB),
        rd32le(USB2OTG_AHB),
        rd32le(USB2OTG_INTRMASK),
        rd32le(USB2OTG_RESET));)
    D(bug("[USB2OTG:CORE %s] HCFG=%08x HFIR=%08x HPRT=%08x HFNUM=%08x HAINTMSK=%08x\n",
        tag,
        rd32le(USB2OTG_HOSTCFG),
        rd32le(USB2OTG_HOSTFRAMEINTERV),
        rd32le(USB2OTG_HOSTPORT),
        rd32le(USB2OTG_HOSTFRAMENO),
        rd32le(USB2OTG_HOSTINTRMASK));)
    D(bug("[USB2OTG:CORE %s] RXF=%08x NPTXF=%08x PTXF=%08x NPTXSTS=%08x HPTXSTS=%08x\n",
        tag,
        rd32le(USB2OTG_RCVSIZE),
        rd32le(USB2OTG_NONPERIFIFOSIZE),
        rd32le(USB2OTG_PERIFIFOSIZE),
        rd32le(USB2OTG_NONPERIFIFOSTATUS),
        rd32le(USB2OTG_HOSTFIFOSTATUS));)
    (void)tag;
}

static inline void usb2otg_hostport_rmw(ULONG set, ULONG clear)
{
    ULONG v = rd32le(USB2OTG_HOSTPORT);
    v &= ~USB2OTG_HOSTPORT_SC_BITS;
    v &= ~clear;
    v |= set;
    wr32le(USB2OTG_HOSTPORT, v);
}

/*
 * TRUE if any host channel has CHENA set. TxFIFO flush is only legal
 * with AHB idle and no active channels (DWC2 programming guide);
 * flushing with a channel enabled corrupts the core's request-queue/
 * FIFO pointers — armed channels then never execute (CHENA stuck,
 * INTR=0, TSIZE untouched) and their halt never completes.
 */
static inline BOOL usb2otg_any_channel_enabled(void)
{
    int chan;

    for (chan = 0; chan < 8; chan++)
    {
        if (rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE)
            return TRUE;
    }
    return FALSE;
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
        UBYTE               hc_CsplitRetry;    /* CSPLIT NYET retries this interval; caps to TT result window */
        UBYTE               hc_SplitState;     /* periodic-split SM: USB2OTG_SPLIT_{IDLE,SS,CS} */
        UWORD               hc_SplitSSUframe;  /* HFNUM&0x3fff uframe the SSPLIT was issued in */
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
        UBYTE               hc_QuietIdleStreak; /* consecutive WAIT-QUIET defers with NPTX queue+FIFO fully idle (wedge signature) */
        UBYTE               hc_OddfrmFlip;    /* invert ODDFRM choice for direct periodic; toggled on deschedule (self-calibrating) */
        ULONG               hc_LastSampleHfnum; /* sample snapshot for WAIT-QUIET liveness check */
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
    UBYTE               hu_PortRecoverState;    /* post-core-reset port bring-up SM (watchdog-driven) */
    UWORD               hu_PortRecoverTick;     /* ticks waited for reconnect in state 1 */
    BOOL                hu_PortForceConnChange; /* synthetic connect-change: recovery PRTRST W1Cs the HW bit */
    BOOL                hu_PortForceDisconnect; /* recovery phase 1: report port as disconnected so the
                                                 * hub class tears the stale tree down before the reconnect */
    ULONG               hu_BootGusbCfg;         /* known-good GUSBCFG captured at boot; a power-cycled
                                                 * core reads back power-on defaults, not the config */

    /*
     * Driver-internal Clear_TT_Buffer request. Statically embedded (no
     * allocation in IRQ context) and single-in-flight; TermIO
     * recognises it by address and must not ReplyMsg it.
     */
    struct IOUsbHWReq   hu_TTClearReq;
    BOOL                hu_TTClearBusy;

    APTR                hu_USB2OTGBase;

    ULONG               hu_XferSizeWidth;
    ULONG               hu_PktSizeWidth;

    ULONG               hu_PIDBits[128];        /* PID 2-bit pairs, one ULONG per device, each ULONG contains 2-bits for every endpoint */

    /*
     * Per-(device, direction) bulk NAK rate gate ([0]=OUT, [1]=IN).
     * Stores the earliest microframe (14-bit HOSTFRAMENO) at which the
     * next bulk retry may arm; 0xFFFF = no gate. Mirrors FreeBSD
     * dwc_otg's did_nak rate-check (dwc_otg.c:1215-1250). Stops
     * NAK-storm CPU0 burn. Split per direction: an idle bulk-IN poll
     * NAKs constantly, and letting that gate the whole device throttled
     * the same device's OUT endpoint to ~1 frame per gate period
     * (lan78xx TX starvation, 100-200 Kbit/s).
     */
    UWORD               hu_NakGate[128][2];

    /*
     * Per-device bulk channel binding: [0]→CHAN_BULK, [1]→CHAN_BULK2.
     * 0 = free. Partitions two concurrent bulk devices onto a channel
     * each so a wedged/NAK-storming device cannot block the other.
     */
    UBYTE               hu_BulkOwnerDev[2];

    /*
     * Consecutive bulk bare-CHHLTD giveups (UHIOERR_TIMEOUT) per
     * device with no progress. An unplugged device answers every
     * transaction with bare CHHLTD; the class keeps retrying the CBW,
     * and each attempt would otherwise churn 250 no-progress rounds.
     * At >= 2 the giveup threshold drops to 25 (fast fail) so the bus
     * frees up for the hub's disconnect report. Reset on any transfer
     * completion for the device.
     */
    UBYTE               hu_BulkGiveupStreak[128];

    /*
     * Bitmask of quarantined channels whose CHENA is stuck (halt +
     * HCLKSOFT + core reset all failed). Schedulers skip these so new
     * requests aren't armed on dead hardware. Cleared after a
     * successful wedge-recovery reset (all channel SMs reset).
     */
    UBYTE               hu_DeadChannels;

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

void                    usb2otg_clear_delayed_channel(int chan);

/* INT-pipe activity counters (usb2otg_intr.c) — hotplug diagnostics. */
extern ULONG usb2otg_int_poll_count[8];
extern ULONG usb2otg_int_comp_count[8];
extern ULONG usb2otg_int_arm_hfnum[8];
extern ULONG usb2otg_int_arm_char[8];
extern ULONG usb2otg_int_hlt_hfnum[8];

/*
 * Quarantine/blackout instrumentation (usb2otg_intr.c). wd_ticks is
 * the 150 ms watchdog tick counter; quar_set_tick records when each
 * channel was quarantined so releases can log the dark time.
 */
extern ULONG usb2otg_wd_ticks;
extern ULONG usb2otg_quar_set_tick[8];

/* Control-pipe lifecycle counters (usb2otg_intr.c). */
extern ULONG usb2otg_ctrl_arm_count;
extern ULONG usb2otg_ctrl_fin_count;
extern ULONG usb2otg_ctrl_err_count;
extern UBYTE usb2otg_ctrl_last_err;

/* Control failure-mode counters: which requeue path ctrl reqs cycle through. */
extern ULONG usb2otg_ctrl_nak_requeues;   /* split-NAK path */
extern ULONG usb2otg_ctrl_chh_requeues;   /* bare-CHHLTD path */
extern ULONG usb2otg_ctrl_xact_retries;   /* XactErr/DTErr/BNA path */

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
 * INT channel partition: split transfers on CHAN_INT1..INT3, direct
 * (non-split) on CHAN_INT4..INT5. Sharing one channel between
 * periodic-split and direct arms made the core kill every direct arm
 * with bare CHHLTD in the arming uframe (hub status polls dead from
 * the moment a LS/FS device enumerated — hotplug blind). Observed on
 * Pi 3B+; suspicion is stale per-channel periodic-split state.
 */
#define CHAN_INT_DIRECT_FIRST   CHAN_INT4

/*
 * Split-control channel. A halted/failed split sequence poisons its
 * channel: every subsequent NON-split arm on it bare-CHHLTDs in the
 * arming uframe (splits keep working). Observed twice on Pi 3B+:
 * chan 2 (split-INT vs hub polls) and CHAN_CTRL (dead-device split
 * ctrl vs live-device ctrl — froze ALL control traffic after unplug).
 * So control to LS/FS devices behind a TT gets its own channel,
 * taken out of the split-INT pool (splits: INT1-2, split-ctrl: INT3,
 * direct INT: INT4-5).
 */
#define CHAN_CTRL_SPLIT         CHAN_INT3


/* Periodic-split sequencer state (hc_SplitState). */
#define USB2OTG_SPLIT_IDLE  0   /* not a split, or split sequence finished */
#define USB2OTG_SPLIT_SS    1   /* SSPLIT issued, awaiting ACK */
#define USB2OTG_SPLIT_CS    2   /* CSPLIT issued/pending, awaiting completion */

/*
 * Testing hook: force one power-cycle recovery at this watchdog tick
 * (150 ms units, 0 = disabled). Lets QEMU exercise the recovery path
 * without a hotplug-induced give-up.
 */
#define USB2OTG_DEBUG_FORCE_RECOVER_TICK 0

/*
 * Core-reset/power-cycle recovery on split-ctrl give-up. DISABLED —
 * final verdict after exhaustive testing: no runtime reinit revives
 * the core. After CSftRst (or mailbox power-cycle + reinit with the
 * boot GUSBCFG snapshot, TATTDB-debounced port bring-up and two-phase
 * synthetic replug — every earlier recovery bug fixed and the whole
 * chain QEMU-verified), the register state is bit-identical to the
 * boot baseline yet the core executes ZERO real transactions, at any
 * uframe (mid-frame arms verified via post-gate armHFN). The delta vs
 * cold boot is invisible to software (suspects: firmware no-opping
 * SETPOWER OFF for the USB block since it powers the LAN chip, or
 * PHY/ULPI state outside the register file). Without recovery the bus
 * and ethernet SURVIVE a failed replug; with it everything dies —
 * so off. Known limitation: after unplugging a LS/FS device behind
 * the TT, the core's non-periodic split engine stays wedged until
 * reboot (re-enumeration of that device fails; HS devices and all
 * existing traffic are unaffected).
 */
#define USB2OTG_ENABLE_CORE_RESET_RECOVERY 0

/*
 * Hub class request Clear_TT_Buffer (USB 2.0 §11.24.2.3, Table 11-16).
 * Not in devices/usb.h. wValue = EP_Num[3:0] | DevAddr[10:4] |
 * EP_Type[12:11] | Dir[15]; wIndex = TT port.
 */
#define USR_CLEAR_TT_BUFFER   0x08
#define USB2OTG_TT_EPTYPE_CTRL 0
#define USB2OTG_TT_EPTYPE_ISO  1
#define USB2OTG_TT_EPTYPE_BULK 2
#define USB2OTG_TT_EPTYPE_INT  3

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

/*
 * Consecutive watchdog-wedge retries for an INT request before it is
 * failed with UHIOERR_TIMEOUT. A timed-out interrupt pipe makes the
 * class drivers (hid/hub) treat the device as unplugged, so wedged
 * INT polls are requeued instead; counter lives in iouh_DriverPrivate2
 * and resets on any liveness (completion/NAK).
 */
#define USB2OTG_INT_WEDGE_RETRY_LIMIT  3

/* Bulk NAK rate gate: 8 microframes = 1 ms at HS. */
#define USB2OTG_NAK_GATE_NONE          0xFFFFU
#define USB2OTG_BULK_NAK_GATE_UFRAMES  8

/*
 * Flash-busy defer cap (WAIT-QUIET). Each cycle is 20 ticks × 150 ms
 * = 3 s. Cap × 3 s = total tolerated NAK-storm time. The halt path
 * triggered when the cap hits can leave Pi 3B+ in an unrecoverable
 * CHENA+CHDIS state (neither NPTXFIFO flush nor HCLKSOFT clears it),
 * so set high (200 × 3 s = 10 min) to tolerate cheap-stick FTL GC
 * pauses.
 */
#define USB2OTG_FLASH_BUSY_DEFER_CAP   200

/*
 * Per-request retry budget shared across transient errors (XactErr,
 * DataTglErr, BNA — USB 2.0 §8.4.6). Counter in iouh_DriverPrivate2;
 * resets on XFERCOMPL/NAK/NYET. Matches FreeBSD dwc_otg errcnt.
 */
#define USB2OTG_TRANSIENT_RETRY_LIMIT  3

/*
 * Split-control NAK give-up budget. A CSPLIT to a dead port behind a
 * TT is NAKed by the HUB forever (the hub is alive, the device is
 * not); without a cap the retry loop runs at ~1 kHz and starves all
 * other control traffic. Counter in iouh_DriverPrivate2, reset on
 * XFERCOMPL. 64 retries x backoff ≈ 0.3 s per attempt before
 * UHIOERR_TIMEOUT, which feeds Poseidon's pd_DeadCount removal path.
 */
#define USB2OTG_CTRL_SPLIT_NAK_LIMIT      64

/*
 * Control retry backoff in frames (ms). Encoded into DriverPrivate1
 * as 0x80000000 | earliest_frame; ScheduleCtrlTDs defers the request
 * until the frame counter passes it (wall time, unlike the legacy
 * call-count delay which a Cause() storm burns through instantly).
 */
#define USB2OTG_CTRL_RETRY_BACKOFF_FRAMES 4
#define USB2OTG_CTRL_BACKOFF_FLAG         0x80000000UL

/*
 * Encode a ctrl retry backoff: flag | parked wd-tick byte [19:12] |
 * earliest frame [10:0]. The tick byte is a failsafe — if the frame
 * counter stops (port disabled after a core reset => no SOF), the
 * frame comparison alone would park the request forever; the
 * watchdog tick keeps counting regardless.
 */
static inline APTR usb2otg_ctrl_backoff(ULONG frnm)
{
    return (APTR)(IPTR)(USB2OTG_CTRL_BACKOFF_FLAG |
        ((usb2otg_wd_ticks & 0xff) << 12) |
        ((frnm + USB2OTG_CTRL_RETRY_BACKOFF_FRAMES) & 0x7ff));
}

/*
 * Clamp an INT scheduling interval to the 11-bit frame window.
 * hub.class submits 2048, which aliases to +0 mod 2048 in the DP1
 * frame math — i.e. poll every single frame.
 */
static inline ULONG usb2otg_clamp_interval(ULONG interval)
{
    if (interval == 0)
        interval = 1;
    if (interval > 1024)
        interval = 1024;
    return interval;
}

static inline ULONG usb2otg_current_uframe(void)
{
    /* HFNUM[13:0] FRNUM: 14-bit microframe counter, wraps every 2.048 s. */
    return rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff;
}

/* Gate-direction index for a request: [0]=OUT, [1]=IN. */
static inline UBYTE usb2otg_gate_dir(struct IOUsbHWReq *req)
{
    return (req->iouh_Dir == UHDIR_IN) ? 1 : 0;
}

static inline BOOL usb2otg_nak_gated(struct USB2OTGUnit *u, UBYTE dev, UBYTE dir)
{
    UWORD gate = u->hu_NakGate[dev & 0x7f][dir & 1];
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
                    bug("[USB2OTG:GATE] cleared dev=%d dir=%d gate=%04x now=%04lx delta=%lu (#%lu)\n",
                        (int)dev, (int)dir, (unsigned)gate, (unsigned long)now,
                        (unsigned long)delta, (unsigned long)cleared);
            }
        )
        u->hu_NakGate[dev & 0x7f][dir & 1] = USB2OTG_NAK_GATE_NONE;
        return FALSE;
    }
    return TRUE;
}

static inline void usb2otg_nak_gate_set(struct USB2OTGUnit *u, UBYTE dev,
    UBYTE dir, ULONG uframes)
{
    UWORD new_gate = (UWORD)((usb2otg_current_uframe() + uframes) & 0x3fff);
    u->hu_NakGate[dev & 0x7f][dir & 1] = new_gate;
    D(
        if (uframes >= 100)
        {
            bug("[USB2OTG:GATE] set dev=%d dir=%d gate=%04x uframes=%lu now=%04lx\n",
                (int)dev, (int)dir, (unsigned)new_gate, (unsigned long)uframes,
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

/*
 * Queue a Clear_TT_Buffer to req's parent hub (USB 2.0 §11.17.5): a
 * split transaction that is abandoned mid-flight — which is exactly
 * what happens when a LS/FS device is unplugged and we force-halt its
 * SSPLIT — leaves the TT buffer holding the orphaned transaction.
 * Until it is cleared, further splits through that TT are refused
 * (bare CHHLTD, no bus activity) while direct HS traffic to the hub
 * keeps working. Nothing else in the stack issues this request: the
 * hub class has no TT handling at all.
 *
 * Best effort: single request in flight, dropped if one is pending.
 * Safe from IRQ context (queue + Cause, like the other requeue paths).
 * Caller must hold hu_Lock.
 */
static inline void usb2otg_queue_clear_tt_buffer(struct USB2OTGUnit *unit,
    struct IOUsbHWReq *req)
{
    struct IOUsbHWReq *tt = &unit->hu_TTClearReq;
    UWORD eptype;
    UWORD wValue;

    if (!(req->iouh_Flags & UHFF_SPLITTRANS) || req->iouh_SplitHubAddr == 0)
        return;
    /* Our emulated root hub has no TT — it would only STALL. */
    if (req->iouh_SplitHubAddr == unit->hu_HubAddr)
        return;
    if (unit->hu_TTClearBusy)
        return;

    switch (req->iouh_Req.io_Command)
    {
        case UHCMD_CONTROLXFER: eptype = USB2OTG_TT_EPTYPE_CTRL; break;
        case UHCMD_ISOXFER:     eptype = USB2OTG_TT_EPTYPE_ISO;  break;
        case UHCMD_BULKXFER:    eptype = USB2OTG_TT_EPTYPE_BULK; break;
        case UHCMD_INTXFER:     eptype = USB2OTG_TT_EPTYPE_INT;  break;
        default:                return;
    }

    wValue = (UWORD)((req->iouh_Endpoint & 0x0f)
                     | ((req->iouh_DevAddr & 0x7f) << 4)
                     | ((eptype & 3) << 11));
    /* Control endpoints are bidirectional — spec uses the IN bit only
     * for the direction the buffer was holding; OUT (0) for SETUP. */
    if (req->iouh_Req.io_Command != UHCMD_CONTROLXFER &&
        req->iouh_Dir == UHDIR_IN)
        wValue |= (1 << 15);

    memset(tt, 0, sizeof(*tt));
    tt->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    tt->iouh_Req.io_Message.mn_Length = sizeof(*tt);
    tt->iouh_Req.io_Command = UHCMD_CONTROLXFER;
    /* IOF_QUICK so TermIO does not ReplyMsg a request nobody sent. */
    tt->iouh_Req.io_Flags = IOF_QUICK;
    tt->iouh_Req.io_Unit = (struct Unit *)unit;
    tt->iouh_DevAddr = req->iouh_SplitHubAddr;   /* the hub, always HS */
    tt->iouh_Endpoint = 0;
    tt->iouh_MaxPktSize = 64;
    tt->iouh_Dir = UHDIR_OUT;
    tt->iouh_Length = 0;
    tt->iouh_Data = NULL;
    tt->iouh_SetupData.bmRequestType = URTF_OUT | URTF_CLASS | URTF_OTHER;
    tt->iouh_SetupData.bRequest = USR_CLEAR_TT_BUFFER;
    tt->iouh_SetupData.wValue = AROS_WORD2LE(wValue);
    tt->iouh_SetupData.wIndex = AROS_WORD2LE(req->iouh_SplitHubPort);
    tt->iouh_SetupData.wLength = 0;

    unit->hu_TTClearBusy = TRUE;
    D(bug("[USB2OTG:TTCLEAR] queueing hub=%d port=%d for dev=%d ep=%d type=%d (wValue=%04x)\n",
        (int)req->iouh_SplitHubAddr, (int)req->iouh_SplitHubPort,
        (int)req->iouh_DevAddr, (int)req->iouh_Endpoint,
        (int)eptype, (unsigned)wValue);)
    ADDHEAD(&unit->hu_CtrlXFerQueue, (struct Node *)tt);
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

            bug("[USB2OTG:DIAG] bulk-finish chan=%d dev=%d ep=%d dir=%s act=%lu/%lu err=%d rq=%u np=%u age=%lu last_intr=%04x split=%u\n",
                chan,
                (int)req->iouh_DevAddr,
                (int)req->iouh_Endpoint,
                req->iouh_Dir == UHDIR_IN ? "IN" : "OUT",
                (unsigned long)req->iouh_Actual,
                (unsigned long)req->iouh_Length,
                (int)req->iouh_Req.io_Error,
                (unsigned int)hc->hc_DiagRequeueCount,
                (unsigned int)hc->hc_DiagNoProgressCount,
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
