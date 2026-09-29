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

    A mode larger than the framebuffer shows it top-left with a border.
    Modes from 340 MHz up are scrambled (DISPLAY-SPEC 7.3): the sink is
    set up over SCDC first, and must report scrambler lock. SPECCTL runs
    our modes with the pixelvalve CONTROL bits of 6.2 instead of the
    firmware's.

    CSC=full|limited replaces the firmware's colour path while the test
    mode runs: the CSC loaded with identity or 219/255 plus 16, the output
    crossbar set as for RGB (XBAR=fw keeps the firmware's), and an AVI
    infoframe saying so, with the mode's VIC where it has one.

    Usage: modetest [MODE=<WxH[@Hz]>] [LIST] [HOLD=<s>] [LOOP=<n>] [FRAME]
                    [SPECCTL] [CSC=full|limited] [XBAR=fw] [GO]
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

/* CEA-861 and VESA DMT timings, clock in kHz. The last one is the
 * native mode of the ASUS VG27AQL5A this was tested on, from its EDID. */
struct mode
{
    const char *name;
    ULONG khz;
    ULONG hact, hfp, hsync, hbp;
    ULONG vact, vfp, vsync, vbp;
    BOOL  hneg, vneg;
    UBYTE vic, aspect;          /* CEA format, AVI M: 1 = 4:3, 2 = 16:9 */
};

static const struct mode modes[] =
{
    { "640x480@60",    25175,  640,  16,  96,  48,  480, 10, 2, 33, TRUE,  TRUE,   1, 1 },
    { "800x600@60",    40000,  800,  40, 128,  88,  600,  1, 4, 23, FALSE, FALSE },
    { "1024x768@60",   65000, 1024,  24, 136, 160,  768,  3, 6, 29, TRUE,  TRUE  },
    { "1280x720@60",   74250, 1280, 110,  40, 220,  720,  5, 5, 20, FALSE, FALSE,  4, 2 },
    { "1280x720@50",   74250, 1280, 440,  40, 220,  720,  5, 5, 20, FALSE, FALSE, 19, 2 },
    { "1280x800@60",   83500, 1280,  72, 128, 200,  800,  3, 6, 22, TRUE,  FALSE },
    { "1440x900@60",  106500, 1440,  80, 152, 232,  900,  3, 6, 25, TRUE,  FALSE },
    { "1280x1024@60", 108000, 1280,  48, 112, 248, 1024,  1, 3, 38, FALSE, FALSE },
    { "1600x900@60",  108000, 1600,  24,  80,  96,  900,  1, 3, 96, FALSE, FALSE },
    { "1680x1050@60", 146250, 1680, 104, 176, 280, 1050,  3, 6, 30, TRUE,  FALSE },
    { "1920x1080@60", 148500, 1920,  88,  44, 148, 1080,  4, 5, 36, FALSE, FALSE, 16, 2 },
    { "2560x1440@60", 241500, 2560,  48,  32,  80, 1440,  3, 5, 33, FALSE, TRUE  },
    { "2560x1440@120", 497750, 2560, 48,  32,  80, 1440,  3, 5, 77, FALSE, TRUE  },
    { "2560x1440@144", 593700, 2560,  8,  32,  72, 1440, 25, 8, 70, FALSE, TRUE  },
};
#define MODES                   (sizeof(modes) / sizeof(modes[0]))

/* Captured by dispdump on a Pi 500+ (D0), HDMI0; DISPLAY-SPEC.md 15. */
static const ULONG pll_misc[9] =
{
    0x810c6000, 0x00b8c451, 0x46402e31, 0x00b8c005, 0x42410261,
    0xcc021001, 0xc8301c80, 0xb0804444, 0xf80f8000,
};
/* CTL_0/1/2/CK by TMDS band, read back from the hardware; spec 15. */
static const ULONG lanes_low[4]  = { 0x80828700, 0x80828700, 0x80828700, 0x80828700 };
static const ULONG lanes_mid[4]  = { 0xc0870000, 0xc0870000, 0xc0870000, 0xc0870800 };
static const ULONG lanes_high[4] = { 0x848f8700, 0x848f8700, 0x848f8700, 0x849f8f00 };
#define SCRAMBLE_MIN            340000          /* kHz */
#define TMDS_MAX                600000          /* kHz */

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
#define HDMI_SCRAMBLER_CTL      0x1e4

/* HDMI0's CSC and output crossbar (spec 7.2), and the AVI infoframe's
 * packet RAM slot (spec 15). */
#define CSC_OFF                 0x700100
#define CSC_CTL                 0x000
#define CSC_COEF(i)             (0x004 + 4 * (i))
#define CSC_CHANNEL_CTL         0x02c
#define CSC_CTL_ON              0x07        /* as wherever a matrix is loaded */
#define DVP_XBAR                (0x701000 + 0x0f4)
#define XBAR_RGB                0x00354021
#define HDMI_RAM_PACKET_CONFIG  0x0c4
#define HDMI_RAM_PACKET_STATUS  0x0cc
#define PKT_AVI_SLOT            2
#define PKT_AVI                 (0x703800 + PKT_AVI_SLOT * 0x24)

/* Identity, and 219/255 plus 16 - read back from the hardware running
 * full and limited range RGB. s2.13, two per register. */
static const ULONG csc_full[6] =
    { 0x00002000, 0x00000000, 0x20000000, 0x00000000, 0x00000000, 0x00002000 };
static const ULONG csc_limited[6] =
    { 0x00001b80, 0x04000000, 0x1b800000, 0x04000000, 0x00000000, 0x04001b80 };

/* The colour path: what the firmware left, or what a test mode sets. */
struct colour
{
    ULONG ctl, coef[6], chan, xbar, avi[9];
    BOOL  avi_on;
};
#define SCRAMBLER_ENABLE        (1 << 0)

/* HDMI0's DDC controller (Broadcom STB I2C, spec 8) and the SCDC
 * registers of the sink behind it (slave 0x54). */
#define DDC_OFF                 0x1508200
#define BSC_CHIP_ADDRESS        0x00
#define BSC_DATA_IN(i)          (0x04 + 4 * (i))
#define BSC_CNT                 0x24
#define BSC_CTL                 0x28
#define BSC_IIC_ENABLE          0x2c
#define BSC_DATA_OUT(i)         (0x30 + 4 * (i))
#define BSC_CTLHI               0x50
#define BSC_CTL_97K5            0x90
#define BSC_EN_ENABLE           (1 << 0)
#define BSC_EN_INTRP            (1 << 1)
#define BSC_EN_NOACK            (1 << 2)
#define BSC_CTLHI_DATAREG_32    (1 << 6)
#define SCDC_ADDR               (0x54 << 1)
#define SCDC_SINK_VERSION       0x01
#define SCDC_SOURCE_VERSION     0x02
#define SCDC_TMDS_CONFIG        0x20            /* 1 = clock ratio 1/40, 0 = scramble */
#define SCDC_SCRAMBLER_STATUS   0x21
#define SCDC_STATUS_FLAGS       0x40            /* clock detected, ch0-2 locked */
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
    ULONG khz, hz;
    ULONG hvs_ctrl0, lptrs;
    ULONG pv_ctl, pv_vctl, pv_horza, pv_horzb, pv_verta, pv_vertb;
    ULONG hd_horza, hd_horzb, hd_verta0, hd_vertb0, hd_verta1, hd_vertb1;
    ULONG vid_ctl;
    BOOL  scramble;
};

static BOOL scrambled;          /* what the sink was last set up for */

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

/* The mode's frame rate +- 2 and its pixel clock within 0.5%. */
static BOOL measure(const char *when, ULONG khz, ULONG hz)
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
    ok  = (fps + 2 >= hz) && (fps <= hz + 2) && (err <= khz * 5);

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
    s->hz        = 60;          /* the boot mode checked for below */
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
    s->scramble  = FALSE;
}

/* Register values for a mode: pixelvalve at 1 pixel/clock with
 * ODD_TIMING (6.1, 6.2), HDMI timings unhalved (7.1), rep = 1. */
static void state_build(struct state *s, const struct state *fw,
                        const struct mode *m, ULONG lptrs, BOOL specctl)
{
    ULONG total = (m->hact + m->hfp + m->hsync + m->hbp)
                * (m->vact + m->vfp + m->vsync + m->vbp);

    s->khz       = m->khz;
    s->hz        = (m->khz * 1000 + total / 2) / total;
    s->scramble  = (m->khz >= SCRAMBLE_MIN);
    s->hvs_ctrl0 = (fw->hvs_ctrl0 & ~HVS_CTRL0_SIZE)
                 | ((m->hact - 1) << 16) | (m->vact - 1);
    s->lptrs     = lptrs;
    /* 6.2: CLR_AT_START | TRIGGER_UNDERFLOW | WAIT_HSTART, the firmware's
     * FIFO level without FIFO_LEVEL_HIGH. */
    s->pv_ctl    = specctl ? ((fw->pv_ctl & (0x3f << 15)) | (7 << 12) | PV_CONTROL_EN)
                           : fw->pv_ctl;
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
    const ULONG *lanes;
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

    lanes = (khz < 222000) ? lanes_low : (khz <= 297000) ? lanes_mid : lanes_high;
    for (i = 0; i < 4; i++)
        wr(PHY_OFF + PHY_CTL(i), lanes[i]);
    wr(PHY_OFF + PHY_TMDS_CLK_WORD_SEL, (khz >= 340000) ? 3 : 0);
    wr(PHY_OFF + PHY_POWERUP_CTL, 0x1cf);

    wr(PHY_OFF + PHY_PLL_POWERUP_CTL, 1);
    wr(PHY_OFF + PHY_PLL_RESET_CTL, rd(PHY_OFF + PHY_PLL_RESET_CTL) & ~1);
    wr(PHY_OFF + PHY_PLL_RESET_CTL, rd(PHY_OFF + PHY_PLL_RESET_CTL) | 1);
}

static inline ULONG now_us(void)
{
    return rd(0x3004);          /* the 1 MHz system timer */
}

/* One polled START..STOP transaction on HDMI0's DDC bus. */
static BOOL ddc_xfer(UBYTE addr8, BOOL read, UBYTE *buf, ULONG len)
{
    ULONG w[8] = { 0 }, en = 0, i, n;

    wr(DDC_OFF + BSC_IIC_ENABLE, 0);
    wr(DDC_OFF + BSC_CHIP_ADDRESS, addr8 | (read ? 1 : 0));
    wr(DDC_OFF + BSC_CNT, len);
    wr(DDC_OFF + BSC_CTL, BSC_CTL_97K5 | (read ? 1 : 0));
    if (!read)
    {
        for (i = 0; i < len; i++)
            w[i >> 2] |= (ULONG)buf[i] << ((i & 3) * 8);
        for (i = 0; i < 8; i++)
            wr(DDC_OFF + BSC_DATA_IN(i), w[i]);
    }
    wr(DDC_OFF + BSC_IIC_ENABLE, BSC_EN_ENABLE);
    for (n = 0; n < 2000000; n++)
        if ((en = rd(DDC_OFF + BSC_IIC_ENABLE)) & BSC_EN_INTRP)
            break;
    wr(DDC_OFF + BSC_IIC_ENABLE, 0);
    if ((n == 2000000) || (en & BSC_EN_NOACK))
        return FALSE;
    if (read)
        for (i = 0; i < len; i++)
            buf[i] = rd(DDC_OFF + BSC_DATA_OUT(i >> 2)) >> ((i & 3) * 8);
    return TRUE;
}

/* SCDC register access. The firmware uses this controller too, so its
 * set-up is put back afterwards. */
static BOOL scdc(UBYTE reg, BOOL read, UBYTE *val)
{
    ULONG ctl = rd(DDC_OFF + BSC_CTL), ctlhi = rd(DDC_OFF + BSC_CTLHI);
    ULONG addr = rd(DDC_OFF + BSC_CHIP_ADDRESS);
    UBYTE b[2] = { reg, read ? 0 : *val };
    BOOL ok;

    wr(DDC_OFF + BSC_CTLHI, ctlhi | BSC_CTLHI_DATAREG_32);
    if (read)
        ok = ddc_xfer(SCDC_ADDR, FALSE, b, 1) && ddc_xfer(SCDC_ADDR, TRUE, val, 1);
    else
        ok = ddc_xfer(SCDC_ADDR, FALSE, b, 2);
    wr(DDC_OFF + BSC_CTL, ctl);
    wr(DDC_OFF + BSC_CTLHI, ctlhi);
    wr(DDC_OFF + BSC_CHIP_ADDRESS, addr);
    return ok;
}

static BOOL scdc_rd(UBYTE reg, UBYTE *val)
{
    return scdc(reg, TRUE, val);
}

static BOOL scdc_wr(UBYTE reg, UBYTE val)
{
    return scdc(reg, FALSE, &val);
}

/* The sink half of 7.3, before the fast clock starts. */
static void scramble_sink(BOOL on)
{
    if (on)
        scdc_wr(SCDC_SOURCE_VERSION, 1);
    if (!scdc_wr(SCDC_TMDS_CONFIG, on ? 3 : 0))
        P("  SCDC: TMDS_Config write not acknowledged\n");
}

/* Wait for the sink to report scrambled input, up to 250 ms. */
static void scramble_wait(void)
{
    ULONG start = now_us(), t = 0;
    UBYTE st = 0, flags = 0;

    while ((t = now_us() - start) < 250000)
        if (scdc_rd(SCDC_SCRAMBLER_STATUS, &st) && (st & 1))
            break;
    scdc_rd(SCDC_STATUS_FLAGS, &flags);
    P("  SCDC: scrambler %s after %u us, status flags %02x\n",
      (st & 1) ? "locked" : "NOT locked", (unsigned)t, flags);
}

static void colour_read(struct colour *c)
{
    ULONG i;

    c->ctl  = rd(CSC_OFF + CSC_CTL);
    for (i = 0; i < 6; i++)
        c->coef[i] = rd(CSC_OFF + CSC_COEF(i));
    c->chan = rd(CSC_OFF + CSC_CHANNEL_CTL);
    c->xbar = rd(DVP_XBAR);
    for (i = 0; i < 9; i++)
        c->avi[i] = rd(PKT_AVI + 4 * i);
    c->avi_on = (rd(HDMI_OFF + HDMI_RAM_PACKET_CONFIG) & (1UL << PKT_AVI_SLOT)) != 0;
}

/* A packet RAM slot is locked while its enable bit is set. */
static void colour_write(const struct colour *c)
{
    ULONG i, start;

    wr(CSC_OFF + CSC_CTL, c->ctl);
    for (i = 0; i < 6; i++)
        wr(CSC_OFF + CSC_COEF(i), c->coef[i]);
    wr(CSC_OFF + CSC_CHANNEL_CTL, c->chan);
    wr(DVP_XBAR, c->xbar);

    wr(HDMI_OFF + HDMI_RAM_PACKET_CONFIG,
       rd(HDMI_OFF + HDMI_RAM_PACKET_CONFIG) & ~(1UL << PKT_AVI_SLOT));
    start = now_us();
    while ((rd(HDMI_OFF + HDMI_RAM_PACKET_STATUS) & (1UL << PKT_AVI_SLOT))
           && ((now_us() - start) < 100000))
        ;
    if (rd(HDMI_OFF + HDMI_RAM_PACKET_STATUS) & (1UL << PKT_AVI_SLOT))
        P("  AVI slot did not unlock\n");
    for (i = 0; i < 9; i++)
        wr(PKT_AVI + 4 * i, c->avi[i]);
    if (c->avi_on)
        wr(HDMI_OFF + HDMI_RAM_PACKET_CONFIG,
           rd(HDMI_OFF + HDMI_RAM_PACKET_CONFIG) | (1UL << PKT_AVI_SLOT));
}

/* RGB at the range asked for, and an AVI infoframe saying so: Q and YQ
 * from the range, VIC and picture aspect from the mode. */
static void colour_build(struct colour *c, const struct colour *fw, const struct mode *m,
                         BOOL limited, BOOL fwxbar)
{
    UBYTE hb[3] = { 0x82, 0x02, 0x0d }, pb[14] = { 0 };
    ULONG i, sum;

    c->ctl = CSC_CTL_ON;
    for (i = 0; i < 6; i++)
        c->coef[i] = limited ? csc_limited[i] : csc_full[i];
    c->chan = 0;
    c->xbar = fwxbar ? fw->xbar : XBAR_RGB;

    pb[1] = 0x12;                               /* RGB, active format, underscan */
    pb[2] = (m->aspect << 4) | 0x08;            /* aspect, same as picture */
    pb[3] = (limited ? 1 : 2) << 2;             /* Q */
    pb[4] = m->vic;
    pb[5] = limited ? 0x00 : 0x40;              /* YQ */
    for (sum = hb[0] + hb[1] + hb[2], i = 1; i < 14; i++)
        sum += pb[i];
    pb[0] = (0x100 - (sum & 0xff)) & 0xff;

    c->avi[0] = hb[0] | (hb[1] << 8) | (hb[2] << 16);
    c->avi[1] = pb[0] | (pb[1] << 8) | (pb[2] << 16) | ((ULONG)pb[3] << 24);
    c->avi[2] = pb[4] | (pb[5] << 8) | (pb[6] << 16);
    c->avi[3] = pb[7] | (pb[8] << 8) | (pb[9] << 16) | ((ULONG)pb[10] << 24);
    c->avi[4] = pb[11] | (pb[12] << 8) | (pb[13] << 16);
    for (i = 5; i < 9; i++)
        c->avi[i] = 0;
    c->avi_on = TRUE;
}

static void colour_print(const char *name, const struct colour *c)
{
    P("  %s: CSC_CTL %02x coef %08x %08x %08x %08x %08x %08x XBAR %06x\n"
      "    AVI %s %08x %08x %08x\n", name, (unsigned)c->ctl, (unsigned)c->coef[0],
      (unsigned)c->coef[1], (unsigned)c->coef[2], (unsigned)c->coef[3], (unsigned)c->coef[4],
      (unsigned)c->coef[5], (unsigned)c->xbar, c->avi_on ? "on " : "off",
      (unsigned)c->avi[0], (unsigned)c->avi[1], (unsigned)c->avi[2]);
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

    /* Scrambling off on both sides while nothing is sent; the sink
     * gets its new setting before the new clock starts. */
    if (scrambled && !s->scramble)
    {
        wr(HDMI_OFF + HDMI_SCRAMBLER_CTL, rd(HDMI_OFF + HDMI_SCRAMBLER_CTL) & ~SCRAMBLER_ENABLE);
        scramble_sink(FALSE);
    }
    else if (s->scramble)
        scramble_sink(TRUE);
    scrambled = s->scramble;

    phy_init(s->khz);
    Delay(1);       /* PLL lock */

    wr(HDMI_OFF + HDMI_HORZA, s->hd_horza);
    wr(HDMI_OFF + HDMI_HORZB, s->hd_horzb);
    wr(HDMI_OFF + HDMI_VERTA0, s->hd_verta0);
    wr(HDMI_OFF + HDMI_VERTB0, s->hd_vertb0);
    wr(HDMI_OFF + HDMI_VERTA1, s->hd_verta1);
    wr(HDMI_OFF + HDMI_VERTB1, s->hd_vertb1);
    wr(HD_VID_CTL, s->vid_ctl);
    if (s->scramble)
        wr(HDMI_OFF + HDMI_SCRAMBLER_CTL, rd(HDMI_OFF + HDMI_SCRAMBLER_CTL) | SCRAMBLER_ENABLE);

    wr(PV_OFF + PV_HORZA, s->pv_horza);
    wr(PV_OFF + PV_HORZB, s->pv_horzb);
    wr(PV_OFF + PV_VERTA, s->pv_verta);
    wr(PV_OFF + PV_VERTB, s->pv_vertb);
    wr(PV_OFF + PV_CONTROL, (s->pv_ctl & ~PV_CONTROL_EN) | PV_CONTROL_FIFO_CLR);
    wr(PV_OFF + PV_CONTROL, s->pv_ctl);
    wr(PV_OFF + PV_V_CONTROL, s->pv_vctl);

    recenter();
    if (s->scramble)
        scramble_wait();
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
static void build_list(ULONG ours, ULONG live, ULONG w, ULONG h)
{
    ULONG i;

    for (i = 0; i < SLOT_WORDS; i++)
        wr(HVS_DLIST + (ours + i) * 4, rd(HVS_DLIST + (live + i) * 4));
    wr(HVS_DLIST + (ours + ENT_POS0) * 4, 0);
    wr(HVS_DLIST + (ours + ENT_POS2) * 4, ((h - 1) << 16) | (w - 1));
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

#define TEMPLATE "MODE/K,LIST/S,HOLD/N,LOOP/N,FRAME/S,SPECCTL/S,CSC/K,XBAR/K,GO/S"

enum { ARG_MODE, ARG_LIST, ARG_HOLD, ARG_LOOP, ARG_FRAME, ARG_SPECCTL, ARG_CSC, ARG_XBAR, ARG_GO,
       ARG_COUNT };

int main(void)
{
    APTR KernelBase = OpenResource("kernel.resource");
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    const struct mode *m = &modes[3];
    struct state fw, ours;
    struct colour fwc, ourc;
    ULONG csc = 0;
    ULONG hold, loops = 1, good = 0, steps = 0, slots = 0, pos2, vco_div, rm_offset, i;
    ULONG plane_w, plane_h;
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
        /* WxH picks the first rate listed, WxH@Hz that one. */
        for (i = 0; i < MODES; i++)
            if (!strncasecmp(modes[i].name, (char *)args[ARG_MODE], strlen((char *)args[ARG_MODE]))
                && ((modes[i].name[strlen((char *)args[ARG_MODE])] == '@')
                    || !modes[i].name[strlen((char *)args[ARG_MODE])]))
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
        || (rd(HVS_DLIST + fw.lptrs * 4) & CTL0_END))
    {
        P("modetest: not the boot state this was written for - refusing\n");
        state_print("live", &fw);
        goto out;
    }
    if ((m->khz > TMDS_MAX) || !pll_params(m->khz, &vco_div, &rm_offset))
    {
        P("modetest: %s is outside what the PHY settings cover\n", m->name);
        goto out;
    }
    if (m->khz >= SCRAMBLE_MIN)
    {
        UBYTE ver = 0, cfg = 0;

        if (!scdc_rd(SCDC_SINK_VERSION, &ver))
        {
            P("modetest: %s needs scrambling, and the sink does not answer SCDC\n", m->name);
            goto out;
        }
        scdc_rd(SCDC_TMDS_CONFIG, &cfg);
        P("modetest: scrambled mode - SCDC sink version %u, TMDS_Config %02x\n", ver, cfg);
    }

    /* The framebuffer plane shows as much of the mode as it covers. */
    plane_w = (pos2 & 0xffff) + 1;
    plane_h = (pos2 >> 16) + 1;
    if (plane_w > m->hact)
        plane_w = m->hact;
    if (plane_h > m->vact)
        plane_h = m->vact;
    if (!find_slots(&slots))
    {
        P("modetest: no free display list slots\n");
        goto out;
    }

    state_build(&ours, &fw, m, slots, args[ARG_SPECCTL] ? TRUE : FALSE);
    if (args[ARG_CSC])
    {
        csc = !strcasecmp((char *)args[ARG_CSC], "limited") ? 2
            : !strcasecmp((char *)args[ARG_CSC], "full") ? 1 : 0;
        if (!csc)
        {
            P("modetest: CSC is full or limited\n");
            goto out;
        }
        colour_read(&fwc);
        colour_build(&ourc, &fwc, m, csc == 2,
                     args[ARG_XBAR] && !strcasecmp((char *)args[ARG_XBAR], "fw"));
    }
    P("modetest: %s, vco_div %u, RM_OFFSET %08x, list at %#06x\n", m->name,
      (unsigned)vco_div, (unsigned)(RM_OFFSET_ONLY | rm_offset), (unsigned)slots);
    state_print("firmware", &fw);
    state_print("planned ", &ours);
    if (csc)
    {
        colour_print("firmware", &fwc);
        colour_print("planned ", &ourc);
    }

    if (!args[ARG_GO])
    {
        P("modetest: dry run - add GO to write\n");
        rc = RETURN_OK;
        goto out;
    }

    if (args[ARG_FRAME])
    {
        draw_frame(fw.lptrs, (pos2 & 0xffff) + 1, (pos2 >> 16) + 1);
        draw_frame(fw.lptrs, plane_w, plane_h);
    }
    measure("before", fw.khz, fw.hz);
    build_list(slots, fw.lptrs, plane_w, plane_h);

    for (i = 1; i <= loops; i++)
    {
        P("modetest: [%u/%u] switching to %s\n", (unsigned)i, (unsigned)loops, m->name);
        mode_set(&ours);
        good += measure("after", ours.khz, ours.hz);
        if (csc)
        {
            colour_write(&ourc);
            P("  colour path set: %s range\n", (csc == 2) ? "limited" : "full");
        }
        Delay(hold * 50);
        if (csc)
            colour_write(&fwc);

        P("modetest: [%u/%u] back to the boot mode\n", (unsigned)i, (unsigned)loops);
        mode_set(&fw);
        good += measure("restored", fw.khz, fw.hz);
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
