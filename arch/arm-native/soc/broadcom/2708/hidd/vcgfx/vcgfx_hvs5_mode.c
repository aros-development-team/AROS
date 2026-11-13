/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore Gfx Hidd - BCM2711 mode setting.

    HDMI0 only, and the way the firmware runs it: PV2 at 2 pixels/clock.
    A mode of our own programs the HVS channel size, PV2, the HDMI
    timings, the PHY PLL and band, the HSM clock (mailbox), PV2's clock
    divider in the clock manager (the mailbox ignores that one), the CSC
    and the AVI infoframe. Every value was read back from the firmware at
    720p, 1080p, 1440p and 4K30 by raspi-dispdump, and the sequence
    proven by raspi-modetest2711: the PLL is retuned live, no PHY reset.

    No scrambling, so nothing from 340 MHz up, and a VCO only where its
    band field was seen.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/mbox.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hardware.h"
#include "vcgfx_hvs5.h"

#ifdef MBoxBase
#undef MBoxBase
#endif
#define MBoxBase            xsd->vcsd_MBoxBase

/* Kill switch: 0 = the boot mode is the only one, as before. */
#define VC4_HVS5_MODESET    1

/* HDMI0's blocks, as the device tree names them. */
#define PV2_BASE            (ARM_PERIIOBASE + 0x20a000)
#define CSC_BASE            (ARM_PERIIOBASE + 0xf00200)
#define HDMI_BASE           (ARM_PERIIOBASE + 0xf00700)
#define PHY_BASE            (ARM_PERIIOBASE + 0xf00f00)
#define RM_BASE             (ARM_PERIIOBASE + 0xf00f80)
#define PKT_BASE            (ARM_PERIIOBASE + 0xf01b00)
#define HD_VID_CTL          (ARM_PERIIOBASE + 0xf20044)

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
#define PV_VC_VIDEN         (1 << 0)
#define PV_VC_INTERLACE     (1 << 4)
#define PV_VC_ODD_TIMING    (1 << 29)

#define HVS_ENB             (1UL << 31)
#define HVS_SIZE            0x1fff1fff      /* width 31:16, height 15:0 */

/* BCM2712's HDMI registers less 8, VERTA1/VERTB1 without its gap. */
#define HDMI_FIFO_CTL       0x074
#define HDMI_RAM_PKT_CONFIG 0x0bc
#define HDMI_RAM_PKT_STATUS 0x0c4
#define HDMI_HORZA          0x0e4
#define HDMI_HORZA_VPOS     (1 << 15)
#define HDMI_HORZA_HPOS     (1 << 14)
#define FIFO_RECENTER       (1 << 6)
#define FIFO_RECENTER_DONE  (1 << 14)
#define VID_CTL_VSYNC_LOW   (1UL << 28)
#define VID_CTL_HSYNC_LOW   (1UL << 27)

#define CSC_COEF(i)         (0x004 + 4 * (i))
#define PKT_AVI_SLOT        2
#define PKT_AVI             (PKT_BASE + PKT_AVI_SLOT * 0x24)

/* PLL and PHY band fields; m5_BootPHY[] keeps them in this order. */
#define PHY_LANES           0x008
#define PHY_BAND            0x010
#define PHY_REFCLK          0x01c
#define PHY_DIV             0x028
#define PHY_FRAC            0x060           /* RM_OFFSET >> 4 */
#define RM_OFFSET           0x018
#define RM_OFFSET_ONLY      (1UL << 31)
#define PHY_DIV_MASK        0xff00
#define PHY_DIV_SHIFT       8
#define PHY_BAND_VCO_SHIFT  16
#define PHY_BAND_MASK       ((0xfUL << 16) | 0x1110 | 0xf)
#define PHY_BAND_HIGH       0x1110
#define PHY_REFCLK_HIGH     0x200
#define VCO_MIN             3600000000ULL   /* the firmware never ran below 3712.5 MHz */

/* Low up to 148.5 MHz, high from 238.55, top at 297: where between the
 * steps fall is a guess. */
#define HIGH_BAND_KHZ       200000
#define TOP_BAND_KHZ        270000
#define TMDS_SCRAMBLE       340000

static const ULONG lanes_low  = 0x0909090c;
static const ULONG lanes_high = 0x0f0f0f0c;
static const ULONG lanes_top  = 0x4d4d4d0c;

/* Identity, and 219/255 plus 16, as the firmware loads them. */
static const ULONG csc_full[6] =
    { 0x00002000, 0x00000000, 0x20000000, 0x00000000, 0x00000000, 0x00002000 };
static const ULONG csc_limited[6] =
    { 0x00001b80, 0x04000000, 0x1b800000, 0x04000000, 0x00000000, 0x04001b80 };

/* PV2's clock, firmware clock 9: a divider off PLLC_PER (648 MHz), /8 at
 * 1080p, /5 at 1440p, /4 at 4K30. The largest that keeps up with 2
 * pixels/clock, at most /8, as the firmware picks it. */
#define CM_PIX_CTL          (ARM_PERIIOBASE + 0x101208)
#define CM_PIX_DIV          (ARM_PERIIOBASE + 0x10120c)
#define CM_PIX_SRC          5
#define CM_PIX_DIV_MAX      8
#define CM_PASSWD           0x5a000000
#define CM_CTL_ENAB         (1 << 4)
#define CM_CTL_BUSY         (1 << 7)

#define VCCLOCK_PIXEL       9
#define VCCLOCK_HSM         13
#define HSM_MIN             120000000       /* the firmware's own at 720p */

static inline ULONG rd(IPTR a)
{
    return *(volatile ULONG *)a;
}

static inline void wr(IPTR a, ULONG v)
{
    *(volatile ULONG *)a = v;
    asm volatile ("dsb sy" ::: "memory");
}

static inline ULONG hvs5_reg(ULONG off)
{
    return rd(HVS5_BASE + off);
}

/* Busy waits on the 1 MHz system timer: callers are not always processes. */
static inline ULONG hvs5m_now(void)
{
    return AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);
}

static void hvs5m_udelay(ULONG us)
{
    ULONG start = hvs5m_now();

    while ((hvs5m_now() - start) < us)
        ;
}

static BOOL hvs5m_poll(IPTR reg, ULONG mask, ULONG want, ULONG us)
{
    ULONG start = hvs5m_now();

    while ((rd(reg) & mask) != want)
        if ((hvs5m_now() - start) >= us)
            return FALSE;
    return TRUE;
}

/* GETCLKMEASURED or SETCLKRATE on a firmware clock. */
static ULONG hvs5m_clock(struct VideoCoreGfx_staticdata *xsd, ULONG tag, ULONG id, ULONG hz)
{
    unsigned int *m = xsd->vcsd_MBoxMessage;
    ULONG rate = 0;

    VC4_MBOX_LOCK(xsd);
    m[0] = AROS_LONG2LE(9 * 4);
    m[1] = AROS_LONG2LE(VCTAG_REQ);
    m[2] = AROS_LONG2LE(tag);
    m[3] = AROS_LONG2LE(12);
    m[4] = 0;
    m[5] = AROS_LONG2LE(id);
    m[6] = AROS_LONG2LE(hz);
    m[7] = 0;
    m[8] = 0;
    if ((MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, m) != (volatile unsigned int *)-1)
        && (m[1] == AROS_LONG2LE(VCTAG_RESP)))
        rate = AROS_LE2LONG(m[6]);
    VC4_MBOX_UNLOCK(xsd);
    return rate;
}

/* Smallest vco_div taking the VCO over VCO_MIN that lands where its band
 * field was read back: 6 at 3712.5 MHz, 5 from 4455 to 4830, 2 at 5940.
 * The rate manager takes VCO / 54 MHz in 11.21 fixed point. */
static BOOL hvs5m_pll(ULONG khz, ULONG *div, ULONG *offset, ULONG *band)
{
    UQUAD bits = (UQUAD)khz * 10000, vco;
    ULONG d;

    for (d = 1; d < 64; d++)
    {
        vco = bits * d;
        if (vco < VCO_MIN)
            continue;
        if (vco < 4200000000ULL)
            *band = 6;
        else if ((vco >= 4400000000ULL) && (vco <= 4850000000ULL))
            *band = 5;
        else if ((vco >= 5900000000ULL) && (vco <= 5950000000ULL))
            *band = 2;
        else
            continue;

        *div    = d;
        *offset = (ULONG)((vco << 21) / 54000000);
        return TRUE;
    }
    return FALSE;
}

/* PV2's clock divider for a pixel clock, 0 if none is fast enough. */
static ULONG hvs5m_pixdiv(struct vc4_hvs5_mode *m5, ULONG khz)
{
    ULONG div = m5->m5_PixSrc / (khz * 500);

    return (div > CM_PIX_DIV_MAX) ? CM_PIX_DIV_MAX : div;
}

/*
 * Record the firmware's boot mode before anything touches it: the values
 * to put back when the boot size is asked for again, and its timings.
 * Mode setting is enabled only for the setup the sequence was proven on.
 */
void vc4_hvs5_mode_capture(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs5_mode *m5 = &xsd->vcsd_HVS5M;
    struct vcgfx_timing *t = &m5->m5_BootTiming;
    ULONG horza, horzb, verta, vertb, div, ch, i;
    UQUAD vco;

    m5->m5_ModeOK = FALSE;
    if (!VC4_HVS5_MODESET || (hvs5_reg(HVS5_ID) != HVS5_ID_MAGIC))
        return;

    for (i = 0; i < PV_REGS; i++)
        m5->m5_BootPV[i] = rd(PV2_BASE + 4 * i);
    if (!(m5->m5_BootPV[PV(PV_CONTROL)] & PV_CONTROL_EN)
        || !(m5->m5_BootPV[PV(PV_V_CONTROL)] & PV_VC_VIDEN)
        || (m5->m5_BootPV[PV(PV_V_CONTROL)] & (PV_VC_ODD_TIMING | PV_VC_INTERLACE)))
    {
        bug("[VC4HVS5] mode setting off: HDMI0 is not running a 2 pixel/clock progressive mode\n");
        return;
    }

    horza = m5->m5_BootPV[PV(PV_HORZA)];
    horzb = m5->m5_BootPV[PV(PV_HORZB)];
    verta = m5->m5_BootPV[PV(PV_VERTA)];
    vertb = m5->m5_BootPV[PV(PV_VERTB)];
    t->hdisp  = (horzb & 0xffff) * 2;
    t->hstart = t->hdisp  + (horzb >> 16) * 2;
    t->hend   = t->hstart + (horza & 0xffff) * 2;
    t->htotal = t->hend   + (horza >> 16) * 2;
    t->vdisp  = vertb & 0xffff;
    t->vstart = t->vdisp  + (vertb >> 16);
    t->vend   = t->vstart + (verta & 0xffff);
    t->vtotal = t->vend   + (verta >> 16);

    for (ch = 0; ch < HVS5_CHANNELS; ch++)
    {
        m5->m5_BootCtrl = hvs5_reg(HVS5_DISPCTRLX(ch));
        if ((m5->m5_BootCtrl & HVS_ENB)
            && ((m5->m5_BootCtrl & HVS_SIZE) == (((ULONG)t->hdisp << 16) | t->vdisp)))
            break;
    }
    if (ch == HVS5_CHANNELS)
    {
        bug("[VC4HVS5] mode setting off: no HVS channel is %ux%u\n", t->hdisp, t->vdisp);
        return;
    }

    for (i = 0; i < 6; i++)
        m5->m5_BootHDMI[i] = rd(HDMI_BASE + HDMI_HORZA + 4 * i);
    m5->m5_BootVidCtl = rd(HD_VID_CTL);
    m5->m5_BootPHY[0] = rd(PHY_BASE + PHY_LANES);
    m5->m5_BootPHY[1] = rd(PHY_BASE + PHY_BAND);
    m5->m5_BootPHY[2] = rd(PHY_BASE + PHY_REFCLK);
    m5->m5_BootPHY[3] = rd(PHY_BASE + PHY_DIV);
    m5->m5_BootPHY[4] = rd(PHY_BASE + PHY_FRAC);
    m5->m5_BootPHY[5] = rd(RM_BASE + RM_OFFSET);
    for (i = 0; i < 6; i++)
        m5->m5_BootCSC[i] = rd(CSC_BASE + CSC_COEF(i));
    for (i = 0; i < 9; i++)
        m5->m5_BootAVI[i] = rd(PKT_AVI + 4 * i);
    m5->m5_BootAVIOn = (rd(HDMI_BASE + HDMI_RAM_PKT_CONFIG) & (1UL << PKT_AVI_SLOT)) != 0;
    t->flags = ((m5->m5_BootHDMI[0] & HDMI_HORZA_HPOS) ? VCGFX_TIMING_PHSYNC : 0)
             | ((m5->m5_BootHDMI[0] & HDMI_HORZA_VPOS) ? VCGFX_TIMING_PVSYNC : 0);

    div = (m5->m5_BootPHY[3] & PHY_DIV_MASK) >> PHY_DIV_SHIFT;
    if (!div)
    {
        bug("[VC4HVS5] mode setting off: PHY divider reads %08x\n", m5->m5_BootPHY[3]);
        return;
    }
    vco = ((UQUAD)(m5->m5_BootPHY[5] & ~RM_OFFSET_ONLY) * 54000000) >> 21;
    t->clock = (ULONG)(vco / (div * 10) / 1000);

    /* The boot mode is brought back without SCDC. */
    if (t->clock >= TMDS_SCRAMBLE)
    {
        bug("[VC4HVS5] mode setting off: boot mode runs %u kHz\n", t->clock);
        return;
    }

    m5->m5_BootPixDiv = (rd(CM_PIX_DIV) >> 12) & 0xfff;
    m5->m5_PixSrc = hvs5m_clock(xsd, VCTAG_GETCLKMEASURED, VCCLOCK_PIXEL, 0) * m5->m5_BootPixDiv;
    if (((rd(CM_PIX_CTL) & 0xf) != CM_PIX_SRC) || !(rd(CM_PIX_CTL) & CM_CTL_ENAB)
        || (rd(CM_PIX_DIV) & 0xfff) || (m5->m5_PixSrc < 600000000) || (m5->m5_PixSrc > 700000000))
    {
        bug("[VC4HVS5] mode setting off: pixel clock CTL %08x DIV %08x, source %u Hz\n",
            rd(CM_PIX_CTL), rd(CM_PIX_DIV), m5->m5_PixSrc);
        return;
    }
    m5->m5_BootHSM = hvs5m_clock(xsd, VCTAG_GETCLKMEASURED, VCCLOCK_HSM, 0);

    m5->m5_ModeOK    = TRUE;
    m5->m5_Ours      = FALSE;
    m5->m5_CurTiming = *t;
    bug("[VC4HVS5] boot mode %ux%u, %u kHz on channel %u: mode setting on\n",
        t->hdisp, t->vdisp, t->clock, ch);
}

/* Can we drive this timing, and is it one the sink says it takes? */
static BOOL hvs5m_usable(struct VideoCoreGfx_staticdata *xsd, const struct vcgfx_timing *t)
{
    const struct vcgfx_edid *e = &xsd->vcsd_EDID;
    ULONG div, off, band, hz;

    if ((t->clock >= TMDS_SCRAMBLE) || (t->flags & VCGFX_TIMING_INTERLACE)
        || ((t->hdisp | t->hstart | t->hend | t->htotal) & 1)
        || !hvs5m_pll(t->clock, &div, &off, &band) || !hvs5m_pixdiv(&xsd->vcsd_HVS5M, t->clock))
        return FALSE;

    if (e->valid)
    {
        /* Nothing larger than the sink's preferred, i.e. native, mode. */
        if (e->ntimings && ((t->hdisp > e->timings[0].hdisp)
                            || (t->vdisp > e->timings[0].vdisp)))
            return FALSE;

        hz = (t->clock * 1000 + t->htotal * t->vtotal / 2) / (t->htotal * t->vtotal);
        if ((e->vmax && ((hz < e->vmin) || (hz > e->vmax)))
            || (e->maxclock && (t->clock > e->maxclock)))
            return FALSE;
    }
    return TRUE;
}

BOOL vc4_hvs5_mode_usable(struct VideoCoreGfx_staticdata *xsd, const struct vcgfx_timing *t)
{
    return xsd->vcsd_HVS5M.m5_ModeOK && hvs5m_usable(xsd, t);
}

/* The i-th mode we offer besides the boot mode, or NULL at the end:
 * the table first, then the sink's own detailed timings. */
const struct vcgfx_timing *vc4_hvs5_mode(struct VideoCoreGfx_staticdata *xsd, ULONG i)
{
    if (!xsd->vcsd_HVS5M.m5_ModeOK)
        return NULL;
    if (i < VCGFX_STD_MODES)
        return &vcgfx_std_modes[i];
    i -= VCGFX_STD_MODES;
    if (i < xsd->vcsd_EDID.ntimings)
        return &xsd->vcsd_EDID.timings[i];
    return NULL;
}

/* One size can run at rates sharing a pixel clock, so a rate is told
 * apart by its totals too. NULL wants any. */
static BOOL hvs5m_same_rate(const struct vcgfx_timing *t, const struct vcgfx_timing *want)
{
    return !want || ((t->clock == want->clock) && (t->htotal == want->htotal)
                     && (t->vtotal == want->vtotal));
}

/* Timings to show w x h with at the rate in want (NULL = the first one
 * offered); NULL for anything we cannot set. */
const struct vcgfx_timing *vc4_hvs5_mode_lookup(struct VideoCoreGfx_staticdata *xsd,
                                                ULONG w, ULONG h,
                                                const struct vcgfx_timing *want, BOOL *boot)
{
    struct vc4_hvs5_mode *m5 = &xsd->vcsd_HVS5M;
    const struct vcgfx_timing *t;
    ULONG i;

    *boot = FALSE;
    if (!m5->m5_ModeOK)
        return NULL;

    *boot = (w == m5->m5_BootTiming.hdisp) && (h == m5->m5_BootTiming.vdisp)
            && hvs5m_same_rate(&m5->m5_BootTiming, want);
    if (*boot)
        return &m5->m5_BootTiming;

    for (i = 0; (t = vc4_hvs5_mode(xsd, i)); i++)
        if ((t->hdisp == w) && (t->vdisp == h) && hvs5m_same_rate(t, want)
            && hvs5m_usable(xsd, t))
            return t;
    return NULL;
}

/* Stop the clock, change the divider, start it again; PV2 is off. */
static void hvs5m_pixclock(ULONG divi)
{
    ULONG ctl = rd(CM_PIX_CTL) & 0xffffff;

    if (((rd(CM_PIX_DIV) >> 12) & 0xfff) == divi)
        return;
    wr(CM_PIX_CTL, CM_PASSWD | (ctl & ~CM_CTL_ENAB));
    if (!hvs5m_poll(CM_PIX_CTL, CM_CTL_BUSY, 0, 10000))
        bug("[VC4HVS5] pixel clock did not stop\n");
    wr(CM_PIX_DIV, CM_PASSWD | (divi << 12));
    wr(CM_PIX_CTL, CM_PASSWD | ctl | CM_CTL_ENAB);
}

/* CSC and AVI infoframe. The slot's RAM is locked while it is enabled. */
static void hvs5m_colour(const ULONG *coef, const ULONG *avi, BOOL avi_on)
{
    ULONG i;

    for (i = 0; i < 6; i++)
        wr(CSC_BASE + CSC_COEF(i), coef[i]);

    wr(HDMI_BASE + HDMI_RAM_PKT_CONFIG,
       rd(HDMI_BASE + HDMI_RAM_PKT_CONFIG) & ~(1UL << PKT_AVI_SLOT));
    if (!hvs5m_poll(HDMI_BASE + HDMI_RAM_PKT_STATUS, 1UL << PKT_AVI_SLOT, 0, 100000))
        bug("[VC4HVS5] AVI infoframe slot did not unlock\n");
    for (i = 0; i < 9; i++)
        wr(PKT_AVI + 4 * i, avi[i]);
    if (avi_on)
        wr(HDMI_BASE + HDMI_RAM_PKT_CONFIG,
           rd(HDMI_BASE + HDMI_RAM_PKT_CONFIG) | (1UL << PKT_AVI_SLOT));
}

static void hvs5m_recenter(void)
{
    ULONG drift = rd(HDMI_BASE + HDMI_FIFO_CTL) & 0xefff;

    wr(HDMI_BASE + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(HDMI_BASE + HDMI_FIFO_CTL, drift | FIFO_RECENTER);
    hvs5m_udelay(1000);
    wr(HDMI_BASE + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(HDMI_BASE + HDMI_FIFO_CTL, drift | FIFO_RECENTER);

    if (!hvs5m_poll(HDMI_BASE + HDMI_FIFO_CTL, FIFO_RECENTER_DONE, FIFO_RECENTER_DONE, 100000))
        bug("[VC4HVS5] FIFO recentre did not complete\n");
}

/*
 * Switch HDMI0 to t - for the boot mode the firmware's own values back -
 * with channel ch scanning `list` (0 = leave it). PV2 stops, then the
 * channel; clocks, PLL and timings change with nothing sent; then PV2
 * runs again and the HDMI FIFO recentres.
 */
BOOL vc4_hvs5_mode_set(struct VideoCoreGfx_staticdata *xsd, ULONG ch, ULONG list,
                       const struct vcgfx_timing *t, BOOL boot)
{
    struct vc4_hvs5_mode *m5 = &xsd->vcsd_HVS5M;
    ULONG pv[PV_REGS], hd[6], phy[6], csc[6], avi[9];
    ULONG vidctl, ctrl, hsm, pixdiv, ctl, i;
    BOOL avi_on = TRUE, limited = FALSE;
    UBYTE vic = 0;

    CopyMem(m5->m5_BootPV, pv, sizeof(pv));
    CopyMem(m5->m5_BootHDMI, hd, sizeof(hd));
    CopyMem(m5->m5_BootPHY, phy, sizeof(phy));
    vidctl = m5->m5_BootVidCtl;
    ctrl   = m5->m5_BootCtrl;
    hsm    = m5->m5_BootHSM;
    pixdiv = m5->m5_BootPixDiv;

    if (boot)
    {
        CopyMem(m5->m5_BootCSC, csc, sizeof(csc));
        CopyMem(m5->m5_BootAVI, avi, sizeof(avi));
        avi_on = m5->m5_BootAVIOn;
    }
    else
    {
        ULONG hfp = t->hstart - t->hdisp, hsync = t->hend - t->hstart;
        ULONG hbp = t->htotal - t->hend;
        ULONG vfp = t->vstart - t->vdisp, vsync = t->vend - t->vstart;
        ULONG vbp = t->vtotal - t->vend;
        BOOL high = (t->clock > HIGH_BAND_KHZ), top = (t->clock > TOP_BAND_KHZ);
        ULONG div, offset, band;

        pv[PV(PV_HORZA)] = ((hbp / 2) << 16) | (hsync / 2);
        pv[PV(PV_HORZB)] = ((hfp / 2) << 16) | (t->hdisp / 2);
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
        vidctl = (vidctl & ~(VID_CTL_VSYNC_LOW | VID_CTL_HSYNC_LOW))
               | ((t->flags & VCGFX_TIMING_PVSYNC) ? 0 : VID_CTL_VSYNC_LOW)
               | ((t->flags & VCGFX_TIMING_PHSYNC) ? 0 : VID_CTL_HSYNC_LOW);
        ctrl = (ctrl & ~HVS_SIZE) | ((ULONG)t->hdisp << 16) | t->vdisp;

        hvs5m_pll(t->clock, &div, &offset, &band);
        phy[0] = top ? lanes_top : high ? lanes_high : lanes_low;
        phy[1] = (phy[1] & ~PHY_BAND_MASK) | (band << PHY_BAND_VCO_SHIFT)
               | (high ? PHY_BAND_HIGH : 0) | (top ? 0xf : 0x3);
        phy[2] = high ? (phy[2] | PHY_REFCLK_HIGH) : (phy[2] & ~PHY_REFCLK_HIGH);
        phy[3] = (phy[3] & ~PHY_DIV_MASK) | (div << PHY_DIV_SHIFT);
        phy[4] = offset >> 4;
        phy[5] = RM_OFFSET_ONLY | offset;

        hsm    = (t->clock * 1010 > HSM_MIN) ? t->clock * 1010 : HSM_MIN;
        pixdiv = hvs5m_pixdiv(m5, t->clock);

        vic = vcgfx_edid_avi(t, avi, &limited);
        CopyMem(limited ? csc_limited : csc_full, csc, sizeof(csc));
    }

    /* PV2 down; the 20 ms keep a pixel from being left in the PV-to-HDMI
     * FIFO, which would shift every line by one. */
    wr(PV2_BASE + PV_V_CONTROL, rd(PV2_BASE + PV_V_CONTROL) & ~PV_VC_VIDEN);
    if (!hvs5m_poll(PV2_BASE + PV_V_CONTROL, PV_VC_VIDEN, 0, 100000))
        bug("[VC4HVS5] VIDEN did not clear\n");
    hvs5m_udelay(20000);
    ctl = rd(PV2_BASE + PV_CONTROL) & ~PV_CONTROL_EN;
    wr(PV2_BASE + PV_CONTROL, ctl);
    wr(PV2_BASE + PV_CONTROL, ctl | PV_CONTROL_FIFO_CLR);

    wr(HVS5_BASE + HVS5_DISPCTRLX(ch), hvs5_reg(HVS5_DISPCTRLX(ch)) & ~HVS_ENB);
    hvs5m_udelay(20000);
    if (list)
        wr(HVS5_BASE + HVS5_DISPLIST(ch), list);
    wr(HVS5_BASE + HVS5_DISPCTRLX(ch), ctrl);

    /* HSM goes up before the faster link, and was lowering fine too. */
    if (hvs5m_clock(xsd, VCTAG_GETCLKMEASURED, VCCLOCK_HSM, 0) / 1000 != hsm / 1000)
        hvs5m_clock(xsd, VCTAG_SETCLKRATE, VCCLOCK_HSM, hsm);
    hvs5m_pixclock(pixdiv);

    wr(RM_BASE + RM_OFFSET, phy[5]);
    wr(PHY_BASE + PHY_FRAC, phy[4]);
    wr(PHY_BASE + PHY_DIV, phy[3]);
    wr(PHY_BASE + PHY_BAND, phy[1]);
    wr(PHY_BASE + PHY_REFCLK, phy[2]);
    wr(PHY_BASE + PHY_LANES, phy[0]);
    hvs5m_udelay(20000);                                    /* PLL lock */

    for (i = 0; i < 6; i++)
        wr(HDMI_BASE + HDMI_HORZA + 4 * i, hd[i]);
    wr(HD_VID_CTL, vidctl);
    hvs5m_colour(csc, avi, avi_on);

    for (i = PV(PV_HORZA); i <= PV(PV_VERTB); i++)
        wr(PV2_BASE + 4 * i, pv[i]);
    wr(PV2_BASE + PV_CONTROL, (pv[PV(PV_CONTROL)] & ~PV_CONTROL_EN) | PV_CONTROL_FIFO_CLR);
    wr(PV2_BASE + PV_CONTROL, pv[PV(PV_CONTROL)]);
    wr(PV2_BASE + PV_V_CONTROL, pv[PV(PV_V_CONTROL)]);

    hvs5m_recenter();

    m5->m5_Ours      = !boot;
    m5->m5_CurTiming = *t;

    for (i = 0; list && (i < 100); i++)
    {
        if (hvs5_reg(HVS5_DISPLACT(ch)) == list)
            break;
        hvs5m_udelay(1000);
    }

    if (boot)
        bug("[VC4HVS5] mode %ux%u, %u kHz (boot mode)%s\n", t->hdisp, t->vdisp, t->clock,
            (i == 100) ? " - list never latched" : "");
    else
        bug("[VC4HVS5] mode %ux%u, %u kHz, pixel clock /%u, VIC %u, %s range RGB%s\n",
            t->hdisp, t->vdisp, t->clock, pixdiv, vic, limited ? "limited" : "full",
            (i == 100) ? " - list never latched" : "");
    return i < 100;
}

/* Back to the firmware's mode, which it rebuilds its lists for. */
void vc4_hvs5_mode_restore(struct VideoCoreGfx_staticdata *xsd)
{
    struct vc4_hvs5_mode *m5 = &xsd->vcsd_HVS5M;

    if (!m5->m5_Ours)
        return;

    VC4_MBOX_LOCK(xsd);
    vc4_hvs5_mode_set(xsd, xsd->vcsd_HVS.hvs_Channel, 0, &m5->m5_BootTiming, TRUE);
    VC4_MBOX_UNLOCK(xsd);
}
