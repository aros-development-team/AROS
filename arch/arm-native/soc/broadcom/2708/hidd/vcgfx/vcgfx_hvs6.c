/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore Gfx Hidd - BCM2712 HVS6 support.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/kernel.h>

#include <string.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hardware.h"
#include "vcgfx_hvs6.h"

/* 0 = leave the display list to the firmware */
#define VC4_HVS6_TAKEOVER 1
#define VC4_HVS6_FLIP     1
/* Flash the back page green for a second at setup */
#define VC4_HVS6_FLIPTEST 0
/* 0 = software pointer */
#define VC4_HVS6_CURSOR   1

/* Register reads; several frames' worth */
#define HVS6_SPIN_LATCH   2000000

/* 1MHz system timer */
static inline ULONG hvs6_now_us(void)
{
    return *(volatile ULONG *)(ARM_PERIIOBASE + 0x3004);
}

/* Byte offset of a slot from the HVS window base */
static inline ULONG hvs6_slot(struct vc4_hvs6_state *st, ULONG list, ULONG n)
{
    return (st->h6_ListBase + st->h6_List[list] + n * HVS6_SLOT_WORDS) * 4;
}

/* Copy a plane entry and retarget it; unauthored words stay the firmware's */
static void hvs6_plane(ULONG dst, ULONG src, UQUAD addr, ULONG pitch,
                       ULONG ptr0, ULONG x, ULONG y, ULONG w, ULONG h)
{
    ULONG i;

    for (i = 0; i < HVS6_ENT_WORDS; i++)
        hvs6_wr(dst + i * 4, hvs6_rd(src + i * 4));

    hvs6_wr(dst + HVS6_ENT_POS0 * 4, HVS6_POS0(x, y));
    hvs6_wr(dst + HVS6_ENT_POS2 * 4, HVS6_POS2(w, h));
    hvs6_wr(dst + HVS6_ENT_PTR0 * 4, ptr0 | HVS6_PTR0_UPPER(addr));
    hvs6_wr(dst + HVS6_ENT_PTR1 * 4, (ULONG)(addr & 0xffffffffULL));
    hvs6_wr(dst + HVS6_ENT_PTR2 * 4, pitch);
}

/* Carve the overlay, cursor and framebuffer slices from the top of the
 * UPM arena; the firmware allocates its planes upwards from 0. */
static void hvs6_upm_carve(struct vc4_hvs6_state *st)
{
    ULONG arena = hvs6_rd(HVS6_UBM_SIZE);
    ULONG top;

    if (st->h6_OvlPTR0)
        return;

    /* UBM_SIZE units are uncertain: small = allocation units */
    if (arena && (arena < 0x1000))
        arena *= HVS6_UPM_GRAN;
    if ((arena < 4 * HVS6_UPM_SLICE) || (arena > 0x400000))
        arena = HVS6_UPM_FALLBACK;

    top = (arena - HVS6_UPM_SLICE) / HVS6_UPM_GRAN;
    st->h6_OvlPTR0 = HVS6_PTR0_BASE(top) | HVS6_PTR0_HANDLE(31)
                   | HVS6_PTR0_LINES2;

    top = (arena - 2 * HVS6_UPM_SLICE) / HVS6_UPM_GRAN;
    st->h6_CurPTR0 = HVS6_PTR0_BASE(top) | HVS6_PTR0_HANDLE(32)
                   | HVS6_PTR0_LINES2;

    /* For a framebuffer of our own: the firmware's slice is sized for
     * the boot surface's pitch, and a wider line overruns it. */
    top = (arena - 3 * HVS6_UPM_SLICE) / HVS6_UPM_GRAN;
    st->h6_FbPTR0  = HVS6_PTR0_BASE(top) | HVS6_PTR0_HANDLE(30)
                   | HVS6_PTR0_LINES2;

    bug("[VC4HVS6] UPM arena %u bytes: overlay PTR0 %08x, cursor PTR0 %08x,"
        " framebuffer PTR0 %08x\n", arena, st->h6_OvlPTR0, st->h6_CurPTR0,
        st->h6_FbPTR0);
}

/* Find a run of slots that still holds the firmware's fill pattern */
static BOOL hvs6_free_slots(ULONG base, ULONG need, ULONG *out)
{
    ULONG rel, words = need * HVS6_SLOT_WORDS;

    for (rel = HVS6_FREE_FROM; rel + words < HVS6_FREE_LIMIT;
         rel += HVS6_SLOT_WORDS)
    {
        ULONG i;
        BOOL  clean = TRUE;

        for (i = 0; i < words && clean; i++)
            if (hvs6_rd((base + rel + i) * 4) != HVS6_FILL)
                clean = FALSE;

        if (clean)
        {
            *out = rel;
            return TRUE;
        }
    }

    return FALSE;
}

void vc4_hvs6_report(struct VideoCoreGfx_staticdata *xsd, ULONG fb_phys,
                     ULONG fb_pitch, ULONG fb_width, ULONG fb_height)
{
    ULONG id = hvs6_rd(HVS6_ID);
    ULONG base, ch;

    if (id != HVS6_ID_MAGIC)
    {
        bug("[VC4HVS6] identity reads %08x, not %08x - not driving this\n",
            id, HVS6_ID_MAGIC);
        return;
    }

    base = HVS6_DLIST_WORD;
    bug("[VC4HVS6] mailbox gave fb 0x%08x pitch %u for %ux%u; version %02x,"
        " dlist %u words, upm %u\n", fb_phys, fb_pitch, fb_width, fb_height,
        hvs6_rd(HVS6_VERSION) & 0xff, hvs6_rd(HVS6_CXM_SIZE),
        hvs6_rd(HVS6_UBM_SIZE));

    for (ch = 0; ch < HVS6_CHANNELS; ch++)
    {
        ULONG ctrl = hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPCTRL);
        ULONG list = hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPLIST);
        ULONG act  = hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPLACT);
        ULONG slot, ctl0, ptr0, pitch, pos2;

        if (!(ctrl & HVS6_DISPCTRL_EN))
        {
            bug("[VC4HVS6] ch%u disabled\n", ch);
            continue;
        }

        slot = (base + list) * 4;
        ctl0 = hvs6_rd(slot + HVS6_ENT_CTL0 * 4);
        pos2 = hvs6_rd(slot + HVS6_ENT_POS2 * 4);
        ptr0 = hvs6_rd(slot + HVS6_ENT_PTR1 * 4);
        pitch = hvs6_rd(slot + HVS6_ENT_PTR2 * 4);

        bug("[VC4HVS6] ch%u out %ux%u list %#06x act %#06x: ctl0 %08x"
            " %ux%u ptr %08x pitch %u%s\n",
            ch, HVS6_DISPCTRL_W(ctrl), HVS6_DISPCTRL_H(ctrl), list, act,
            ctl0, (pos2 & 0xffff) + 1, (pos2 >> 16) + 1, ptr0, pitch,
            (ptr0 == fb_phys) ? "  <= SCANNING OUR FB" : "  <= NOT OUR FB");
    }
}

/* Back to list 0 with only the framebuffer plane, at the current size.
 * Single-word writes: either list may be the one being scanned. */
static BOOL hvs6_retake(struct vc4_hvs6_state *st)
{
    ULONG list, spin;

    for (list = 0; list < HVS6_LISTS; list++)
    {
        hvs6_wr(hvs6_slot(st, list, HVS6_SLOT_OVL), HVS6_CTL0_EMPTY);
        hvs6_wr(hvs6_slot(st, list, HVS6_SLOT_CUR), HVS6_CTL0_EMPTY);
    }
    hvs6_wr(HVS6_CHAN(st->h6_Chan) + HVS6_DISPLIST, st->h6_List[0]);

    st->h6_Pages   = 1;
    st->h6_Overlay = FALSE;
    st->h6_Cursor  = FALSE;

    for (spin = 0; spin < HVS6_SPIN_LATCH; spin++)
        if (hvs6_rd(HVS6_CHAN(st->h6_Chan) + HVS6_DISPLACT) == st->h6_List[0])
            return TRUE;

    bug("[VC4HVS6] ch%u never latched %#06x\n", st->h6_Chan, st->h6_List[0]);
    return FALSE;
}

/* Replace the framebuffer channel's list with our own copy of the
 * firmware's plane entry, setting the output to fb_width x fb_height at
 * the rate in want (NULL = any) first if needed; at the boot mode the
 * screen should not change. */
BOOL vc4_hvs6_takeover(struct VideoCoreGfx_staticdata *xsd, ULONG fb_phys,
                       ULONG fb_pitch, ULONG fb_width, ULONG fb_height,
                       const struct vcgfx_timing *want)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG base, ch, list = 0, ours = 0, i, spin, out;
    ULONG src = 0, dst;
    BOOL resize;

    if (!VC4_HVS6_TAKEOVER)
        return FALSE;

    if (hvs6_rd(HVS6_ID) != HVS6_ID_MAGIC)
        return FALSE;

    base = HVS6_DLIST_WORD;

    if (st->h6_Active)
    {
        /* Ours already: the same mode only needs the front page back;
         * any other is a mode set from the firmware's entry. */
        ch  = st->h6_Chan;
        out = hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPCTRL);
        if ((HVS6_DISPCTRL_W(out) == fb_width) && (HVS6_DISPCTRL_H(out) == fb_height)
            && vc4_hvs6_mode_current(xsd, want))
            return hvs6_retake(st);

        if (!vc4_hvs6_mode_ok(xsd, ch, fb_width, fb_height, want))
        {
            bug("[VC4HVS6] ch%u cannot show %ux%u\n", ch, fb_width, fb_height);
            return FALSE;
        }
        list = st->h6_FWList;
        src  = (base + list) * 4;
    }
    else
    {
        /* The firmware picks the channel by mode */
        for (ch = 0; ch < HVS6_CHANNELS; ch++)
        {
            ULONG ctl0;

            if (!(hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPCTRL) & HVS6_DISPCTRL_EN))
                continue;

            list = hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPLIST);
            src  = (base + list) * 4;
            ctl0 = hvs6_rd(src + HVS6_ENT_CTL0 * 4);

            if ((ctl0 & HVS6_CTL0_END) || !(ctl0 & HVS6_CTL0_VALID))
                continue;
            if (hvs6_rd(src + HVS6_ENT_PTR1 * 4) == xsd->vcsd_BootFB)
                break;
        }

        if (ch == HVS6_CHANNELS)
        {
            bug("[VC4HVS6] no channel is scanning 0x%08x - staying on firmware\n",
                xsd->vcsd_BootFB);
            return FALSE;
        }
    }

    /* A mode we cannot set is shown top-left at the current one */
    out    = hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPCTRL);
    resize = ((HVS6_DISPCTRL_W(out) != fb_width) || (HVS6_DISPCTRL_H(out) != fb_height)
              || !vc4_hvs6_mode_current(xsd, want))
             && vc4_hvs6_mode_ok(xsd, ch, fb_width, fb_height, want);

    /* Reuse our lists: they no longer hold the fill pattern */
    if (st->h6_List[0] && (st->h6_ListBase == base))
        ours = st->h6_List[0];
    else if (!hvs6_free_slots(base, HVS6_LISTS * HVS6_LIST_SLOTS, &ours))
    {
        bug("[VC4HVS6] no free list slots - staying on firmware\n");
        return FALSE;
    }

    st->h6_ListBase = base;
    st->h6_List[0]  = ours;
    st->h6_List[1]  = ours + HVS6_LIST_SLOTS * HVS6_SLOT_WORDS;

    /* List 0 may be live; nothing scans it once stopped */
    if (resize)
        vc4_hvs6_mode_stop(xsd, ch);

    /* List 0 shows the firmware's surface, or ours with its own slice */
    dst = hvs6_slot(st, 0, HVS6_SLOT_FB);
    if (fb_phys == xsd->vcsd_BootFB)
    {
        for (i = 0; i < HVS6_SLOT_WORDS; i++)
            hvs6_wr(dst + i * 4, hvs6_rd(src + i * 4));
        hvs6_wr(dst + HVS6_ENT_POS0 * 4, HVS6_POS0(0, 0));
        hvs6_wr(dst + HVS6_ENT_POS2 * 4, HVS6_POS2(fb_width, fb_height));
    }
    else
    {
        hvs6_upm_carve(st);
        hvs6_plane(dst, src, (UQUAD)fb_phys, fb_pitch, st->h6_FbPTR0,
                   0, 0, fb_width, fb_height);
    }
    hvs6_wr(hvs6_slot(st, 0, HVS6_SLOT_OVL), HVS6_CTL0_EMPTY);
    hvs6_wr(hvs6_slot(st, 0, HVS6_SLOT_CUR), HVS6_CTL0_EMPTY);
    hvs6_wr(hvs6_slot(st, 0, HVS6_SLOT_END), HVS6_CTL0_END);

    if (resize)
    {
        if (!vc4_hvs6_mode_start(xsd, ch, fb_width, fb_height, want, ours))
        {
            bug("[VC4HVS6] ch%u: mode set to %ux%u failed\n", ch, fb_width, fb_height);
            return FALSE;
        }
    }
    else
    {
        hvs6_wr(HVS6_CHAN(ch) + HVS6_DISPLIST, ours);

        if (hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPLIST) != ours)
        {
            bug("[VC4HVS6] ch%u DISPLIST did not take %#06x - restoring\n",
                ch, ours);
            hvs6_wr(HVS6_CHAN(ch) + HVS6_DISPLIST, list);
            return FALSE;
        }

        /* DISPLIST reaches DISPLACT at the next frame start */
        for (spin = 0; spin < HVS6_SPIN_LATCH; spin++)
            if (hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPLACT) == ours)
                break;

        if (spin == HVS6_SPIN_LATCH)
        {
            bug("[VC4HVS6] ch%u never latched %#06x - restoring\n", ch, ours);
            hvs6_wr(HVS6_CHAN(ch) + HVS6_DISPLIST, list);
            return FALSE;
        }
    }

    st->h6_Chan    = ch;
    st->h6_FWList  = list;
    st->h6_Pages   = 1;
    st->h6_Active  = TRUE;
    st->h6_Overlay = FALSE;
    st->h6_Cursor  = FALSE;

    bug("[VC4HVS6] ch%u: took over the display list, %#06x -> %#06x\n",
        ch, list, ours);
    return TRUE;
}

/* Scanout memory: page-aligned, flushed, and remapped write-combining
 * since the HVS does not see the CPU caches. A block that is already big
 * enough is kept. */
static BOOL hvs6_scanout(APTR *raw, ULONG *rawsize, ULONG *mapped, UQUAD *phys,
                         ULONG size)
{
    ULONG need = (size + 4095) & ~4095UL;
    IPTR  page;

    if (*raw && (*mapped >= need))
        return TRUE;

    if (*raw)
    {
        FreeMem(*raw, *rawsize);
        *raw = NULL;
    }

    /* 31-bit because vcsd_FBPage[] is a ULONG. Over-allocate so the
     * page-rounded mapping stays inside our block. */
    *rawsize = need + 4095;
    if (!(*raw = AllocMem(*rawsize, MEMF_31BIT)))
    {
        *rawsize = 0;
        return FALSE;
    }

    page = ((IPTR)*raw + 4095) & ~(IPTR)4095;

    /* Flush before remapping, or a later writeback hits our pixels */
    CacheClearE((APTR)page, need, CACRF_ClearD);

    /* MAP_WriteThrough = Normal-NC; MAP_CacheInhibit would be Device */
    if (!KrnMapGlobal((APTR)page, (APTR)page, need,
                      MAP_WriteThrough | MAP_Readable | MAP_Writable))
    {
        FreeMem(*raw, *rawsize);
        *raw     = NULL;
        *rawsize = 0;
        return FALSE;
    }

    *phys   = (UQUAD)page;
    *mapped = need;
    return TRUE;
}

/* A framebuffer for a mode larger than the firmware's boot surface */
APTR vc4_hvs6_alloc_fb(struct VideoCoreGfx_staticdata *xsd, ULONG pitch, ULONG height)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG size = pitch * height;

    if (!hvs6_scanout(&st->h6_FBRaw, &st->h6_FBSize, &st->h6_FBMapped,
                      &st->h6_FBPhys, size))
    {
        bug("[VC4HVS6] no memory for a %u byte framebuffer\n", size);
        return NULL;
    }

    memset((APTR)(IPTR)st->h6_FBPhys, 0, size);
    return (APTR)(IPTR)st->h6_FBPhys;
}

/* Allocate a second framebuffer page, remapped write-combining since
 * the HVS does not see the CPU caches. */
BOOL vc4_hvs6_add_backpage(struct VideoCoreGfx_staticdata *xsd,
                           ULONG fb_pitch, ULONG fb_height)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG size = fb_pitch * fb_height;
    ULONG i, dst, src;
    IPTR  page;

    if (!st->h6_Active || !VC4_HVS6_FLIP)
        return FALSE;

    st->h6_Pages = 1;

    if (!hvs6_scanout(&st->h6_BackRaw, &st->h6_BackSize, &st->h6_BackMapped,
                      &st->h6_BackPhys, size))
    {
        bug("[VC4HVS6] no memory for a %u byte back page\n", size);
        return FALSE;
    }
    page = (IPTR)st->h6_BackPhys;

    memset((APTR)page, 0, size);

    /* List 1 = list 0 retargeted at the back page */
    src = hvs6_slot(st, 0, HVS6_SLOT_FB);
    dst = hvs6_slot(st, 1, HVS6_SLOT_FB);
    for (i = 0; i < HVS6_SLOT_WORDS; i++)
        hvs6_wr(dst + i * 4, hvs6_rd(src + i * 4));
    hvs6_wr(dst + HVS6_ENT_PTR0 * 4,
            (hvs6_rd(src + HVS6_ENT_PTR0 * 4) & ~0xffUL)
            | HVS6_PTR0_UPPER((UQUAD)page));
    hvs6_wr(dst + HVS6_ENT_PTR1 * 4, (ULONG)((UQUAD)page & 0xffffffffULL));

    for (i = HVS6_SLOT_OVL; i <= HVS6_SLOT_CUR; i++)
        hvs6_wr(hvs6_slot(st, 1, i), hvs6_rd(hvs6_slot(st, 0, i)));
    hvs6_wr(hvs6_slot(st, 1, HVS6_SLOT_END), HVS6_CTL0_END);

    xsd->vcsd_FBPage[1] = (ULONG)page;
    xsd->vcsd_FBPages   = 2;
    st->h6_Pages        = 2;

#if VC4_HVS6_FLIPTEST
    {
        ULONG *px = (ULONG *)page;
        ULONG  i, start;

        for (i = 0; i < size / 4; i++)
            px[i] = 0xff00ff00;             /* AABBGGRR, so green */

        if (vc4_hvs6_flip_page(xsd, (ULONG)page))
        {
            bug("[VC4HVS6] fliptest: back page shown\n");
            start = hvs6_now_us();
            while ((hvs6_now_us() - start) < 1000000)
                ;
            vc4_hvs6_flip_page(xsd, xsd->vcsd_FBPage[0]);
            bug("[VC4HVS6] fliptest: front page restored\n");
        }
        else
            bug("[VC4HVS6] fliptest: flip refused\n");

        memset((APTR)page, 0, size);
    }
#endif

    bug("[VC4HVS6] back page at 0x%08x (%u bytes, write-combining),"
        " lists %#06x/%#06x\n", (ULONG)page, size,
        st->h6_List[0], st->h6_List[1]);
    return TRUE;
}

/* Wait for the latch, or the caller draws into the page being scanned */
BOOL vc4_hvs6_flip_page(struct VideoCoreGfx_staticdata *xsd, ULONG page_phys)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG which, spin;

    if (!st->h6_Active || (st->h6_Pages < 2))
        return FALSE;

    for (which = 0; which < HVS6_LISTS; which++)
        if (hvs6_rd(hvs6_slot(st, which, HVS6_SLOT_FB) + HVS6_ENT_PTR1 * 4)
            == page_phys)
            break;

    if (which == HVS6_LISTS)
        return FALSE;

    hvs6_wr(HVS6_CHAN(st->h6_Chan) + HVS6_DISPLIST, st->h6_List[which]);

    for (spin = 0; spin < HVS6_SPIN_LATCH; spin++)
        if (hvs6_rd(HVS6_CHAN(st->h6_Chan) + HVS6_DISPLACT)
            == st->h6_List[which])
            return TRUE;

    bug("[VC4HVS6] flip to %#06x never latched\n", st->h6_List[which]);
    return FALSE;
}

/* The replaced buffer is scanned until FRCNT moves on */
static void hvs6_ovl_arm(struct vc4_hvs6_state *st)
{
    st->h6_OvlFrame = HVS6_DISPSTAT_FRCNT(hvs6_rd(HVS6_CHAN(st->h6_Chan)
                                                  + HVS6_DISPSTAT));
    st->h6_OvlLatchDue = TRUE;
}

static void hvs6_ovl_latch_wait(struct vc4_hvs6_state *st)
{
    ULONG spin;

    if (!st->h6_OvlLatchDue)
        return;
    st->h6_OvlLatchDue = FALSE;

    for (spin = 0; spin < HVS6_SPIN_LATCH; spin++)
        if (HVS6_DISPSTAT_FRCNT(hvs6_rd(HVS6_CHAN(st->h6_Chan) + HVS6_DISPSTAT))
            != st->h6_OvlFrame)
            return;

    bug("[VC4HVS6] overlay: frame counter stuck at %lu\n",
        (unsigned long)st->h6_OvlFrame);
}

/* Show or hide the overlay plane. Scaled or clipped overlays are refused
 * and the caller blits instead. */
BOOL vc4_hvs6_overlay(struct VideoCoreGfx_staticdata *xsd,
                      struct vc4gfx_overlay *ovl)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG out_w, out_h, list, geom, pos;

    if (!st->h6_Active)
        return FALSE;

    if (!ovl)
    {
        if (st->h6_Overlay)
        {
            for (list = 0; list < HVS6_LISTS; list++)
                hvs6_wr(hvs6_slot(st, list, HVS6_SLOT_OVL), HVS6_CTL0_EMPTY);
            st->h6_Overlay = FALSE;
            hvs6_ovl_arm(st);
        }
        return TRUE;
    }

    if (!ovl->ovl_Phys || !ovl->ovl_Pitch || !ovl->ovl_Width || !ovl->ovl_Height)
        return FALSE;

    if ((ovl->ovl_DestW && (ovl->ovl_DestW != ovl->ovl_Width))
        || (ovl->ovl_DestH && (ovl->ovl_DestH != ovl->ovl_Height)))
        return FALSE;

    out_w = hvs6_rd(HVS6_CHAN(st->h6_Chan) + HVS6_DISPCTRL);
    out_h = HVS6_DISPCTRL_H(out_w);
    out_w = HVS6_DISPCTRL_W(out_w);

    if ((ovl->ovl_X < 0) || (ovl->ovl_Y < 0)
        || ((ULONG)ovl->ovl_X + ovl->ovl_Width > out_w)
        || ((ULONG)ovl->ovl_Y + ovl->ovl_Height > out_h))
        return FALSE;

    hvs6_upm_carve(st);

    geom = (ovl->ovl_Width << 16) | ovl->ovl_Height;
    pos  = HVS6_POS0((ULONG)ovl->ovl_X, (ULONG)ovl->ovl_Y);

    for (list = 0; list < HVS6_LISTS; list++)
    {
        ULONG ent = hvs6_slot(st, list, HVS6_SLOT_OVL);

        if (st->h6_Overlay && (st->h6_OvlGeom == geom)
            && (st->h6_OvlPitch == ovl->ovl_Pitch))
        {
            /* Live entry: patch single words, never rebuild it */
            if (st->h6_OvlPos != pos)
                hvs6_wr(ent + HVS6_ENT_POS0 * 4, pos);
            hvs6_wr(ent + HVS6_ENT_PTR1 * 4, ovl->ovl_Phys);
        }
        else
        {
            hvs6_plane(ent, hvs6_slot(st, list, HVS6_SLOT_FB),
                       (UQUAD)ovl->ovl_Phys, ovl->ovl_Pitch, st->h6_OvlPTR0,
                       (ULONG)ovl->ovl_X, (ULONG)ovl->ovl_Y,
                       ovl->ovl_Width, ovl->ovl_Height);

            /* Gallium output is the opposite byte order to the fb */
            hvs6_wr(ent + HVS6_ENT_CTL0 * 4,
                    hvs6_rd(ent + HVS6_ENT_CTL0 * 4) & ~HVS6_CTL0_RGBA);
        }
    }

    st->h6_OvlGeom  = geom;
    st->h6_OvlPos   = pos;
    st->h6_OvlPitch = ovl->ovl_Pitch;
    st->h6_Overlay  = TRUE;

    /* NOWAIT callers wait later via aoHidd_VideoCoreGfxBitMap_LatchWait */
    hvs6_ovl_arm(st);
    if (!(ovl->ovl_Flags & VC4GFX_OVL_NOWAIT))
        hvs6_ovl_latch_wait(st);
    return TRUE;
}

void vc4_hvs6_latch_wait(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;

    if (st->h6_Active)
        hvs6_ovl_latch_wait(st);
}

/* No firmware ALLOCMEM on this generation, so allocate the cursor buffer */
BOOL vc4_hvs6_init_cursor(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    IPTR page;

    if (!VC4_HVS6_CURSOR)
        return FALSE;

    xsd->vcsd_CurBufHandle = 0;
    xsd->vcsd_CurBufBus    = 0;
    xsd->vcsd_CurVisible   = FALSE;

    st->h6_CurRaw = AllocMem(VC4_CURSOR_BUF_BYTES + 4095,
                             MEMF_ANY | MEMF_CLEAR);
    if (!st->h6_CurRaw)
        return FALSE;

    page = ((IPTR)st->h6_CurRaw + 4095) & ~(IPTR)4095;
    st->h6_CurPhys   = (UQUAD)page;
    xsd->vcsd_CurBuf = (APTR)page;
    CacheClearE(xsd->vcsd_CurBuf, VC4_CURSOR_BUF_BYTES, CACRF_ClearD);

    D(bug("[VC4HVS6] cursor buffer @ 0x%p\n", xsd->vcsd_CurBuf));
    return TRUE;
}

/* The HVS does not clip planes: clip by offsetting the source address
 * and shrinking the extent, keeping the full pitch. */
void vc4_hvs6_cursor(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG pitch, out_w, out_h, w, h, list, geom, pos, addr;
    UQUAD phys;
    LONG  x, y;

    if (!st->h6_Active)
        return;

    if (!xsd->vcsd_CurVisible || !xsd->vcsd_CurBuf
        || !xsd->vcsd_CurWidth || !xsd->vcsd_CurHeight)
        goto hide;

    x     = xsd->vcsd_CurX;
    y     = xsd->vcsd_CurY;
    w     = xsd->vcsd_CurWidth;
    h     = xsd->vcsd_CurHeight;
    pitch = w * 4;
    phys  = st->h6_CurPhys;

    out_w = hvs6_rd(HVS6_CHAN(st->h6_Chan) + HVS6_DISPCTRL);
    out_h = HVS6_DISPCTRL_H(out_w);
    out_w = HVS6_DISPCTRL_W(out_w);

    if (x < 0)
    {
        if ((ULONG)-x >= w)
            goto hide;
        phys += (ULONG)-x * 4;
        w    -= (ULONG)-x;
        x     = 0;
    }
    if (y < 0)
    {
        if ((ULONG)-y >= h)
            goto hide;
        phys += (ULONG)-y * pitch;
        h    -= (ULONG)-y;
        y     = 0;
    }
    if (((ULONG)x >= out_w) || ((ULONG)y >= out_h))
        goto hide;
    if ((ULONG)x + w > out_w)
        w = out_w - (ULONG)x;
    if ((ULONG)y + h > out_h)
        h = out_h - (ULONG)y;

    hvs6_upm_carve(st);

    geom = (w << 16) | h;
    pos  = HVS6_POS0((ULONG)x, (ULONG)y);
    addr = (ULONG)(phys & 0xffffffffULL);

    for (list = 0; list < HVS6_LISTS; list++)
    {
        ULONG ent = hvs6_slot(st, list, HVS6_SLOT_CUR);

        if (st->h6_Cursor && (st->h6_CurGeom == geom)
            && (st->h6_CurAddr == addr))
        {
            /* Rebuilding a live entry hides a moving pointer */
            hvs6_wr(ent + HVS6_ENT_POS0 * 4, pos);
            continue;
        }

        hvs6_plane(ent, hvs6_slot(st, list, HVS6_SLOT_FB),
                   phys, pitch, st->h6_CurPTR0, (ULONG)x, (ULONG)y, w, h);

        /* Per-pixel alpha */
        hvs6_wr(ent + HVS6_ENT_CTL0 * 4,
                hvs6_rd(ent + HVS6_ENT_CTL0 * 4) & ~HVS6_CTL0_ALPHAMASK);
    }

    st->h6_CurGeom = geom;
    st->h6_CurPos  = pos;
    st->h6_CurAddr = addr;
    st->h6_Cursor  = TRUE;
    return;

hide:
    if (st->h6_Cursor)
    {
        for (list = 0; list < HVS6_LISTS; list++)
            hvs6_wr(hvs6_slot(st, list, HVS6_SLOT_CUR), HVS6_CTL0_EMPTY);
        st->h6_Cursor = FALSE;
    }
}
