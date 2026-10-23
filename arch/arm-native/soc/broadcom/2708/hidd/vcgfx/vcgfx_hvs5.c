/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2711 display: HVS5 display lists and PixelValve vblank.

    HVS5, not HVS6: the compositor block carries its own revision number,
    which is not the VideoCore generation. A Pi 4 is VideoCore VI with an
    HVS5 and a V3D 4.2, where a Pi 3 is VideoCore IV with an HVS4 and a
    V3D 2.1 - the numbers lining up there is a coincidence. The identity
    register reads the same 0x64647276 on both, so it does not tell them
    apart; HVS5 is simply the established name for this block.

    The layout it drives was measured, not assumed. vc4_hvs5_dump() finds
    the display list by searching the register window for the framebuffer
    address FBALLOC already reported, and everything else - entry length,
    where the pointer sits inside an entry, which channel is live, how
    POS0 packs - was read out of a live firmware-built list on real
    hardware. See vcgfx_hvs5.h for what came out of it.

    From there the driver owns the list: it inherits the framebuffer plane
    verbatim, authors its own cursor and overlay planes, and paces flips
    off the PixelValve interrupt.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hardware.h"
#include "vcgfx_hvs5.h"

/*
 * Off by default, and it has to stay that way: QEMU's raspi4b models no
 * HVS, so the first read of 0xfe400000 takes an external abort and brings
 * the machine down (ESR 0x96000010). Real hardware has the block alive -
 * the firmware is driving the display through it - but nothing can tell
 * the two apart from the ARM side, and the Pi 4 device tree marks the
 * node "disabled" either way. So this is a deliberate one-line opt-in for
 * a real Pi 4 bring-up session, exactly like VC4_HVS_PROBE next door.
 */
#define VC4_HVS5_DUMP       1

/* Words printed either side of a framebuffer address hit. A VideoCore IV
 * plane entry was 7 words unity / 14 scaled; allow for HVS5 being wider. */
#define HVS5_CONTEXT_WORDS  24

/* Cap the reported hits: a plane pointer appears once or twice (the
 * firmware double buffers its lists), but a wild match pattern should not
 * flood the serial log. */
#define HVS5_MAX_HITS       8

static const struct hvs5_pv hvs5_pvs[HVS5_PV_COUNT] =
{
    { "PV0", 0x206000, 32 + 109 },
    { "PV1", 0x207000, 32 + 110 },
    { "PV2", 0x20a000, 32 + 101 },
    { "PV3", 0xc12000, 32 + 106 },
    { "PV4", 0x216000, 32 + 110 },
};

/*
 * Owning the display list is how flips become atomic, so this is on. The
 * cost is that QEMU's raspi4b cannot boot the result at all - it models no
 * HVS and aborts on the first register read - but that machine is too
 * incomplete to test against anyway. Clear this to get back to the
 * firmware paths.
 */
#define VC4_HVS5_TAKEOVER   1

/*
 * Vblank pacing. Installing the handler touches no registers and the
 * frame-rate-bit probe only runs after a takeover has succeeded, so this
 * does not need the caution the two flags above do. Must be defined up
 * here: the takeover below is guarded by it, and a definition further
 * down would preprocess to 0 there and silently drop the call.
 */
#define VC4_HVS5_VSYNC_IRQ  1

/* A 50 Hz frame is ~20ms, roughly 150k uncached register reads. */
#define HVS5_SPIN_LATCH     5000000


static inline ULONG hvs5_rd(ULONG offset)
{
    return *(volatile ULONG *)(HVS5_BASE + offset);
}

static inline void hvs5_wr(ULONG offset, ULONG value)
{
    *(volatile ULONG *)(HVS5_BASE + offset) = value;
}

static inline ULONG hvs5_dl_rd(ULONG word)
{
    return hvs5_rd(HVS5_DLIST_START + 4 * word);
}

static inline void hvs5_dl_wr(ULONG word, ULONG value)
{
    hvs5_wr(HVS5_DLIST_START + 4 * word, value);
}

static inline ULONG pv_rd(ULONG base_off, ULONG offset)
{
    return *(volatile ULONG *)(ARM_PERIIOBASE + base_off + offset);
}

/* Print one 16-word block, offsets included so the log can be read back
 * against the register window. */
static void hvs5_dump_words(ULONG start, ULONG words)
{
    ULONG i;

    for (i = 0; i < words; i += 8)
    {
        ULONG j, n = (words - i) > 8 ? 8 : (words - i);

        bug("[VC4HVS5]   +%04x:", (unsigned)(start + i * 4));
        for (j = 0; j < n; j++)
            bug(" %08x", hvs5_rd(start + (i + j) * 4));
        bug("\n");
    }
}

/*
 * Find the framebuffer pointer in the register window. The firmware may
 * store it as a plain ARM physical address or through one of the
 * VideoCore bus aliases, so compare with the alias bits masked off.
 */
static void hvs5_find_fb(ULONG fb_phys)
{
    ULONG want = fb_phys & 0x3fffffff;
    ULONG off, hits = 0;

    bug("[VC4HVS5] scanning +%04x..+%04x for fb 0x%08x\n",
        (unsigned)HVS5_SCAN_START, (unsigned)HVS5_SCAN_END, fb_phys);

    for (off = HVS5_SCAN_START; off < HVS5_SCAN_END && hits < HVS5_MAX_HITS;
         off += 4)
    {
        ULONG v = hvs5_rd(off);

        if ((v & 0x3fffffff) != want || v == 0)
            continue;

        hits++;
        bug("[VC4HVS5] hit %u: fb pointer 0x%08x at +%04x\n",
            (unsigned)hits, v, (unsigned)off);

        /* The entry containing it, with room for its header and tail. */
        {
            ULONG ctx = HVS5_CONTEXT_WORDS * 4;
            ULONG from = (off > HVS5_SCAN_START + ctx) ? off - ctx
                                                       : HVS5_SCAN_START;
            ULONG to = off + ctx;

            if (to > HVS5_SCAN_END)
                to = HVS5_SCAN_END;
            hvs5_dump_words(from, (to - from) / 4);
        }
    }

    if (!hits)
        bug("[VC4HVS5] fb pointer not found - firmware may hold the list "
            "elsewhere, or store it in another form\n");
}

void vc4_hvs5_dump(struct VideoCoreGfx_staticdata *xsd,
                   ULONG fb_phys, ULONG fb_pitch,
                   ULONG fb_width, ULONG fb_height)
{
#if VC4_HVS5_DUMP
    ULONG i;

    (void)xsd;

    bug("[VC4HVS5] BCM2711 HVS5 @ 0x%08x, fb 0x%08x pitch %u %ux%u\n",
        (unsigned)HVS5_BASE, fb_phys, (unsigned)fb_pitch,
        (unsigned)fb_width, (unsigned)fb_height);

    /* Control registers. Whatever the layout, the channel heads and the
     * enable/size fields are in here - the VideoCore IV ones all lived
     * below +0x80. */
    bug("[VC4HVS5] control registers:\n");
    hvs5_dump_words(0, 64);

    hvs5_find_fb(fb_phys);

    /* Only an enabled PixelValve carries timing, which is what identifies
     * the one driving HDMI0 and, with it, the live HVS channel. */
    for (i = 0; i < HVS5_PV_COUNT; i++)
    {
        const struct hvs5_pv *pv = &hvs5_pvs[i];
        ULONG ctrl = pv_rd(pv->pv_Offset, 0x00);

        bug("[VC4HVS5] %s @ 0x%08x (irq %u): "
            "%08x %08x %08x %08x %08x %08x %08x %08x%s\n",
            pv->pv_Name, (unsigned)(ARM_PERIIOBASE + pv->pv_Offset),
            (unsigned)pv->pv_Irq,
            ctrl, pv_rd(pv->pv_Offset, 0x04), pv_rd(pv->pv_Offset, 0x08),
            pv_rd(pv->pv_Offset, 0x0c), pv_rd(pv->pv_Offset, 0x10),
            pv_rd(pv->pv_Offset, 0x14), pv_rd(pv->pv_Offset, 0x18),
            pv_rd(pv->pv_Offset, 0x1c), (ctrl & 1) ? " ENABLED" : "");

        /* +0x20 onwards, where VideoCore IV kept INTEN and INTSTAT. Read
         * only, and the values identify them: an unused INTEN reads 0. */
        if (ctrl & 1)
            bug("[VC4HVS5]   +0x20..: %08x %08x %08x %08x %08x %08x %08x %08x\n",
                pv_rd(pv->pv_Offset, 0x20), pv_rd(pv->pv_Offset, 0x24),
                pv_rd(pv->pv_Offset, 0x28), pv_rd(pv->pv_Offset, 0x2c),
                pv_rd(pv->pv_Offset, 0x30), pv_rd(pv->pv_Offset, 0x34),
                pv_rd(pv->pv_Offset, 0x38), pv_rd(pv->pv_Offset, 0x3c));
    }
#else
    (void)xsd; (void)fb_pitch; (void)fb_width; (void)fb_height;

    bug("[VC4HVS5] BCM2711 - firmware keeps the display, fb 0x%08x "
        "(set VC4_HVS5_DUMP for the bring-up dump, real Pi 4 only)\n",
        fb_phys);
#endif
}

/* ------------------------------------------------------------------ */
/* Phase 2: display list ownership                                     */
/* ------------------------------------------------------------------ */

static void hvs5_write_cursor(struct VideoCoreGfx_staticdata *xsd, ULONG base);
static void hvs5_latch_until(struct vc4_hvs_state *st, ULONG armed);

static inline void hvs5_latch_wait(struct vc4_hvs_state *st)
{
    hvs5_latch_until(st, st->hvs_FlipArmed);
}
static BOOL hvs5_vsync_live(struct vc4_hvs_state *st);
#if VC4_HVS5_VSYNC_IRQ
static void hvs5_vsync_start(struct vc4_hvs_state *st);
#endif

/*
 * Locate the plane carrying a known buffer, without assuming where an
 * entry keeps its pointer: walk the entries and compare every word in
 * each against the address, bus alias masked off. That is how the dump
 * found the framebuffer, and unlike a fixed offset it survives scaled
 * entries, whose pointer sits elsewhere.
 */
static BOOL hvs5_find_plane(ULONG head, ULONG phys, ULONG *entry_off,
                            ULONG *ptr_off, ULONG *entry_words)
{
    ULONG want = phys & 0x3fffffff;
    ULONG idx = head;

    if (!want)
        return FALSE;

    while (idx < HVS5_DLIST_WORDS)
    {
        ULONG ctl0 = hvs5_dl_rd(idx), size, w;

        if (ctl0 & HVS5_CTL0_END)
            return FALSE;

        size = (ctl0 >> HVS5_CTL0_SIZE_SHIFT) & HVS5_CTL0_SIZE_MASK;
        if (size == 0 || idx + size >= HVS5_DLIST_WORDS)
            return FALSE;

        for (w = 1; w < size; w++)
        {
            if ((hvs5_dl_rd(idx + w) & 0x3fffffff) == want)
            {
                *entry_off = idx - head;
                *ptr_off = w;
                *entry_words = size;
                return TRUE;
            }
        }
        idx += size;
    }
    return FALSE;
}

/* One line per plane in a list: what it is, where it points, and the
 * POS0 the firmware chose for it. */
static void hvs5_log_list(ULONG head)
{
    ULONG idx = head;

    while (idx < HVS5_DLIST_WORDS)
    {
        ULONG ctl0 = hvs5_dl_rd(idx), size, pos2;

        if (ctl0 & HVS5_CTL0_END)
            return;

        size = (ctl0 >> HVS5_CTL0_SIZE_SHIFT) & HVS5_CTL0_SIZE_MASK;
        if (size < 4 || idx + size >= HVS5_DLIST_WORDS)
            return;

        pos2 = hvs5_dl_rd(idx + ((size == HVS5_SCALED_WORDS)
                                 ? HVS5_SC_POS2 : 3));
        bug("[VC4HVS5]   plane +%u: %u words fmt %u %ux%u POS0=%08x "
            "[2]=%08x ptr=%08x\n",
            (unsigned)(idx - head), (unsigned)size,
            (unsigned)(ctl0 & HVS5_CTL0_FORMAT_MASK),
            (unsigned)(pos2 & 0xfff), (unsigned)((pos2 >> 16) & 0xfff),
            hvs5_dl_rd(idx + 1), hvs5_dl_rd(idx + 2),
            hvs5_dl_rd(idx + ((size == HVS5_SCALED_WORDS)
                              ? HVS5_SC_PTR0
                              : size - HVS5_PTROFF_FROM_END)));

        /* Raw words too: a firmware-scaled plane is longer than the
         * 8-word entry decoded so far. */
        {
            ULONG i;

            for (i = 0; i < size; i++)
                bug("[VC4HVS5]     [%u] %08x\n",
                    (unsigned)i, hvs5_dl_rd(idx + i));
        }

        idx += size;
    }
}

/* Total length of a list, END word included. 0 = malformed. */
static ULONG hvs5_list_length(ULONG head)
{
    ULONG idx = head;

    while (idx < HVS5_DLIST_WORDS)
    {
        ULONG ctl0 = hvs5_dl_rd(idx), size;

        if (ctl0 & HVS5_CTL0_END)
            return idx - head + 1;

        size = (ctl0 >> HVS5_CTL0_SIZE_SHIFT) & HVS5_CTL0_SIZE_MASK;
        if (size == 0 || idx + size >= HVS5_DLIST_WORDS)
            return 0;
        idx += size;
    }
    return 0;
}

/*
 * Lay down a whole plane entry, invalid but well formed. Every word
 * matters, not just the constant ones: a plane that is not currently
 * shown never gets its position or pointer written, and list RAM is full
 * of stale entries, so anything left unwritten hands the HVS a garbage
 * address and size behind a clear VALID bit. [4] and [6] are context the
 * HVS fills in during scanout.
 */
static void hvs5_init_plane(ULONG base, ULONG alpha_mode, ULONG words)
{
    ULONG i;

    for (i = 0; i < words; i++)
        hvs5_dl_wr(base + i, 0);

    hvs5_dl_wr(base + 0, ((words == HVS5_SCALED_WORDS) ? HVS5_CTL0_SCALED
                                                       : HVS5_CTL0_CURSOR)
                         & ~HVS5_CTL0_VALID);
    hvs5_dl_wr(base + 2, alpha_mode);
}

/* A plane to place: source pixels and where they go. x/y/w/h are in a
 * cw x ch space that fills the fb plane's rectangle - the fb's own size,
 * or a screen's mode size, so every screen fills the display as its own
 * mode would. w/h exceed the source for an upscaled overlay. */
struct hvs5_src
{
    ULONG phys, pitch;
    ULONG pxf, bpp;         /* pxf 0 = the template's 32bpp format */
    ULONG srcw, srch;
    LONG  x, y;
    ULONG w, h;
    ULONG cw, ch;
};

/* The plane's screen rectangle. FALSE when empty. */
static BOOL hvs5_src_rect(struct vc4_hvs_state *st, const struct hvs5_src *s,
                          LONG *dx, LONG *dy, ULONG *dw, ULONG *dh)
{
    if (!s->cw || !s->ch || !s->w || !s->h)
        return FALSE;

    *dx = (LONG)st->hvs_FBX + s->x * (LONG)st->hvs_DestW / (LONG)s->cw;
    *dy = (LONG)st->hvs_FBY + s->y * (LONG)st->hvs_DestH / (LONG)s->ch;
    *dw = s->w * st->hvs_DestW / s->cw;
    *dh = s->h * st->hvs_DestH / s->ch;

    return (*dw != 0) && (*dh != 0);
}

/* Unity or scaled. lbm is the line buffer already handed out this list;
 * a scaled plane that would not fit shows unscaled instead. */
static ULONG hvs5_src_words(struct vc4_hvs_state *st, const struct hvs5_src *s,
                            ULONG lbm)
{
    LONG dx, dy;
    ULONG dw, dh;

    if (!hvs5_src_rect(st, s, &dx, &dy, &dw, &dh))
        return HVS5_PLANE_WORDS;
    if (dw == s->srcw && dh == s->srch)
        return HVS5_PLANE_WORDS;
    if (!HVS5_PPF_FITS(s->srcw, dw) || !HVS5_PPF_FITS(s->srch, dh))
        return HVS5_PLANE_WORDS;
    /* Our LBM grows up from 0; a scaled fb plane's grows down from the top. */
    if (lbm + HVS5_LBM_BYTES(s->srcw) > st->hvs_FBLBM)
        return HVS5_PLANE_WORDS;
    return HVS5_SCALED_WORDS;
}

/* Skip unchanged words: the HVS cannot catch those half written. */
static void hvs5_dl_set(ULONG idx, ULONG val)
{
    if (hvs5_dl_rd(idx) != val)
        hvs5_dl_wr(idx, val);
}

/* Fill in an entry laid down by hvs5_init_plane, clipped to
 * [left,right) x [top,bottom) on screen with the source trimmed in
 * proportion. Left invalid when nothing remains. */
static void hvs5_write_src(struct vc4_hvs_state *st, ULONG base, ULONG words,
                           ULONG lbm, const struct hvs5_src *s,
                           LONG left, LONG top, LONG right, LONG bottom)
{
    BOOL scaled = (words == HVS5_SCALED_WORDS);
    ULONG ctl0 = scaled ? HVS5_CTL0_SCALED : HVS5_CTL0_CURSOR;
    ULONG ptr = s->phys & ~HVS5_PTR_BUS_ALIAS;
    ULONG srcw = s->srcw, srch = s->srch;
    LONG ox, oy;
    ULONG ow, oh;

    if (s->pxf)
        ctl0 = (ctl0 & ~HVS5_CTL0_FORMAT_MASK) | s->pxf;

    if (!hvs5_src_rect(st, s, &ox, &oy, &ow, &oh))
    {
        hvs5_dl_wr(base, ctl0 & ~HVS5_CTL0_VALID);
        return;
    }

    /* Clip on screen and trim the source in proportion. */
    if (ox < left)
    {
        ULONG skip = (ULONG)(left - ox);
        ULONG ssk = skip * srcw / ow;

        ptr += ssk * s->bpp;
        srcw -= ssk;
        ow -= skip;
        ox = left;
    }
    if (oy < top)
    {
        ULONG skip = (ULONG)(top - oy);
        ULONG ssk = skip * srch / oh;

        ptr += ssk * s->pitch;
        srch -= ssk;
        oh -= skip;
        oy = top;
    }
    if (ox + (LONG)ow > right)
    {
        ULONG keep = (right > ox) ? (ULONG)(right - ox) : 0;

        srcw = srcw * keep / ow;
        ow = keep;
    }
    if (oy + (LONG)oh > bottom)
    {
        ULONG keep = (bottom > oy) ? (ULONG)(bottom - oy) : 0;

        srch = srch * keep / oh;
        oh = keep;
    }

    if ((LONG)ow <= 0 || (LONG)oh <= 0 || !srcw || !srch)
    {
        hvs5_dl_wr(base, ctl0 & ~HVS5_CTL0_VALID);
        return;
    }

    hvs5_dl_set(base + 1, HVS5_POS0(ox, oy));

    if (!scaled)
    {
        hvs5_dl_set(base + 3, HVS5_POS2(ow, oh));
        hvs5_dl_set(base + HVS5_PLANE_WORDS - HVS5_PTROFF_FROM_END,
                    HVS5_PTR_BUS_ALIAS | ptr);
        hvs5_dl_set(base + HVS5_PLANE_WORDS - 1, s->pitch);
        hvs5_dl_set(base, ctl0);
        return;
    }

    hvs5_dl_set(base + HVS5_SC_POS1, HVS5_POS2(ow, oh));
    hvs5_dl_set(base + HVS5_SC_POS2, HVS5_POS2(srcw, srch));
    hvs5_dl_set(base + HVS5_SC_PTR0, HVS5_PTR_BUS_ALIAS | ptr);
    hvs5_dl_set(base + HVS5_SC_PITCH, s->pitch);
    hvs5_dl_set(base + HVS5_SC_LBM, lbm);
    hvs5_dl_set(base + HVS5_SC_PPFX, HVS5_PPF(srcw, ow));
    hvs5_dl_set(base + HVS5_SC_PPFY, HVS5_PPF(srch, oh));
    hvs5_dl_set(base + HVS5_SC_KRN0, HVS5_FW_KERNEL);
    hvs5_dl_set(base + HVS5_SC_KRN1, HVS5_FW_KERNEL);

    hvs5_dl_set(base, ctl0);
}

/* The overlay plane over the fb: a GL buffer composited with fixed
 * alpha, since what GL leaves in the alpha channel is undefined. */
static void hvs5_ovl_src(struct vc4_hvs_state *st, struct hvs5_src *s)
{
    s->phys  = st->hvs_OvlPhys;
    s->pitch = st->hvs_OvlPitch;
    s->pxf   = st->hvs_OvlPxf;
    s->bpp   = st->hvs_OvlBpp;
    s->srcw  = st->hvs_OvlW;
    s->srch  = st->hvs_OvlH;
    s->x     = st->hvs_OvlX;
    s->y     = st->hvs_OvlY;
    s->w     = st->hvs_OvlDestW ? st->hvs_OvlDestW : st->hvs_OvlW;
    s->h     = st->hvs_OvlDestH ? st->hvs_OvlDestH : st->hvs_OvlH;
    s->cw    = st->hvs_SrcW;
    s->ch    = st->hvs_SrcH;
}

/* Patch the live overlay entry in place (screens not planes). */
static void hvs5_write_overlay(struct VideoCoreGfx_staticdata *xsd, ULONG base)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    struct hvs5_src s;

    hvs5_ovl_src(st, &s);
    hvs5_write_src(st, base, st->hvs_OvlWords, st->hvs_OvlLBM, &s,
                   (LONG)st->hvs_FBX, (LONG)st->hvs_FBY,
                   (LONG)(st->hvs_FBX + st->hvs_DestW),
                   (LONG)(st->hvs_FBY + st->hvs_DestH));
}

/* Append a plane at word n of the list; returns the new length. */
static ULONG hvs5_emit_src(struct vc4_hvs_state *st, ULONG base, ULONG n,
                           const struct hvs5_src *s, ULONG *lbm,
                           LONG left, LONG top, LONG right, LONG bottom,
                           ULONG *words_out, ULONG *lbm_out)
{
    ULONG words = hvs5_src_words(st, s, *lbm);
    ULONG at = 0;

    if (words == HVS5_SCALED_WORDS)
    {
        at = *lbm;
        *lbm += HVS5_LBM_BYTES(s->srcw);
    }
    hvs5_init_plane(base + n, HVS5_ALPHA_FIXED, words);
    hvs5_write_src(st, base + n, words, at, s, left, top, right, bottom);

    if (words_out)
        *words_out = words;
    if (lbm_out)
        *lbm_out = at;
    return n + words;
}

/* Our own buffer behind a token firmware framebuffer, filling the fb
 * plane's rectangle. */
static void hvs5_fb_src(struct vc4_hvs_state *st, struct hvs5_src *s)
{
    s->phys  = st->hvs_FBPtr;
    s->pitch = st->hvs_FBPitch;
    s->pxf   = 0;
    s->bpp   = 4;
    s->srcw  = s->w = s->cw = st->hvs_SrcW;
    s->srch  = s->h = s->ch = st->hvs_SrcH;
    s->x     = 0;
    s->y     = 0;
}

/* The fb plane, verbatim but for its pointer - or our own buffer. */
static ULONG hvs5_emit_fb(struct vc4_hvs_state *st, ULONG base, ULONG n, ULONG *lbm)
{
    ULONG i;

    if (st->hvs_Tiny)
    {
        struct hvs5_src s;

        hvs5_fb_src(st, &s);
        return hvs5_emit_src(st, base, n, &s, lbm,
                             (LONG)st->hvs_FBX, (LONG)st->hvs_FBY,
                             (LONG)(st->hvs_FBX + st->hvs_DestW),
                             (LONG)(st->hvs_FBY + st->hvs_DestH), NULL, NULL);
    }

    for (i = 0; i < st->hvs_FBWords; i++)
        hvs5_dl_wr(base + n + i, st->hvs_FBEntry[i]);
    hvs5_dl_wr(base + n + st->hvs_FBPtrOff, HVS5_PTR_BUS_ALIAS | st->hvs_FBPtr);
    return n + st->hvs_FBWords;
}

/* A screen as a plane, in its own mode's coordinates. */
static void hvs5_plane_src(struct vcgfx_plane *pl, struct hvs5_src *s)
{
    struct BitmapData *d = pl->pl_Data;

    s->phys  = (ULONG)(IPTR)d->VideoData;
    s->pitch = d->bytesperrow;
    s->pxf   = 0;
    s->bpp   = 4;
    s->srcw  = s->w = d->width;
    s->srch  = s->h = d->height;
    s->x     = pl->pl_X;
    s->y     = pl->pl_Y;
    s->cw    = d->modew;
    s->ch    = d->modeh;
}

/* A pool overlay over the screen at orgx/orgy, in that screen's space. */
static void hvs5_pool_src(const struct vcgfx_ovl *e, struct hvs5_src *s,
                          LONG orgx, LONG orgy, ULONG cw, ULONG ch)
{
    const struct vc4gfx_overlay *d = &e->ov_Desc;

    s->phys  = d->ovl_Phys;
    s->pitch = d->ovl_Pitch;
    s->pxf   = vc4_ovl_pxf(d->ovl_Flags);
    s->bpp   = vc4_ovl_bpp(d->ovl_Flags);
    s->srcw  = d->ovl_Width;
    s->srch  = d->ovl_Height;
    s->x     = orgx + d->ovl_X;
    s->y     = orgy + d->ovl_Y;
    s->w     = d->ovl_DestW ? d->ovl_DestW : d->ovl_Width;
    s->h     = d->ovl_DestH ? d->ovl_DestH : d->ovl_Height;
    s->cw    = cw;
    s->ch    = ch;
}

/* Words and line buffer for a plane, added to the running totals. */
static ULONG hvs5_take(struct vc4_hvs_state *st, const struct hvs5_src *s, ULONG *lbm)
{
    ULONG words = hvs5_src_words(st, s, *lbm);

    if (words == HVS5_SCALED_WORDS)
        *lbm += HVS5_LBM_BYTES(s->srcw);
    return words;
}

/*
 * The screens, back to front, each cut off where the screens in front
 * hide it: one spanning the display's width down to the bottom of what is
 * still visible covers everything behind it below its top edge. Worked out
 * on screen, as every screen scales to fill the display like its own mode.
 * The fb, kept black, is the backdrop where no screen covers. A screen's
 * overlays sit right above it.
 *
 * The slot and the line buffer go to the screens first, as they would
 * without overlays, then to the overlays, front screen first. A plane
 * placed back to front then never finds less than this pass left it.
 */
static ULONG hvs5_emit_planes(struct VideoCoreGfx_staticdata *xsd,
                              ULONG base, ULONG n, ULONG *lbm)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    LONG limit[VCGFX_MAX_PLANES];
    BOOL inlist[VCGFX_MAX_OVERLAYS];
    LONG left = (LONG)st->hvs_FBX, right = left + (LONG)st->hvs_DestW;
    LONG top = (LONG)st->hvs_FBY, bottom = top + (LONG)st->hvs_DestH;
    ULONG room = HVS5_OWN_SLOT_STRIDE - n - 1, used = 0, need = 0, words, k;
    struct hvs5_src s, o;
    LONG i, px, py, dx, dy;
    ULONG pw, ph, dw, dh;

    for (i = (LONG)xsd->vcsd_NumPlanes - 1; i >= 0; i--)
    {
        limit[i] = bottom;
        hvs5_plane_src(&xsd->vcsd_Plane[i], &s);
        if (hvs5_src_rect(st, &s, &px, &py, &pw, &ph))
            bottom = vcgfx_plane_occlude(px, py, pw, ph, left, top, right, bottom);
    }

    if (xsd->vcsd_CurBuf)
        room -= HVS5_PLANE_WORDS;
    if (bottom > top)
    {
        hvs5_fb_src(st, &s);
        room -= st->hvs_Tiny ? hvs5_take(st, &s, &need) : st->hvs_FBWords;
    }
    for (i = 0; i < (LONG)xsd->vcsd_NumPlanes; i++)
    {
        hvs5_plane_src(&xsd->vcsd_Plane[i], &s);
        if (hvs5_src_rect(st, &s, &px, &py, &pw, &ph) && (limit[i] > py) && (limit[i] > top))
            room -= hvs5_take(st, &s, &need);
    }

    for (k = 0; k < VCGFX_MAX_OVERLAYS; k++)
    {
        inlist[k] = FALSE;
        xsd->vcsd_Ovl[k].ov_Shown = FALSE;
    }
    for (i = (LONG)xsd->vcsd_NumPlanes - 1; i >= 0; i--)
    {
        struct vcgfx_plane *pl = &xsd->vcsd_Plane[i];
        BOOL vis;

        hvs5_plane_src(pl, &s);
        vis = hvs5_src_rect(st, &s, &px, &py, &pw, &ph) && (limit[i] > py) && (limit[i] > top);

        for (k = 0; k < VCGFX_MAX_OVERLAYS; k++)
        {
            struct vcgfx_ovl *e = &xsd->vcsd_Ovl[k];
            ULONG lb = need;

            if (!e->ov_Client || (e->ov_BM != pl->pl_BM))
                continue;

            /* A size the scaler can't make is left out, not shown wrong */
            hvs5_pool_src(e, &o, pl->pl_X, pl->pl_Y, s.cw, s.ch);
            words = hvs5_take(st, &o, &lb);
            if (hvs5_src_rect(st, &o, &dx, &dy, &dw, &dh)
                && ((dw != o.srcw) || (dh != o.srch)) && (words != HVS5_SCALED_WORDS))
                continue;

            /* Its screen hidden: taken, with nothing of it to show */
            if (!vis)
            {
                e->ov_Shown = TRUE;
                continue;
            }
            if (used + words > room)
                continue;

            used += words;
            need = lb;
            e->ov_Shown = inlist[k] = TRUE;
        }
    }

    if (bottom > top)
        n = hvs5_emit_fb(st, base, n, lbm);

    for (i = 0; i < (LONG)xsd->vcsd_NumPlanes; i++)
    {
        struct vcgfx_plane *pl = &xsd->vcsd_Plane[i];
        LONG cb = limit[i];
        LONG ol, ot, orr, ob;

        hvs5_plane_src(pl, &s);
        if (!hvs5_src_rect(st, &s, &px, &py, &pw, &ph) || cb <= py || cb <= top)
            continue;

        n = hvs5_emit_src(st, base, n, &s, lbm, left, top, right, cb, NULL, NULL);

        /* No further out than the screen itself. */
        ol  = (px > left) ? px : left;
        ot  = (py > top) ? py : top;
        orr = (px + (LONG)pw < right) ? px + (LONG)pw : right;
        ob  = (py + (LONG)ph < cb) ? py + (LONG)ph : cb;

        for (k = 0; k < VCGFX_MAX_OVERLAYS; k++)
        {
            if (!inlist[k] || (xsd->vcsd_Ovl[k].ov_BM != pl->pl_BM))
                continue;
            hvs5_pool_src(&xsd->vcsd_Ovl[k], &o, pl->pl_X, pl->pl_Y, s.cw, s.ch);
            n = hvs5_emit_src(st, base, n, &o, lbm, ol, ot, orr, ob, NULL, NULL);
        }
    }
    return n;
}

/* A slot nothing can be reading: not the one scanned, not the last one
 * repointed at, not the one waiting for vblank. The pending list is read
 * first - the interrupt records it as the last before it lets go of it. */
static ULONG hvs5_free_slot(struct vc4_hvs_state *st)
{
    ULONG pend = __atomic_load_n(&st->hvs_PendList, __ATOMIC_ACQUIRE);
    ULONG last = __atomic_load_n(&st->hvs_LastList, __ATOMIC_ACQUIRE);
    ULONG act  = hvs5_rd(HVS5_DISPLACT(st->hvs_Channel));
    ULONG i, base = HVS5_OWN_SLOT_BASE;

    for (i = 0; i < HVS5_OWN_SLOTS; i++)
    {
        base = HVS5_OWN_SLOT_BASE + HVS5_OWN_SLOT_STRIDE * st->hvs_Slot;
        st->hvs_Slot = (st->hvs_Slot + 1) % HVS5_OWN_SLOTS;
        if (base != pend && base != last && base != act)
            break;
    }
    return base;
}

/*
 * Compose a list into a free slot: the framebuffer plane (or the screens,
 * once they are planes), then the overlay if one is up, then the cursor,
 * then END.
 *
 * The returned head goes to DISPLIST through hvs5_commit(), or directly
 * where a mode set needs it. Steady-state updates that keep the shape of
 * the list patch the live one instead.
 */
static ULONG hvs5_build_list(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    ULONG base = hvs5_free_slot(st);
    ULONG n = 0, lbm = 0;

    st->hvs_OvlOff = 0;
    st->hvs_OvlWords = 0;

    if (xsd->vcsd_PlanesMode)
        n = hvs5_emit_planes(xsd, base, n, &lbm);
    else
    {
        n = hvs5_emit_fb(st, base, n, &lbm);
        if (st->hvs_OvlActive)
        {
            struct hvs5_src s;

            hvs5_ovl_src(st, &s);
            st->hvs_OvlOff = n;
            n = hvs5_emit_src(st, base, n, &s, &lbm,
                              (LONG)st->hvs_FBX, (LONG)st->hvs_FBY,
                              (LONG)(st->hvs_FBX + st->hvs_DestW),
                              (LONG)(st->hvs_FBY + st->hvs_DestH),
                              &st->hvs_OvlWords, &st->hvs_OvlLBM);
        }
    }

    /* Authored whenever a buffer exists, visible or not, so VALID alone
     * carries visibility and showing the pointer never reshapes the list. */
    st->hvs_CurOff = 0;
    if (xsd->vcsd_CurBuf)
    {
        st->hvs_CurOff    = n;
        st->hvs_CurWords  = HVS5_PLANE_WORDS;
        st->hvs_CurPtrOff = HVS5_PLANE_WORDS - HVS5_PTROFF_FROM_END;
        hvs5_init_plane(base + n, HVS5_ALPHA_PERPIXEL, HVS5_PLANE_WORDS);
        hvs5_write_cursor(xsd, base + n);
        n += HVS5_PLANE_WORDS;
    }

    hvs5_dl_wr(base + n++, HVS5_CTL0_END);
    st->hvs_ListBase = base;
    return base;
}

/* DISPLIST is about to be written outside hvs5_commit(): nothing may stay
 * queued behind it. */
static void hvs5_note_list(struct vc4_hvs_state *st, ULONG base)
{
    __atomic_store_n(&st->hvs_PendList, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&st->hvs_LastList, base, __ATOMIC_RELEASE);
}

/* Repoint at a built list. A repoint takes effect mid-scanout, so while
 * the vblank interrupt runs it does the write; otherwise now, torn. */
static void hvs5_commit(struct vc4_hvs_state *st, ULONG base)
{
    if (hvs5_vsync_live(st))
    {
        __atomic_store_n(&st->hvs_PendList, base, __ATOMIC_RELEASE);
        return;
    }
    hvs5_note_list(st, base);
    hvs5_wr(HVS5_DISPLIST(st->hvs_Channel), base);
}

BOOL vc4_hvs5_takeover(struct VideoCoreGfx_staticdata *xsd,
                       ULONG fb_phys, ULONG fb_pitch)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    st->hvs_Active = FALSE;
    hvs5_note_list(st, 0);
    (void)fb_pitch;

#if VC4_HVS5_TAKEOVER
    {
        ULONG ctrl, head, len, base, i, ch;
        ULONG fb_entry, fb_ptr, fb_words;

        if (hvs5_rd(HVS5_ID) != HVS5_ID_MAGIC)
            return FALSE;

        /* Enabled with a plausible head is not enough: the test is that
         * the list points at our framebuffer, which also hands us the
         * plane to inherit. */
        for (ch = 0; ch < HVS5_CHANNELS; ch++)
        {
            ctrl = hvs5_rd(HVS5_DISPCTRLX(ch));
            head = hvs5_rd(HVS5_DISPLACT(ch));

            if (!(ctrl & (1UL << 31)) || head == 0
                || head >= HVS5_DLIST_WORDS)
                continue;
            if (hvs5_find_plane(head, fb_phys, &fb_entry, &fb_ptr, &fb_words))
                break;
        }

        if (ch == HVS5_CHANNELS)
        {
            for (i = 0; i < HVS5_CHANNELS; i++)
                bug("[VC4HVS5] takeover: channel %u CTRL=%08x head=%u\n",
                    (unsigned)i, hvs5_rd(HVS5_DISPCTRLX(i)),
                    (unsigned)hvs5_rd(HVS5_DISPLACT(i)));
            bug("[VC4HVS5] takeover: no channel scans out fb 0x%08x\n",
                fb_phys);
            return FALSE;
        }

        st->hvs_Channel = ch;
        head = hvs5_rd(HVS5_DISPLACT(ch));

        len = hvs5_list_length(head);
        if (len == 0 || len > HVS5_OWN_SLOT_STRIDE)
        {
            bug("[VC4HVS5] takeover: list at %u is %u words, refusing\n",
                (unsigned)head, (unsigned)len);
            return FALSE;
        }

        /* Keep the framebuffer entry, so the list can be rebuilt later
         * without the firmware's copy still being there to read. */
        if (fb_words > HVS_FB_ENTRY_MAX)
        {
            bug("[VC4HVS5] takeover: fb entry is %u words, too long\n",
                (unsigned)fb_words);
            return FALSE;
        }
        for (i = 0; i < fb_words; i++)
            st->hvs_FBEntry[i] = hvs5_dl_rd(head + fb_entry + i);

        st->hvs_FBPtr    = fb_phys & ~HVS5_PTR_BUS_ALIAS;
        st->hvs_FBPtrOff = fb_ptr;
        st->hvs_FBWords  = fb_words;

        /* The framebuffer plane is what the other planes sit on: its
         * destination rectangle is their clip region and their origin. A
         * mode below the panel's own gets centred and scaled by the
         * firmware, so anything positioned in fb coords has to be mapped
         * through that rectangle. */
        {
            ULONG ctl0 = st->hvs_FBEntry[0];
            ULONG pos0 = st->hvs_FBEntry[1];

            st->hvs_FBX = pos0 & 0xffff;
            st->hvs_FBY = (pos0 >> 16) & 0xffff;

            if (ctl0 & HVS5_CTL0_UNITY)
            {
                ULONG pos2 = st->hvs_FBEntry[3];

                st->hvs_SrcW  = pos2 & 0xffff;
                st->hvs_SrcH  = (pos2 >> 16) & 0xffff;
                st->hvs_DestW = st->hvs_SrcW;
                st->hvs_DestH = st->hvs_SrcH;
            }
            else
            {
                ULONG pos1 = st->hvs_FBEntry[3];
                ULONG pos2 = st->hvs_FBEntry[4];

                st->hvs_DestW = pos1 & 0xffff;
                st->hvs_DestH = (pos1 >> 16) & 0xffff;
                st->hvs_SrcW  = pos2 & 0xffff;
                st->hvs_SrcH  = (pos2 >> 16) & 0xffff;
            }

            if (!st->hvs_SrcW || !st->hvs_SrcH
                || !st->hvs_DestW || !st->hvs_DestH)
            {
                bug("[VC4HVS5] takeover: fb plane geometry %ux%u -> %ux%u"
                    " at %u,%u makes no sense, refusing\n",
                    (unsigned)st->hvs_SrcW, (unsigned)st->hvs_SrcH,
                    (unsigned)st->hvs_DestW, (unsigned)st->hvs_DestH,
                    (unsigned)st->hvs_FBX, (unsigned)st->hvs_FBY);
                return FALSE;
            }
        }

        /* A scaled fb plane's LBM base is the ceiling for ours. */
        st->hvs_OvlActive = FALSE;
        st->hvs_OvlUsable = (fb_words == HVS5_PLANE_WORDS)
                         || (fb_words == HVS5_SCALED_WORDS);
        st->hvs_FBLBM = (fb_words == HVS5_SCALED_WORDS)
                      ? st->hvs_FBEntry[HVS5_SC_LBM] : HVS5_LBM_TOP;
        st->hvs_OvlLBM = 0;

        /* Behind a token framebuffer the firmware's plane only gives the
         * rectangle; the mode's size and our buffer go in it. */
        if (st->hvs_Tiny)
        {
            st->hvs_SrcW      = st->hvs_ShowW;
            st->hvs_SrcH      = st->hvs_ShowH;
            st->hvs_FBPtr     = st->hvs_ShowPtr;
            st->hvs_FBPitch   = st->hvs_ShowPitch;
            st->hvs_OvlUsable = TRUE;
            st->hvs_FBLBM     = HVS5_LBM_TOP;
        }

        /* Planes may be committing from another task. */
        VC4_MBOX_LOCK(xsd);
        base = hvs5_build_list(xsd);

        hvs5_log_list(base);

        if (!st->hvs_OvlUsable)
        {
            ULONG i;

            bug("[VC4HVS5] fb plane is scaled: %u words, kernel at +%03x:\n",
                (unsigned)st->hvs_FBWords, (unsigned)HVS5_FW_KERNEL);
            for (i = 0; i < 16; i++)
                bug("[VC4HVS5]   krn[%u] %08x\n", (unsigned)i,
                    hvs5_dl_rd(HVS5_FW_KERNEL + i));
        }

        hvs5_note_list(st, base);
        hvs5_wr(HVS5_DISPLIST(st->hvs_Channel), base);
        for (i = 0; i < HVS5_SPIN_LATCH; i++)
        {
            if (hvs5_rd(HVS5_DISPLACT(st->hvs_Channel)) == base)
                break;
        }
        if (hvs5_rd(HVS5_DISPLACT(st->hvs_Channel)) == base)
            st->hvs_Active = TRUE;
        else
            hvs5_wr(HVS5_DISPLIST(st->hvs_Channel), head);
        VC4_MBOX_UNLOCK(xsd);

        if (st->hvs_Active)
        {
            st->hvs_FlipArmed = st->hvs_VSyncCount;     /* nothing pending */
#if VC4_HVS5_VSYNC_IRQ
            hvs5_vsync_start(st);
#endif
        }

        if (st->hvs_Active)
            bug("[VC4HVS5] takeover: ACTIVE - %u words at %u (from %u), "
                "fb plane ptr +%u, cursor +%u, fb %ux%u -> %ux%u at %u,%u\n",
                (unsigned)(fb_words + (st->hvs_CurOff ? HVS5_PLANE_WORDS : 0)
                           + 1),
                (unsigned)base, (unsigned)head, (unsigned)fb_ptr,
                (unsigned)st->hvs_CurOff,
                (unsigned)st->hvs_SrcW, (unsigned)st->hvs_SrcH,
                (unsigned)st->hvs_DestW, (unsigned)st->hvs_DestH,
                (unsigned)st->hvs_FBX, (unsigned)st->hvs_FBY);
        else
            bug("[VC4HVS5] takeover: never latched, firmware restored\n");
    }
#else
    (void)fb_phys;
#endif

    return st->hvs_Active;
}

/*
 * After a takeover: if w x h at the rate in want is a mode we can set,
 * set it, showing the framebuffer as a unity plane at 0,0 - whatever the
 * firmware had scaled it into.
 */
BOOL vc4_hvs5_mode_apply(struct VideoCoreGfx_staticdata *xsd, ULONG w, ULONG h,
                         ULONG pitch, const struct vcgfx_timing *want)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    const struct vcgfx_timing *t;
    ULONG base;
    BOOL boot, ok;

    if (!st->hvs_Active || !(t = vc4_hvs5_mode_lookup(xsd, w, h, want, &boot)) || boot)
        return FALSE;
    if (pitch < w * 4)
    {
        bug("[VC4HVS5] fb pitch %u is short of %ux%u - firmware clamped it?\n",
            (unsigned)pitch, (unsigned)w, (unsigned)h);
        return FALSE;
    }

    st->hvs_FBEntry[0] = HVS5_CTL0_CURSOR;
    st->hvs_FBEntry[1] = HVS5_POS0(0, 0);
    st->hvs_FBEntry[2] = HVS5_ALPHA_FIXED;
    st->hvs_FBEntry[3] = HVS5_POS2(w, h);
    st->hvs_FBEntry[4] = 0;
    st->hvs_FBEntry[5] = 0;
    st->hvs_FBEntry[6] = 0;
    st->hvs_FBEntry[7] = pitch;
    st->hvs_FBWords    = HVS5_PLANE_WORDS;
    st->hvs_FBPtrOff   = HVS5_PLANE_WORDS - HVS5_PTROFF_FROM_END;
    st->hvs_FBX        = 0;
    st->hvs_FBY        = 0;
    st->hvs_SrcW       = st->hvs_DestW = w;
    st->hvs_SrcH       = st->hvs_DestH = h;
    st->hvs_OvlUsable  = TRUE;
    st->hvs_FBLBM      = HVS5_LBM_TOP;
    st->hvs_FBPitch    = pitch;

    VC4_MBOX_LOCK(xsd);
    base = hvs5_build_list(xsd);
    hvs5_note_list(st, base);
    ok = vc4_hvs5_mode_set(xsd, st->hvs_Channel, base, t, FALSE);
    VC4_MBOX_UNLOCK(xsd);
    return ok;
}

/*
 * A new mode behind a token framebuffer: our buffer of w x h as a unity
 * plane in our own list, and the mode set by us - the boot mode too. The
 * firmware builds no list once a mode of ours has been on, so it is never
 * asked again (BCM2712 works like this throughout). FALSE, with nothing
 * changed, when w x h is not a mode we set.
 */
BOOL vc4_hvs5_mode_switch(struct VideoCoreGfx_staticdata *xsd, ULONG w, ULONG h,
                          ULONG fb_phys, ULONG pitch, const struct vcgfx_timing *want)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    const struct vcgfx_timing *t, *cur = &xsd->vcsd_HVS5M.m5_CurTiming;
    ULONG base;
    BOOL boot;

    if (!st->hvs_Active || !st->hvs_Tiny || (pitch < w * 4)
        || !(t = vc4_hvs5_mode_lookup(xsd, w, h, want, &boot)))
        return FALSE;

    st->hvs_ShowPtr   = fb_phys;
    st->hvs_ShowPitch = pitch;
    st->hvs_ShowW     = w;
    st->hvs_ShowH     = h;
    st->hvs_FBPtr     = fb_phys;
    st->hvs_FBPitch   = pitch;
    st->hvs_FBX       = 0;
    st->hvs_FBY       = 0;
    st->hvs_SrcW      = st->hvs_DestW = w;
    st->hvs_SrcH      = st->hvs_DestH = h;
    st->hvs_OvlUsable = TRUE;
    st->hvs_FBLBM     = HVS5_LBM_TOP;

    VC4_MBOX_LOCK(xsd);
    base = hvs5_build_list(xsd);
    if ((t->hdisp == cur->hdisp) && (t->vdisp == cur->vdisp) && (t->clock == cur->clock))
    {
        /* Same timing: only the buffer changes, at the next vblank */
        hvs5_commit(st, base);
        st->hvs_FlipArmed = st->hvs_VSyncCount + 1;
        hvs5_latch_wait(st);
    }
    else
    {
        /* A list that never latches is logged there; the buffer is in use */
        hvs5_note_list(st, base);
        vc4_hvs5_mode_set(xsd, st->hvs_Channel, base, t, boot);
    }
    VC4_MBOX_UNLOCK(xsd);
    return TRUE;
}

BOOL vc4_hvs5_flip_page(struct VideoCoreGfx_staticdata *xsd, ULONG page_phys)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    /* With screens as planes the fb is only the backdrop. */
    if (!st->hvs_Active || xsd->vcsd_PlanesMode)
        return FALSE;

    /* One word, reloaded by the HVS at frame start, so the flip is
     * vblank-latched without a repoint. Unpaced until the PixelValve
     * interrupt is wired up. */
    st->hvs_FBPtr = page_phys & ~HVS5_PTR_BUS_ALIAS;
    hvs5_dl_wr(st->hvs_ListBase + st->hvs_FBPtrOff,
               HVS5_PTR_BUS_ALIAS | st->hvs_FBPtr);
    st->hvs_FlipArmed = st->hvs_VSyncCount + 1;

    hvs5_latch_wait(st);
    return TRUE;
}

/*
 * Position, size and pointer of our cursor plane. Split out from the
 * public entry point so the takeover can prime the entry before the
 * channel is ours and hvs_Active is still FALSE.
 *
 * vcsd_CurX/Y are fb coords with the hotspot applied, while POS0 is in
 * output coords, so the position is mapped through the fb plane's
 * destination rectangle - origin plus scale. The plane itself stays
 * unity-sized, like the firmware cursor, so only the position scales.
 * The coords go negative past the top and left edges, so clip by walking
 * into the image and shrinking the plane - a 16-bit position field would
 * read a negative value as 65535.
 */
static void hvs5_write_cursor(struct VideoCoreGfx_staticdata *xsd, ULONG base)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    LONG left = (LONG)st->hvs_FBX, top = (LONG)st->hvs_FBY;
    LONG right = left + (LONG)st->hvs_DestW;
    LONG bottom = top + (LONG)st->hvs_DestH;
    LONG cx = left + xsd->vcsd_CurX * (LONG)st->hvs_DestW / (LONG)st->hvs_SrcW;
    LONG cy = top + xsd->vcsd_CurY * (LONG)st->hvs_DestH / (LONG)st->hvs_SrcH;
    LONG cw = xsd->vcsd_CurWidth, ch = xsd->vcsd_CurHeight;
    ULONG pitch = xsd->vcsd_CurWidth * 4;
    ULONG ptr = xsd->vcsd_CurBufBus & ~HVS5_PTR_BUS_ALIAS;

    if (cx < left) { ptr += (ULONG)(left - cx) * 4;     cw -= left - cx; cx = left; }
    if (cy < top)  { ptr += (ULONG)(top - cy) * pitch;  ch -= top - cy;  cy = top; }
    if (cx + cw > right)  cw = right - cx;
    if (cy + ch > bottom) ch = bottom - cy;

    if (!xsd->vcsd_CurVisible || cw <= 0 || ch <= 0)
    {
        /* Report the transition only: whichever of these two reasons hides
         * the pointer, it is worth knowing which, and neither should happen
         * while it is sitting in the middle of the screen. */
        if (st->hvs_CurShown)
            bug("[VC4HVS5] cursor hidden: %s, fb %d,%d -> %d,%d size %dx%d"
                " in %ux%u at %u,%u\n",
                xsd->vcsd_CurVisible ? "clipped away" : "not visible",
                (int)xsd->vcsd_CurX, (int)xsd->vcsd_CurY, (int)cx, (int)cy,
                (int)cw, (int)ch,
                (unsigned)st->hvs_DestW, (unsigned)st->hvs_DestH,
                (unsigned)st->hvs_FBX, (unsigned)st->hvs_FBY);
        st->hvs_CurShown = FALSE;
        hvs5_dl_wr(base, HVS5_CTL0_CURSOR & ~HVS5_CTL0_VALID);
        return;
    }

    hvs5_dl_wr(base + 1, HVS5_POS0(cx, cy));
    hvs5_dl_wr(base + 3, HVS5_POS2(cw, ch));
    hvs5_dl_wr(base + st->hvs_CurPtrOff, HVS5_PTR_BUS_ALIAS | ptr);
    hvs5_dl_wr(base + st->hvs_CurWords - 1, pitch);

    /* VALID last: the HVS must never see a half-written entry. Written
     * whole rather than OR-ed in - CTL0 is a word the HVS itself touches
     * during scanout, so a read-modify-write races with it. */
    hvs5_dl_wr(base, HVS5_CTL0_CURSOR);
    st->hvs_CurShown = TRUE;
}

BOOL vc4_hvs5_overlay(struct VideoCoreGfx_staticdata *xsd,
                      const struct vc4gfx_overlay *ovl, OOP_Object *owner)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    struct hvs5_src s;
    BOOL structural;

    /* Screens as planes place their overlays themselves (vcgfx_ovl_show). */
    if (!st->hvs_Active || !st->hvs_OvlUsable)
    {
        /* Once: which precondition failed decides the fix (takeover
         * failure vs a firmware-scaled fb plane). */
        static BOOL said = FALSE;

        if (!said && ovl)
        {
            said = TRUE;
            bug("[VC4HVS5] overlay refused: %s (fb entry %u words,"
                " lbm floor %u)\n",
                !st->hvs_Active ? "no display list takeover"
                                : "fb plane shape not understood",
                (unsigned)st->hvs_FBWords, (unsigned)st->hvs_FBLBM);
        }
        return FALSE;
    }

    VC4_MBOX_LOCK(xsd);

    if (!ovl)
    {
        if (st->hvs_OvlActive)
        {
            st->hvs_OvlActive = FALSE;
            hvs5_commit(st, hvs5_build_list(xsd));
            /* The hidden page scans until this latches: arm it so the
             * caller's LatchWait covers the clear. */
            st->hvs_FlipArmed = st->hvs_VSyncCount + 1;
        }
        VC4_MBOX_UNLOCK(xsd);
        return TRUE;
    }

    /* Without planes the entry is 32bpp, scaled only like the fb */
    if (vc4_ovl_bpp(ovl->ovl_Flags) != 4)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }

    structural = !st->hvs_OvlActive || !st->hvs_OvlOff
              || st->hvs_OvlW != ovl->ovl_Width
              || st->hvs_OvlH != ovl->ovl_Height;

    st->hvs_OvlActive = TRUE;
    /* Set before hvs5_src_words(), which reads them. */
    st->hvs_OvlPhys  = ovl->ovl_Phys & ~HVS5_PTR_BUS_ALIAS;
    st->hvs_OvlPitch = ovl->ovl_Pitch;
    st->hvs_OvlW     = ovl->ovl_Width;
    st->hvs_OvlH     = ovl->ovl_Height;
    st->hvs_OvlDestW = ovl->ovl_Width;
    st->hvs_OvlDestH = ovl->ovl_Height;
    st->hvs_OvlPxf   = vc4_ovl_pxf(ovl->ovl_Flags);
    st->hvs_OvlBpp   = vc4_ovl_bpp(ovl->ovl_Flags);
    st->hvs_OvlX     = ovl->ovl_X;
    st->hvs_OvlY     = ovl->ovl_Y;

    /* Unity <-> scaled moves the words: rebuild, don't patch. */
    hvs5_ovl_src(st, &s);
    if (hvs5_src_words(st, &s, 0) != st->hvs_OvlWords)
        structural = TRUE;

    if (structural)
        hvs5_commit(st, hvs5_build_list(xsd));
    else
        hvs5_write_overlay(xsd, st->hvs_ListBase + st->hvs_OvlOff);

    /* The buffer just replaced stays on screen until this latches, so
     * pace the producer exactly like a page flip, unless deferred. */
    st->hvs_FlipArmed = st->hvs_VSyncCount + 1;
    if (!(ovl->ovl_Flags & VC4GFX_OVL_NOWAIT))
        hvs5_latch_wait(st);

    VC4_MBOX_UNLOCK(xsd);
    return TRUE;
}

void vc4_hvs5_latch_wait(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    if (st->hvs_Active)
        hvs5_latch_wait(st);
}

/* Until the commit that armed `armed` has latched; later commits by
 * other clients do not move it. */
void vc4_hvs5_latch_until(struct VideoCoreGfx_staticdata *xsd, ULONG armed)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    if (st->hvs_Active)
        hvs5_latch_until(st, armed);
}

void vc4_hvs5_update_cursor(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    if (!st->hvs_Active || !st->hvs_CurOff)
        return;

    hvs5_write_cursor(xsd, st->hvs_ListBase + st->hvs_CurOff);
}

/* ------------------------------------------------------------------ */
/* Phase 3: vblank pacing                                              */
/* ------------------------------------------------------------------ */

/*
 * Timings are wall clock, read off the 1 MHz system timer, not spin
 * counts. The VideoCore IV probe counted uncached register reads, and a
 * Pi 4 gets through 100000 of them in about a millisecond - enough to see
 * the line-rate sources but not one whole frame, so the ratio test gave
 * up. The same mistake would have left the flip wait timing out well
 * inside a single 60 Hz frame.
 */
#define HVS5_PROBE_US       100000  /* per bit: ~6 frames, ~6400 lines */
#define HVS5_VERIFY_US      200000  /* long enough for the 5 ticks below */
#define HVS5_FLIP_US        50000   /* ~3 frames, then give up on pacing */
#define HVS5_LIVE_US        40000   /* IRQ seen this recently: may sleep */

static inline ULONG hvs5_now_us(void)
{
    return AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);
}

static inline void pv_wr(ULONG base_off, ULONG offset, ULONG value)
{
    *(volatile ULONG *)(ARM_PERIIOBASE + base_off + offset) = value;
}

#if VC4_HVS5_VSYNC_IRQ
/* IRQ context: count only, no printing. Some pixelvalves share a GIC line
 * (PV1 and PV4 both sit on SPI 110), and the other one is disabled and
 * masked, so a status of zero is not ours. */
static void hvs5_vsync_irq(struct vc4_hvs_state *st, struct ExecBase *sysBase)
{
    ULONG stat = pv_rd(st->hvs_PVOffset, HVS5_PV_INTSTAT);
    struct vc4gfx_vblank *vbl;

    if (stat)
    {
        pv_wr(st->hvs_PVOffset, HVS5_PV_INTSTAT, stat);   /* W1C */
        if (stat & st->hvs_VSyncMask)
        {
            ULONG pend = __atomic_load_n(&st->hvs_PendList, __ATOMIC_ACQUIRE);

            /* Recorded as the last list before it stops being pending, so
             * hvs5_free_slot() never sees it as neither. */
            if (pend && st->hvs_Active)
            {
                __atomic_store_n(&st->hvs_LastList, pend, __ATOMIC_RELEASE);
                hvs5_wr(HVS5_DISPLIST(st->hvs_Channel), pend);
                __atomic_compare_exchange_n(&st->hvs_PendList, &pend, 0, FALSE,
                                            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
            }

            st->hvs_VSyncCount++;
            st->hvs_VSyncStamp = hvs5_now_us();
            if (st->hvs_VSyncTask)
                Signal(st->hvs_VSyncTask, st->hvs_VSyncSigMask);
            if ((vbl = st->hvs_VBlank))
                vbl->vbl_Func(vbl->vbl_Data);
        }
    }
}

void vc4_hvs5_irq_init(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    ULONG i;

    /* Only an enabled pixelvalve carries timing, and which one that is
     * follows the mode the firmware set - so ask, rather than assume. */
    for (i = 0; i < HVS5_PV_COUNT; i++)
    {
        if (!(pv_rd(hvs5_pvs[i].pv_Offset, HVS5_PV_CONTROL) & 1))
            continue;
        st->hvs_PVOffset = hvs5_pvs[i].pv_Offset;
        st->hvs_PVIrq    = hvs5_pvs[i].pv_Irq;
        bug("[VC4HVS5] vsync: %s drives the display (irq %u)\n",
            hvs5_pvs[i].pv_Name, (unsigned)st->hvs_PVIrq);
        break;
    }

    if (!st->hvs_PVOffset)
    {
        bug("[VC4HVS5] vsync: no enabled pixelvalve, flips will not pace\n");
        return;
    }

    st->hvs_VSyncIrq = KrnAddIRQHandler(st->hvs_PVIrq, hvs5_vsync_irq,
                                        st, SysBase);
    if (!st->hvs_VSyncIrq)
        bug("[VC4HVS5] vsync: KrnAddIRQHandler failed\n");
}

/*
 * Find the per-frame interrupt bit by measurement rather than by
 * documentation, which was wrong on VideoCore IV: arm one bit alone for a
 * short window and count what arrives. Line-rate sources fire about a
 * vertical total more often than frame-rate ones, and PV_VERTA/B give
 * that ratio, so no timer is needed.
 */
static void hvs5_vsync_start(struct vc4_hvs_state *st)
{
    ULONG verta = pv_rd(st->hvs_PVOffset, HVS5_PV_VERTA);
    ULONG vertb = pv_rd(st->hvs_PVOffset, HVS5_PV_VERTB);
    ULONG vtot = (verta & 0xffff) + (verta >> 16)
               + (vertb & 0xffff) + (vertb >> 16);
    ULONG rate[10], line_max = 0, expect, c0, b, start;

    if (!st->hvs_VSyncIrq)
        return;

    if (!st->hvs_VSyncMask)
    {
        bug("[VC4HVS5] vsync: per-bit probe, vtot=%u\n", (unsigned)vtot);

        for (b = 0; b < 10; b++)
        {
            c0 = st->hvs_VSyncCount;
            st->hvs_VSyncMask = 1UL << b;       /* handler counts this bit */
            pv_wr(st->hvs_PVOffset, HVS5_PV_INTSTAT, HVS5_PV_INT_ALL);
            pv_wr(st->hvs_PVOffset, HVS5_PV_INTEN, 1UL << b);
            start = hvs5_now_us();
            while ((hvs5_now_us() - start) < HVS5_PROBE_US)
                ;
            pv_wr(st->hvs_PVOffset, HVS5_PV_INTEN, 0);
            pv_wr(st->hvs_PVOffset, HVS5_PV_INTSTAT, HVS5_PV_INT_ALL);
            rate[b] = st->hvs_VSyncCount - c0;
            bug("[VC4HVS5] vsync: bit %u -> %u ticks\n",
                (unsigned)b, (unsigned)rate[b]);
        }
        st->hvs_VSyncMask = 0;

        for (b = 0; b < 10; b++)
        {
            if (rate[b] > line_max)
                line_max = rate[b];
        }
        if (vtot == 0 || line_max < vtot / 4)
        {
            bug("[VC4HVS5] vsync: inconclusive (max %u) - flips stay "
                "unpaced\n", (unsigned)line_max);
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
                bug("[VC4HVS5] vsync: bit %u runs at frame rate\n",
                    (unsigned)b);
                break;
            }
        }
        if (!st->hvs_VSyncMask)
        {
            bug("[VC4HVS5] vsync: no frame-rate bit found\n");
            return;
        }
    }

    pv_wr(st->hvs_PVOffset, HVS5_PV_INTSTAT, HVS5_PV_INT_ALL);
    pv_wr(st->hvs_PVOffset, HVS5_PV_INTEN, st->hvs_VSyncMask);

    c0 = st->hvs_VSyncCount;
    start = hvs5_now_us();
    while ((hvs5_now_us() - start) < HVS5_VERIFY_US)
    {
        if (st->hvs_VSyncCount >= c0 + 5)
            break;
    }
    if (st->hvs_VSyncCount == c0)
    {
        pv_wr(st->hvs_PVOffset, HVS5_PV_INTEN, 0);
        bug("[VC4HVS5] vsync: nothing delivered, re-masked\n");
    }
    else
        bug("[VC4HVS5] vsync: alive on INTEN=0x%03x, %u ticks\n",
            (unsigned)st->hvs_VSyncMask,
            (unsigned)(st->hvs_VSyncCount - c0));
}
#else
void vc4_hvs5_irq_init(struct VideoCoreGfx_staticdata *xsd)
{
    xsd->vcsd_HVS.hvs_VSyncIrq = NULL;
}
#endif

/*
 * A page retargeted away from stays on screen until the write latches at
 * the next frame start, and the caller starts drawing into it the moment
 * this returns - so wait for our own latch, not the previous one. Only
 * when the interrupt is genuinely armed, and bounded, so a dead counter
 * degrades to unpaced rather than stalling.
 * Sleeps instead of spinning while the interrupt is demonstrably live.
 */
static void hvs5_latch_until(struct vc4_hvs_state *st, ULONG armed)
{
#if VC4_HVS5_VSYNC_IRQ
    ULONG start;

    if (!st->hvs_VSyncIrq || !st->hvs_VSyncMask
        || !(pv_rd(st->hvs_PVOffset, HVS5_PV_INTEN) & st->hvs_VSyncMask))
        return;

    start = hvs5_now_us();

    /* One waiter slot: a second caller spins. */
    if (!st->hvs_VSyncTask
        && (start - st->hvs_VSyncStamp) < HVS5_LIVE_US)
    {
        BYTE sig = AllocSignal(-1);

        if (sig != -1)
        {
            st->hvs_VSyncSigMask = 1UL << sig;
            Disable();
            st->hvs_VSyncTask = FindTask(NULL);
            Enable();

            while ((LONG)(st->hvs_VSyncCount - armed) < 0)
            {
                if ((hvs5_now_us() - start) >= HVS5_FLIP_US)
                    break;
                /* Clear, re-test, Wait(): a latch in the gap leaves
                 * the signal set. */
                SetSignal(0, st->hvs_VSyncSigMask);
                if ((LONG)(st->hvs_VSyncCount - armed) >= 0)
                    break;
                Wait(st->hvs_VSyncSigMask);
            }

            Disable();
            st->hvs_VSyncTask = NULL;
            Enable();
            FreeSignal(sig);
            return;
        }
    }

    while ((LONG)(st->hvs_VSyncCount - armed) < 0)
    {
        if ((hvs5_now_us() - start) >= HVS5_FLIP_US)
            break;
    }
#else
    (void)st;
    (void)armed;
#endif
}

/* The interrupt is armed and has ticked within the last few frames. */
static BOOL hvs5_vsync_live(struct vc4_hvs_state *st)
{
#if VC4_HVS5_VSYNC_IRQ
    return st->hvs_VSyncIrq && st->hvs_VSyncMask
        && (pv_rd(st->hvs_PVOffset, HVS5_PV_INTEN) & st->hvs_VSyncMask)
        && (hvs5_now_us() - st->hvs_VSyncStamp) < HVS5_LIVE_US;
#else
    (void)st;
    return FALSE;
#endif
}

/* ------------------------------------------------------------------ */
/* Screens as planes                                                   */
/* ------------------------------------------------------------------ */

BOOL vc4_hvs5_planes_ok(void)
{
    return VC4_HVS5_TAKEOVER;
}

BOOL vc4_hvs5_plane_fits(struct VideoCoreGfx_staticdata *xsd, ULONG w, ULONG h)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    return st->hvs_DestW && st->hvs_DestH
        && HVS5_PPF_FITS(w, st->hvs_DestW) && HVS5_PPF_FITS(h, st->hvs_DestH);
}

/* Caller holds the mailbox lock. Armed like a flip, so LatchWait tells
 * when the previous list - and the pages it showed - are out of use. */
void vc4_hvs5_planes_commit(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    if (!st->hvs_Active)
        return;

    hvs5_commit(st, hvs5_build_list(xsd));
    st->hvs_FlipArmed = st->hvs_VSyncCount + 1;
}
