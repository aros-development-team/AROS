/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2711 HVS5 read-only bring-up dump.

    Phase 1 of VideoCore VI display support. Nothing here writes to the
    hardware: the firmware keeps driving the display throughout, and the
    point is only to learn the HVS5 display list layout the way the
    VideoCore IV one was learned - by decoding a live firmware-built list
    rather than assuming a format.

    The trick that makes that possible without knowing the layout: the
    framebuffer address is already known from FBALLOC, so scanning the
    register window for it finds both where the display list RAM lives and
    where the plane entry keeps its pointer. Everything phase 2 needs
    (entry length, the pointer's offset within it, which channel is live)
    falls out of the words printed around each hit.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>

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
#define VC4_HVS5_DUMP       0

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
 * Phase 2 rides on the same opt-in as the dump. It writes to the HVS, so
 * on top of the QEMU abort it can disturb a working display - though only
 * transiently: any anomaly hands straight back to the firmware, and a
 * firmware list rebuild heals whatever we left behind.
 */
#define VC4_HVS5_TAKEOVER   0

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

        /* Author our own list: the framebuffer plane inherited verbatim,
         * then a cursor plane of our own, then END. The firmware's list
         * cannot supply the cursor - whether it has such a plane at all
         * depends on how far Intuition got before the mode was set, and
         * both a 9-word and a 17-word list have been seen on the same
         * board across boots. */
        base = HVS5_OWN_SLOT_BASE + HVS5_OWN_SLOT_STRIDE * st->hvs_Slot;
        st->hvs_Slot = (st->hvs_Slot + 1) % HVS5_OWN_SLOTS;

        for (i = 0; i < fb_words; i++)
            hvs5_dl_wr(base + i, hvs5_dl_rd(head + fb_entry + i));

        st->hvs_FBPtr    = fb_phys & ~HVS5_PTR_BUS_ALIAS;
        st->hvs_FBPtrOff = fb_ptr;
        st->hvs_FBWords  = fb_words;
        st->hvs_ListBase = base;

        /* The framebuffer plane covers the screen, so its POS2 is what a
         * cursor has to be clipped against. */
        {
            ULONG pos2 = hvs5_dl_rd(base + 3);

            st->hvs_SrcW = pos2 & 0xffff;
            st->hvs_SrcH = (pos2 >> 16) & 0xffff;
        }
        hvs5_dl_wr(base + st->hvs_FBPtrOff,
                   HVS5_PTR_BUS_ALIAS | st->hvs_FBPtr);

        /* The cursor plane is always authored when there is a buffer for
         * it, visible or not: VALID then carries visibility and the list
         * never has to change shape again. */
        st->hvs_CurOff = 0;
        if (xsd->vcsd_CurBuf)
        {
            const ULONG *px = (const ULONG *)xsd->vcsd_CurBuf;

            st->hvs_CurOff    = fb_words;
            st->hvs_CurWords  = HVS5_CURSOR_WORDS;
            st->hvs_CurPtrOff = HVS5_CURSOR_WORDS - HVS5_PTROFF_FROM_END;

            hvs5_dl_wr(base + st->hvs_CurOff + 0, HVS5_CTL0_CURSOR
                                                & ~HVS5_CTL0_VALID);
            hvs5_dl_wr(base + st->hvs_CurOff + 2, HVS5_ALPHA_PERPIXEL);
            hvs5_dl_wr(base + st->hvs_CurOff + 4, 0);
            hvs5_dl_wr(base + st->hvs_CurOff + 6, 0);

            /* Per-pixel blending means a clear alpha byte is the whole
             * difference between a pointer and nothing at all. */
            bug("[VC4HVS5] cursor plane authored at +%u, visible %d, "
                "%ux%u at %d,%d, first pixels %08x %08x %08x %08x\n",
                (unsigned)st->hvs_CurOff, (int)xsd->vcsd_CurVisible,
                (unsigned)xsd->vcsd_CurWidth, (unsigned)xsd->vcsd_CurHeight,
                (int)xsd->vcsd_CurX, (int)xsd->vcsd_CurY,
                px[0], px[1], px[2], px[3]);

            hvs5_dl_wr(base + st->hvs_CurOff + HVS5_CURSOR_WORDS,
                       HVS5_CTL0_END);
            hvs5_write_cursor(xsd, base + st->hvs_CurOff);
        }
        else
            hvs5_dl_wr(base + fb_words, HVS5_CTL0_END);

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
            bug("[VC4HVS5] takeover: ACTIVE - %u words copied %u -> %u, "
                "fb plane +%u ptr +%u\n", (unsigned)len, (unsigned)head,
                (unsigned)base, (unsigned)fb_entry, (unsigned)fb_ptr);
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
    return TRUE;
}

/*
 * Position, size and pointer of our cursor plane. Split out from the
 * public entry point so the takeover can prime the entry before the
 * channel is ours and hvs_Active is still FALSE.
 *
 * vcsd_CurX/Y already have the hotspot applied and go negative past the
 * top and left edges, so clip by walking into the image and shrinking the
 * plane - a 16-bit position field would read a negative value as 65535.
 */
static void hvs5_write_cursor(struct VideoCoreGfx_staticdata *xsd, ULONG base)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    LONG cx = xsd->vcsd_CurX, cy = xsd->vcsd_CurY;
    LONG cw = xsd->vcsd_CurWidth, ch = xsd->vcsd_CurHeight;
    ULONG pitch = xsd->vcsd_CurWidth * 4;
    ULONG ptr = xsd->vcsd_CurBufBus & ~HVS5_PTR_BUS_ALIAS;

    if (cx < 0) { ptr += (ULONG)(-cx) * 4;     cw += cx; cx = 0; }
    if (cy < 0) { ptr += (ULONG)(-cy) * pitch; ch += cy; cy = 0; }
    if (cx + cw > (LONG)st->hvs_SrcW) cw = (LONG)st->hvs_SrcW - cx;
    if (cy + ch > (LONG)st->hvs_SrcH) ch = (LONG)st->hvs_SrcH - cy;

    if (!xsd->vcsd_CurVisible || cw <= 0 || ch <= 0)
    {
        hvs5_dl_wr(base, hvs5_dl_rd(base) & ~HVS5_CTL0_VALID);
        return;
    }

    hvs5_dl_wr(base + 1, HVS5_POS0(cx, cy));
    hvs5_dl_wr(base + 3, HVS5_POS2(cw, ch));
    hvs5_dl_wr(base + st->hvs_CurPtrOff, HVS5_PTR_BUS_ALIAS | ptr);
    hvs5_dl_wr(base + st->hvs_CurWords - 1, pitch);

    /* VALID last: the HVS must never see a half-written entry. */
    hvs5_dl_wr(base, hvs5_dl_rd(base) | HVS5_CTL0_VALID);
}

void vc4_hvs5_update_cursor(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;

    if (!st->hvs_Active || !st->hvs_CurOff)
        return;

    hvs5_write_cursor(xsd, st->hvs_ListBase + st->hvs_CurOff);
}
