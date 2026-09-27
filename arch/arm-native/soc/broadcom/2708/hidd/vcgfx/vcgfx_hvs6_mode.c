/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore Gfx Hidd - BCM2712 mode setting.

    Programs the HVS channel, the pixelvalve, the HDMI timing registers
    and the HDMI PHY PLL for a mode of our own. The firmware clocks, the
    CSC, the infoframes and the rest of the HDMI controller keep what the
    firmware set up at boot. Proven step by step on a Pi 500+ with the
    raspi-pvtest, -phytest and -modetest tools.

    HDMI0 only: the shared hd block puts VID_CTL elsewhere for HDMI1, and
    that port has not been exercised.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hardware.h"
#include "vcgfx_hvs6.h"

/* Kill switch: 0 = the boot mode is the only one, as before. */
#define VC4_HVS6_MODESET 1

#define PV_BASE             (ARM_PERIIOBASE + 0x410000)
#define HDMI_BASE           (ARM_PERIIOBASE + 0x701400)
#define PHY_BASE            (ARM_PERIIOBASE + 0x701d00)
#define RM_BASE             (ARM_PERIIOBASE + 0x702000)
#define HD_VID_CTL          (ARM_PERIIOBASE + 0x720044)

/* Pixelvalve registers, also the index into h6_BootPV[] times 4. */
#define PV_CONTROL          0x00
#define PV_V_CONTROL        0x04
#define PV_HORZA            0x0c
#define PV_HORZB            0x10
#define PV_VERTA            0x14
#define PV_VERTB            0x18
#define PV_REGS             7
#define PV(r)               ((r) / 4)
#define PV_CONTROL_EN       (1 << 0)
#define PV_CONTROL_FIFO_CLR (1 << 1)
#define PV_CONTROL_CLKSEL   (3 << 2)
#define PV_CONTROL_REP      (3 << 4)
#define PV_VC_VIDEN         (1 << 0)
#define PV_VC_INTERLACE     (1 << 4)
#define PV_VC_ODD_TIMING    (1 << 29)

#define HDMI_FIFO_CTL       0x07c
#define HDMI_HORZA          0x0ec
#define HDMI_HORZB          0x0f0
#define HDMI_VERTA0         0x0f4
#define HDMI_VERTB0         0x0f8
#define HDMI_VERTA1         0x100
#define HDMI_VERTB1         0x104
#define HDMI_HORZA_VPOS     (1 << 15)
#define HDMI_HORZA_HPOS     (1 << 14)
#define FIFO_RECENTER       (1 << 6)
#define FIFO_RECENTER_DONE  (1 << 14)
#define VID_CTL_VSYNC_LOW   (1UL << 28)
#define VID_CTL_HSYNC_LOW   (1UL << 27)

#define PHY_RESET_CTL       0x000
#define PHY_POWERUP_CTL     0x004
#define PHY_CTL(i)          (0x008 + 4 * (i))
#define PHY_PLL_REFCLK      0x01c
#define PHY_PLL_POST_KDIV   0x028
#define PHY_PLL_VCOCLK_DIV  0x02c
#define PHY_PLL_CFG         0x044
#define PHY_TMDS_WORD_SEL   0x054
#define PHY_PLL_MISC(i)     (0x060 + 4 * (i))
#define PHY_PLL_RESET_CTL   0x190
#define PHY_PLL_POWERUP_CTL 0x194
#define RM_OFFSET           0x018
#define RM_OFFSET_ONLY      (1UL << 31)

#define HVS6_CTRL0_RESET    (1UL << 30)
#define HVS6_CTRL0_SIZE     ((0x1fffUL << 16) | 0x1fff)
#define HVS6_VERSION_D0     0x54

/* Mode-independent PLL constants, read back from the firmware's own PHY
 * setup on a Pi 500+ (D0). Mode setting stays off where the live PHY
 * holds anything else. */
static const ULONG pll_misc[9] =
{
    0x810c6000, 0x00b8c451, 0x46402e31, 0x00b8c005, 0x42410261,
    0xcc021001, 0xc8301c80, 0xb0804444, 0xf80f8000,
};

/* Lane drive for TMDS below 222 MHz, the only band captured so far. */
static const ULONG lane_ctl[4] =
{
    0x80828700, 0x80828700, 0x80828700, 0x80828700,
};
#define LANE_BAND_MAX       222000          /* kHz */

#define P_H                 VCGFX_TIMING_PHSYNC
#define P_V                 VCGFX_TIMING_PVSYNC

/* CEA-861 and VESA DMT, 60 Hz: clock, h disp/start/end/total, v ditto. */
static const struct vcgfx_timing hvs6_modes[] =
{
    {  25175,  640,  656,  752,  800,  480,  490,  492,  525, 0         },
    {  40000,  800,  840,  968, 1056,  600,  601,  605,  628, P_H | P_V },
    {  65000, 1024, 1048, 1184, 1344,  768,  771,  777,  806, 0         },
    {  74250, 1280, 1390, 1430, 1650,  720,  725,  730,  750, P_H | P_V },
    {  83500, 1280, 1352, 1480, 1680,  800,  803,  809,  831, P_V       },
    { 106500, 1440, 1520, 1672, 1904,  900,  903,  909,  934, P_V       },
    { 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, P_H | P_V },
    { 108000, 1600, 1624, 1704, 1800,  900,  901,  904, 1000, P_H | P_V },
    { 146250, 1680, 1784, 1960, 2240, 1050, 1053, 1059, 1089, P_V       },
    { 148500, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125, P_H | P_V },
};
#define HVS6_MODES          (sizeof(hvs6_modes) / sizeof(hvs6_modes[0]))

static inline ULONG rd(IPTR a)
{
    return *(volatile ULONG *)a;
}

static inline void wr(IPTR a, ULONG v)
{
    *(volatile ULONG *)a = v;
    asm volatile ("dsb sy" ::: "memory");
}

/* Busy wait on the 1 MHz system timer: callers are not always
 * processes, so dos.library's Delay() is not an option. */
static void hvs6_udelay(ULONG us)
{
    volatile ULONG *timer = (volatile ULONG *)(ARM_PERIIOBASE + 0x3004);
    ULONG start = *timer;

    while ((*timer - start) < us)
        ;
}

/* Poll until (reg & mask) == want, for at most `us`. */
static BOOL hvs6_poll(IPTR reg, ULONG mask, ULONG want, ULONG us)
{
    volatile ULONG *timer = (volatile ULONG *)(ARM_PERIIOBASE + 0x3004);
    ULONG start = *timer;

    while ((rd(reg) & mask) != want)
        if ((*timer - start) >= us)
            return FALSE;
    return TRUE;
}

/* DISPLAY-SPEC 9.1: vco_div at the middle of the 8-12 GHz window, and
 * the rate manager's 9.22 fixed-point VCO:54 MHz ratio. */
static BOOL hvs6_pll(ULONG khz, ULONG *vco_div, ULONG *rm_offset)
{
    UQUAD tmds = (UQUAD)khz * 1000, vco;
    ULONG min_div = 0, max_div = 0, div;

    for (div = 1; div < 1024; div++)
    {
        vco = tmds * div * 10;
        if (!min_div && (vco >= 8000000000ULL))
            min_div = div;
        if (vco < 12000000000ULL)
            max_div = div;
    }
    if (!min_div || (max_div < min_div))
        return FALSE;

    *vco_div   = min_div + (max_div - min_div) / 2;
    vco        = tmds * *vco_div * 10;
    *rm_offset = (ULONG)((((vco * 2) << 22) / 54000000) >> 2);
    return TRUE;
}

/*
 * Record the firmware's boot mode before anything touches it: the values
 * to restore when the boot size is asked for again, and its timings for
 * the mode list. Mode setting is enabled only for a setup these register
 * sequences were proven on.
 */
void vc4_hvs6_mode_capture(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    struct vcgfx_timing *t = &st->h6_BootTiming;
    ULONG ctrl0, ppc, div, horza, horzb, verta, vertb, i;
    UQUAD vco;

    st->h6_ModeOK = FALSE;
    if (!VC4_HVS6_MODESET || (hvs6_rd(HVS6_ID) != HVS6_ID_MAGIC)
        || ((hvs6_rd(HVS6_VERSION) & 0xff) != HVS6_VERSION_D0))
        return;

    ctrl0 = hvs6_rd(HVS6_CHAN(0) + HVS6_DISPCTRL);
    for (i = 0; i < PV_REGS; i++)
        st->h6_BootPV[i] = rd(PV_BASE + 4 * i);
    st->h6_BootHDMI[0] = rd(HDMI_BASE + HDMI_HORZA);
    st->h6_BootHDMI[1] = rd(HDMI_BASE + HDMI_HORZB);
    st->h6_BootHDMI[2] = rd(HDMI_BASE + HDMI_VERTA0);
    st->h6_BootHDMI[3] = rd(HDMI_BASE + HDMI_VERTB0);
    st->h6_BootHDMI[4] = rd(HDMI_BASE + HDMI_VERTA1);
    st->h6_BootHDMI[5] = rd(HDMI_BASE + HDMI_VERTB1);
    st->h6_BootVidCtl  = rd(HD_VID_CTL);
    st->h6_BootCtrl0   = ctrl0;
    for (i = 0; i < 4; i++)
        st->h6_BootLane[i] = rd(PHY_BASE + PHY_CTL(i));

    if (!(ctrl0 & HVS6_DISPCTRL_EN) || !(st->h6_BootPV[PV(PV_CONTROL)] & PV_CONTROL_EN)
        || !(st->h6_BootPV[PV(PV_V_CONTROL)] & PV_VC_VIDEN)
        || (st->h6_BootPV[PV(PV_V_CONTROL)] & PV_VC_INTERLACE)
        || (st->h6_BootPV[PV(PV_CONTROL)] & (PV_CONTROL_CLKSEL | PV_CONTROL_REP)))
    {
        bug("[VC4HVS6] mode setting off: HDMI0 is not running a plain progressive mode\n");
        return;
    }
    for (i = 0; i < 9; i++)
        if (rd(PHY_BASE + PHY_PLL_MISC(i)) != pll_misc[i])
        {
            bug("[VC4HVS6] mode setting off: PLL_MISC_%u reads %08x\n", i,
                rd(PHY_BASE + PHY_PLL_MISC(i)));
            return;
        }

    /* The live mode, from the pixelvalve and the PHY. */
    div = rd(PHY_BASE + PHY_PLL_VCOCLK_DIV) & 0x3ff;
    st->h6_BootVcoDiv   = div;
    st->h6_BootRmOffset = rd(RM_BASE + RM_OFFSET) & 0x7fffffff;
    vco = (UQUAD)st->h6_BootRmOffset * 54000000 / (1 << 21);
    if (!div)
        return;
    ppc   = (st->h6_BootPV[PV(PV_V_CONTROL)] & PV_VC_ODD_TIMING) ? 1 : 2;
    horza = st->h6_BootPV[PV(PV_HORZA)];
    horzb = st->h6_BootPV[PV(PV_HORZB)];
    verta = st->h6_BootPV[PV(PV_VERTA)];
    vertb = st->h6_BootPV[PV(PV_VERTB)];

    t->clock  = (ULONG)(vco / (div * 10) / 1000);
    t->hdisp  = (horzb & 0xffff) * ppc;
    t->hstart = t->hdisp  + (horzb >> 16) * ppc;
    t->hend   = t->hstart + (horza & 0xffff) * ppc;
    t->htotal = t->hend   + (horza >> 16) * ppc;
    t->vdisp  = vertb & 0xffff;
    t->vstart = t->vdisp  + (vertb >> 16);
    t->vend   = t->vstart + (verta & 0xffff);
    t->vtotal = t->vend   + (verta >> 16);
    t->flags  = ((st->h6_BootHDMI[0] & HDMI_HORZA_HPOS) ? VCGFX_TIMING_PHSYNC : 0)
              | ((st->h6_BootHDMI[0] & HDMI_HORZA_VPOS) ? VCGFX_TIMING_PVSYNC : 0);

    /* Scrambling (above 340 MHz) is not done here, so a boot mode that
     * needs it could not be brought back. */
    if (t->clock >= 340000)
    {
        bug("[VC4HVS6] mode setting off: boot mode runs %u kHz\n", t->clock);
        return;
    }
    if ((t->hdisp != HVS6_DISPCTRL_W(ctrl0)) || (t->vdisp != HVS6_DISPCTRL_H(ctrl0)))
    {
        bug("[VC4HVS6] mode setting off: pixelvalve %ux%u, HVS %ux%u\n", t->hdisp,
            t->vdisp, HVS6_DISPCTRL_W(ctrl0), HVS6_DISPCTRL_H(ctrl0));
        return;
    }

    st->h6_ModeOK = TRUE;
    bug("[VC4HVS6] boot mode %ux%u, %u kHz: mode setting on\n", t->hdisp, t->vdisp,
        t->clock);
}

/* Can we drive this timing, and is it one the sink says it takes? */
static BOOL hvs6_usable(struct VideoCoreGfx_staticdata *xsd, const struct vcgfx_timing *t)
{
    const struct vcgfx_edid *e = &xsd->vcsd_EDID;
    ULONG hz, div, off;

    if ((t->clock >= LANE_BAND_MAX) || !hvs6_pll(t->clock, &div, &off)
        || (t->flags & VCGFX_TIMING_INTERLACE) || (t->hdisp & 15)
        || (t->hdisp > xsd->vcsd_BootFBWidth) || (t->vdisp > xsd->vcsd_BootFBHeight))
        return FALSE;

    if (e->valid)
    {
        hz = (t->clock * 1000 + t->htotal * t->vtotal / 2) / (t->htotal * t->vtotal);
        if ((e->vmax && ((hz < e->vmin) || (hz > e->vmax)))
            || (e->maxclock && (t->clock > e->maxclock)))
            return FALSE;
    }
    return TRUE;
}

/* The i-th mode we offer besides the boot mode, or NULL at the end:
 * the table first, then the sink's own detailed timings. */
const struct vcgfx_timing *vc4_hvs6_mode(struct VideoCoreGfx_staticdata *xsd, ULONG i)
{
    if (!xsd->vcsd_HVS6.h6_ModeOK)
        return NULL;
    if (i < HVS6_MODES)
        return &hvs6_modes[i];
    i -= HVS6_MODES;
    if (i < xsd->vcsd_EDID.ntimings)
        return &xsd->vcsd_EDID.timings[i];
    return NULL;
}

/* Timings to show w x h with, in the order vc4_hvs6_mode() offers them;
 * NULL for the boot mode's size or anything we cannot set. */
static const struct vcgfx_timing *hvs6_lookup(struct VideoCoreGfx_staticdata *xsd,
                                              ULONG w, ULONG h, BOOL *boot)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    const struct vcgfx_timing *t;
    ULONG i;

    *boot = (w == st->h6_BootTiming.hdisp) && (h == st->h6_BootTiming.vdisp);
    if (*boot)
        return &st->h6_BootTiming;

    for (i = 0; (t = vc4_hvs6_mode(xsd, i)); i++)
        if ((t->hdisp == w) && (t->vdisp == h) && hvs6_usable(xsd, t))
            return t;
    return NULL;
}

BOOL vc4_hvs6_mode_usable(struct VideoCoreGfx_staticdata *xsd,
                          const struct vcgfx_timing *t)
{
    return xsd->vcsd_HVS6.h6_ModeOK && hvs6_usable(xsd, t);
}

/* Can channel ch be switched to show w x h? */
BOOL vc4_hvs6_mode_ok(struct VideoCoreGfx_staticdata *xsd, ULONG ch, ULONG w, ULONG h)
{
    BOOL boot;

    return xsd->vcsd_HVS6.h6_ModeOK && (ch == 0) && hvs6_lookup(xsd, w, h, &boot);
}

/*
 * Take the running mode down (DISPLAY-SPEC 10.2 steps 1, 2, 5, 6): the
 * pixelvalve first, with the 20 ms that keeps a pixel from being left in
 * the PV-to-HDMI FIFO, then the HVS channel.
 */
void vc4_hvs6_mode_stop(struct VideoCoreGfx_staticdata *xsd, ULONG ch)
{
    ULONG ctl, ctrl0;

    wr(PV_BASE + PV_V_CONTROL, rd(PV_BASE + PV_V_CONTROL) & ~PV_VC_VIDEN);
    if (!hvs6_poll(PV_BASE + PV_V_CONTROL, PV_VC_VIDEN, 0, 100000))
        bug("[VC4HVS6] VIDEN did not clear\n");
    hvs6_udelay(20000);

    ctl = rd(PV_BASE + PV_CONTROL) & ~PV_CONTROL_EN;
    wr(PV_BASE + PV_CONTROL, ctl);
    wr(PV_BASE + PV_CONTROL, ctl | PV_CONTROL_FIFO_CLR);

    ctrl0 = hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPCTRL);
    hvs6_wr(HVS6_CHAN(ch) + HVS6_DISPCTRL, ctrl0 | HVS6_CTRL0_RESET);
    hvs6_wr(HVS6_CHAN(ch) + HVS6_DISPCTRL, (ctrl0 | HVS6_CTRL0_RESET) & ~HVS6_DISPCTRL_EN);
    if (!hvs6_poll(HVS6_BASE + HVS6_CHAN(ch) + HVS6_DISPSTAT, 3 << 13, 0, 100000))
        bug("[VC4HVS6] ch%u did not stop\n", ch);
    hvs6_udelay(20000);
}

/* DISPLAY-SPEC 9.4, steps 1-16. */
static void hvs6_phy_init(ULONG vco_div, ULONG rm_offset, const ULONG *lanes)
{
    ULONG i;

    wr(PHY_BASE + PHY_RESET_CTL, 0);
    wr(PHY_BASE + PHY_POWERUP_CTL, 0);
    wr(PHY_BASE + PHY_PLL_POST_KDIV, 1 << 4);               /* BYPASS_EN */

    for (i = 0; i < 9; i++)
        wr(PHY_BASE + PHY_PLL_MISC(i), pll_misc[i]);
    wr(PHY_BASE + PHY_PLL_REFCLK, (1 << 13) | 54);          /* CMOS, 54 MHz */
    wr(PHY_BASE + PHY_RESET_CTL, 0x7f);

    wr(RM_BASE + RM_OFFSET, RM_OFFSET_ONLY | rm_offset);
    wr(PHY_BASE + PHY_PLL_VCOCLK_DIV, (1 << 10) | vco_div);
    wr(PHY_BASE + PHY_PLL_CFG, 0);
    wr(PHY_BASE + PHY_PLL_POST_KDIV, (2 << 2) | 1);         /* CLK0_SEL 2, KDIV 1 */

    for (i = 0; i < 4; i++)
        wr(PHY_BASE + PHY_CTL(i), lanes[i]);
    wr(PHY_BASE + PHY_TMDS_WORD_SEL, 0);                    /* < 340 MHz */
    wr(PHY_BASE + PHY_POWERUP_CTL, 0x1cf);

    wr(PHY_BASE + PHY_PLL_POWERUP_CTL, 1);
    wr(PHY_BASE + PHY_PLL_RESET_CTL, rd(PHY_BASE + PHY_PLL_RESET_CTL) & ~1);
    wr(PHY_BASE + PHY_PLL_RESET_CTL, rd(PHY_BASE + PHY_PLL_RESET_CTL) | 1);
}

/* DISPLAY-SPEC 10.1. */
static void hvs6_recenter(void)
{
    ULONG drift = rd(HDMI_BASE + HDMI_FIFO_CTL) & 0xefff;

    wr(HDMI_BASE + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(HDMI_BASE + HDMI_FIFO_CTL, drift | FIFO_RECENTER);
    hvs6_udelay(1000);
    wr(HDMI_BASE + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(HDMI_BASE + HDMI_FIFO_CTL, drift | FIFO_RECENTER);

    if (!hvs6_poll(HDMI_BASE + HDMI_FIFO_CTL, FIFO_RECENTER_DONE, FIFO_RECENTER_DONE, 100000))
        bug("[VC4HVS6] FIFO recentre did not complete\n");
}

/*
 * Bring channel ch up at w x h, scanning `list`, in the order of
 * DISPLAY-SPEC 10: HVS channel, PHY, HDMI timings, pixelvalve, FIFO
 * recentre. The boot size gets back exactly what the firmware had; any
 * other runs the pixelvalve at 1 pixel/clock with ODD_TIMING.
 */
BOOL vc4_hvs6_mode_start(struct VideoCoreGfx_staticdata *xsd, ULONG ch,
                         ULONG w, ULONG h, ULONG list)
{
    struct vc4_hvs6_state *st = &xsd->vcsd_HVS6;
    const struct vcgfx_timing *t;
    ULONG pv[PV_REGS], hd[6], vidctl, ctrl0, vco_div, rm_offset, i;
    BOOL boot;

    if (!(t = hvs6_lookup(xsd, w, h, &boot)))
        return FALSE;

    CopyMem(st->h6_BootPV, pv, sizeof(pv));
    CopyMem(st->h6_BootHDMI, hd, sizeof(hd));
    vidctl    = st->h6_BootVidCtl;
    ctrl0     = st->h6_BootCtrl0;
    vco_div   = st->h6_BootVcoDiv;
    rm_offset = st->h6_BootRmOffset;

    if (!boot)
    {
        ULONG hfp = t->hstart - t->hdisp, hsync = t->hend - t->hstart;
        ULONG hbp = t->htotal - t->hend;
        ULONG vfp = t->vstart - t->vdisp, vsync = t->vend - t->vstart;
        ULONG vbp = t->vtotal - t->vend;

        pv[PV(PV_V_CONTROL)] |= PV_VC_ODD_TIMING;
        pv[PV(PV_HORZA)] = (hbp << 16) | hsync;
        pv[PV(PV_HORZB)] = (hfp << 16) | t->hdisp;
        pv[PV(PV_VERTA)] = (vbp << 16) | vsync;
        pv[PV(PV_VERTB)] = (vfp << 16) | t->vdisp;

        hd[0] = ((t->flags & VCGFX_TIMING_PVSYNC) ? HDMI_HORZA_VPOS : 0)
              | ((t->flags & VCGFX_TIMING_PHSYNC) ? HDMI_HORZA_HPOS : 0)
              | (hfp << 16) | t->hdisp;
        hd[1] = (hbp << 16) | hsync;
        hd[2] = (vsync << 24) | (vfp << 16) | t->vdisp;
        hd[3] = vbp;
        hd[4] = hd[2];
        hd[5] = hd[3];

        /* Polarity lives in both the HDMI block and the shared hd block. */
        vidctl = (vidctl & ~(VID_CTL_VSYNC_LOW | VID_CTL_HSYNC_LOW))
               | ((t->flags & VCGFX_TIMING_PVSYNC) ? 0 : VID_CTL_VSYNC_LOW)
               | ((t->flags & VCGFX_TIMING_PHSYNC) ? 0 : VID_CTL_HSYNC_LOW);
        ctrl0  = (ctrl0 & ~HVS6_CTRL0_SIZE) | ((w - 1) << 16) | (h - 1);
        hvs6_pll(t->clock, &vco_div, &rm_offset);
    }

    hvs6_wr(HVS6_CHAN(ch) + HVS6_DISPLIST, list);
    hvs6_wr(HVS6_CHAN(ch) + HVS6_DISPCTRL, HVS6_CTRL0_RESET);
    hvs6_wr(HVS6_CHAN(ch) + HVS6_DISPCTRL, ctrl0 & ~HVS6_CTRL0_RESET);

    hvs6_phy_init(vco_div, rm_offset, boot ? st->h6_BootLane : lane_ctl);
    hvs6_udelay(20000);                                     /* PLL lock */

    wr(HDMI_BASE + HDMI_HORZA, hd[0]);
    wr(HDMI_BASE + HDMI_HORZB, hd[1]);
    wr(HDMI_BASE + HDMI_VERTA0, hd[2]);
    wr(HDMI_BASE + HDMI_VERTB0, hd[3]);
    wr(HDMI_BASE + HDMI_VERTA1, hd[4]);
    wr(HDMI_BASE + HDMI_VERTB1, hd[5]);
    wr(HD_VID_CTL, vidctl);

    for (i = PV(PV_HORZA); i <= PV(PV_VERTB); i++)
        wr(PV_BASE + 4 * i, pv[i]);
    wr(PV_BASE + PV_CONTROL, (pv[PV(PV_CONTROL)] & ~PV_CONTROL_EN) | PV_CONTROL_FIFO_CLR);
    wr(PV_BASE + PV_CONTROL, pv[PV(PV_CONTROL)]);
    wr(PV_BASE + PV_V_CONTROL, pv[PV(PV_V_CONTROL)]);

    hvs6_recenter();

    for (i = 0; i < 100; i++)
    {
        if ((hvs6_rd(HVS6_CHAN(ch) + HVS6_DISPLACT) & 0xfff) == list)
            break;
        hvs6_udelay(1000);
    }

    bug("[VC4HVS6] mode %ux%u, %u kHz%s%s\n", w, h, t->clock,
        boot ? " (boot mode)" : "", (i == 100) ? " - list never latched" : "");
    return i < 100;
}
