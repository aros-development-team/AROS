/*
    Copyright 2025, The AROS Development Team. All rights reserved.

    VC4 Gallium 3D HIDD - V3D hardware initialization and control
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <hardware/bcm2708.h>

#include "vc4gallium_intern.h"
#include "vc4_v3d.h"

/* V3D interrupt sources delivered as real IRQs. Determined on
 * hardware: arming ANY V3D interrupt kills the firmware property
 * mailbox after the first frame's burst (ALLOCMEM MBoxCall timeouts
 * from the second submit on), even though the ARM side services
 * perfectly, and the ENABLE_QPU (0x00030012) "ARM owns V3D" handshake
 * does NOT help. The firmware co-listens on the V3D interrupt line (it
 * has its own V3D driver); the assertion or our INTCTL W1C wedges its
 * mailbox task. Unlike the PV2 vsync line, which the firmware ignores.
 * So: keep 0 — poll servicing in the wait paths (INTCTL latches status
 * even while masked) is the permanent design as long as the firmware
 * owns the mailbox/property services. */
#define VC4_V3D_IRQ_ENA 0

/* The interrupt handler and the (render_lock-serialized) task-side
 * poll paths share the service state machine (pending_render,
 * overflow_handed, CT1 kicks, W1C of INTCTL). This is a UP-exec build
 * (__AROSEXEC_SMP__ unset), so Disable()/Enable() around the shared
 * sections is complete protection. */

void vc4_v3d_kick_pending_render(struct vc4_v3d_state *v3d, const char *reason)
{
    static ULONG kicklog = 0;

    Disable();

    if (!v3d->pending_render)
    {
        Enable();
        return;
    }

    /* NEVER write CT1CA while the previous render is still running: the
     * executor ignores CA writes while CTRUN is set, and since pool slots
     * alternate, the new EA can equal the running job's EA — the frame is
     * then silently dropped (renderer stops at CA==EA, the bin semaphore
     * stays pending, RFC never advances). Defer: the poll paths call this
     * again, and once the previous frame's FRDONE lands the kick goes
     * through: the next render job is only ever submitted from the
     * render-done path. */
    if (V3D_READ(v3d, V3D_CT1CS) & V3D_CTCS_CTRUN)
    {
        static ULONG deferlog = 0;
        if (deferlog < 8)
        {
            deferlog++;
            bug("[VC4Gallium] kick deferred (%s): CT1 busy, CT1CA=0x%08x\n",
                reason, V3D_READ(v3d, V3D_CT1CA));
        }
        Enable();
        return;
    }

    /* Hang forensics: log the first kicks with full pre-kick state so the
     * FLDONE-vs-BFC handoff timeline can be reconstructed from serial. */
    if (kicklog < 16)
    {
        kicklog++;
        bug("[VC4Gallium] kick CT1 (%s): CT1CS=0x%08x CA=0x%08x -> 0x%08x "
            "EA -> 0x%08x BFC=%d RFC=%d INTCTL=0x%08x PCS=0x%08x\n",
            reason, V3D_READ(v3d, V3D_CT1CS), V3D_READ(v3d, V3D_CT1CA),
            v3d->pending_ct1ca, v3d->pending_ct1ea,
            V3D_READ(v3d, V3D_BFC) & 0xff, V3D_READ(v3d, V3D_RFC) & 0xff,
            V3D_READ(v3d, V3D_INTCTL), V3D_READ(v3d, V3D_PCS));
    }

    v3d->pending_render = FALSE;
    v3d->kick_count++;
    V3D_WRITE(v3d, V3D_CT1CA, v3d->pending_ct1ca);
    V3D_WRITE(v3d, V3D_CT1EA, v3d->pending_ct1ea);

    Enable();
}

/*
 * Sample BFC/RFC and advance the software completion counters by the
 * (mod-256) delta since the last sample.
 *
 * Under Disable(), like the rest of the shared V3D state: this is a
 * read-modify-write of last_bfc/last_rfc and the accumulators, and both
 * wait loops plus the interrupt path reach it. Losing one update leaves
 * rfc_completed permanently short of the hardware, and since the waited-for
 * seqno can then never be reached, every later frame waits out the full
 * timeout (seen as want=N fin=N-1 with the GPU idle and BFC==RFC).
 */
void vc4_v3d_advance_counters(struct vc4_v3d_state *v3d)
{
    ULONG bfc, rfc, done;

    Disable();

    bfc = V3D_READ(v3d, V3D_BFC) & 0xff;
    rfc = V3D_READ(v3d, V3D_RFC) & 0xff;
    v3d->bfc_completed += (bfc - v3d->last_bfc) & 0xff;
    v3d->rfc_completed += (rfc - v3d->last_rfc) & 0xff;
    v3d->last_bfc = bfc;
    v3d->last_rfc = rfc;

    if (v3d->pending_render && v3d->bfc_completed >= v3d->seqno)
        vc4_v3d_kick_pending_render(v3d, "BFC");

    /*
     * Resync when the hardware says the work is done but the accumulator
     * disagrees: nothing left to kick, both control threads idle and the
     * render count equal to the submission being waited for means every
     * render has retired, whatever the accumulator may have lost. Without
     * this the skew is permanent — the counters only ever move by deltas.
     */
    if ((v3d->rfc_completed < v3d->seqno) && !v3d->pending_render &&
        (rfc == (v3d->seqno & 0xff)) &&
        !(V3D_READ(v3d, V3D_CT0CS) & V3D_CTCS_CTRUN) &&
        !(V3D_READ(v3d, V3D_CT1CS) & V3D_CTCS_CTRUN))
    {
        static ULONG resynclog = 0;

        if (resynclog < 8)
        {
            resynclog++;
            bug("[VC4Gallium] counter resync: seqno=%d rfc_completed=%d "
                "(RFC=%d) — GPU idle, adopting seqno\n",
                v3d->seqno, v3d->rfc_completed, rfc);
        }
        v3d->bfc_completed = v3d->seqno;
        v3d->rfc_completed = v3d->seqno;
    }

    done = v3d->rfc_completed;
    if (done > v3d->seqno)
        done = v3d->seqno;
    v3d->finished_seqno = done;

    Enable();
}

/* Interrupt service helper. Three jobs:
 *  - OUTOMEM: binner ran out of pool memory; we count + log and feed
 *    the per-frame overspill BO.
 *  - FLDONE: binner finished its FLUSH — trigger for kicking CT1 (the
 *    renderer) with the RCL addresses submit_cl stashed in
 *    pending_ct1{ca,ea}.
 *  - FRDONE: renderer finished; we count, the wait loop picks up the RFC.
 * Called from both the IRQ handler and the polling wait paths (IRQ_VC_3D
 * isn't delivered reliably on all RPi setups yet). Must stay IRQ-safe: no
 * allocations, no signals. */
void vc4_v3d_service_interrupts(struct vc4_v3d_state *v3d)
{
    ULONG ints;

    Disable();
    ints = V3D_READ(v3d, V3D_INTCTL);

    /* Check the level condition (PCS.BMOOM) as well as the INTCTL latch:
     * the latch is edge-triggered and may already have been W1C-cleared
     * while the binner is still stalled waiting for memory. */
    if ((ints & V3D_INT_OUTOMEM) || (V3D_READ(v3d, V3D_PCS) & V3D_PCS_BMOOM))
    {
        v3d->int_outomem++;
        /* Feed the binner the per-frame overspill BO (BPOA/BPOS).
         * Once per submission: if the binner
         * exhausts the overspill too, let it stall — v3d_wait_seqno times
         * out, dumps the hang state and recovers. */
        if (v3d->overflow_handed == 0 && v3d->overflow_bus != 0)
        {
            v3d->overflow_handed = 1;
            V3D_WRITE(v3d, V3D_BPOA, v3d->overflow_bus);
            V3D_WRITE(v3d, V3D_BPOS, v3d->overflow_size);
            if (v3d->int_outomem <= 8)
                bug("[VC4Gallium] OUTOMEM: fed overspill BO (0x%08x, %dKB) PCS=0x%08x "
                    "INTCTL=0x%08x CT0CA=0x%08x BFC=%d RFC=%d\n",
                    v3d->overflow_bus, v3d->overflow_size >> 10,
                    V3D_READ(v3d, V3D_PCS), ints, V3D_READ(v3d, V3D_CT0CA),
                    V3D_READ(v3d, V3D_BFC) & 0xff, V3D_READ(v3d, V3D_RFC) & 0xff);
        }
        else if (v3d->overflow_handed == 1)
        {
            v3d->overflow_handed = 2;
            bug("[VC4Gallium] OUTOMEM: overspill exhausted (PCS=0x%08x) — binner stalls\n",
                V3D_READ(v3d, V3D_PCS));
        }
    }
    if (ints & V3D_INT_FLDONE)
    {
        v3d->int_fldone++;
        if (v3d->int_fldone <= 8)
            bug("[VC4Gallium] FLDONE seen: BFC=%d RFC=%d PCS=0x%08x\n",
                V3D_READ(v3d, V3D_BFC) & 0xff, V3D_READ(v3d, V3D_RFC) & 0xff,
                V3D_READ(v3d, V3D_PCS));
        vc4_v3d_kick_pending_render(v3d, "FLDONE");
    }
    if (ints & V3D_INT_FRDONE)
    {
        v3d->int_frdone++;
        if (v3d->int_frdone <= 8)
            bug("[VC4Gallium] FRDONE seen: BFC=%d RFC=%d\n",
                V3D_READ(v3d, V3D_BFC) & 0xff, V3D_READ(v3d, V3D_RFC) & 0xff);
    }

    /* W1C: writing 1 to a bit in INTCTL clears that pending interrupt. */
    if (ints)
        V3D_WRITE(v3d, V3D_INTCTL, ints);

    Enable();
}

static void v3d_irq_handler(struct vc4_v3d_state *v3d, struct ExecBase *sysBase)
{
    /* Record what reached the ARM via a real interrupt BEFORE
     * service_interrupts() W1C-clears INTCTL. Bumped only here (never the
     * poll path), so if they climb the IRQ genuinely fires. No printing —
     * IRQ context. */
    ULONG ints = V3D_READ(v3d, V3D_INTCTL);
    v3d->irq_calls++;
    if (ints & V3D_INT_FLDONE)  v3d->irq_fldone++;
    if (ints & V3D_INT_FRDONE)  v3d->irq_frdone++;
    if (ints & V3D_INT_OUTOMEM) v3d->irq_outomem++;

    vc4_v3d_service_interrupts(v3d);
}

BOOL vc4_v3d_init(struct vc4_v3d_state *v3d)
{
    /* Peripheral regs are identity-mapped at ARM_PERIIOBASE on RPi, so
     * V3D_BASE is directly accessible. */
    v3d->v3d_regs = (volatile ULONG *)V3D_BASE;

    v3d->ident0 = V3D_READ(v3d, V3D_IDENT0);
    v3d->ident1 = V3D_READ(v3d, V3D_IDENT1);
    v3d->ident2 = V3D_READ(v3d, V3D_IDENT2);

    /* IDENT0 magic: a valid V3D block returns 0x02443356 ("V3D" + version 2),
     * top byte is the version. */
    v3d->v3d_ver = (v3d->ident0 >> V3D_IDENT0_VER_SHIFT) & 0xFF;

    if (v3d->ident0 == 0 || v3d->ident0 == 0xFFFFFFFF)
    {
        bug("[VC4Gallium] V3D not found (IDENT0=0x%08x)\n", v3d->ident0);
        v3d->v3d_available = FALSE;
        return FALSE;
    }

    /* Check that this is V3D version 2.x (VideoCore IV) */
    if (v3d->v3d_ver < 1 || v3d->v3d_ver > 3)
    {
        bug("[VC4Gallium] Unexpected V3D version %d (IDENT0=0x%08x)\n",
            v3d->v3d_ver, v3d->ident0);
        v3d->v3d_available = FALSE;
        return FALSE;
    }

    v3d->seqno = 0;
    v3d->finished_seqno = 0;
    v3d->v3d_available = TRUE;

    D(bug("[VC4Gallium] driver build " __DATE__ " " __TIME__ "\n"));

    D(bug("[VC4Gallium] V3D version %d detected\n", v3d->v3d_ver));
    D(bug("[VC4Gallium]   IDENT0=0x%08x IDENT1=0x%08x IDENT2=0x%08x\n",
        v3d->ident0, v3d->ident1, v3d->ident2));

    V3D_WRITE(v3d, V3D_L2CACTL, V3D_L2CACTL_L2CENA);

    /* Reset both control list executors FIRST, to clear any stale binner state
     * (notably a BMOOM left in PCS by the firmware/boot) before we enable any
     * interrupt. Without this, enabling OUTOMEM fires instantly on the stale
     * condition, and the firmware — which co-owns the V3D IRQ in this
     * firmware-framebuffer setup — chokes on the shared, never-cleared
     * condition and stops servicing the property mailbox. */
    V3D_WRITE(v3d, V3D_CT0CS, V3D_CTCS_CTRSTA);
    V3D_WRITE(v3d, V3D_CT1CS, V3D_CTCS_CTRSTA);

    /* Clear any pending interrupts */
    V3D_WRITE(v3d, V3D_INTCTL, V3D_INT_FLDONE | V3D_INT_FRDONE | V3D_INT_OUTOMEM);

    /* Install the IRQ handler (always) and reset the service state:
     *  - FLDONE: binner finished its FLUSH. Service kicks CT1 with the
     *    pending_ct1{ca,ea} that submit_cl stashed.
     *  - FRDONE: render frame done; counted, waits poll RFC.
     *  - OUTOMEM: binner out of pool memory; service feeds the
     *    per-frame overspill BO (BPOA/BPOS). */
    v3d->int_outomem = 0;
    v3d->int_fldone  = 0;
    v3d->int_frdone  = 0;
    v3d->irq_calls   = 0;
    v3d->kick_count  = 0;
    v3d->irq_fldone  = 0;
    v3d->irq_frdone  = 0;
    v3d->irq_outomem = 0;
    v3d->pending_render = FALSE;
    v3d->pending_ct1ca = 0;
    v3d->pending_ct1ea = 0;
    v3d->overflow_bus = 0;
    v3d->overflow_size = 0;
    v3d->overflow_handed = 0;
    v3d->irq_handle = KrnAddIRQHandler(IRQ_VC_3D, v3d_irq_handler, v3d, SysBase);
    if (!v3d->irq_handle)
        bug("[VC4Gallium] Failed to install V3D IRQ handler\n");
    /* Mask everything, then arm the bring-up set (see VC4_V3D_IRQ_ENA).
     * The wait paths keep polling V3D_INTCTL as fallback; the IRQ path
     * merely services the same state machine sooner (immediate
     * FLDONE->CT1 kick instead of waiting for the next poll). */
    V3D_WRITE(v3d, V3D_INTDIS, V3D_INT_OUTOMEM | V3D_INT_FLDONE | V3D_INT_FRDONE);
    if (VC4_V3D_IRQ_ENA && v3d->irq_handle)
    {
        V3D_WRITE(v3d, V3D_INTENA, VC4_V3D_IRQ_ENA);
        bug("[VC4Gallium] V3D IRQs armed: 0x%02x (OUTOMEM %s, poll fallback on)\n",
            (ULONG)VC4_V3D_IRQ_ENA,
            (VC4_V3D_IRQ_ENA & V3D_INT_OUTOMEM) ? "irq" : "poll-fed");
    }
    else
        bug("[VC4Gallium] V3D IRQs masked, poll-only\n");

    /* VPM allocator state. Hardware init only needs VPMBASE written;
     * VPACNTL must be left at its reset value — the HW reset defaults for
     * the per-thread row counts (VPMURS_VS/CS/US in VPACNTL) are non-zero.
     * Writing VPACNTL=0 zeros those counts and turns every subsequent
     * VS/CS VPM allocation into a VPAERGL (ERRSTAT bit 12) error. Log the
     * reset value for the record but leave the register alone. */
    V3D_WRITE(v3d, V3D_VPMBASE, 0);
    D(bug("[VC4Gallium] V3D VPACNTL reset default = 0x%08x\n",
        V3D_READ(v3d, V3D_VPACNTL)));

    /* QPU scheduler reservation. SQRSV0/SQRSV1 hold a 4-bit-per-QPU field
     * for which workloads (user program / FS / VS / CS) each QPU may run.
     * The Pi firmware can leave stale bits set: if QPUs end up reserved for
     * the user-program queue only, the renderer stalls at the first
     * primitive (no QPU free for FS/VS/CS). Clear so all QPUs run anything. */
    D(bug("[VC4Gallium] V3D SQRSV0/SQRSV1/SQCNTL reset = 0x%08x 0x%08x 0x%08x\n",
        V3D_READ(v3d, V3D_SQRSV0),
        V3D_READ(v3d, V3D_SQRSV1),
        V3D_READ(v3d, V3D_SQCNTL)));
    V3D_WRITE(v3d, V3D_SQRSV0, 0);
    V3D_WRITE(v3d, V3D_SQRSV1, 0);

    /* Clear any latched error bits (W1C). */
    V3D_WRITE(v3d, V3D_ERRSTAT, 0xffffffff);

    /* Snapshot frame counters so future progress is relative. */
    v3d->last_bfc = V3D_READ(v3d, V3D_BFC) & 0xff;
    v3d->last_rfc = V3D_READ(v3d, V3D_RFC) & 0xff;
    v3d->bfc_completed = 0;
    v3d->rfc_completed = 0;

    return TRUE;
}
