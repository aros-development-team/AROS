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
static void hvs5_latch_wait(struct vc4_hvs_state *st);
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

        pos2 = hvs5_dl_rd(idx + 3);
        bug("[VC4HVS5]   plane +%u: %u words fmt %u %ux%u POS0=%08x "
            "[2]=%08x ptr=%08x\n",
            (unsigned)(idx - head), (unsigned)size,
            (unsigned)(ctl0 & HVS5_CTL0_FORMAT_MASK),
            (unsigned)(pos2 & 0xfff), (unsigned)((pos2 >> 16) & 0xfff),
            hvs5_dl_rd(idx + 1), hvs5_dl_rd(idx + 2),
            hvs5_dl_rd(idx + size - HVS5_PTROFF_FROM_END));

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
static void hvs5_init_plane(ULONG base, ULONG alpha_mode)
{
    ULONG i;

    for (i = 0; i < HVS5_PLANE_WORDS; i++)
        hvs5_dl_wr(base + i, 0);

    hvs5_dl_wr(base + 0, HVS5_CTL0_CURSOR & ~HVS5_CTL0_VALID);
    hvs5_dl_wr(base + 2, alpha_mode);
}

/*
 * The overlay plane: a buffer some other producer - the GL stack - renders
 * into, composited straight over the framebuffer instead of being blitted
 * into it. Fixed alpha rather than per-pixel, because what GL leaves in
 * the alpha channel is undefined.
 *
 * hvs_OvlX/Y are fb coords, so they are shifted by the fb plane origin
 * and clipped against its destination rectangle, like the cursor. No
 * scaling term: the overlay is only offered on a unity framebuffer
 * (hvs_OvlUsable), where destination and source are the same size.
 */
static void hvs5_write_overlay(struct VideoCoreGfx_staticdata *xsd, ULONG base)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    LONG left = (LONG)st->hvs_FBX, top = (LONG)st->hvs_FBY;
    LONG ox = left + st->hvs_OvlX, oy = top + st->hvs_OvlY;
    LONG ow = st->hvs_OvlW, oh = st->hvs_OvlH;
    ULONG ptr = st->hvs_OvlPhys & ~HVS5_PTR_BUS_ALIAS;

    if (ox < left) { ptr += (ULONG)(left - ox) * 4;                ow -= left - ox; ox = left; }
    if (oy < top)  { ptr += (ULONG)(top - oy) * st->hvs_OvlPitch;  oh -= top - oy;  oy = top; }
    if (ox + ow > left + (LONG)st->hvs_DestW) ow = left + (LONG)st->hvs_DestW - ox;
    if (oy + oh > top + (LONG)st->hvs_DestH)  oh = top + (LONG)st->hvs_DestH - oy;

    if (ow <= 0 || oh <= 0)
    {
        hvs5_dl_wr(base, HVS5_CTL0_CURSOR & ~HVS5_CTL0_VALID);
        return;
    }

    hvs5_dl_wr(base + 1, HVS5_POS0(ox, oy));
    hvs5_dl_wr(base + 3, HVS5_POS2(ow, oh));
    hvs5_dl_wr(base + HVS5_PLANE_WORDS - HVS5_PTROFF_FROM_END,
               HVS5_PTR_BUS_ALIAS | ptr);
    hvs5_dl_wr(base + HVS5_PLANE_WORDS - 1, st->hvs_OvlPitch);

    hvs5_dl_wr(base, HVS5_CTL0_CURSOR);
}

/*
 * Compose a list into the next slot: the framebuffer plane, then the
 * overlay if one is up, then the cursor, then END. Round-robin so the
 * slot being written is never the one the HVS is scanning.
 *
 * The caller must write the returned head to DISPLIST. Do that only for
 * structural changes: the HVS applies a repoint mid-scanout, which shows
 * as a torn frame, so steady-state updates patch the live list instead.
 */
static ULONG hvs5_build_list(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    ULONG base = HVS5_OWN_SLOT_BASE + HVS5_OWN_SLOT_STRIDE * st->hvs_Slot;
    ULONG n = 0, i;

    st->hvs_Slot = (st->hvs_Slot + 1) % HVS5_OWN_SLOTS;

    for (i = 0; i < st->hvs_FBWords; i++)
        hvs5_dl_wr(base + n++, st->hvs_FBEntry[i]);
    hvs5_dl_wr(base + st->hvs_FBPtrOff, HVS5_PTR_BUS_ALIAS | st->hvs_FBPtr);

    st->hvs_OvlOff = 0;
    st->hvs_OvlWords = 0;
    if (st->hvs_OvlActive)
    {
        st->hvs_OvlOff = n;
        st->hvs_OvlWords = HVS5_PLANE_WORDS;
        hvs5_init_plane(base + n, HVS5_ALPHA_FIXED);
        hvs5_write_overlay(xsd, base + n);
        n += HVS5_PLANE_WORDS;
    }

    /* Authored whenever a buffer exists, visible or not, so VALID alone
     * carries visibility and showing the pointer never reshapes the list. */
    st->hvs_CurOff = 0;
    if (xsd->vcsd_CurBuf)
    {
        st->hvs_CurOff    = n;
        st->hvs_CurWords  = HVS5_PLANE_WORDS;
        st->hvs_CurPtrOff = HVS5_PLANE_WORDS - HVS5_PTROFF_FROM_END;
        hvs5_init_plane(base + n, HVS5_ALPHA_PERPIXEL);
        hvs5_write_cursor(xsd, base + n);
        n += HVS5_PLANE_WORDS;
    }

    hvs5_dl_wr(base + n++, HVS5_CTL0_END);
    st->hvs_ListBase = base;
    return base;
}

BOOL vc4_hvs5_takeover(struct VideoCoreGfx_staticdata *xsd,
                       ULONG fb_phys, ULONG fb_pitch)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    st->hvs_Active = FALSE;
    (void)fb_pitch;

#if VC4_HVS5_TAKEOVER
    {
        ULONG ctrl, head, len, base, i;
        ULONG fb_entry, fb_ptr, fb_words;

        if (hvs5_rd(HVS5_ID) != HVS5_ID_MAGIC)
            return FALSE;

        ctrl = hvs5_rd(HVS5_DISPCTRLX(HVS5_CHANNEL_HDMI));
        head = hvs5_rd(HVS5_DISPLACT(HVS5_CHANNEL_HDMI));
        if (!(ctrl & (1UL << 31)) || head == 0 || head >= HVS5_DLIST_WORDS)
        {
            bug("[VC4HVS5] takeover: channel %u unusable (CTRL=%08x head=%u)\n",
                (unsigned)HVS5_CHANNEL_HDMI, ctrl, (unsigned)head);
            return FALSE;
        }

        len = hvs5_list_length(head);
        if (len == 0 || len > HVS5_OWN_SLOT_STRIDE)
        {
            bug("[VC4HVS5] takeover: list at %u is %u words, refusing\n",
                (unsigned)head, (unsigned)len);
            return FALSE;
        }

        if (!hvs5_find_plane(head, fb_phys, &fb_entry, &fb_ptr, &fb_words))
        {
            bug("[VC4HVS5] takeover: no plane points at fb 0x%08x\n", fb_phys);
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

        /* A framebuffer the firmware scales carries extra words we have
         * not decoded, so refuse to compose over one. */
        st->hvs_OvlActive = FALSE;
        st->hvs_OvlUsable = (fb_words == HVS5_PLANE_WORDS);

        base = hvs5_build_list(xsd);

        hvs5_log_list(base);

        VC4_MBOX_LOCK(xsd);
        hvs5_wr(HVS5_DISPLIST(HVS5_CHANNEL_HDMI), base);
        for (i = 0; i < HVS5_SPIN_LATCH; i++)
        {
            if (hvs5_rd(HVS5_DISPLACT(HVS5_CHANNEL_HDMI)) == base)
                break;
        }
        if (hvs5_rd(HVS5_DISPLACT(HVS5_CHANNEL_HDMI)) == base)
            st->hvs_Active = TRUE;
        else
            hvs5_wr(HVS5_DISPLIST(HVS5_CHANNEL_HDMI), head);
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

BOOL vc4_hvs5_flip_page(struct VideoCoreGfx_staticdata *xsd, ULONG page_phys)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    if (!st->hvs_Active)
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
                      const struct vc4gfx_overlay *ovl)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    BOOL structural;

    if (!st->hvs_Active || !st->hvs_OvlUsable)
        return FALSE;

    VC4_MBOX_LOCK(xsd);

    if (!ovl)
    {
        if (st->hvs_OvlActive)
        {
            st->hvs_OvlActive = FALSE;
            hvs5_wr(HVS5_DISPLIST(HVS5_CHANNEL_HDMI), hvs5_build_list(xsd));
        }
        VC4_MBOX_UNLOCK(xsd);
        return TRUE;
    }

    /* Scaling needs the filter kernel and the extra entry words that go
     * with it, neither of which has been decoded on HVS5 - refused, and
     * the caller blits instead. */
    if ((ovl->ovl_DestW && ovl->ovl_DestW != ovl->ovl_Width)
        || (ovl->ovl_DestH && ovl->ovl_DestH != ovl->ovl_Height))
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }

    structural = !st->hvs_OvlActive || !st->hvs_OvlOff
              || st->hvs_OvlW != ovl->ovl_Width
              || st->hvs_OvlH != ovl->ovl_Height;

    st->hvs_OvlActive = TRUE;
    st->hvs_OvlPhys  = ovl->ovl_Phys & ~HVS5_PTR_BUS_ALIAS;
    st->hvs_OvlPitch = ovl->ovl_Pitch;
    st->hvs_OvlW     = ovl->ovl_Width;
    st->hvs_OvlH     = ovl->ovl_Height;
    st->hvs_OvlDestW = ovl->ovl_Width;
    st->hvs_OvlDestH = ovl->ovl_Height;
    st->hvs_OvlX     = ovl->ovl_X;
    st->hvs_OvlY     = ovl->ovl_Y;

    if (structural)
        hvs5_wr(HVS5_DISPLIST(HVS5_CHANNEL_HDMI), hvs5_build_list(xsd));
    else
        hvs5_write_overlay(xsd, st->hvs_ListBase + st->hvs_OvlOff);

    /* The buffer just replaced stays on screen until this latches, so
     * pace the producer exactly like a page flip. */
    st->hvs_FlipArmed = st->hvs_VSyncCount + 1;
    hvs5_latch_wait(st);

    VC4_MBOX_UNLOCK(xsd);
    return TRUE;
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

static inline ULONG hvs5_now_us(void)
{
    return AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);
}

static inline void pv_wr(ULONG base_off, ULONG offset, ULONG value)
{
    *(volatile ULONG *)(ARM_PERIIOBASE + base_off + offset) = value;
}

#if VC4_HVS5_VSYNC_IRQ
/* IRQ context: count only, no printing. The line is shared with PV1,
 * which is disabled and masked, so a status of zero is not ours. */
static void hvs5_vsync_irq(struct vc4_hvs_state *st, struct ExecBase *sysBase)
{
    ULONG stat = pv_rd(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTSTAT);

    if (stat)
    {
        pv_wr(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTSTAT, stat);   /* W1C */
        if (stat & st->hvs_VSyncMask)
            st->hvs_VSyncCount++;
    }
}

void vc4_hvs5_irq_init(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    st->hvs_VSyncIrq = KrnAddIRQHandler(HVS5_PV_HDMI_IRQ, hvs5_vsync_irq,
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
    ULONG verta = pv_rd(HVS5_PV_HDMI_OFFSET, HVS5_PV_VERTA);
    ULONG vertb = pv_rd(HVS5_PV_HDMI_OFFSET, HVS5_PV_VERTB);
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
            pv_wr(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTSTAT, HVS5_PV_INT_ALL);
            pv_wr(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTEN, 1UL << b);
            start = hvs5_now_us();
            while ((hvs5_now_us() - start) < HVS5_PROBE_US)
                ;
            pv_wr(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTEN, 0);
            pv_wr(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTSTAT, HVS5_PV_INT_ALL);
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

    pv_wr(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTSTAT, HVS5_PV_INT_ALL);
    pv_wr(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTEN, st->hvs_VSyncMask);

    c0 = st->hvs_VSyncCount;
    start = hvs5_now_us();
    while ((hvs5_now_us() - start) < HVS5_VERIFY_US)
    {
        if (st->hvs_VSyncCount >= c0 + 5)
            break;
    }
    if (st->hvs_VSyncCount == c0)
    {
        pv_wr(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTEN, 0);
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
 */
static void hvs5_latch_wait(struct vc4_hvs_state *st)
{
#if VC4_HVS5_VSYNC_IRQ
    if (st->hvs_VSyncIrq && st->hvs_VSyncMask
        && (pv_rd(HVS5_PV_HDMI_OFFSET, HVS5_PV_INTEN) & st->hvs_VSyncMask))
    {
        ULONG start = hvs5_now_us();

        while ((LONG)(st->hvs_VSyncCount - st->hvs_FlipArmed) < 0)
        {
            if ((hvs5_now_us() - start) >= HVS5_FLIP_US)
                break;
        }
    }
#else
    (void)st;
#endif
}
