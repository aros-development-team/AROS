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

/* Vblank from pixelvalve 0 (GIC SPI 101); the bit is measured, as the
 * documented one was wrong on VideoCore IV */
#define HVS6_PV0          0x410000
#define HVS6_PV_INTEN     0x24
#define HVS6_PV_INTSTAT   0x28
#define HVS6_PV_INT_ALL   0x3ff
#define HVS6_PV0_IRQ      (32 + 101)
#define HVS6_PROBE_US     60000     /* per bit: ~4 frames at 60 Hz        */
#define HVS6_WAIT_US      50000     /* a latch wait gives up after this   */
#define HVS6_LIVE_US      40000     /* a tick this recent means sleep     */
#define HVS6_SETTLE_US    2000      /* from the tick to the latch, spun   */

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

    /* Screens that are planes get theirs from what is left below */
    st->h6_UpmTop   = top * HVS6_UPM_GRAN;
    st->h6_UpmFloor = HVS6_UPM_FW_RESERVE;

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

static inline volatile ULONG *hvs6_pv0(void)
{
    return (volatile ULONG *)(ARM_PERIIOBASE + HVS6_PV0);
}

/* Interrupt context: count, date and wake - no printing */
static void hvs6_vsync_irq(struct vc4_hvs6_state *st, struct ExecBase *sysBase)
{
    volatile ULONG *pv = hvs6_pv0();
    ULONG stat = pv[HVS6_PV_INTSTAT / 4];
    struct vc4gfx_vblank *vbl;

    if (!stat)
        return;
    pv[HVS6_PV_INTSTAT / 4] = stat;                 /* W1C */

    if (stat & st->h6_VSyncMask)
    {
        st->h6_VSyncCount++;
        st->h6_VSyncStamp = hvs6_now_us();
        if (st->h6_VSyncTask)
            Signal(st->h6_VSyncTask, st->h6_VSyncSigMask);
        if ((vbl = st->h6_VBlank))
            vbl->vbl_Func(vbl->vbl_Data);
    }
}

/* Install the vblank handler and find the per-frame bit, once at driver
 * init: KrnAddIRQHandler allocates in supervisor mode, so never mid mode
 * set. Measured against the HVS frame counter, the spec's bit first, in
 * short windows so a line-rate bit cannot trip the stuck-IRQ guard. */
void vc4_hvs6_irq_init(struct VideoCoreGfx_staticdata *xsd)
{
    static const UBYTE order[] = { 7, 0, 1, 2, 3, 4, 5, 6, 8, 9 };
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    volatile ULONG *pv = hvs6_pv0();
    ULONG i, b = 0, c0, f0, frames = 0, ticks = 0, start;

    if ((hvs6_rd(HVS6_ID) != HVS6_ID_MAGIC) || !(pv[0] & 1)
        || !(hvs6_rd(HVS6_CHAN(0) + HVS6_DISPCTRL) & HVS6_DISPCTRL_EN))
    {
        bug("[VC4HVS6] vsync: pixelvalve 0 is not running - flips stay unpaced\n");
        return;
    }

    st->h6_VSyncIrq = KrnAddIRQHandler(HVS6_PV0_IRQ, hvs6_vsync_irq, st, SysBase);
    if (!st->h6_VSyncIrq)
    {
        bug("[VC4HVS6] vsync: KrnAddIRQHandler failed\n");
        return;
    }

    for (i = 0; i < sizeof(order); i++)
    {
        b = order[i];
        pv[HVS6_PV_INTEN / 4]   = 0;
        pv[HVS6_PV_INTSTAT / 4] = HVS6_PV_INT_ALL;
        c0 = st->h6_VSyncCount;
        f0 = HVS6_DISPSTAT_FRCNT(hvs6_rd(HVS6_CHAN(0) + HVS6_DISPSTAT));

        st->h6_VSyncMask      = 1UL << b;
        pv[HVS6_PV_INTEN / 4] = 1UL << b;
        start = hvs6_now_us();
        while ((hvs6_now_us() - start) < HVS6_PROBE_US)
            ;
        pv[HVS6_PV_INTEN / 4]   = 0;
        pv[HVS6_PV_INTSTAT / 4] = HVS6_PV_INT_ALL;

        ticks  = st->h6_VSyncCount - c0;
        frames = (HVS6_DISPSTAT_FRCNT(hvs6_rd(HVS6_CHAN(0) + HVS6_DISPSTAT)) - f0) & 0x3f;
        if ((frames >= 2) && (ticks + 1 >= frames) && (ticks <= frames + 1))
            break;
    }

    if (i == sizeof(order))
    {
        st->h6_VSyncMask = 0;
        bug("[VC4HVS6] vsync: no INTEN bit runs at frame rate - flips stay unpaced\n");
        return;
    }

    pv[HVS6_PV_INTEN / 4] = st->h6_VSyncMask;
    bug("[VC4HVS6] vsync: pixelvalve 0 INTEN bit %u, %u ticks in %u frames\n",
        b, ticks, frames);
}

/* Sleep until the next vblank if the interrupt runs and the one waiter
 * slot is free; FALSE otherwise, and the caller spins. Any signal ends
 * the sleep; the caller re-checks either way. */
static BOOL hvs6_vsync_sleep(struct vc4_hvs6_state *st)
{
    ULONG target;
    BYTE sig;

    if (!st->h6_VSyncMask || st->h6_VSyncTask
        || ((hvs6_now_us() - st->h6_VSyncStamp) >= HVS6_LIVE_US))
        return FALSE;
    if ((sig = AllocSignal(-1)) == -1)
        return FALSE;

    st->h6_VSyncSigMask = 1UL << sig;
    target = st->h6_VSyncCount + 1;
    Disable();
    st->h6_VSyncTask = FindTask(NULL);
    Enable();

    /* Clear, re-test, sleep: a tick in the gap leaves the signal set */
    SetSignal(0, st->h6_VSyncSigMask);
    if ((LONG)(st->h6_VSyncCount - target) < 0)
        Wait(st->h6_VSyncSigMask);

    Disable();
    st->h6_VSyncTask = NULL;
    Enable();
    FreeSignal(sig);
    return TRUE;
}

/* Wait until (reg & mask) == val, or != val when !equal: sleep on the
 * vblank, spin briefly for the latch, give up after HVS6_WAIT_US */
static BOOL hvs6_frame_wait(struct vc4_hvs6_state *st, ULONG reg, ULONG mask,
                            ULONG val, BOOL equal)
{
    ULONG start = hvs6_now_us(), t;

    for (;;)
    {
        if (((hvs6_rd(reg) & mask) == val) == equal)
            return TRUE;
        if ((hvs6_now_us() - start) >= HVS6_WAIT_US)
            return FALSE;

        if (hvs6_vsync_sleep(st))
        {
            t = hvs6_now_us();
            while ((((hvs6_rd(reg) & mask) == val) != equal)
                   && ((hvs6_now_us() - t) < HVS6_SETTLE_US))
                ;
        }
    }
}

/* Wait for the latch, or the caller draws into the page being scanned */
BOOL vc4_hvs6_flip_page(struct VideoCoreGfx_staticdata *xsd, ULONG page_phys)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG which;

    if (!st->h6_Active || (st->h6_Pages < 2))
        return FALSE;

    for (which = 0; which < HVS6_LISTS; which++)
        if (hvs6_rd(hvs6_slot(st, which, HVS6_SLOT_FB) + HVS6_ENT_PTR1 * 4)
            == page_phys)
            break;

    if (which == HVS6_LISTS)
        return FALSE;

    hvs6_wr(HVS6_CHAN(st->h6_Chan) + HVS6_DISPLIST, st->h6_List[which]);

    if (hvs6_frame_wait(st, HVS6_CHAN(st->h6_Chan) + HVS6_DISPLACT, 0xffffffff,
                        st->h6_List[which], TRUE))
        return TRUE;

    bug("[VC4HVS6] flip to %#06x never latched\n", st->h6_List[which]);
    return FALSE;
}

/* The replaced buffer is scanned until FRCNT moves on; bounded in time */
static void hvs6_ovl_arm(struct vc4_hvs6_state *st)
{
    st->h6_OvlFrame = HVS6_DISPSTAT_FRCNT(hvs6_rd(HVS6_CHAN(st->h6_Chan)
                                                  + HVS6_DISPSTAT));
    st->h6_OvlLatchDue = TRUE;
}

static BOOL hvs6_planes_overlay(struct VideoCoreGfx_staticdata *xsd,
                                const struct vc4gfx_overlay *ovl, OOP_Object *owner);

static void hvs6_ovl_latch_wait(struct vc4_hvs6_state *st)
{
    if (!st->h6_OvlLatchDue)
        return;
    st->h6_OvlLatchDue = FALSE;

    if (hvs6_frame_wait(st, HVS6_CHAN(st->h6_Chan) + HVS6_DISPSTAT, 0x3fUL << 16,
                        st->h6_OvlFrame << 16, FALSE))
        return;

    bug("[VC4HVS6] overlay: frame counter stuck at %lu\n",
        (unsigned long)st->h6_OvlFrame);
}

/* Show or hide the overlay plane. Scaled overlays are refused, and
 * clipped ones unless screens are planes; the caller blits instead. */
BOOL vc4_hvs6_overlay(struct VideoCoreGfx_staticdata *xsd,
                      const struct vc4gfx_overlay *ovl, OOP_Object *owner)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG out_w, out_h, list, geom, pos;

    if (!st->h6_Active)
        return FALSE;

    if (xsd->vcsd_PlanesMode)
        return hvs6_planes_overlay(xsd, ovl, owner);

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

            /* Gallium renders B,G,R,A; the fb entry we copied reads R,G,B,A */
            hvs6_wr(ent + HVS6_ENT_CTL0 * 4,
                    (hvs6_rd(ent + HVS6_ENT_CTL0 * 4) & ~HVS6_CTL0_ORDER) | HVS6_ORDER_BGRA);
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

/* The HVS does not clip planes: clip to [l,r) x [t,b) by offsetting the
 * source address and shrinking the extent, keeping the full pitch. FALSE
 * when nothing is left. */
static BOOL hvs6_clip(LONG *x, LONG *y, ULONG *w, ULONG *h, UQUAD *phys,
                      ULONG pitch, LONG l, LONG t, LONG r, LONG b)
{
    if (*x < l)
    {
        ULONG skip = (ULONG)(l - *x);

        if (skip >= *w)
            return FALSE;
        *phys += skip * 4;
        *w    -= skip;
        *x     = l;
    }
    if (*y < t)
    {
        ULONG skip = (ULONG)(t - *y);

        if (skip >= *h)
            return FALSE;
        *phys += (UQUAD)skip * pitch;
        *h    -= skip;
        *y     = t;
    }
    if ((*x >= r) || (*y >= b))
        return FALSE;
    if (*x + (LONG)*w > r)
        *w = (ULONG)(r - *x);
    if (*y + (LONG)*h > b)
        *h = (ULONG)(b - *y);
    return TRUE;
}

/* The cursor plane's source and place on screen; FALSE when hidden. */
static BOOL hvs6_cursor_geom(struct VideoCoreGfx_staticdata *xsd, UQUAD *phys,
                             ULONG *pitch, LONG *x, LONG *y, ULONG *w, ULONG *h)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG out = hvs6_rd(HVS6_CHAN(st->h6_Chan) + HVS6_DISPCTRL);

    if (!xsd->vcsd_CurVisible || !xsd->vcsd_CurBuf
        || !xsd->vcsd_CurWidth || !xsd->vcsd_CurHeight)
        return FALSE;

    *x     = xsd->vcsd_CurX;
    *y     = xsd->vcsd_CurY;
    *w     = xsd->vcsd_CurWidth;
    *h     = xsd->vcsd_CurHeight;
    *pitch = *w * 4;
    *phys  = st->h6_CurPhys;

    return hvs6_clip(x, y, w, h, phys, *pitch, 0, 0,
                     (LONG)HVS6_DISPCTRL_W(out), (LONG)HVS6_DISPCTRL_H(out));
}

/* Byte offset of a slot in one of the planes lists */
static inline ULONG hvs6_pslot(struct vc4_hvs6_state *st, ULONG list, ULONG n)
{
    return (st->h6_ListBase + list + n * HVS6_SLOT_WORDS) * 4;
}

void vc4_hvs6_cursor(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG pitch, w, h, list, geom, pos, addr, ents[HVS6_LISTS], n = 0;
    UQUAD phys;
    LONG  x, y;

    if (!st->h6_Active)
        return;

    /* Screens as planes: the newest list, which carries the cursor next */
    if (xsd->vcsd_PlanesMode)
    {
        if (!st->h6_PlLast)
            return;
        ents[n++] = hvs6_pslot(st, st->h6_PlLast, HVS6_PL_CUR);
    }
    else
    {
        for (list = 0; list < HVS6_LISTS; list++)
            ents[n++] = hvs6_slot(st, list, HVS6_SLOT_CUR);
    }

    if (!hvs6_cursor_geom(xsd, &phys, &pitch, &x, &y, &w, &h))
        goto hide;

    hvs6_upm_carve(st);

    geom = (w << 16) | h;
    pos  = HVS6_POS0((ULONG)x, (ULONG)y);
    addr = (ULONG)(phys & 0xffffffffULL);

    for (list = 0; list < n; list++)
    {
        ULONG ent = ents[list];

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
        for (list = 0; list < n; list++)
            hvs6_wr(ents[list], HVS6_CTL0_EMPTY);
        st->h6_Cursor = FALSE;
    }
}

/* ------------------------------------------------------------------ */
/* Screens as planes                                                   */
/* ------------------------------------------------------------------ */

/*
 * No firmware list shows a scaled HVS6 entry, so this one is DISPLAY-SPEC
 * 5.3's word order filled in the way HVS5's measured entry is. It ran on
 * a BCM2712 D0 (1024x768 to 1280x720): the HVS wrote [5] and [12] as context,
 * and a 1.067 shrink used the ratio field's bit 24. The first list that
 * uses one is still checked to latch and run; if not, scaling turns off.
 */
#define VC4_HVS6_SCALE      1

#define HVS6_LBM_SIZE       0x0008
#define HVS6_FETCHER_STATUS 0x0024
#define HVS6_HANDLE_ERROR   0x002c
#define HVS6_KERNEL_WORD    1           /* firmware's kernel at +0x4004 */
#define HVS6_SC_WORDS       17
#define HVS6_PTR0_LINES4    (1UL << 8)
#define HVS6_SCL_MASK       ((7UL << 8) | (7UL << 5))
#define HVS6_PPF(src, dst)  ((1UL << 30) | ((((ULONG)(src) << 16) / (ULONG)(dst)) << 8) | 0x60)
#define HVS6_PPF_FITS(src, dst) ((((ULONG)(src) << 16) / (ULONG)(dst)) <= 0x18000)
#define HVS6_SCALE_MAX      2           /* scaled planes per list */
#define HVS6_VERIFY_US      100000

BOOL vc4_hvs6_planes_ok(void)
{
    return VC4_HVS6_TAKEOVER;
}

BOOL vc4_hvs6_scales(struct VideoCoreGfx_staticdata *xsd)
{
    return VC4_HVS6_SCALE && !xsd->vcsd_HVS6.h6_ScaleOff;
}

void vc4_hvs6_out_size(struct VideoCoreGfx_staticdata *xsd, ULONG *w, ULONG *h)
{
    ULONG out = hvs6_rd(HVS6_CHAN(xsd->vcsd_HVS6.h6_Chan) + HVS6_DISPCTRL);

    *w = HVS6_DISPCTRL_W(out);
    *h = HVS6_DISPCTRL_H(out);
}

/* Can a screen of a w x h mode fill the output without a mode set? */
BOOL vc4_hvs6_plane_fits(struct VideoCoreGfx_staticdata *xsd, ULONG w, ULONG h)
{
    ULONG ow, oh;

    vc4_hvs6_out_size(xsd, &ow, &oh);
    if ((ow == w) && (oh == h))
        return TRUE;
    return vc4_hvs6_scales(xsd) && ow && oh
        && HVS6_PPF_FITS(w, ow) && HVS6_PPF_FITS(h, oh);
}

/* Prefetch for a plane of this pitch, lines deep */
static ULONG hvs6_upm_size(ULONG pitch, ULONG lines)
{
    return ((((pitch + 62) / 32) * 32 * lines) + HVS6_UPM_GRAN - 1)
           & ~(HVS6_UPM_GRAN - 1);
}

/* Three lists of HVS6_PL_SLOTS, found once and marked used at once so no
 * later free-slot search takes them. */
static BOOL hvs6_planes_lists(struct vc4_hvs6_state *st)
{
    ULONG at, i, n;

    if (st->h6_PlList[0])
        return TRUE;

    if (!hvs6_free_slots(st->h6_ListBase, HVS6_PL_LISTS * HVS6_PL_SLOTS, &at))
    {
        bug("[VC4HVS6] no free list slots for screen planes\n");
        return FALSE;
    }

    for (i = 0; i < HVS6_PL_LISTS; i++)
    {
        st->h6_PlList[i] = at + i * HVS6_PL_SLOTS * HVS6_SLOT_WORDS;
        for (n = 0; n < HVS6_PL_END; n++)
            hvs6_wr(hvs6_pslot(st, st->h6_PlList[i], n), HVS6_CTL0_EMPTY);
        hvs6_wr(hvs6_pslot(st, st->h6_PlList[i], HVS6_PL_END), HVS6_CTL0_END);
    }
    return TRUE;
}

/* A screen's rectangle on the output: from its own mode's coordinates
 * when scaling, so it fills the output as that mode would. */
static void hvs6_plane_rect(struct VideoCoreGfx_staticdata *xsd, struct vcgfx_plane *pl,
                            LONG out_w, LONG out_h, LONG *dx, LONG *dy, ULONG *dw, ULONG *dh)
{
    struct BitmapData *d = pl->pl_Data;
    LONG cw = out_w, ch = out_h;

    if (vc4_hvs6_scales(xsd) && d->modew && d->modeh)
    {
        cw = (LONG)d->modew;
        ch = (LONG)d->modeh;
    }
    *dx = pl->pl_X * out_w / cw;
    *dy = pl->pl_Y * out_h / ch;
    *dw = d->width * (ULONG)out_w / (ULONG)cw;
    *dh = d->height * (ULONG)out_h / (ULONG)ch;
}

/* A scaled entry, after the fb entry's CTL0/CTL2; CTL0 goes in last. */
static void hvs6_scaled(ULONG dst, ULONG fb, UQUAD addr, ULONG pitch, ULONG ptr0,
                        LONG dx, LONG dy, ULONG dw, ULONG dh, ULONG sw, ULONG sh,
                        ULONG lbm)
{
    ULONG ctl0 = hvs6_rd(fb + HVS6_ENT_CTL0 * 4) & ~(HVS6_CTL0_UNITY | HVS6_SCL_MASK);
    ULONG i;

    hvs6_wr(dst, HVS6_CTL0_EMPTY);
    hvs6_wr(dst + 1 * 4, HVS6_POS0((ULONG)dx, (ULONG)dy));
    hvs6_wr(dst + 2 * 4, hvs6_rd(fb + HVS6_ENT_CTL2 * 4));
    hvs6_wr(dst + 3 * 4, ((dh - 1) << 16) | ((dw - 1) & 0x1fff));
    hvs6_wr(dst + 4 * 4, ((sh - 1) << 16) | ((sw - 1) & 0x1fff));
    hvs6_wr(dst + 5 * 4, 0xc0c0c0c0);
    hvs6_wr(dst + 6 * 4, ptr0 | HVS6_PTR0_UPPER(addr));
    hvs6_wr(dst + 7 * 4, (ULONG)(addr & 0xffffffffULL));
    hvs6_wr(dst + 8 * 4, pitch);
    hvs6_wr(dst + 9 * 4, lbm);
    hvs6_wr(dst + 10 * 4, HVS6_PPF(sw, dw));
    hvs6_wr(dst + 11 * 4, HVS6_PPF(sh, dh));
    hvs6_wr(dst + 12 * 4, 0xc0c0c0c0);
    for (i = 13; i < HVS6_SC_WORDS; i++)
        hvs6_wr(dst + i * 4, HVS6_KERNEL_WORD);
    hvs6_wr(dst, ctl0);
}

/*
 * One list: the screens back to front, each cut off where a screen in
 * front spans the output's width down to the bottom of what is still
 * visible; the fb, kept black, as backdrop where none covers; the overlay
 * over the front screen it belongs to; the cursor. UPM goes to the front
 * screens first, and a screen left without any is not shown. Returns the
 * number of scaled planes.
 */
static ULONG hvs6_build_planes(struct VideoCoreGfx_staticdata *xsd, ULONG list)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG fb = hvs6_slot(st, 0, HVS6_SLOT_FB);
    ULONG out = hvs6_rd(HVS6_CHAN(st->h6_Chan) + HVS6_DISPCTRL);
    LONG out_w = (LONG)HVS6_DISPCTRL_W(out), out_h = (LONG)HVS6_DISPCTRL_H(out);
    LONG limit[VCGFX_MAX_PLANES], bottom = out_h, front = (LONG)xsd->vcsd_NumPlanes - 1;
    LONG px[VCGFX_MAX_PLANES], py[VCGFX_MAX_PLANES];
    ULONG pw[VCGFX_MAX_PLANES], ph[VCGFX_MAX_PLANES];
    ULONG ptr0[VCGFX_MAX_PLANES] = { 0 };
    ULONG upm = st->h6_UpmTop, lbm = 0, scaled = 0, i, pitch, w, h, ent;
    UQUAD phys;
    LONG x, y, j;

    for (j = front; j >= 0; j--)
    {
        struct vcgfx_plane *pl = &xsd->vcsd_Plane[j];
        struct BitmapData *d = pl->pl_Data;
        BOOL sc;
        ULONG size;

        hvs6_plane_rect(xsd, pl, out_w, out_h, &px[j], &py[j], &pw[j], &ph[j]);
        sc = (pw[j] != d->width) || (ph[j] != d->height);
        size = hvs6_upm_size(d->bytesperrow, sc ? 4 : HVS6_UPM_LINES);

        limit[j] = bottom;
        if (!pw[j] || !ph[j] || (limit[j] <= py[j]) || (limit[j] <= 0)
            || (upm < st->h6_UpmFloor + size))
            continue;

        upm -= size;
        ptr0[j] = HVS6_PTR0_BASE(upm / HVS6_UPM_GRAN)
                | HVS6_PTR0_HANDLE(HVS6_PL_HANDLE + j)
                | (sc ? HVS6_PTR0_LINES4 : HVS6_PTR0_LINES2);

        if ((px[j] <= 0) && (px[j] + (LONG)pw[j] >= out_w)
            && (py[j] < bottom) && (py[j] + (LONG)ph[j] >= bottom))
            bottom = (py[j] > 0) ? py[j] : 0;
    }

    ent = hvs6_pslot(st, list, HVS6_PL_BACK);
    if (bottom > 0)
    {
        for (i = 0; i < HVS6_SLOT_WORDS; i++)
            hvs6_wr(ent + i * 4, hvs6_rd(fb + i * 4));
    }
    else
        hvs6_wr(ent, HVS6_CTL0_EMPTY);

    for (i = 0; i < VCGFX_MAX_PLANES; i++)
    {
        struct vcgfx_plane *pl = &xsd->vcsd_Plane[i];
        struct BitmapData *d;
        ULONG ow, oh, sw, sh;
        LONG ox, oy;

        ent = hvs6_pslot(st, list, HVS6_PL_SCREEN + i);
        if ((i >= xsd->vcsd_NumPlanes) || !ptr0[i])
        {
            hvs6_wr(ent, HVS6_CTL0_EMPTY);
            continue;
        }

        d = pl->pl_Data;
        pitch = d->bytesperrow;
        phys = (UQUAD)(IPTR)d->VideoData;

        if ((pw[i] == d->width) && (ph[i] == d->height))
        {
            x = px[i];
            y = py[i];
            w = pw[i];
            h = ph[i];
            if (hvs6_clip(&x, &y, &w, &h, &phys, pitch, 0, 0, out_w, limit[i]))
                hvs6_plane(ent, fb, phys, pitch, ptr0[i], (ULONG)x, (ULONG)y, w, h);
            else
                hvs6_wr(ent, HVS6_CTL0_EMPTY);
            continue;
        }

        /* Clip on screen and trim the source in proportion. */
        ox = px[i]; oy = py[i]; ow = pw[i]; oh = ph[i];
        sw = d->width; sh = d->height;
        if (ox < 0)
        {
            ULONG skip = (ULONG)-ox, ssk = skip * sw / ow;

            phys += ssk * 4; sw -= ssk; ow -= skip; ox = 0;
        }
        if (oy < 0)
        {
            ULONG skip = (ULONG)-oy, ssk = skip * sh / oh;

            phys += (UQUAD)ssk * pitch; sh -= ssk; oh -= skip; oy = 0;
        }
        if (ox + (LONG)ow > out_w)
        {
            ULONG keep = (out_w > ox) ? (ULONG)(out_w - ox) : 0;

            sw = sw * keep / ow; ow = keep;
        }
        if (oy + (LONG)oh > limit[i])
        {
            ULONG keep = (limit[i] > oy) ? (ULONG)(limit[i] - oy) : 0;

            sh = sh * keep / oh; oh = keep;
        }

        if (!ow || !oh || !sw || !sh || (scaled >= HVS6_SCALE_MAX)
            || !HVS6_PPF_FITS(sw, ow) || !HVS6_PPF_FITS(sh, oh))
        {
            hvs6_wr(ent, HVS6_CTL0_EMPTY);
            continue;
        }

        hvs6_scaled(ent, fb, phys, pitch, ptr0[i], ox, oy, ow, oh, sw, sh, lbm);
        lbm += d->width * 8;
        scaled++;
    }

    /* The overlay slot is above every screen: the front one only, unscaled,
     * and no further out than it. */
    ent = hvs6_pslot(st, list, HVS6_PL_OVL);
    if (st->h6_Overlay && (front >= 0) && ptr0[front]
        && (st->h6_OvlOwner == xsd->vcsd_Plane[front].pl_BM)
        && (pw[front] == xsd->vcsd_Plane[front].pl_Data->width)
        && (ph[front] == xsd->vcsd_Plane[front].pl_Data->height))
    {
        struct vcgfx_plane *pl = &xsd->vcsd_Plane[front];
        LONG r = pl->pl_X + (LONG)pl->pl_Data->width;
        LONG b = pl->pl_Y + (LONG)pl->pl_Data->height;

        x = pl->pl_X + st->h6_OvlX;
        y = pl->pl_Y + st->h6_OvlY;
        w = st->h6_OvlW;
        h = st->h6_OvlH;
        phys = st->h6_OvlPhys;
        if (hvs6_clip(&x, &y, &w, &h, &phys, st->h6_OvlPitch,
                      (pl->pl_X > 0) ? pl->pl_X : 0, (pl->pl_Y > 0) ? pl->pl_Y : 0,
                      (r < out_w) ? r : out_w, (b < limit[front]) ? b : limit[front]))
        {
            hvs6_plane(ent, fb, phys, st->h6_OvlPitch, st->h6_OvlPTR0,
                       (ULONG)x, (ULONG)y, w, h);
            hvs6_wr(ent + HVS6_ENT_CTL0 * 4,
                    (hvs6_rd(ent + HVS6_ENT_CTL0 * 4) & ~HVS6_CTL0_ORDER) | HVS6_ORDER_BGRA);
        }
        else
            hvs6_wr(ent, HVS6_CTL0_EMPTY);
    }
    else
        hvs6_wr(ent, HVS6_CTL0_EMPTY);

    ent = hvs6_pslot(st, list, HVS6_PL_CUR);
    st->h6_Cursor = hvs6_cursor_geom(xsd, &phys, &pitch, &x, &y, &w, &h);
    if (st->h6_Cursor)
    {
        hvs6_plane(ent, fb, phys, pitch, st->h6_CurPTR0, (ULONG)x, (ULONG)y, w, h);
        hvs6_wr(ent + HVS6_ENT_CTL0 * 4,
                hvs6_rd(ent + HVS6_ENT_CTL0 * 4) & ~HVS6_CTL0_ALPHAMASK);
        st->h6_CurGeom = (w << 16) | h;
        st->h6_CurPos  = HVS6_POS0((ULONG)x, (ULONG)y);
        st->h6_CurAddr = (ULONG)(phys & 0xffffffffULL);
    }
    else
        hvs6_wr(ent, HVS6_CTL0_EMPTY);

    hvs6_wr(hvs6_pslot(st, list, HVS6_PL_END), HVS6_CTL0_END);
    return scaled;
}

/* The list with the first scaled plane: it must latch, and the frame
 * counter must keep going. FALSE when the HVS did not take it. */
static BOOL hvs6_scale_verify(struct vc4_hvs6_state *st, ULONG list)
{
    ULONG chan = HVS6_CHAN(st->h6_Chan), start = hvs6_now_us(), f0 = 0, i, slot;
    BOOL latched = FALSE, ran = FALSE;

    while ((hvs6_now_us() - start) < HVS6_VERIFY_US)
    {
        if (!latched && (hvs6_rd(chan + HVS6_DISPLACT) == list))
        {
            latched = TRUE;
            f0 = HVS6_DISPSTAT_FRCNT(hvs6_rd(chan + HVS6_DISPSTAT));
        }
        if (latched && (HVS6_DISPSTAT_FRCNT(hvs6_rd(chan + HVS6_DISPSTAT)) != f0))
        {
            ran = TRUE;
            break;
        }
    }

    bug("[VC4HVS6] scaled planes: %s, status %08x, fetcher %08x, handle error %08x,"
        " lbm %u\n", ran ? "running" : (latched ? "latched, no frames" : "never latched"),
        hvs6_rd(chan + HVS6_DISPSTAT), hvs6_rd(HVS6_FETCHER_STATUS),
        hvs6_rd(HVS6_HANDLE_ERROR), hvs6_rd(HVS6_LBM_SIZE));
    for (slot = HVS6_PL_SCREEN; slot < HVS6_PL_OVL; slot++)
    {
        ULONG ent = hvs6_pslot(st, list, slot);

        if (hvs6_rd(ent) & HVS6_CTL0_UNITY || !(hvs6_rd(ent) & HVS6_CTL0_VALID))
            continue;
        bug("[VC4HVS6]   slot %u:", slot);
        for (i = 0; i < HVS6_SC_WORDS; i++)
            bug(" %08x", hvs6_rd(ent + i * 4));
        bug("\n");
    }
    return ran;
}

/* Build into the list neither latched nor last written, and repoint: the
 * HVS takes it at the next frame start, whole. Caller holds the mailbox
 * lock. Armed like an overlay write, so LatchWait tells when the pages
 * the old list showed are out of use. */
void vc4_hvs6_planes_commit(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG act, list = 0, i;

    if (!st->h6_Active || !hvs6_planes_lists(st))
        return;

    hvs6_upm_carve(st);

    act = hvs6_rd(HVS6_CHAN(st->h6_Chan) + HVS6_DISPLACT);
    for (i = 0; i < HVS6_PL_LISTS; i++)
    {
        list = st->h6_PlList[i];
        if ((list != act) && (list != st->h6_PlLast))
            break;
    }

    i = hvs6_build_planes(xsd, list);
    hvs6_wr(HVS6_CHAN(st->h6_Chan) + HVS6_DISPLIST, list);
    st->h6_PlLast = list;

    if (i && !st->h6_ScaleOK)
    {
        if (hvs6_scale_verify(st, list))
            st->h6_ScaleOK = TRUE;
        else
        {
            bug("[VC4HVS6] scaled planes refused - scaling off, showing screens 1:1\n");
            st->h6_ScaleOff = TRUE;
            vc4_hvs6_planes_commit(xsd);
            return;
        }
    }
    hvs6_ovl_arm(st);
}

/* The overlay over a screen that is a plane: x/y relative to it */
static BOOL hvs6_planes_overlay(struct VideoCoreGfx_staticdata *xsd,
                                const struct vc4gfx_overlay *ovl, OOP_Object *owner)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    ULONG n = xsd->vcsd_NumPlanes;
    BOOL show = FALSE;

    if (ovl)
    {
        if (!ovl->ovl_Phys || !ovl->ovl_Pitch || !ovl->ovl_Width || !ovl->ovl_Height
            || (ovl->ovl_DestW && (ovl->ovl_DestW != ovl->ovl_Width))
            || (ovl->ovl_DestH && (ovl->ovl_DestH != ovl->ovl_Height)))
            return FALSE;
        show = n && (xsd->vcsd_Plane[n - 1].pl_BM == owner);
    }

    VC4_MBOX_LOCK(xsd);
    if (show)
    {
        hvs6_upm_carve(st);
        st->h6_OvlOwner = owner;
        st->h6_OvlPhys  = ovl->ovl_Phys;
        st->h6_OvlPitch = ovl->ovl_Pitch;
        st->h6_OvlW     = ovl->ovl_Width;
        st->h6_OvlH     = ovl->ovl_Height;
        st->h6_OvlX     = ovl->ovl_X;
        st->h6_OvlY     = ovl->ovl_Y;
        st->h6_Overlay  = TRUE;
        vc4_hvs6_planes_commit(xsd);
    }
    else if (st->h6_Overlay)
    {
        st->h6_Overlay = FALSE;
        vc4_hvs6_planes_commit(xsd);
    }
    VC4_MBOX_UNLOCK(xsd);

    if (!ovl)
        return TRUE;
    if (show && !(ovl->ovl_Flags & VC4GFX_OVL_NOWAIT))
        hvs6_ovl_latch_wait(st);
    return show;
}
