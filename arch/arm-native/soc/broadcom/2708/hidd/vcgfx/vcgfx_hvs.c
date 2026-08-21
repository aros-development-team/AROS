/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 Gfx Hidd - HVS/PixelValve state dump and
          display-list takeover probe.

    Phase-1 bring-up: dumps the HVS global/channel registers, walks and
    decodes the firmware's live display list, and prints the PixelValve
    timing registers. The decode was validated against a real Pi 3B+
    (720p50, overscan fb plane + firmware cursor plane).

    Phase-2 probe (VC4_HVS_PROBE): proves we can author a display list
    the HVS accepts, with no lasting effect — copy the live list to our
    own dlist slot, repoint the HDMI channel at the copy, verify the HVS
    latches it at the next frame, hold it for a few frames (the screen
    must stay pixel-identical), then repoint back to the firmware list.
    Any failure before the repoint aborts without writing the head; a
    failed latch is restored immediately. Worst case is a transient
    glitch that the next firmware list rebuild (cursor move, SETVOFFSET)
    self-heals. Skipped on QEMU (no HVS model).
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hardware.h"
#include "vcgfx_hvs.h"
#include "vcgfx_hvs5.h"

/* Set to 0 to silence the dump / skip the probe / leave the firmware
 * in control of the display list (kill switch) / leave the PV2 vsync
 * interrupt masked / flip without waiting for vblank latch (uncapped
 * FPS for benchmarking, at the cost of rendering into live scanout). */
#define VC4_HVS_DUMP      1
#define VC4_HVS_PROBE     0
#define VC4_HVS_TAKEOVER  1
#define VC4_HVS_VSYNC_IRQ 1
#define VC4_HVS_FLIP_SYNC 1
#define VC4_HVS_FLIPSTATS 0     /* flip cadence log, every 256th flip */

/* Where the probe puts its list copy: high in the known-readable dlist
 * RAM, clear of the firmware allocator's observed range (~word 2500). */
#define HVS_PROBE_SLOT  3840
#define HVS_PROBE_MAX   64          /* refuse lists longer than this */

/* Bounded MMIO-read spins. A 50 Hz frame is ~20 ms ~ 150k uncached
 * register reads; these give roughly half a second / two seconds. */
#define HVS_SPIN_LATCH  5000000
#define HVS_SPIN_HOLD   10000000
#define HVS_SPIN_PROBEBIT 100000    /* ~2-4 frames per probed INTEN bit */
#define HVS_SPIN_FLIP   500000      /* flip-latch wait bound, ~3 frames */

/* Every register below is VideoCore IV's; BCM2711 carries HVS5 instead.
 * Gates the entry points that reach hardware before an hvs_Active check -
 * the rest already no-ops while hvs_Active is FALSE. */
static inline BOOL hvs_hw_known(struct VideoCoreGfx_staticdata *xsd)
{
    return !xsd->vcsd_IsBCM2711;
}

static inline ULONG hvs_rd(ULONG offset)
{
    return *(volatile ULONG *)(VC4_HVS_BASE + offset);
}

static inline void hvs_wr(ULONG offset, ULONG value)
{
    *(volatile ULONG *)(VC4_HVS_BASE + offset) = value;
}

static inline ULONG pv_rd(IPTR base, ULONG offset)
{
    return *(volatile ULONG *)(base + offset);
}

static inline void pv_wr(IPTR base, ULONG offset, ULONG value)
{
    *(volatile ULONG *)(base + offset) = value;
}

static const char * const hvs_fmt_names[] =
{
    "RGB332", "RGBA4444", "RGB555", "RGBA5551", "RGB565", "RGB888",
    "RGBA6666", "RGBA8888", "YUV420-3P", "YUV420-2P", "YUV422-3P",
    "YUV422-2P", "H264", "13?", "14?", "15?"
};

/* Walk one channel's display list. Every entry is printed raw first,
 * then decoded (unity layout, validated on real HW). */
static void hvs_dump_list(ULONG head, ULONG fb_phys, ULONG fb_pitch,
                          ULONG fb_width, ULONG fb_height)
{
    ULONG idx = head;
    int entry;

    for (entry = 0; entry < 8; entry++)
    {
        ULONG ctl0, size, w;

        if (idx >= HVS_DLIST_WORDS)
        {
            bug("[VC4HVS]   dlist index %u out of range, stopping\n", idx);
            return;
        }

        ctl0 = hvs_rd(HVS_DLIST_START + 4 * idx);
        if (ctl0 & HVS_CTL0_END)
        {
            bug("[VC4HVS]   [%04u] 0x%08x END\n", idx, ctl0);
            return;
        }

        size = (ctl0 >> HVS_CTL0_SIZE_SHIFT) & HVS_CTL0_SIZE_MASK;
        if (size == 0 || idx + size > HVS_DLIST_WORDS)
        {
            bug("[VC4HVS]   [%04u] 0x%08x bad entry size %u, stopping\n",
                idx, ctl0, size);
            return;
        }

        bug("[VC4HVS]   [%04u] entry, %u words:", idx, size);
        for (w = 0; w < size && w < 16; w++)
            bug(" %08x", hvs_rd(HVS_DLIST_START + 4 * (idx + w)));
        if (size > 16)
            bug(" ...");
        bug("\n");

        bug("[VC4HVS]     fmt=%s order=%u unity=%d valid=%d\n",
            hvs_fmt_names[ctl0 & HVS_CTL0_FORMAT_MASK],
            (ctl0 >> HVS_CTL0_ORDER_SHIFT) & HVS_CTL0_ORDER_MASK,
            (ctl0 & HVS_CTL0_UNITY) ? 1 : 0,
            (ctl0 & HVS_CTL0_VALID) ? 1 : 0);

        /* Unity: POS0/POS2/POS3/PTR0/PTRCTX/PITCH0. Scaled entries
         * insert POS1 (dest size) after POS0 and append LBM/PPF words
         * after PITCH0 — observed 14-word single-plane layout has ptr0
         * at +5 and pitch at +7. */
        if (size >= HVS_UNITY_WORDS)
        {
            BOOL unity  = (ctl0 & HVS_CTL0_UNITY) != 0;
            ULONG pos0  = hvs_rd(HVS_DLIST_START + 4 * (idx + 1));
            ULONG pos2  = hvs_rd(HVS_DLIST_START + 4 * (idx + (unity ? 2 : 3)));
            ULONG ptr0  = hvs_rd(HVS_DLIST_START + 4 * (idx + (unity ? size - 3 : 5)));
            ULONG pitch = hvs_rd(HVS_DLIST_START + 4 * (idx + (unity ? size - 1 : 7)));

            if (!unity)
            {
                ULONG pos1 = hvs_rd(HVS_DLIST_START + 4 * (idx + 2));

                bug("[VC4HVS]     pos1: dest w=%u h=%u (scaled)\n",
                    pos1 & 0xfff, (pos1 >> 16) & 0xfff);
            }
            bug("[VC4HVS]     pos0: x=%u y=%u alpha=%u pos2: w=%u h=%u amode=%u\n",
                (pos0 >> HVS_POS0_X_SHIFT) & 0xfff,
                (pos0 >> HVS_POS0_Y_SHIFT) & 0xfff,
                pos0 >> HVS_POS0_ALPHA_SHIFT,
                (pos2 >> HVS_POS2_W_SHIFT) & 0xfff,
                (pos2 >> HVS_POS2_H_SHIFT) & 0xfff,
                pos2 >> HVS_POS2_AMODE_SHIFT);
            bug("[VC4HVS]     ptr0=0x%08x pitch=%u", ptr0, pitch);
            if ((ptr0 & ~HVS_PTR_BUS_ALIAS) == (fb_phys & ~HVS_PTR_BUS_ALIAS))
                bug(" <= MATCHES FBALLOC page0 (0x%08x)", fb_phys);
            if (pitch == fb_pitch)
                bug(" pitch-match");
            bug("\n");
        }

        idx += size;
    }
    bug("[VC4HVS]   (more entries, stopping after 8)\n");
}

static void pv_dump(const char *name, IPTR base)
{
    ULONG control = pv_rd(base, PV_CONTROL);
    ULONG horza   = pv_rd(base, PV_HORZA);
    ULONG horzb   = pv_rd(base, PV_HORZB);
    ULONG verta   = pv_rd(base, PV_VERTA);
    ULONG vertb   = pv_rd(base, PV_VERTB);

    bug("[VC4HVS] %s @ 0x%08x: CTRL=%08x VCTRL=%08x INTEN=%08x "
        "INTSTAT=%08x STAT=%08x%s\n",
        name, (ULONG)base, control, pv_rd(base, PV_V_CONTROL),
        pv_rd(base, PV_INTEN), pv_rd(base, PV_INTSTAT),
        pv_rd(base, PV_STAT), (control & 1) ? " enabled" : "");
    if (control & 1)
        bug("[VC4HVS] %s: hactive=%u hfp=%u hsync=%u hbp=%u "
            "vactive=%u vfp=%u vsync=%u vbp=%u\n", name,
            horzb & 0xffff, horzb >> 16, horza & 0xffff, horza >> 16,
            vertb & 0xffff, vertb >> 16, verta & 0xffff, verta >> 16);
}

#if VC4_HVS_PROBE
/* Copy the live HDMI-channel list, point the channel at the copy for a
 * few frames, then hand back. See the file header for the risk model. */
static void vc4_hvs_probe(void)
{
    ULONG buf[HVS_PROBE_MAX];
    ULONG head = 0, words = 0;
    ULONG i, lact;
    int attempt, stable = 0;

    if (!(hvs_rd(HVS_DISPCTRLX(HVS_CHANNEL_HDMI)) & HVS_DISPCTRLX_ENABLE))
    {
        bug("[VC4HVS] probe: HDMI channel not enabled, skipping\n");
        return;
    }

    /* Snapshot the live list; retry if the firmware rewrote it under
     * us (cursor move / flip lands a new list mid-copy). */
    for (attempt = 0; attempt < 3 && !stable; attempt++)
    {
        head = hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI));
        if (head == 0 || head >= HVS_DLIST_WORDS)
        {
            bug("[VC4HVS] probe: bad list head %u, aborting\n", head);
            return;
        }

        /* Copy entry by entry: the END bit is only meaningful in a
         * control word. Payload words legitimately have bit 31 set
         * (POS0 carries alpha 0xff, PTR0 bus addresses are 0xC/0xF...),
         * so a word-by-word END scan truncates the list — the first
         * probe run copied 2 of 15 words and the HVS scanned out the
         * stale junk behind the copy (white-dot noise on screen). */
        words = 0;
        while (words < HVS_PROBE_MAX)
        {
            ULONG ctl0 = hvs_rd(HVS_DLIST_START + 4 * (head + words));
            ULONG esize;

            buf[words] = ctl0;
            if (ctl0 & HVS_CTL0_END)
            {
                words++;
                break;
            }
            esize = (ctl0 >> HVS_CTL0_SIZE_SHIFT) & HVS_CTL0_SIZE_MASK;
            if (esize == 0 || words + esize >= HVS_PROBE_MAX)
            {
                bug("[VC4HVS] probe: entry size %u at +%u won't fit, aborting\n",
                    esize, words);
                return;
            }
            for (i = 1; i < esize; i++)
                buf[words + i] = hvs_rd(HVS_DLIST_START + 4 * (head + words + i));
            words += esize;
        }
        if (!(buf[words - 1] & HVS_CTL0_END))
        {
            bug("[VC4HVS] probe: no END within %u words, aborting\n",
                (ULONG)HVS_PROBE_MAX);
            return;
        }

        stable = 1;
        for (i = 0; i < words; i++)
        {
            if (hvs_rd(HVS_DLIST_START + 4 * (head + i)) != buf[i])
            {
                stable = 0;
                break;
            }
        }
    }
    if (!stable)
    {
        bug("[VC4HVS] probe: list kept changing, aborting\n");
        return;
    }

    /* Write our copy and verify the readback before repointing —
     * up to here nothing the HVS uses has been touched. */
    for (i = 0; i < words; i++)
        hvs_wr(HVS_DLIST_START + 4 * (HVS_PROBE_SLOT + i), buf[i]);
    for (i = 0; i < words; i++)
    {
        if (hvs_rd(HVS_DLIST_START + 4 * (HVS_PROBE_SLOT + i)) != buf[i])
        {
            bug("[VC4HVS] probe: dlist readback mismatch at +%u, aborting\n", i);
            return;
        }
    }

    bug("[VC4HVS] probe: %u words copied %u -> %u, repointing\n",
        words, head, (ULONG)HVS_PROBE_SLOT);
    hvs_wr(HVS_DISPLIST(HVS_CHANNEL_HDMI), HVS_PROBE_SLOT);

    for (i = 0; i < HVS_SPIN_LATCH; i++)
    {
        if (hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI)) == HVS_PROBE_SLOT)
            break;
    }
    lact = hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI));

    if (lact == HVS_PROBE_SLOT)
    {
        /* Hold our list on screen for a couple of seconds so a glitch
         * would be visible, then hand back. */
        for (i = 0; i < HVS_SPIN_HOLD; i++)
        {
            if (hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI)) != HVS_PROBE_SLOT)
                break;
        }
        bug("[VC4HVS] probe: PASS - HVS scanned out our list (LACT=%u), "
            "restoring firmware list %u\n", lact, head);
    }
    else
        bug("[VC4HVS] probe: FAIL - never latched (LACT=%u), restoring %u\n",
            lact, head);

    hvs_wr(HVS_DISPLIST(HVS_CHANNEL_HDMI), head);
    for (i = 0; i < HVS_SPIN_LATCH; i++)
    {
        if (hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI)) == head)
            break;
    }
    bug("[VC4HVS] probe: handed back, LACT=%u\n",
        hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI)));
}
#endif

/* ------------------------------------------------------------------ */
/* Phase 2: display-list ownership                                     */
/* ------------------------------------------------------------------ */

/* Compute the cursor plane entry (7 words) from the vcsd_Cur* state.
 * Returns FALSE when no plane should be shown (hidden, no shape, or
 * clipped away entirely). vcsd_CurX/Y are fb coords (hotspot
 * pre-applied); the plane position is in HVS output coords, so add the
 * fb plane offset and clip against the output edges by advancing the
 * source pointer / shrinking the source size. */
static BOOL hvs_cursor_entry(struct VideoCoreGfx_staticdata *xsd, ULONG *w)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    LONG cx, cy, cw, ch;
    ULONG pitch, ptr;

    if (!xsd->vcsd_CurVisible || !xsd->vcsd_CurBuf
        || !xsd->vcsd_CurWidth || !xsd->vcsd_CurHeight)
        return FALSE;

    /* vcsd_CurX/Y are fb coords; when the fb plane is scaled, map the
     * cursor position into output coords (the cursor plane itself
     * stays unity-sized, like the firmware cursor). */
    cx = (LONG)st->hvs_FBX
       + (LONG)(xsd->vcsd_CurX * (LONG)st->hvs_DestW) / (LONG)st->hvs_SrcW;
    cy = (LONG)st->hvs_FBY
       + (LONG)(xsd->vcsd_CurY * (LONG)st->hvs_DestH) / (LONG)st->hvs_SrcH;
    cw = xsd->vcsd_CurWidth;
    ch = xsd->vcsd_CurHeight;
    pitch = xsd->vcsd_CurWidth * 4;
    ptr = xsd->vcsd_CurBufBus;

    if (cx < 0) { ptr += (ULONG)(-cx) * 4;     cw += cx; cx = 0; }
    if (cy < 0) { ptr += (ULONG)(-cy) * pitch; ch += cy; cy = 0; }
    if (cx + cw > (LONG)st->hvs_OutW) cw = (LONG)st->hvs_OutW - cx;
    if (cy + ch > (LONG)st->hvs_OutH) ch = (LONG)st->hvs_OutH - cy;

    if (cw <= 0 || ch <= 0)
        return FALSE;

    w[0] = HVS_CTL0_XRGB_UNITY;
    w[1] = (0xffUL << HVS_POS0_ALPHA_SHIFT)
         | ((ULONG)cy << HVS_POS0_Y_SHIFT)
         | ((ULONG)cx << HVS_POS0_X_SHIFT);
    w[2] = ((ULONG)ch << HVS_POS2_H_SHIFT)      /* amode 0 = per-pixel */
         | ((ULONG)cw << HVS_POS2_W_SHIFT);
    w[3] = 0;
    w[4] = ptr;
    w[5] = 0;
    w[6] = pitch;
    return TRUE;
}

/* Compute the overlay plane entry from the hvs_Ovl* state; returns the
 * word count (0 = nothing to show). Opaque 32bpp plane at fb
 * coordinates. Unity entries (dest == src) are clipped against the
 * output edges like the cursor; scaled entries (validated fully
 * onscreen by vc4_hvs_overlay) get the 14-word PPF layout with our
 * kernel copy. Only offered on unity fb planes, so fb coords == output
 * coords minus the plane offset. */
static ULONG hvs_overlay_entry(struct VideoCoreGfx_staticdata *xsd, ULONG *w)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    LONG ox, oy, ow, oh;
    ULONG ptr;

    if (!st->hvs_OvlActive)
        return 0;

    ox = (LONG)st->hvs_FBX + st->hvs_OvlX;
    oy = (LONG)st->hvs_FBY + st->hvs_OvlY;
    ptr = HVS_PTR_BUS_ALIAS | st->hvs_OvlPhys;

    if (st->hvs_OvlDestW != st->hvs_OvlW || st->hvs_OvlDestH != st->hvs_OvlH)
    {
        /* Scaled: layout per the decoded firmware entries. [4]/[6]/[11]
         * are HVS-written context words. */
        w[0]  = HVS_CTL0_XRGB_SCALED;
        w[1]  = (0xffUL << HVS_POS0_ALPHA_SHIFT)
              | ((ULONG)oy << HVS_POS0_Y_SHIFT)
              | ((ULONG)ox << HVS_POS0_X_SHIFT);
        w[2]  = (st->hvs_OvlDestH << HVS_POS2_H_SHIFT)      /* POS1: dest */
              | (st->hvs_OvlDestW << HVS_POS2_W_SHIFT);
        w[3]  = (1UL << HVS_POS2_AMODE_SHIFT)               /* POS2: src */
              | (st->hvs_OvlH << HVS_POS2_H_SHIFT)
              | (st->hvs_OvlW << HVS_POS2_W_SHIFT);
        w[4]  = 0;
        w[5]  = ptr;
        w[6]  = 0;
        w[7]  = st->hvs_OvlPitch;
        w[8]  = HVS_OVL_LBM;
        w[9]  = HVS_PPF(st->hvs_OvlW, st->hvs_OvlDestW);
        w[10] = HVS_PPF(st->hvs_OvlH, st->hvs_OvlDestH);
        w[11] = 0;
        w[12] = HVS_OWN_KERNEL;
        w[13] = HVS_OWN_KERNEL;
        return HVS_SCALED_WORDS;
    }

    ow = st->hvs_OvlW;
    oh = st->hvs_OvlH;

    if (ox < 0) { ptr += (ULONG)(-ox) * 4;               ow += ox; ox = 0; }
    if (oy < 0) { ptr += (ULONG)(-oy) * st->hvs_OvlPitch; oh += oy; oy = 0; }
    if (ox + ow > (LONG)st->hvs_OutW) ow = (LONG)st->hvs_OutW - ox;
    if (oy + oh > (LONG)st->hvs_OutH) oh = (LONG)st->hvs_OutH - oy;

    if (ow <= 0 || oh <= 0)
        return 0;

    w[0] = HVS_CTL0_XRGB_UNITY;
    w[1] = (0xffUL << HVS_POS0_ALPHA_SHIFT)
         | ((ULONG)oy << HVS_POS0_Y_SHIFT)
         | ((ULONG)ox << HVS_POS0_X_SHIFT);
    w[2] = (1UL << HVS_POS2_AMODE_SHIFT)        /* fixed alpha: GL alpha
                                                 * bytes are undefined */
         | ((ULONG)oh << HVS_POS2_H_SHIFT)
         | ((ULONG)ow << HVS_POS2_W_SHIFT);
    w[3] = 0;
    w[4] = ptr;
    w[5] = 0;
    w[6] = st->hvs_OvlPitch;
    return HVS_UNITY_WORDS;
}

/* Author our display list (fb plane + overlay + cursor plane) into the
 * next round-robin slot and return its head index. The fb plane entry
 * is the verbatim words inherited from the firmware at takeover (unity
 * or scaled — the scaling words stay opaque), with only PTR0
 * retargeted; the cursor plane mirrors the firmware's own cursor
 * (per-pixel alpha) reading vcsd_CurBuf directly.
 * Caller holds the mailbox lock and must write the returned head to
 * HVS_DISPLIST — the HVS applies a repoint MID-SCANOUT (observed on
 * HW as lower-screen corruption), so this is only for structural
 * changes; steady-state flips and cursor moves patch the live list
 * in place instead. */
static ULONG hvs_build_list(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    ULONG base = HVS_OWN_SLOT_BASE + HVS_OWN_SLOT_STRIDE * st->hvs_Slot;
    ULONG w[HVS_FB_ENTRY_MAX + HVS_SCALED_WORDS + HVS_UNITY_WORDS + 1];
    ULONG n = 0, i, ovlw;

    st->hvs_Slot = (st->hvs_Slot + 1) % HVS_OWN_SLOTS;

    for (i = 0; i < st->hvs_FBWords; i++)
        w[n++] = st->hvs_FBEntry[i];
    w[st->hvs_FBPtrOff] = HVS_PTR_BUS_ALIAS | st->hvs_FBPtr;

    st->hvs_OvlOff = 0;
    st->hvs_OvlWords = 0;
    ovlw = hvs_overlay_entry(xsd, &w[n]);
    if (ovlw)
    {
        st->hvs_OvlOff = n;
        st->hvs_OvlWords = ovlw;
        n += ovlw;
    }

    st->hvs_CurOff = 0;
    if (hvs_cursor_entry(xsd, &w[n]))
    {
        st->hvs_CurOff = n;
        n += HVS_UNITY_WORDS;
    }

    w[n++] = HVS_CTL0_END;

    /* A dlist must never reach HVS_OWN_KERNEL — overwriting the PPF filter
     * kernel corrupts scaled scanout and wedges the HVS. HVS_OWN_SLOTS is
     * sized to keep every slot clear; warn loudly if that ever breaks. */
    if (base + n > HVS_OWN_KERNEL)
        bug("[VC4HVS] WARN: dlist at base %lu + %lu words overruns "
            "HVS_OWN_KERNEL %u\n", (unsigned long)base, (unsigned long)n,
            (unsigned)HVS_OWN_KERNEL);

    for (i = 0; i < n; i++)
        hvs_wr(HVS_DLIST_START + 4 * (base + i), w[i]);

    st->hvs_ListBase = base;
    return base;
}

#if VC4_HVS_VSYNC_IRQ
/* PV2 vsync interrupt: the "frame done" tick for flip pacing. The
 * firmware leaves PV_INTEN at 0 (it doesn't use this interrupt), so
 * unlike the V3D IRQ there is no known co-ownership hazard; the
 * bring-up below still verifies delivery and re-masks on failure.
 * IRQ context: count only, no printing. */
static void hvs_vsync_irq(struct vc4_hvs_state *st, struct ExecBase *sysBase)
{
    ULONG stat = pv_rd(VC4_PV2_BASE, PV_INTSTAT);

    if (stat)
    {
        pv_wr(VC4_PV2_BASE, PV_INTSTAT, stat);      /* W1C */
        if (stat & st->hvs_VSyncMask)
        {
            st->hvs_VSyncCount++;
        }
    }
}

/* Install the vsync IRQ handler. Called ONCE from driver init: task
 * context at module load, mirroring vc4gallium's vc4_v3d_init pattern.
 * KrnAddIRQHandler goes to supervisor mode and calls AllocMem there —
 * calling it later, mid mode-set (as the first takeover build did),
 * can catch the memory lock held and wedge the machine in supervisor
 * context. The interrupt source stays masked (PV_INTEN untouched)
 * until a takeover arms it. */
void vc4_hvs_init(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    if (!hvs_hw_known(xsd))
    {
        vc4_hvs5_irq_init(xsd);
        return;
    }

    st->hvs_VSyncIrq = KrnAddIRQHandler(IRQ_PIXELVALVE1, hvs_vsync_irq,
                                        st, SysBase);
    if (!st->hvs_VSyncIrq)
        bug("[VC4HVS] vsync: KrnAddIRQHandler failed\n");
}

/* Called with ownership established. First time: find the per-frame
 * INTSTAT bit empirically — the documented bit numbering is wrong on
 * real HW (the assumed VFP_START bit 7 fires at LINE rate). INTSTAT
 * latches events even while INTEN is masked, and the vertical total
 * from PV_VERTA/B gives the expected line/frame ratio, so a poll+W1C
 * sampling window self-calibrates without a timer: line-rate bits
 * count ~vtot times more than frame-rate bits. Then arm the chosen
 * bit (a firmware mode set clears PV_INTEN, so re-arm on every
 * takeover) and verify ticks arrive. */
/* Find the per-frame interrupt bit empirically. INTSTAT does NOT latch
 * events for masked sources (verified: all-zero across a full masked
 * sampling window), so passive profiling is impossible — instead each
 * bit is armed alone for a short window and the IRQ handler counts its
 * deliveries. Line-rate bits count thousands of times more than
 * frame-rate bits over the same window; the vertical total from
 * PV_VERTA/B gives the expected ratio, so no timer is needed. A short
 * burst of line-rate interrupts is known-survivable (bit 7 ran at
 * 67 kHz for a whole session during bring-up). */
static void hvs_vsync_start(struct vc4_hvs_state *st)
{
    ULONG c0, i, b;

    if (!st->hvs_VSyncIrq)
    {
        bug("[VC4HVS] vsync: no IRQ handler installed, staying masked\n");
        return;
    }

    if (!st->hvs_VSyncMask)
    {
        ULONG rate[10];
        ULONG verta = pv_rd(VC4_PV2_BASE, PV_VERTA);
        ULONG vertb = pv_rd(VC4_PV2_BASE, PV_VERTB);
        ULONG vtot = (verta & 0xffff) + (verta >> 16)
                   + (vertb & 0xffff) + (vertb >> 16);
        ULONG line_max = 0, expect;

        bug("[VC4HVS] vsync: per-bit IRQ probe, vtot=%u\n", vtot);

        for (b = 0; b < 10; b++)
        {
            c0 = st->hvs_VSyncCount;
            st->hvs_VSyncMask = 1UL << b;       /* handler counts this bit */
            pv_wr(VC4_PV2_BASE, PV_INTSTAT, 0x3ff);
            pv_wr(VC4_PV2_BASE, PV_INTEN, 1UL << b);
            for (i = 0; i < HVS_SPIN_PROBEBIT; i++)
                (void)pv_rd(VC4_PV2_BASE, PV_STAT);     /* pace */
            pv_wr(VC4_PV2_BASE, PV_INTEN, 0);
            pv_wr(VC4_PV2_BASE, PV_INTSTAT, 0x3ff);
            rate[b] = st->hvs_VSyncCount - c0;
            bug("[VC4HVS] vsync: bit %u -> %u ticks\n", b, rate[b]);
        }
        st->hvs_VSyncMask = 0;

        for (b = 0; b < 10; b++)
        {
            if (rate[b] > line_max)
                line_max = rate[b];
        }
        if (vtot == 0 || line_max < vtot / 4)
        {
            bug("[VC4HVS] vsync: probe inconclusive (max %u), leaving masked\n",
                line_max);
            return;
        }
        expect = line_max / vtot;
        if (expect == 0)
            expect = 1;

        for (b = 0; b < 10; b++)
        {
            if (rate[b] >= (expect + 1) / 2 && rate[b] <= 2 * expect + 2)
            {
                st->hvs_VSyncMask = 1UL << b;
                bug("[VC4HVS] vsync: bit %u ticks per frame, using it\n", b);
                break;
            }
        }
        if (!st->hvs_VSyncMask)
        {
            bug("[VC4HVS] vsync: no frame-rate bit found, leaving masked\n");
            return;
        }
    }

    bug("[VC4HVS] vsync: step6 W1C + arm INTEN=0x%03x\n", st->hvs_VSyncMask);
    pv_wr(VC4_PV2_BASE, PV_INTSTAT, 0x3ff);
    pv_wr(VC4_PV2_BASE, PV_INTEN, st->hvs_VSyncMask);
    bug("[VC4HVS] vsync: step6 ok, verifying\n");

    c0 = st->hvs_VSyncCount;
    for (i = 0; i < HVS_SPIN_LATCH; i++)
    {
        (void)pv_rd(VC4_PV2_BASE, PV_STAT);         /* pace the spin */
        if (st->hvs_VSyncCount >= c0 + 5)
            break;
    }

    if (st->hvs_VSyncCount == c0)
    {
        pv_wr(VC4_PV2_BASE, PV_INTEN, 0);
        bug("[VC4HVS] vsync: no interrupts delivered, re-masked\n");
    }
    else
        bug("[VC4HVS] vsync: alive, %u ticks during check (count=%u)\n",
            st->hvs_VSyncCount - c0, st->hvs_VSyncCount);
}
#else
void vc4_hvs_init(struct VideoCoreGfx_staticdata *xsd)
{
    if (!hvs_hw_known(xsd))
    {
        vc4_hvs5_irq_init(xsd);
        return;
    }
    xsd->vcsd_HVS.hvs_VSyncIrq = NULL;
}
#endif

BOOL vc4_hvs_takeover(struct VideoCoreGfx_staticdata *xsd,
                      ULONG fb_phys, ULONG fb_pitch)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    st->hvs_Active = FALSE;

    if (!hvs_hw_known(xsd))
        return vc4_hvs5_takeover(xsd, fb_phys, fb_pitch);

#if VC4_HVS_TAKEOVER
    {
        ULONG ctrl, head, idx, our, i;
        BOOL found = FALSE;

        if (hvs_rd(HVS_ID) == 0 || hvs_rd(HVS_ID) == 0xffffffff)
            return FALSE;

        ctrl = hvs_rd(HVS_DISPCTRLX(HVS_CHANNEL_HDMI));
        head = hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI));
        if (!(ctrl & HVS_DISPCTRLX_ENABLE) || head == 0
            || head >= HVS_DLIST_WORDS)
        {
            bug("[VC4HVS] takeover: HDMI channel not usable "
                "(CTRL=%08x head=%u)\n", ctrl, head);
            return FALSE;
        }

        /* Inherit the fb plane from the firmware's live list. */
        idx = head;
        while (idx < HVS_DLIST_WORDS)
        {
            ULONG ctl0 = hvs_rd(HVS_DLIST_START + 4 * idx);
            ULONG size;

            if (ctl0 & HVS_CTL0_END)
                break;
            size = (ctl0 >> HVS_CTL0_SIZE_SHIFT) & HVS_CTL0_SIZE_MASK;
            if (size == 0 || idx + size >= HVS_DLIST_WORDS)
                break;

            /* Recognize the fb plane by its PTR0. Two known layouts,
             * both single-plane RGBA: unity (7 words, ptr at +4) and
             * scaled (14 words, ptr at +5, POS1 = dest size at +2). */
            if ((ctl0 & HVS_CTL0_FORMAT_MASK) == HVS_PXF_RGBA8888)
            {
                ULONG ptroff = 0;

                if ((ctl0 & HVS_CTL0_UNITY) && size == HVS_UNITY_WORDS)
                    ptroff = 4;
                else if (!(ctl0 & HVS_CTL0_UNITY) && size == HVS_SCALED_WORDS)
                    ptroff = 5;

                if (ptroff)
                {
                    ULONG ptr0 = hvs_rd(HVS_DLIST_START + 4 * (idx + ptroff));

                    if ((ptr0 & ~HVS_PTR_BUS_ALIAS) == (fb_phys & ~HVS_PTR_BUS_ALIAS))
                    {
                        ULONG pos0 = hvs_rd(HVS_DLIST_START + 4 * (idx + 1));
                        ULONG pos2;

                        st->hvs_FBX = (pos0 >> HVS_POS0_X_SHIFT) & 0xfff;
                        st->hvs_FBY = (pos0 >> HVS_POS0_Y_SHIFT) & 0xfff;
                        if (ptroff == 4)
                        {
                            pos2 = hvs_rd(HVS_DLIST_START + 4 * (idx + 2));
                            st->hvs_SrcW = (pos2 >> HVS_POS2_W_SHIFT) & 0xfff;
                            st->hvs_SrcH = (pos2 >> HVS_POS2_H_SHIFT) & 0xfff;
                            st->hvs_DestW = st->hvs_SrcW;
                            st->hvs_DestH = st->hvs_SrcH;
                        }
                        else
                        {
                            ULONG pos1 = hvs_rd(HVS_DLIST_START + 4 * (idx + 2));

                            st->hvs_DestW = pos1 & 0xfff;
                            st->hvs_DestH = (pos1 >> 16) & 0xfff;
                            pos2 = hvs_rd(HVS_DLIST_START + 4 * (idx + 3));
                            st->hvs_SrcW = (pos2 >> HVS_POS2_W_SHIFT) & 0xfff;
                            st->hvs_SrcH = (pos2 >> HVS_POS2_H_SHIFT) & 0xfff;
                        }

                        for (i = 0; i < size; i++)
                            st->hvs_FBEntry[i] =
                                hvs_rd(HVS_DLIST_START + 4 * (idx + i));
                        st->hvs_FBWords  = size;
                        st->hvs_FBPtrOff = ptroff;
                        found = TRUE;
                        break;
                    }
                }
            }
            idx += size;
        }

        if (!found || !st->hvs_SrcW || !st->hvs_SrcH)
        {
            bug("[VC4HVS] takeover: no usable fb plane for 0x%08x in list %u, "
                "staying on firmware\n", fb_phys, head);
            return FALSE;
        }

        st->hvs_OutW  = (ctrl >> HVS_DISPCTRLX_W_SHIFT) & 0xfff;
        st->hvs_OutH  = (ctrl >> HVS_DISPCTRLX_H_SHIFT) & 0xfff;
        st->hvs_FBPtr = fb_phys & ~HVS_PTR_BUS_ALIAS;
        st->hvs_Slot  = 0;
        st->hvs_FlipArmed = st->hvs_VSyncCount;     /* no flip pending */
        st->hvs_OvlActive = FALSE;                  /* fresh mode, no overlay */
        (void)fb_pitch;

        /* Copy the firmware's PPF filter kernel (11 words at 0xff4)
         * into our reserved area so scaled overlay entries never
         * depend on firmware-owned dlist RAM. All-zero source = the
         * firmware never initialized one; fall back to the kernel
         * captured from a real Pi 3B+. */
        {
            static const ULONG defkern[HVS_KERNEL_WORDS] = HVS_KERNEL_DEFAULT;
            ULONG k, nz = 0;

            for (k = 0; k < HVS_KERNEL_WORDS; k++)
            {
                if (hvs_rd(HVS_DLIST_START + 4 * (HVS_FW_KERNEL + k)))
                    nz++;
            }
            for (k = 0; k < HVS_KERNEL_WORDS; k++)
            {
                ULONG kw = nz
                    ? hvs_rd(HVS_DLIST_START + 4 * (HVS_FW_KERNEL + k))
                    : defkern[k];

                hvs_wr(HVS_DLIST_START + 4 * (HVS_OWN_KERNEL + k), kw);
            }
            st->hvs_KernelOK = TRUE;
        }

        VC4_MBOX_LOCK(xsd);
        our = hvs_build_list(xsd);
        hvs_wr(HVS_DISPLIST(HVS_CHANNEL_HDMI), our);
        for (i = 0; i < HVS_SPIN_LATCH; i++)
        {
            if (hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI)) == our)
                break;
        }
        if (hvs_rd(HVS_DISPLACT(HVS_CHANNEL_HDMI)) == our)
        {
            st->hvs_Active = TRUE;
        }
        else
            hvs_wr(HVS_DISPLIST(HVS_CHANNEL_HDMI), head);
        VC4_MBOX_UNLOCK(xsd);

        if (st->hvs_Active)
        {
            bug("[VC4HVS] takeover: ACTIVE - list %u, out %ux%u, fb %ux%u -> "
                "%ux%u at %u,%u\n", our, st->hvs_OutW, st->hvs_OutH,
                st->hvs_SrcW, st->hvs_SrcH, st->hvs_DestW, st->hvs_DestH,
                st->hvs_FBX, st->hvs_FBY);
#if VC4_HVS_VSYNC_IRQ
            hvs_vsync_start(st);
#endif
        }
        else
            bug("[VC4HVS] takeover: FAIL - never latched, firmware restored\n");
    }
#else
    (void)fb_phys;
    (void)fb_pitch;
#endif

    return st->hvs_Active;
}

/* Back-pressure: a page (or overlay buffer) just retargeted away from
 * stays on scanout until the write latches at the next frame start,
 * and the caller starts rendering into it the moment we return — so
 * wait for our own latch (waiting only for the previous one left up to
 * a frame of rendering into live scanout, visible as rolling
 * corruption whenever the frame rate fell below the refresh rate).
 * Only when the vsync interrupt is genuinely armed (INTEN check — a
 * firmware mode set can mask it), and bounded, so a dead counter
 * degrades to unpaced behavior instead of stalling the presenter. */
static void hvs_latch_wait(struct vc4_hvs_state *st)
{
#if VC4_HVS_FLIP_SYNC
    if (st->hvs_VSyncIrq && st->hvs_VSyncMask
        && (pv_rd(VC4_PV2_BASE, PV_INTEN) & st->hvs_VSyncMask))
    {
        ULONG i;

        for (i = 0; i < HVS_SPIN_FLIP; i++)
        {
            if ((LONG)(st->hvs_VSyncCount - st->hvs_FlipArmed) >= 0)
                break;
            (void)pv_rd(VC4_PV2_BASE, PV_STAT);     /* pace the spin */
        }

        /* Flip cadence stats: ticks between consecutive flips. All-1s
         * = locked to refresh; 2s = clean halving (frame straddled a
         * tick); >2 = something stalled a whole frame or more. */
#if VC4_HVS_FLIPSTATS
        {
            static ULONG lastc = 0, n = 0, d1 = 0, d2 = 0, dmore = 0;
            ULONG c = st->hvs_VSyncCount;
            ULONG d = c - lastc;

            lastc = c;
            n++;
            if (d <= 1)
                d1++;
            else if (d == 2)
                d2++;
            else
                dmore++;
            if ((n & 255) == 0)
                bug("[VC4HVS] flip stats: %u flips, tick interval 1:%u 2:%u >2:%u\n",
                    n, d1, d2, dmore);
        }
#endif
    }
#endif
}

BOOL vc4_hvs_flip_page(struct VideoCoreGfx_staticdata *xsd, ULONG page_phys)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    if (!hvs_hw_known(xsd))
        return vc4_hvs5_flip_page(xsd, page_phys);

    if (!st->hvs_Active)
        return FALSE;

    /* Single atomic word: retarget the fb plane's PTR0 in the live
     * list. The HVS reloads it at frame start, so the flip itself is
     * vblank-latched. No repoint — repoints apply mid-scanout. */
    st->hvs_FBPtr = page_phys & ~HVS_PTR_BUS_ALIAS;
    hvs_wr(HVS_DLIST_START + 4 * (st->hvs_ListBase + st->hvs_FBPtrOff),
           HVS_PTR_BUS_ALIAS | st->hvs_FBPtr);
    st->hvs_FlipArmed = st->hvs_VSyncCount + 1;

    hvs_latch_wait(st);
    return TRUE;
}

BOOL vc4_hvs_overlay(struct VideoCoreGfx_staticdata *xsd,
                     const struct vc4gfx_overlay *ovl)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    ULONG dw, dh;
    BOOL structural;

    if (!hvs_hw_known(xsd))
        return vc4_hvs5_overlay(xsd, ovl);

    VC4_MBOX_LOCK(xsd);

    /* Only on an owned, unity fb plane: on scaled desktops the overlay
     * contents would need scaling too to line up — blit path instead. */
    if (!st->hvs_Active
        || st->hvs_DestW != st->hvs_SrcW || st->hvs_DestH != st->hvs_SrcH)
    {
        st->hvs_OvlActive = FALSE;
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }

    if (!ovl)
    {
        if (st->hvs_OvlActive)
        {
            st->hvs_OvlActive = FALSE;
            hvs_wr(HVS_DISPLIST(HVS_CHANNEL_HDMI), hvs_build_list(xsd));
        }
        VC4_MBOX_UNLOCK(xsd);
        return TRUE;
    }

    dw = ovl->ovl_DestW ? ovl->ovl_DestW : ovl->ovl_Width;
    dh = ovl->ovl_DestH ? ovl->ovl_DestH : ovl->ovl_Height;

    if (dw != ovl->ovl_Width || dh != ovl->ovl_Height)
    {
        /* Scaled plane: needs the filter kernel, upscale only (downscale
         * uses a different HW mode), and — since clipping a scaled
         * plane means rescaling the source window — must lie fully
         * within the output. Refused = caller blits. */
        LONG sx = (LONG)st->hvs_FBX + ovl->ovl_X;
        LONG sy = (LONG)st->hvs_FBY + ovl->ovl_Y;

        if (!st->hvs_KernelOK
            || dw < ovl->ovl_Width || dh < ovl->ovl_Height
            || dw > 0xfff || dh > 0xfff
            || sx < 0 || sy < 0
            || sx + (LONG)dw > (LONG)st->hvs_OutW
            || sy + (LONG)dh > (LONG)st->hvs_OutH)
        {
            VC4_MBOX_UNLOCK(xsd);
            return FALSE;
        }
    }

    /* Appearing, or a size/scaling change, alters the list shape:
     * rebuild + repoint (one-frame artifact at worst, entry/exit only).
     * Steady state (new buffer and/or position) patches the live
     * entry. */
    structural = !st->hvs_OvlActive || !st->hvs_OvlOff
              || st->hvs_OvlW != ovl->ovl_Width
              || st->hvs_OvlH != ovl->ovl_Height
              || st->hvs_OvlDestW != dw
              || st->hvs_OvlDestH != dh;

    st->hvs_OvlActive = TRUE;
    st->hvs_OvlPhys  = ovl->ovl_Phys & ~HVS_PTR_BUS_ALIAS;
    st->hvs_OvlPitch = ovl->ovl_Pitch;
    st->hvs_OvlW     = ovl->ovl_Width;
    st->hvs_OvlH     = ovl->ovl_Height;
    st->hvs_OvlDestW = dw;
    st->hvs_OvlDestH = dh;
    st->hvs_OvlX     = ovl->ovl_X;
    st->hvs_OvlY     = ovl->ovl_Y;

    if (structural)
    {
        hvs_wr(HVS_DISPLIST(HVS_CHANNEL_HDMI), hvs_build_list(xsd));
        if (!st->hvs_OvlOff)
        {
            /* Clipped away entirely (fully offscreen) — report shown
             * anyway; the caller's pixels are simply not visible. */
        }
    }
    else
    {
        ULONG w[HVS_SCALED_WORDS];
        ULONG base = st->hvs_ListBase + st->hvs_OvlOff;
        ULONG words = hvs_overlay_entry(xsd, w);

        if (words == st->hvs_OvlWords)
        {
            /* Patch in place, skipping the HVS-written context words
             * (unity: [3] and [5]; scaled: [4], [6] and [11]). */
            BOOL scaled = (words == HVS_SCALED_WORDS);
            ULONG i;

            for (i = 1; i < words; i++)
            {
                if (scaled ? (i == 4 || i == 6 || i == 11)
                           : (i == 3 || i == 5))
                    continue;
                hvs_wr(HVS_DLIST_START + 4 * (base + i), w[i]);
            }
        }
        else
        {
            /* Entry shape changed after clipping (or moved fully
             * offscreen): the stale entry must go. */
            hvs_wr(HVS_DISPLIST(HVS_CHANNEL_HDMI), hvs_build_list(xsd));
        }
    }

    /* The old overlay buffer stays on scanout until the update latches;
     * pace the presenter exactly like a page flip. */
    st->hvs_FlipArmed = st->hvs_VSyncCount + 1;
    hvs_latch_wait(st);

    VC4_MBOX_UNLOCK(xsd);
    return TRUE;
}

void vc4_hvs_update_cursor(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    ULONG w[HVS_UNITY_WORDS];
    BOOL present;

    VC4_MBOX_LOCK(xsd);
    if (!hvs_hw_known(xsd))
    {
        vc4_hvs5_update_cursor(xsd);
        VC4_MBOX_UNLOCK(xsd);
        return;
    }
    if (!st->hvs_Active)
    {
        VC4_MBOX_UNLOCK(xsd);
        return;
    }

    present = hvs_cursor_entry(xsd, w);

    if (present && st->hvs_CurOff)
    {
        /* Cursor plane already in the list: patch it in place. Only
         * pos0/pos2/ptr0/pitch change — pos3/ptrctx are the words the
         * HVS itself writes during scanout, leave them alone. */
        ULONG base = st->hvs_ListBase + st->hvs_CurOff;

        hvs_wr(HVS_DLIST_START + 4 * (base + 1), w[1]);
        hvs_wr(HVS_DLIST_START + 4 * (base + 2), w[2]);
        hvs_wr(HVS_DLIST_START + 4 * (base + 4), w[4]);
        hvs_wr(HVS_DLIST_START + 4 * (base + 6), w[6]);
    }
    else if (present != (st->hvs_CurOff != 0))
    {
        /* Cursor appeared or disappeared: the list changes shape, so
         * rebuild + repoint. Rare (show/hide/shape), and a mid-scanout
         * repoint here is at worst a single-frame artifact. */
        hvs_wr(HVS_DISPLIST(HVS_CHANNEL_HDMI), hvs_build_list(xsd));
    }
    VC4_MBOX_UNLOCK(xsd);
}

void vc4_hvs_dump(struct VideoCoreGfx_staticdata *xsd,
                  ULONG fb_phys, ULONG fb_pitch,
                  ULONG fb_width, ULONG fb_height)
{
#if VC4_HVS_DUMP
    ULONG id;
    int ch;

    if (!hvs_hw_known(xsd))
    {
        vc4_hvs5_dump(xsd, fb_phys, fb_pitch, fb_width, fb_height);
        return;
    }

    id = hvs_rd(HVS_ID);
    if (id == 0 || id == 0xffffffff)
    {
        bug("[VC4HVS] no HVS found (ID=0x%08x) - QEMU or unmapped, skipping\n", id);
        return;
    }

    bug("[VC4HVS] ID=0x%08x DISPCTRL=0x%08x DISPSTAT=0x%08x "
        "DISPLSTAT=0x%08x\n", id, hvs_rd(HVS_DISPCTRL),
        hvs_rd(HVS_DISPSTAT), hvs_rd(HVS_DISPLSTAT));
    bug("[VC4HVS] expecting fb: phys=0x%08x pitch=%u %ux%u\n",
        fb_phys, fb_pitch, fb_width, fb_height);

    for (ch = 0; ch < HVS_CHANNELS; ch++)
    {
        ULONG list = hvs_rd(HVS_DISPLIST(ch));
        ULONG lact = hvs_rd(HVS_DISPLACT(ch));

        bug("[VC4HVS] ch%d: LIST=%04x LACT=%04x CTRL=%08x BKGND=%08x "
            "STAT=%08x BASE=%08x\n", ch, list, lact,
            hvs_rd(HVS_DISPCTRLX(ch)), hvs_rd(HVS_DISPBKGNDX(ch)),
            hvs_rd(HVS_DISPSTATX(ch)), hvs_rd(HVS_DISPBASEX(ch)));

        /* An idle channel parks LIST/LACT at 0 or at an END word; only
         * walk lists that lead anywhere. */
        if (lact != 0 && lact < HVS_DLIST_WORDS)
        {
            bug("[VC4HVS]  walking active list of ch%d:\n", ch);
            hvs_dump_list(lact, fb_phys, fb_pitch, fb_width, fb_height);
        }
    }

    pv_dump("PV0", VC4_PV0_BASE);
    pv_dump("PV1", VC4_PV1_BASE);
    pv_dump("PV2", VC4_PV2_BASE);

    /* The firmware's PPF filter kernel (scaled-plane entries point
     * their kernel words at 0xff4) — captured for the record. */
    {
        ULONG k;

        bug("[VC4HVS] fw kernel @%u:", (ULONG)HVS_FW_KERNEL);
        for (k = 0; k < HVS_KERNEL_WORDS; k++)
            bug(" %08x", hvs_rd(HVS_DLIST_START + 4 * (HVS_FW_KERNEL + k)));
        bug("\n");
    }

#if VC4_HVS_PROBE
    vc4_hvs_probe();
#endif
#endif
}
