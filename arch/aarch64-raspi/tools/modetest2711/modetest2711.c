/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2711 mode set test - the Pi 4 counterpart of modetest.

    Switches HDMI0 from the firmware's boot mode to a mode of our own and,
    after HOLD seconds, back again. Programs the HVS channel size, PV2 at
    2 pixels/clock as the firmware runs it, the HDMI timings, the PLL, the
    HSM clock and PV2's clock divider. CSC, infoframes and the display
    list keep what they have, so the framebuffer shows unscaled at the
    top-left.

    The PLL is retuned live, through the fields the dispdump captures
    showed to follow the rate: rm OFFSET and its copy at PHY +0x060,
    vco_div at PHY +0x028, and the band bits at +0x008/+0x010/+0x01c.
    Whether the PHY takes that without a reset is what this finds out;
    the frame rate measured after each step tells.

    MODE=1920x1080 programs the boot mode from our own values first.
    KHZ= runs the mode at another pixel clock, VCOBAND= forces the field
    at PHY +0x010 bits 19:16 - to map where the VCO bands end.
    CPRMAN lists every enabled clock divider in the clock manager and the
    non-zero words of its PLL half.

    Without GO nothing is written. Everything also goes to the serial log.

    Usage: modetest2711 [MODE=<WxH[@Hz]>] [KHZ=<n>] [VCOBAND=<n>] [CPRMAN] [LIST] [HOLD=<s>] [GO]
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

/* CEA-861 and VESA DMT timings, clock in kHz. All horizontal values are
 * even, as PV2 runs 2 pixels/clock. The last one is the ASUS VG27AQL5A's
 * native mode, from its EDID. */
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
    { "1024x768@60",   65000, 1024,  24, 136, 160,  768,  3, 6, 29, TRUE,  TRUE  },
    { "1280x720@60",   74250, 1280, 110,  40, 220,  720,  5, 5, 20, FALSE, FALSE },
    { "1280x1024@60", 108000, 1280,  48, 112, 248, 1024,  1, 3, 38, FALSE, FALSE },
    { "1680x1050@60", 146250, 1680, 104, 176, 280, 1050,  3, 6, 30, TRUE,  FALSE },
    { "1920x1080@60", 148500, 1920,  88,  44, 148, 1080,  4, 5, 36, FALSE, FALSE },
    { "2560x1440@60", 241500, 2560,  48,  32,  80, 1440,  3, 5, 33, FALSE, TRUE  },
};
#define MODES                   (sizeof(modes) / sizeof(modes[0]))

/* HDMI0 path, addresses as the device tree gave them to dispdump. */
#define PV_OFF                  0x20a000        /* PV2 */
#define HVS_OFF                 0x400000
#define HDMI_OFF                0xf00700
#define PHY_OFF                 0xf00f00
#define RM_OFF                  0xf00f80
#define HD_OFF                  0xf20000

#define HVS_DISPCTRL(ch)        (HVS_OFF + 0x40 + 0x10 * (ch))
#define HVS_CHANNELS            3
#define HVS_ENB                 (1UL << 31)
#define HVS_SIZE                0x1fff1fff      /* width 31:16, height 15:0 */

#define PV_CONTROL              0x00
#define PV_V_CONTROL            0x04
#define PV_HORZA                0x0c
#define PV_HORZB                0x10
#define PV_VERTA                0x14
#define PV_VERTB                0x18
#define PV_CONTROL_EN           (1 << 0)
#define PV_CONTROL_FIFO_CLR     (1 << 1)
#define PV_VC_VIDEN             (1 << 0)
#define PV_VC_ODD_TIMING        (1 << 29)

/* BCM2712's HDMI registers less 8, VERTA1/VERTB1 without its gap. */
#define HDMI_FIFO_CTL           0x074
#define HDMI_HORZA              0x0e4
#define HDMI_HORZB              0x0e8
#define HDMI_VERTA0             0x0ec
#define HDMI_VERTB0             0x0f0
#define HDMI_VERTA1             0x0f4
#define HDMI_VERTB1             0x0f8
#define HDMI_HSM_PER_FRAME      0x160
#define HDMI_HORZA_VPOS         (1 << 15)
#define HDMI_HORZA_HPOS         (1 << 14)
#define FIFO_RECENTER           (1 << 6)
#define FIFO_RECENTER_DONE      (1 << 14)
#define HD_VID_CTL              (HD_OFF + 0x044)
#define HD_FRAME_COUNT          (HD_OFF + 0x060)
#define VID_CTL_VSYNC_LOW       (1UL << 28)
#define VID_CTL_HSYNC_LOW       (1UL << 27)

/* PLL and band fields, from captures at 74.25, 148.5 and 238.55 MHz. */
#define PHY_LANES               0x008
#define PHY_BAND                0x010
#define PHY_BAND_VCO_SHIFT      16
#define PHY_BAND_VCO_MASK       (0xfUL << 16)
#define PHY_BAND_HIGH           0x1110
#define PHY_BAND_TOP_MASK       0xf
#define PHY_BAND_TOP            0xf             /* 3 below */
#define PHY_REFCLK              0x01c
#define PHY_REFCLK_HIGH         0x200
#define PHY_DIV                 0x028
#define PHY_DIV_MASK            0xff00
#define PHY_DIV_SHIFT           8
#define PHY_FRAC                0x060           /* rm OFFSET >> 4 */
#define RM_OFFSET               0x018
#define RM_OFFSET_ONLY          (1UL << 31)
#define LANES_LOW               0x0909090c
#define LANES_HIGH              0x0f0f0f0c
#define LANES_TOP               0x4d4d4d0c
#define HIGH_BAND_KHZ           200000          /* seen low at 148.5, high at 238.55 */
#define TOP_BAND_KHZ            270000          /* seen high at 238.55, top at 297 */
#define VCO_MIN                 3600000000ULL   /* firmware ran 3712.5..4771 MHz */
#define VCO_BAND5               4000000000ULL   /* field 6 at 3712.5, 5 from 4455 */

#define CLK_HSM                 13
#define HSM_MIN                 120000000       /* the firmware's 720p rate */
/* PV2's clock, firmware clock 9: CM +0x208 off PLLC_PER (648 MHz), /8 at
 * 1080p, /5 at 1440p, /4 at 4K30. /8 capped PV2 at 162 Mpixel/s and 1440p
 * ran at 40.2 Hz. The mailbox accepts a rate for it but changes nothing,
 * so the divider is written here: the largest that keeps up with 2
 * pixels/clock, at most /8, as the firmware picks it. */
#define CLK_PIXEL               9
#define CM_PIX_CTL              (0x101000 + 0x208)
#define CM_PIX_DIV              (0x101000 + 0x20c)
#define CM_PIX_SRC              5
#define CM_PIX_DIV_MAX          8
#define CM_PASSWD               0x5a000000

/* Clock manager: CTL then DIV per clock, DIV = PASSWD | DIVI 23:12 | DIVF. */
#define CM_OFF                  0x101000
#define CM_SIZE                 0x1000
#define A2W_OFF                 (CM_OFF + 0x1000)
#define A2W_SIZE                0x1000
#define CM_CTL_ENAB             (1 << 4)
#define CM_CTL_BUSY             (1 << 7)

struct state
{
    ULONG khz, hz, hsm, pixel, pixdiv;
    ULONG hvs_ctrl;
    ULONG pv_ctl, pv_vctl, pv_horza, pv_horzb, pv_verta, pv_vertb;
    ULONG hd_horza, hd_horzb, hd_verta0, hd_vertb0, hd_verta1, hd_vertb1, vid_ctl;
    ULONG rm_offset, lanes, band, refclk, div, frac;
};

static ULONG hvs_ch;
static ULONG vco_band;          /* 0 = by VCO_BAND5 */
static ULONG pix_src;           /* Hz into PV2's divider */
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

static ULONG clock_tag(ULONG tag, ULONG id, ULONG hz)
{
    mb[0] = AROS_LONG2LE(9 * 4);
    mb[1] = AROS_LONG2LE(VCTAG_REQ);
    mb[2] = AROS_LONG2LE(tag);
    mb[3] = AROS_LONG2LE(12);
    mb[4] = 0;
    mb[5] = AROS_LONG2LE(id);
    mb[6] = AROS_LONG2LE(hz);
    mb[7] = 0;
    mb[8] = 0;

    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, mb) == (volatile unsigned int *)-1
        || mb[1] != AROS_LONG2LE(VCTAG_RESP))
        return 0;
    return AROS_LE2LONG(mb[6]);
}

static void clock_info(void)
{
    static const ULONG ids[] = { CLK_PIXEL, CLK_HSM, 14 };
    ULONG i;

    for (i = 0; i < 3; i++)
        P("  clock %2u: state %u, set %u, min %u, max %u, measured %u\n", (unsigned)ids[i],
          (unsigned)clock_tag(VCTAG_GETCLKSTATE, ids[i], 0),
          (unsigned)clock_tag(VCTAG_GETCLKRATE, ids[i], 0),
          (unsigned)clock_tag(VCTAG_GETCLKMIN, ids[i], 0),
          (unsigned)clock_tag(VCTAG_GETCLKMAX, ids[i], 0),
          (unsigned)clock_tag(VCTAG_GETCLKMEASURED, ids[i], 0));
}

/* The firmware reports clock 9 as off, so switch it on before the rate. */
static void clock_set(ULONG id, ULONG hz)
{
    ULONG got;

    if (!(clock_tag(VCTAG_GETCLKSTATE, id, 0) & 1))
        P("  clock %u state: set on, answered %u\n", (unsigned)id,
          (unsigned)clock_tag(VCTAG_SETCLKSTATE, id, 1));
    got = clock_tag(VCTAG_SETCLKRATE, id, hz);

    P("  set clock %u to %u: answered %u, measured %u\n", (unsigned)id, (unsigned)hz,
      (unsigned)got, (unsigned)clock_tag(VCTAG_GETCLKMEASURED, id, 0));
}

/* Enabled CTL/DIV pairs with an integer divider. */
static void cm_scan(void)
{
    ULONG off, ctl, div;

    P("  clock manager dividers (CTL, DIV, src, DIVI.DIVF):\n");
    for (off = 0; off + 4 < CM_SIZE; off += 4)
    {
        ctl = rd(CM_OFF + off);
        div = rd(CM_OFF + off + 4);
        if ((ctl & CM_CTL_ENAB) && !(ctl & 0xff000000) && (((div >> 12) & 0xfff) >= 2)
            && !(div & 0xff000000))
            P("    +%03x: %08x %08x  src %u, %u.%u%s\n", (unsigned)off, (unsigned)ctl,
              (unsigned)div, (unsigned)(ctl & 0xf), (unsigned)((div >> 12) & 0xfff),
              (unsigned)(div & 0xfff), (ctl & CM_CTL_BUSY) ? " busy" : "");
    }

    P("  PLL half, non-zero words (+offset from the clock manager):\n");
    for (off = 0; off < A2W_SIZE; off += 16)
    {
        ULONG w[4], i;

        for (i = 0; i < 4; i++)
            w[i] = rd(A2W_OFF + off + 4 * i);
        if (w[0] | w[1] | w[2] | w[3])
            P("    +%04x: %08x %08x %08x %08x\n", (unsigned)(0x1000 + off),
              (unsigned)w[0], (unsigned)w[1], (unsigned)w[2], (unsigned)w[3]);
    }
}

/* Smallest vco_div that lifts the VCO over VCO_MIN, as the firmware does. */
static BOOL pll_params(ULONG khz, ULONG *div, ULONG *offset, UQUAD *vco)
{
    UQUAD bits = (UQUAD)khz * 10000;
    ULONG d;

    for (d = 1; d < 64; d++)
        if (bits * d >= VCO_MIN)
            break;
    if (d == 64)
        return FALSE;
    *div = d;
    *vco = bits * d;
    *offset = (ULONG)((*vco << 21) / 54000000);
    return TRUE;
}

/* Frames per second from the HDMI frame counter, and the rate to the
 * millihertz from the HSM cycles the controller counts per frame. */
static BOOL measure(const char *when, ULONG hz)
{
    ULONG f0, f1, fps, hsm, pixel, cycles, mhz = 0;
    BOOL ok;

    f0 = rd(HD_FRAME_COUNT);
    Delay(50);
    f1 = rd(HD_FRAME_COUNT);
    fps = (f1 - f0) & 0xffff;
    hsm = clock_tag(VCTAG_GETCLKMEASURED, CLK_HSM, 0);
    pixel = clock_tag(VCTAG_GETCLKMEASURED, CLK_PIXEL, 0);
    cycles = rd(HDMI_OFF + HDMI_HSM_PER_FRAME);
    if (cycles)
        mhz = (ULONG)((UQUAD)hsm * 1000 / cycles);
    ok = (fps + 2 >= hz) && (fps <= hz + 2);

    P("  %s: %u frames/s, pixel %u Hz, HSM %u Hz, %u HSM/frame = %u.%03u Hz, FIFO_CTL %08x%s\n",
      when, (unsigned)fps, (unsigned)pixel, (unsigned)hsm, (unsigned)cycles, (unsigned)(mhz / 1000),
      (unsigned)(mhz % 1000), (unsigned)rd(HDMI_OFF + HDMI_FIFO_CTL), ok ? "" : "  <= BAD");
    return ok;
}

static void state_read(struct state *s)
{
    ULONG hact = (rd(PV_OFF + PV_HORZB) & 0xffff) * 2, vact = rd(PV_OFF + PV_VERTB) & 0xffff;
    ULONG htot = (hact / 2 + (rd(PV_OFF + PV_HORZB) >> 16) + (rd(PV_OFF + PV_HORZA) & 0xffff)
                  + (rd(PV_OFF + PV_HORZA) >> 16)) * 2;
    ULONG vtot = vact + (rd(PV_OFF + PV_VERTB) >> 16) + (rd(PV_OFF + PV_VERTA) & 0xffff)
               + (rd(PV_OFF + PV_VERTA) >> 16);
    UQUAD vco;

    s->rm_offset = rd(RM_OFF + RM_OFFSET);
    s->lanes     = rd(PHY_OFF + PHY_LANES);
    s->band      = rd(PHY_OFF + PHY_BAND);
    s->refclk    = rd(PHY_OFF + PHY_REFCLK);
    s->div       = rd(PHY_OFF + PHY_DIV);
    s->frac      = rd(PHY_OFF + PHY_FRAC);

    vco = ((UQUAD)(s->rm_offset & ~RM_OFFSET_ONLY) * 54000000) >> 21;
    s->khz = ((s->div & PHY_DIV_MASK) >> PHY_DIV_SHIFT)
           ? (ULONG)(vco / (((s->div & PHY_DIV_MASK) >> PHY_DIV_SHIFT) * 10) / 1000) : 0;
    s->hz  = (htot && vtot) ? (s->khz * 1000 + htot * vtot / 2) / (htot * vtot) : 0;
    s->hsm = clock_tag(VCTAG_GETCLKMEASURED, CLK_HSM, 0);
    s->pixel = clock_tag(VCTAG_GETCLKMEASURED, CLK_PIXEL, 0);
    s->pixdiv = (rd(CM_PIX_DIV) >> 12) & 0xfff;

    s->hvs_ctrl  = rd(HVS_DISPCTRL(hvs_ch));
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

static void state_build(struct state *s, const struct state *fw, const struct mode *m)
{
    ULONG total = (m->hact + m->hfp + m->hsync + m->hbp)
                * (m->vact + m->vfp + m->vsync + m->vbp);
    BOOL high = (m->khz > HIGH_BAND_KHZ), top = (m->khz > TOP_BAND_KHZ);
    ULONG div, offset;
    UQUAD vco;

    pll_params(m->khz, &div, &offset, &vco);

    s->khz       = m->khz;
    s->hz        = (m->khz * 1000 + total / 2) / total;
    s->hsm       = (m->khz * 1010 > HSM_MIN) ? m->khz * 1010 : HSM_MIN;
    s->pixdiv    = pix_src / (m->khz * 500);
    if (s->pixdiv > CM_PIX_DIV_MAX)
        s->pixdiv = CM_PIX_DIV_MAX;
    s->pixel     = s->pixdiv ? pix_src / s->pixdiv : 0;
    s->hvs_ctrl  = (fw->hvs_ctrl & ~HVS_SIZE) | (m->hact << 16) | m->vact;
    s->pv_ctl    = fw->pv_ctl;
    s->pv_vctl   = fw->pv_vctl;
    s->pv_horza  = ((m->hbp / 2) << 16) | (m->hsync / 2);
    s->pv_horzb  = ((m->hfp / 2) << 16) | (m->hact / 2);
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

    s->rm_offset = RM_OFFSET_ONLY | offset;
    s->frac      = offset >> 4;
    s->div       = (fw->div & ~PHY_DIV_MASK) | (div << PHY_DIV_SHIFT);
    s->band      = (fw->band & ~(PHY_BAND_VCO_MASK | PHY_BAND_HIGH | PHY_BAND_TOP_MASK))
                 | ((vco_band ? vco_band : (vco < VCO_BAND5) ? 6UL : 5UL) << PHY_BAND_VCO_SHIFT)
                 | (high ? PHY_BAND_HIGH : 0) | (top ? PHY_BAND_TOP : 3);
    s->refclk    = high ? (fw->refclk | PHY_REFCLK_HIGH) : (fw->refclk & ~PHY_REFCLK_HIGH);
    s->lanes     = top ? LANES_TOP : high ? LANES_HIGH : LANES_LOW;
}

static void state_print(const char *name, const struct state *s)
{
    P("  %s: %u kHz, %u Hz, HSM %u, pixel %u (/%u), HVS DISPCTRL%u %08x\n"
      "    PV CONTROL %08x V_CONTROL %08x HORZA %08x HORZB %08x VERTA %08x VERTB %08x\n"
      "    HDMI HORZA %08x HORZB %08x VERTA %08x/%08x VERTB %08x/%08x VID_CTL %08x\n"
      "    RM OFFSET %08x, PHY lanes %08x band %08x refclk %08x div %08x frac %08x\n", name,
      (unsigned)s->khz, (unsigned)s->hz, (unsigned)s->hsm, (unsigned)s->pixel,
      (unsigned)s->pixdiv, (unsigned)hvs_ch,
      (unsigned)s->hvs_ctrl, (unsigned)s->pv_ctl, (unsigned)s->pv_vctl,
      (unsigned)s->pv_horza, (unsigned)s->pv_horzb, (unsigned)s->pv_verta,
      (unsigned)s->pv_vertb, (unsigned)s->hd_horza, (unsigned)s->hd_horzb,
      (unsigned)s->hd_verta0, (unsigned)s->hd_verta1, (unsigned)s->hd_vertb0,
      (unsigned)s->hd_vertb1, (unsigned)s->vid_ctl, (unsigned)s->rm_offset,
      (unsigned)s->lanes, (unsigned)s->band, (unsigned)s->refclk, (unsigned)s->div,
      (unsigned)s->frac);
}

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

/* Stop the clock, change the divider, start it again; PV2 is off. */
static void pixel_div_set(ULONG divi)
{
    ULONG ctl = rd(CM_PIX_CTL) & 0xffffff, i;

    if (((rd(CM_PIX_DIV) >> 12) & 0xfff) == divi)
        return;
    wr(CM_PIX_CTL, CM_PASSWD | (ctl & ~CM_CTL_ENAB));
    for (i = 0; i < 100000 && (rd(CM_PIX_CTL) & CM_CTL_BUSY); i++)
        ;
    if (i == 100000)
        P("  clock 9 did not stop\n");
    wr(CM_PIX_DIV, CM_PASSWD | (divi << 12));
    wr(CM_PIX_CTL, CM_PASSWD | ctl | CM_CTL_ENAB);
    P("  clock 9 divider now /%u, measured %u\n", (unsigned)((rd(CM_PIX_DIV) >> 12) & 0xfff),
      (unsigned)clock_tag(VCTAG_GETCLKMEASURED, CLK_PIXEL, 0));
}

/* Stop the pixelvalve and the HVS channel, retune, then bring HVS,
 * HDMI and pixelvalve up again - BCM2712's order. */
static void mode_set(const struct state *s)
{
    ULONG ctl, i;

    wr(PV_OFF + PV_V_CONTROL, rd(PV_OFF + PV_V_CONTROL) & ~PV_VC_VIDEN);
    for (i = 0; i < 10 && (rd(PV_OFF + PV_V_CONTROL) & PV_VC_VIDEN); i++)
        Delay(1);
    if (i == 10)
        P("  VIDEN did not read back clear\n");
    Delay(2);

    ctl = rd(PV_OFF + PV_CONTROL) & ~PV_CONTROL_EN;
    wr(PV_OFF + PV_CONTROL, ctl);
    wr(PV_OFF + PV_CONTROL, ctl | PV_CONTROL_FIFO_CLR);

    wr(HVS_DISPCTRL(hvs_ch), rd(HVS_DISPCTRL(hvs_ch)) & ~HVS_ENB);
    Delay(1);
    wr(HVS_DISPCTRL(hvs_ch), s->hvs_ctrl);

    if (clock_tag(VCTAG_GETCLKMEASURED, CLK_HSM, 0) < s->hsm)
        clock_set(CLK_HSM, s->hsm);
    pixel_div_set(s->pixdiv);

    wr(RM_OFF + RM_OFFSET, s->rm_offset);
    wr(PHY_OFF + PHY_FRAC, s->frac);
    wr(PHY_OFF + PHY_DIV, s->div);
    wr(PHY_OFF + PHY_BAND, s->band);
    wr(PHY_OFF + PHY_REFCLK, s->refclk);
    wr(PHY_OFF + PHY_LANES, s->lanes);
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

    /* Down only once the slower clock is in. */
    if (clock_tag(VCTAG_GETCLKMEASURED, CLK_HSM, 0) > s->hsm + s->hsm / 50)
        clock_set(CLK_HSM, s->hsm);

    P("  PHY now: band %08x refclk %08x div %08x frac %08x, RM OFFSET %08x\n",
      (unsigned)rd(PHY_OFF + PHY_BAND), (unsigned)rd(PHY_OFF + PHY_REFCLK),
      (unsigned)rd(PHY_OFF + PHY_DIV), (unsigned)rd(PHY_OFF + PHY_FRAC),
      (unsigned)rd(RM_OFF + RM_OFFSET));
}

#define TEMPLATE "MODE/K,KHZ/K/N,VCOBAND/K/N,CPRMAN/S,LIST/S,HOLD/N,GO/S"

enum { ARG_MODE, ARG_KHZ, ARG_VCOBAND, ARG_CPRMAN, ARG_LIST, ARG_HOLD, ARG_GO, ARG_COUNT };

int main(void)
{
    APTR KernelBase = OpenResource("kernel.resource");
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    const struct mode *m = &modes[MODES - 1];
    struct state fw, ours;
    struct mode tm;
    ULONG hold, good = 0, i, hact, vact;
    APTR mbbuf = NULL;
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "modetest2711");
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
        /* WxH picks the first rate listed, WxH@Hz that one. */
        for (i = 0; i < MODES; i++)
            if (!strncasecmp(modes[i].name, (char *)args[ARG_MODE], strlen((char *)args[ARG_MODE]))
                && ((modes[i].name[strlen((char *)args[ARG_MODE])] == '@')
                    || !modes[i].name[strlen((char *)args[ARG_MODE])]))
                break;
        if (i == MODES)
        {
            P("modetest2711: no mode %s - try LIST\n", (char *)args[ARG_MODE]);
            goto out;
        }
        m = &modes[i];
    }
    if (args[ARG_KHZ])
    {
        tm = *m;
        tm.khz = *(LONG *)args[ARG_KHZ];
        m = &tm;
    }
    if (args[ARG_VCOBAND])
        vco_band = *(LONG *)args[ARG_VCOBAND] & 0xf;
    hold = args[ARG_HOLD] ? *(LONG *)args[ARG_HOLD] : 10;

    periiobase = KernelBase ? (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) : 0;
    if (periiobase != BCM2711_PERIIOBASE)
    {
        P("modetest2711: not a BCM2711 - refusing\n");
        goto out;
    }
    if (!(MBoxBase = OpenResource("mbox.resource"))
        || !(mbbuf = AllocMem(MBOX_MSG_ALIGN + 64, MEMF_31BIT | MEMF_CLEAR)))
        goto out;
    mb = (ULONG *)(((IPTR)mbbuf + MBOX_MSG_ALIGN - 1) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    /* Read-only, so before the boot state is judged. */
    clock_info();
    if (args[ARG_CPRMAN])
        cm_scan();

    /* The boot state this is written against: PV2 running HDMI0 at 2
     * pixels/clock, fed by the one HVS channel of its size. */
    hact = (rd(PV_OFF + PV_HORZB) & 0xffff) * 2;
    vact = rd(PV_OFF + PV_VERTB) & 0xffff;
    for (hvs_ch = 0; hvs_ch < HVS_CHANNELS; hvs_ch++)
        if ((rd(HVS_DISPCTRL(hvs_ch)) & HVS_ENB)
            && ((rd(HVS_DISPCTRL(hvs_ch)) & HVS_SIZE) == ((hact << 16) | vact)))
            break;
    if (!(rd(PV_OFF + PV_CONTROL) & PV_CONTROL_EN) || !(rd(PV_OFF + PV_V_CONTROL) & PV_VC_VIDEN)
        || (rd(PV_OFF + PV_V_CONTROL) & PV_VC_ODD_TIMING) || (hvs_ch == HVS_CHANNELS)
        || !(rd(PHY_OFF + PHY_DIV) & PHY_DIV_MASK))
    {
        P("modetest2711: not the boot state this was written for - refusing\n"
          "  PV2 CONTROL %08x V_CONTROL %08x (%ux%u), PHY div %08x, HVS DISPCTRL %08x %08x %08x\n",
          (unsigned)rd(PV_OFF + PV_CONTROL), (unsigned)rd(PV_OFF + PV_V_CONTROL),
          (unsigned)hact, (unsigned)vact, (unsigned)rd(PHY_OFF + PHY_DIV),
          (unsigned)rd(HVS_DISPCTRL(0)), (unsigned)rd(HVS_DISPCTRL(1)),
          (unsigned)rd(HVS_DISPCTRL(2)));
        goto out;
    }

    /* PV2's clock must be the integer divider off PLLC_PER it was
     * measured as. */
    pix_src = clock_tag(VCTAG_GETCLKMEASURED, CLK_PIXEL, 0) * ((rd(CM_PIX_DIV) >> 12) & 0xfff);
    if (((rd(CM_PIX_CTL) & 0xf) != CM_PIX_SRC) || !(rd(CM_PIX_CTL) & CM_CTL_ENAB)
        || (rd(CM_PIX_DIV) & 0xfff) || (pix_src < 600000000) || (pix_src > 700000000))
    {
        P("modetest2711: clock 9 is not the divider this expects - refusing\n"
          "  CM CTL %08x DIV %08x, source %u Hz\n", (unsigned)rd(CM_PIX_CTL),
          (unsigned)rd(CM_PIX_DIV), (unsigned)pix_src);
        goto out;
    }

    state_read(&fw);
    state_build(&ours, &fw, m);
    if (!ours.pixdiv)
    {
        P("modetest2711: %s is faster than clock 9 can follow\n", m->name);
        goto out;
    }
    P("modetest2711: %s on HVS channel %u\n", m->name, (unsigned)hvs_ch);
    state_print("firmware", &fw);
    state_print("planned ", &ours);

    if (!args[ARG_GO])
    {
        P("modetest2711: dry run - add GO to write\n");
        rc = RETURN_OK;
        goto out;
    }

    measure("before", fw.hz);
    P("modetest2711: switching to %s\n", m->name);
    mode_set(&ours);
    good += measure("after", ours.hz);
    Delay(hold * 50);

    P("modetest2711: back to the boot mode\n");
    mode_set(&fw);
    good += measure("restored", fw.hz);

    P("modetest2711: done, %u of 2 steps ok\n", (unsigned)good);
    rc = (good == 2) ? RETURN_OK : RETURN_WARN;

out:
    if (mbbuf)
        FreeMem(mbbuf, MBOX_MSG_ALIGN + 64);
    FreeArgs(rda);
    return rc;
}
