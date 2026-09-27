/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 mode set test - step 5 of real modesetting.

    Switches HDMI from the firmware's boot mode to a mode of our own and,
    after HOLD seconds, back again. HVS channel size, pixelvalve, HDMI
    timings and the PHY PLL are all programmed here; the firmware clocks,
    CSC, infoframes and the rest of the HDMI controller keep what the
    firmware set. The top-left of the existing framebuffer is shown,
    unscaled, through a plane entry copied from the driver's own list.

    MODE=1080 programs the boot mode itself from our own table, which
    separates "our register values" from "a new rate" if 720 fails.

    Without GO nothing is written. Everything goes to the serial log.

    LOOP=n switches back and forth n times and checks every step, which is
    what shakes out the one-pixel line shift of DISPLAY-SPEC 10.2. FRAME
    draws a 1-pixel border round the mode and the framebuffer first: with
    the shift, the right border vanishes and the left one doubles.

    Usage: modetest [MODE=<WxH>] [LIST] [HOLD=<s>] [LOOP=<n>] [FRAME] [GO]
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/macros.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/mbox.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

static IPTR periiobase;

#define ARM_PERIIOBASE periiobase
#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#define P(fmt, args...) do { printf(fmt, ##args); bug(fmt, ##args); } while (0)

/* The mbox.resource stubs resolve the base from a symbol of this name. */
APTR MBoxBase;

#define VCMB_OFFSET_BCM2712     0x013880

/* CEA-861 and VESA DMT timings, clock in kHz. Only what the captured
 * lane settings cover (< 222 MHz) and the 1920x1080 plane can show. */
struct mode
{
    const char *name;
    ULONG khz;
    ULONG hact, hfp, hsync, hbp;
    ULONG vact, vfp, vsync, vbp;
    BOOL  hneg, vneg;
};

static const struct mode modes[] =
{
    { "640x480@60",    25175,  640,  16,  96,  48,  480, 10, 2, 33, TRUE,  TRUE  },
    { "800x600@60",    40000,  800,  40, 128,  88,  600,  1, 4, 23, FALSE, FALSE },
    { "1024x768@60",   65000, 1024,  24, 136, 160,  768,  3, 6, 29, TRUE,  TRUE  },
    { "1280x720@60",   74250, 1280, 110,  40, 220,  720,  5, 5, 20, FALSE, FALSE },
    { "1280x800@60",   83500, 1280,  72, 128, 200,  800,  3, 6, 22, TRUE,  FALSE },
    { "1440x900@60",  106500, 1440,  80, 152, 232,  900,  3, 6, 25, TRUE,  FALSE },
    { "1280x1024@60", 108000, 1280,  48, 112, 248, 1024,  1, 3, 38, FALSE, FALSE },
    { "1600x900@60",  108000, 1600,  24,  80,  96,  900,  1, 3, 96, FALSE, FALSE },
    { "1680x1050@60", 146250, 1680, 104, 176, 280, 1050,  3, 6, 30, TRUE,  FALSE },
    { "1920x1080@60", 148500, 1920,  88,  44, 148, 1080,  4, 5, 36, FALSE, FALSE },
};
#define MODES                   (sizeof(modes) / sizeof(modes[0]))

/* Captured by dispdump on a Pi 500+ (D0), HDMI0; DISPLAY-SPEC.md 15. */
static const ULONG pll_misc[9] =
{
    0x810c6000, 0x00b8c451, 0x46402e31, 0x00b8c005, 0x42410261,
    0xcc021001, 0xc8301c80, 0xb0804444, 0xf80f8000,
};
static const ULONG lane_ctl[4] =
{
    0x80828700, 0x80828700, 0x80828700, 0x80828700,
};
#define LANE_BAND_MAX           222000          /* kHz */

/* HDMI0 path only: HVS channel 0 -> pixelvalve 0 -> HDMI0. */
#define PV_OFF                  0x410000
#define HDMI_OFF                0x701400
#define PHY_OFF                 0x701d00
#define RM_OFF                  0x702000
#define HVS_OFF                 0x580000
#define HD_FRAME_COUNT          (0x720000 + 0x060)
#define HD_VID_CTL              (0x720000 + 0x044)
#define VID_CTL_VSYNC_LOW       (1UL << 28)
#define VID_CTL_HSYNC_LOW       (1UL << 27)

#define HVS_CH0                 (HVS_OFF + 0x100)       /* D0 layout */
#define HVS_CTRL0               0x00
#define HVS_LPTRS               0x10
#define HVS_STATUS              0x18
#define HVS_DL                  0x1c
#define HVS_CTRL0_ENB           (1UL << 31)
#define HVS_CTRL0_RESET         (1UL << 30)
#define HVS_CTRL0_SIZE          ((0x1fffUL << 16) | 0x1fff)
#define HVS_STATUS_MODE(s)      (((s) >> 13) & 3)
#define HVS_DLIST               (HVS_OFF + 0x4000)
#define SLOT_WORDS              32
#define SLOT_FILL               0xb0b0b0b0
#define FREE_FROM               0x0500          /* words, as vcgfx_hvs6.h */
#define FREE_LIMIT              0x1300
#define ENT_POS0                1
#define ENT_POS2                3
#define ENT_PTR0                5
#define ENT_PTR1                6
#define ENT_PTR2                7
#define CTL0_END                (1UL << 31)

#define PV_CONTROL              0x00
#define PV_V_CONTROL            0x04
#define PV_HORZA                0x0c
#define PV_HORZB                0x10
#define PV_VERTA                0x14
#define PV_VERTB                0x18
#define PV_CONTROL_EN           (1 << 0)
#define PV_CONTROL_FIFO_CLR     (1 << 1)
#define PV_CONTROL_CLK_SELECT   (3 << 2)
#define PV_VC_VIDEN             (1 << 0)
#define PV_VC_INTERLACE         (1 << 4)
#define PV_VC_ODD_TIMING        (1 << 29)

#define HDMI_FIFO_CTL           0x07c
#define HDMI_HORZA              0x0ec
#define HDMI_HORZB              0x0f0
#define HDMI_VERTA0             0x0f4
#define HDMI_VERTB0             0x0f8
#define HDMI_VERTA1             0x100
#define HDMI_VERTB1             0x104
#define HDMI_HOTPLUG            0x1c8
#define HDMI_HORZA_VPOS         (1 << 15)
#define HDMI_HORZA_HPOS         (1 << 14)
#define FIFO_RECENTER           (1 << 6)
#define FIFO_RECENTER_DONE      (1 << 14)

#define PHY_RESET_CTL           0x000
#define PHY_POWERUP_CTL         0x004
#define PHY_CTL(i)              (0x008 + 4 * (i))
#define PHY_PLL_REFCLK          0x01c
#define PHY_PLL_POST_KDIV       0x028
#define PHY_PLL_VCOCLK_DIV      0x02c
#define PHY_PLL_CFG             0x044
#define PHY_TMDS_CLK_WORD_SEL   0x054
#define PHY_PLL_MISC(i)         (0x060 + 4 * (i))
#define PHY_PLL_RESET_CTL       0x190
#define PHY_PLL_POWERUP_CTL     0x194
#define RM_OFFSET               0x018
#define RM_OFFSET_ONLY          (1UL << 31)

/* Everything a mode set writes, so the boot mode can be put back as it
 * was read. */
struct state
{
    ULONG khz;
    ULONG hvs_ctrl0, lptrs;
    ULONG pv_ctl, pv_vctl, pv_horza, pv_horzb, pv_verta, pv_vertb;
    ULONG hd_horza, hd_horzb, hd_verta0, hd_vertb0, hd_verta1, hd_vertb1;
    ULONG vid_ctl;
};

static ULONG *mb;

static inline ULONG rd(ULONG off)
{
    return *(volatile ULONG *)(periiobase + off);
}

static inline void wr(ULONG off, ULONG v)
{
    *(volatile ULONG *)(periiobase + off) = v;
    asm volatile ("dsb sy" ::: "memory");
}

/* DISPLAY-SPEC 9.1: vco_div at the middle of the 8-12 GHz window. */
static BOOL pll_params(ULONG khz, ULONG *vco_div, ULONG *rm_offset)
{
    UQUAD tmds = (UQUAD)khz * 1000, vco;
    ULONG min_div = 0, max_div = 0, div;

    for (div = 1; div < 1024; div++)
    {
        vco = tmds * div * 10;
        if (!min_div && vco >= 8000000000ULL)
            min_div = div;
        if (vco < 12000000000ULL)
            max_div = div;
    }
    if (!min_div || max_div < min_div)
        return FALSE;

    *vco_div = min_div + (max_div - min_div) / 2;
    vco = tmds * *vco_div * 10;
    *rm_offset = (ULONG)((((vco * 2) << 22) / 54000000) >> 2);
    return TRUE;
}

static ULONG measured_pixel_clock(void)
{
    mb[0] = AROS_LONG2LE(8 * 4);
    mb[1] = AROS_LONG2LE(VCTAG_REQ);
    mb[2] = AROS_LONG2LE(VCTAG_GETCLKMEASURED);
    mb[3] = AROS_LONG2LE(8);
    mb[4] = 0;
    mb[5] = AROS_LONG2LE(VCCLOCK_PIXEL);
    mb[6] = 0;
    mb[7] = 0;

    if (MBoxCall((void *)(periiobase + VCMB_OFFSET_BCM2712), VCMB_PROPCHAN, mb)
            == (volatile unsigned int *)-1 || mb[1] != AROS_LONG2LE(VCTAG_RESP))
        return 0;
    return AROS_LE2LONG(mb[6]);
}

/* 60 +- 2 frames/s and the pixel clock within 0.5% of the mode's. */
static BOOL measure(const char *when, ULONG khz)
{
    ULONG f0, f1, ctrl0, clk, fps, err;
    BOOL ok;

    f0 = rd(HD_FRAME_COUNT);
    Delay(50);
    f1 = rd(HD_FRAME_COUNT);
    ctrl0 = rd(HVS_CH0 + HVS_CTRL0);
    clk = measured_pixel_clock();
    fps = f1 - f0;
    err = (clk > khz * 1000) ? clk - khz * 1000 : khz * 1000 - clk;
    ok  = (fps >= 58) && (fps <= 62) && (err <= khz * 5);

    P("  %s: HDMI %u frames/s, pixel clock %u Hz, HVS ch0 %ux%u DL %#06x mode %u,"
      " FIFO_CTL %08x%s\n", when, (unsigned)fps, (unsigned)clk,
      (unsigned)(((ctrl0 >> 16) & 0x1fff) + 1), (unsigned)((ctrl0 & 0x1fff) + 1),
      (unsigned)(rd(HVS_CH0 + HVS_DL) & 0xfff),
      (unsigned)HVS_STATUS_MODE(rd(HVS_CH0 + HVS_STATUS)),
      (unsigned)rd(HDMI_OFF + HDMI_FIFO_CTL), ok ? "" : "  <= BAD");
    return ok;
}

static void state_read(struct state *s, ULONG khz)
{
    s->khz       = khz;
    s->hvs_ctrl0 = rd(HVS_CH0 + HVS_CTRL0);
    s->lptrs     = rd(HVS_CH0 + HVS_LPTRS) & 0xfff;
    s->pv_ctl    = rd(PV_OFF + PV_CONTROL);
    s->pv_vctl   = rd(PV_OFF + PV_V_CONTROL);
    s->pv_horza  = rd(PV_OFF + PV_HORZA);
    s->pv_horzb  = rd(PV_OFF + PV_HORZB);
    s->pv_verta  = rd(PV_OFF + PV_VERTA);
    s->pv_vertb  = rd(PV_OFF + PV_VERTB);
    s->hd_horza  = rd(HDMI_OFF + HDMI_HORZA);
    s->hd_horzb  = rd(HDMI_OFF + HDMI_HORZB);
    s->hd_verta0 = rd(HDMI_OFF + HDMI_VERTA0);
    s->hd_vertb0 = rd(HDMI_OFF + HDMI_VERTB0);
    s->hd_verta1 = rd(HDMI_OFF + HDMI_VERTA1);
    s->hd_vertb1 = rd(HDMI_OFF + HDMI_VERTB1);
    s->vid_ctl   = rd(HD_VID_CTL);
}

/* Register values for a mode: pixelvalve at 1 pixel/clock with
 * ODD_TIMING (6.1, 6.2), HDMI timings unhalved (7.1), rep = 1. */
static void state_build(struct state *s, const struct state *fw,
                        const struct mode *m, ULONG lptrs)
{
    s->khz       = m->khz;
    s->hvs_ctrl0 = (fw->hvs_ctrl0 & ~HVS_CTRL0_SIZE)
                 | ((m->hact - 1) << 16) | (m->vact - 1);
    s->lptrs     = lptrs;
    s->pv_ctl    = fw->pv_ctl;
    s->pv_vctl   = fw->pv_vctl | PV_VC_ODD_TIMING;
    s->pv_horza  = (m->hbp << 16) | m->hsync;
    s->pv_horzb  = (m->hfp << 16) | m->hact;
    s->pv_verta  = (m->vbp << 16) | m->vsync;
    s->pv_vertb  = (m->vfp << 16) | m->vact;
    s->hd_horza  = (m->vneg ? 0 : HDMI_HORZA_VPOS) | (m->hneg ? 0 : HDMI_HORZA_HPOS)
                 | (m->hfp << 16) | m->hact;
    s->hd_horzb  = (m->hbp << 16) | m->hsync;
    s->hd_verta0 = (m->vsync << 24) | (m->vfp << 16) | m->vact;
    s->hd_vertb0 = m->vbp;
    s->hd_verta1 = s->hd_verta0;
    s->hd_vertb1 = s->hd_vertb0;
    s->vid_ctl   = (fw->vid_ctl & ~(VID_CTL_VSYNC_LOW | VID_CTL_HSYNC_LOW))
                 | (m->vneg ? VID_CTL_VSYNC_LOW : 0) | (m->hneg ? VID_CTL_HSYNC_LOW : 0);
}

static void state_print(const char *name, const struct state *s)
{
    P("  %s: %u kHz, HVS CTRL0 %08x LPTRS %#06x\n"
      "    PV CONTROL %08x V_CONTROL %08x HORZA %08x HORZB %08x VERTA %08x VERTB %08x\n"
      "    HDMI HORZA %08x HORZB %08x VERTA %08x/%08x VERTB %08x/%08x VID_CTL %08x\n", name,
      (unsigned)s->khz, (unsigned)s->hvs_ctrl0, (unsigned)s->lptrs,
      (unsigned)s->pv_ctl, (unsigned)s->pv_vctl, (unsigned)s->pv_horza,
      (unsigned)s->pv_horzb, (unsigned)s->pv_verta, (unsigned)s->pv_vertb,
      (unsigned)s->hd_horza, (unsigned)s->hd_horzb, (unsigned)s->hd_verta0,
      (unsigned)s->hd_verta1, (unsigned)s->hd_vertb0, (unsigned)s->hd_vertb1,
      (unsigned)s->vid_ctl);
}

/* DISPLAY-SPEC 9.4, steps 1-16. */
static void phy_init(ULONG khz)
{
    ULONG vco_div, rm_offset, i;

    pll_params(khz, &vco_div, &rm_offset);

    wr(PHY_OFF + PHY_RESET_CTL, 0);
    wr(PHY_OFF + PHY_POWERUP_CTL, 0);
    wr(PHY_OFF + PHY_PLL_POST_KDIV, 1 << 4);            /* BYPASS_EN */

    for (i = 0; i < 9; i++)
        wr(PHY_OFF + PHY_PLL_MISC(i), pll_misc[i]);
    wr(PHY_OFF + PHY_PLL_REFCLK, (1 << 13) | 54);       /* CMOS, 54 MHz */
    wr(PHY_OFF + PHY_RESET_CTL, 0x7f);

    wr(RM_OFF + RM_OFFSET, RM_OFFSET_ONLY | rm_offset);
    wr(PHY_OFF + PHY_PLL_VCOCLK_DIV, (1 << 10) | vco_div);
    wr(PHY_OFF + PHY_PLL_CFG, 0);
    wr(PHY_OFF + PHY_PLL_POST_KDIV, (2 << 2) | 1);      /* CLK0_SEL 2, KDIV 1 */

    for (i = 0; i < 4; i++)
        wr(PHY_OFF + PHY_CTL(i), lane_ctl[i]);
    wr(PHY_OFF + PHY_TMDS_CLK_WORD_SEL, (khz >= 340000) ? 3 : 0);
    wr(PHY_OFF + PHY_POWERUP_CTL, 0x1cf);

    wr(PHY_OFF + PHY_PLL_POWERUP_CTL, 1);
    wr(PHY_OFF + PHY_PLL_RESET_CTL, rd(PHY_OFF + PHY_PLL_RESET_CTL) & ~1);
    wr(PHY_OFF + PHY_PLL_RESET_CTL, rd(PHY_OFF + PHY_PLL_RESET_CTL) | 1);
}

/* DISPLAY-SPEC 10.1. */
static void recenter(void)
{
    ULONG drift = rd(HDMI_OFF + HDMI_FIFO_CTL) & 0xefff, i;

    wr(HDMI_OFF + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(HDMI_OFF + HDMI_FIFO_CTL, drift | FIFO_RECENTER);
    Delay(1);
    wr(HDMI_OFF + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(HDMI_OFF + HDMI_FIFO_CTL, drift | FIFO_RECENTER);

    for (i = 0; i < 50 && !(rd(HDMI_OFF + HDMI_FIFO_CTL) & FIFO_RECENTER_DONE); i++)
        Delay(1);
    if (i == 50)
        P("  RECENTER_DONE never came\n");
}

/*
 * Stop what is running (10.2 steps 1, 2, 5, 6), then bring the new mode
 * up in the order of 10: HVS channel, PHY, HDMI timings, pixelvalve,
 * FIFO recentre. The HDMI controller itself keeps running throughout.
 */
static void mode_set(const struct state *s)
{
    ULONG ctl, ctrl0, i;

    wr(PV_OFF + PV_V_CONTROL, rd(PV_OFF + PV_V_CONTROL) & ~PV_VC_VIDEN);
    for (i = 0; i < 10 && (rd(PV_OFF + PV_V_CONTROL) & PV_VC_VIDEN); i++)
        Delay(1);
    if (i == 10)
        P("  VIDEN did not read back clear\n");
    /* A pixel left in the PV-to-HDMI FIFO shifts every line by one. */
    Delay(2);

    ctl = rd(PV_OFF + PV_CONTROL) & ~PV_CONTROL_EN;
    wr(PV_OFF + PV_CONTROL, ctl);
    wr(PV_OFF + PV_CONTROL, ctl | PV_CONTROL_FIFO_CLR);

    ctrl0 = rd(HVS_CH0 + HVS_CTRL0);
    wr(HVS_CH0 + HVS_CTRL0, ctrl0 | HVS_CTRL0_RESET);
    wr(HVS_CH0 + HVS_CTRL0, (ctrl0 | HVS_CTRL0_RESET) & ~HVS_CTRL0_ENB);
    for (i = 0; i < 10 && HVS_STATUS_MODE(rd(HVS_CH0 + HVS_STATUS)); i++)
        Delay(1);
    if (i == 10)
        P("  HVS ch0 did not reach MODE 0 (STATUS %08x)\n",
          (unsigned)rd(HVS_CH0 + HVS_STATUS));
    Delay(1);

    wr(HVS_CH0 + HVS_LPTRS, s->lptrs);
    wr(HVS_CH0 + HVS_CTRL0, HVS_CTRL0_RESET);
    wr(HVS_CH0 + HVS_CTRL0, s->hvs_ctrl0 & ~HVS_CTRL0_RESET);

    phy_init(s->khz);
    Delay(1);       /* PLL lock */

    wr(HDMI_OFF + HDMI_HORZA, s->hd_horza);
    wr(HDMI_OFF + HDMI_HORZB, s->hd_horzb);
    wr(HDMI_OFF + HDMI_VERTA0, s->hd_verta0);
    wr(HDMI_OFF + HDMI_VERTB0, s->hd_vertb0);
    wr(HDMI_OFF + HDMI_VERTA1, s->hd_verta1);
    wr(HDMI_OFF + HDMI_VERTB1, s->hd_vertb1);
    wr(HD_VID_CTL, s->vid_ctl);

    wr(PV_OFF + PV_HORZA, s->pv_horza);
    wr(PV_OFF + PV_HORZB, s->pv_horzb);
    wr(PV_OFF + PV_VERTA, s->pv_verta);
    wr(PV_OFF + PV_VERTB, s->pv_vertb);
    wr(PV_OFF + PV_CONTROL, (s->pv_ctl & ~PV_CONTROL_EN) | PV_CONTROL_FIFO_CLR);
    wr(PV_OFF + PV_CONTROL, s->pv_ctl);
    wr(PV_OFF + PV_V_CONTROL, s->pv_vctl);

    recenter();
}

/* Two free slots - plane and END - found by the fill pattern. */
static BOOL find_slots(ULONG *out)
{
    ULONG rel, i;

    for (rel = FREE_FROM; rel + 2 * SLOT_WORDS < FREE_LIMIT; rel += SLOT_WORDS)
    {
        for (i = 0; i < 2 * SLOT_WORDS; i++)
            if (rd(HVS_DLIST + (rel + i) * 4) != SLOT_FILL)
                break;
        if (i == 2 * SLOT_WORDS)
        {
            *out = rel;
            return TRUE;
        }
    }
    return FALSE;
}

/* The live list's first entry is the driver's framebuffer plane. Copy it
 * with the new extent, so everything else about it stays known good. */
static void build_list(ULONG ours, ULONG live, const struct mode *m)
{
    ULONG i;

    for (i = 0; i < SLOT_WORDS; i++)
        wr(HVS_DLIST + (ours + i) * 4, rd(HVS_DLIST + (live + i) * 4));
    wr(HVS_DLIST + (ours + ENT_POS0) * 4, 0);
    wr(HVS_DLIST + (ours + ENT_POS2) * 4, ((m->vact - 1) << 16) | (m->hact - 1));
    wr(HVS_DLIST + (ours + SLOT_WORDS) * 4, CTL0_END);
}

static void free_list(ULONG ours)
{
    ULONG i;

    for (i = 0; i < 2 * SLOT_WORDS; i++)
        wr(HVS_DLIST + (ours + i) * 4, SLOT_FILL);
}

/* A 1-pixel white border round w x h at the top-left of the plane's
 * surface, which RAM identity-maps. */
static void draw_frame(ULONG live, ULONG w, ULONG h)
{
    UQUAD fb = ((UQUAD)(rd(HVS_DLIST + (live + ENT_PTR0) * 4) & 0xff) << 32)
             | rd(HVS_DLIST + (live + ENT_PTR1) * 4);
    ULONG pitch = rd(HVS_DLIST + (live + ENT_PTR2) * 4);
    ULONG x, y;

    for (x = 0; x < w; x++)
    {
        ((volatile ULONG *)(IPTR)fb)[x] = 0xffffffff;
        ((volatile ULONG *)(IPTR)(fb + (UQUAD)(h - 1) * pitch))[x] = 0xffffffff;
    }
    for (y = 0; y < h; y++)
    {
        ((volatile ULONG *)(IPTR)(fb + (UQUAD)y * pitch))[0] = 0xffffffff;
        ((volatile ULONG *)(IPTR)(fb + (UQUAD)y * pitch))[w - 1] = 0xffffffff;
    }
    CacheClearE((APTR)(IPTR)fb, pitch * h, CACRF_ClearD);
}

#define TEMPLATE "MODE/K,LIST/S,HOLD/N,LOOP/N,FRAME/S,GO/S"

enum { ARG_MODE, ARG_LIST, ARG_HOLD, ARG_LOOP, ARG_FRAME, ARG_GO, ARG_COUNT };

int main(void)
{
    APTR KernelBase = OpenResource("kernel.resource");
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    const struct mode *m = &modes[3];
    struct state fw, ours;
    ULONG hold, loops = 1, good = 0, steps = 0, slots = 0, pos2, vco_div, rm_offset, i;
    APTR mbbuf = NULL;
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "modetest");
        return RETURN_FAIL;
    }
    if (args[ARG_LIST])
    {
        for (i = 0; i < MODES; i++)
            P("  %-13s %7u kHz\n", modes[i].name, (unsigned)modes[i].khz);
        rc = RETURN_OK;
        goto out;
    }
    if (args[ARG_MODE])
    {
        for (i = 0; i < MODES; i++)
            if (!strncasecmp(modes[i].name, (char *)args[ARG_MODE], strlen((char *)args[ARG_MODE]))
                && modes[i].name[strlen((char *)args[ARG_MODE])] == '@')
                break;
        if (i == MODES)
        {
            P("modetest: no mode %s - try LIST\n", (char *)args[ARG_MODE]);
            goto out;
        }
        m = &modes[i];
    }
    if (args[ARG_LOOP])
        loops = *(LONG *)args[ARG_LOOP];
    hold = args[ARG_HOLD] ? *(LONG *)args[ARG_HOLD] : (args[ARG_LOOP] ? 3 : 10);

    periiobase = KernelBase ? (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) : 0;
    if (periiobase != BCM2712_PERIIOBASE)
    {
        P("modetest: not a BCM2712 - refusing\n");
        goto out;
    }
    if (!(MBoxBase = OpenResource("mbox.resource"))
        || !(mbbuf = AllocMem(MBOX_MSG_ALIGN + 64, MEMF_31BIT | MEMF_CLEAR)))
        goto out;
    mb = (ULONG *)(((IPTR)mbbuf + MBOX_MSG_ALIGN - 1) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    if (!(rd(HDMI_OFF + HDMI_HOTPLUG) & 1))
    {
        P("modetest: no sink on HDMI0 - refusing\n");
        goto out;
    }

    /* The boot state this is written against: the firmware's 1080p60 at
     * 2 pixels/clock, and a live list whose first entry is a plane. */
    state_read(&fw, 148500);
    pos2 = rd(HVS_DLIST + (fw.lptrs + ENT_POS2) * 4);
    if (!(fw.pv_ctl & PV_CONTROL_EN) || !(fw.pv_vctl & PV_VC_VIDEN)
        || (fw.pv_ctl & PV_CONTROL_CLK_SELECT) || (fw.pv_vctl & PV_VC_INTERLACE)
        || (fw.pv_vctl & PV_VC_ODD_TIMING) || (rd(PHY_OFF + PHY_PLL_VCOCLK_DIV) != 0x407)
        || (rd(HVS_DLIST + fw.lptrs * 4) & CTL0_END)
        || ((pos2 & 0xffff) + 1 < m->hact) || ((pos2 >> 16) + 1 < m->vact))
    {
        P("modetest: not the boot state this was written for - refusing\n");
        state_print("live", &fw);
        goto out;
    }
    if (m->khz >= LANE_BAND_MAX || !pll_params(m->khz, &vco_div, &rm_offset))
    {
        P("modetest: %s is outside what the captured PHY settings cover\n", m->name);
        goto out;
    }
    if (!find_slots(&slots))
    {
        P("modetest: no free display list slots\n");
        goto out;
    }

    state_build(&ours, &fw, m, slots);
    P("modetest: %s, vco_div %u, RM_OFFSET %08x, list at %#06x\n", m->name,
      (unsigned)vco_div, (unsigned)(RM_OFFSET_ONLY | rm_offset), (unsigned)slots);
    state_print("firmware", &fw);
    state_print("planned ", &ours);

    if (!args[ARG_GO])
    {
        P("modetest: dry run - add GO to write\n");
        rc = RETURN_OK;
        goto out;
    }

    if (args[ARG_FRAME])
    {
        draw_frame(fw.lptrs, (pos2 & 0xffff) + 1, (pos2 >> 16) + 1);
        draw_frame(fw.lptrs, m->hact, m->vact);
    }
    measure("before", fw.khz);
    build_list(slots, fw.lptrs, m);

    for (i = 1; i <= loops; i++)
    {
        P("modetest: [%u/%u] switching to %s\n", (unsigned)i, (unsigned)loops, m->name);
        mode_set(&ours);
        good += measure("after", ours.khz);
        Delay(hold * 50);

        P("modetest: [%u/%u] back to the boot mode\n", (unsigned)i, (unsigned)loops);
        mode_set(&fw);
        good += measure("restored", fw.khz);
        steps += 2;
        if (i < loops)
            Delay(hold * 50);
    }
    free_list(slots);

    P("modetest: done, %u of %u steps ok\n", (unsigned)good, (unsigned)steps);
    rc = (good == steps) ? RETURN_OK : RETURN_WARN;

out:
    if (mbbuf)
        FreeMem(mbbuf, MBOX_MSG_ALIGN + 64);
    FreeArgs(rda);
    return rc;
}
