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

static inline ULONG hvs5_rd(ULONG offset)
{
    return *(volatile ULONG *)(HVS5_BASE + offset);
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
