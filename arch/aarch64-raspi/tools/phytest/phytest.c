/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 HDMI PHY takeover test - step 4 of real modesetting.

    Stops the pixelvalve, runs the whole PHY init sequence of
    DISPLAY-SPEC.md 9.4 from our own arithmetic and the constants
    dispdump captured, recentres the transmitter FIFO and starts the
    pixelvalve again. Success is an unchanged picture with the pixel
    clock still measuring 148.5 MHz.

    Without GO nothing is written. GO refuses unless the computed PLL
    values and the captured constants match what the PHY holds now;
    FORCE writes anyway. SPEC releases the lanes with 0x7f as the spec
    says, rather than the firmware's 0x4f.

    Usage: phytest [PORT=<0|1>] [RATE=<kHz>] [GO] [SPEC] [FORCE]
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

static IPTR periiobase;

#define ARM_PERIIOBASE periiobase
#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#define P(fmt, args...) do { printf(fmt, ##args); bug(fmt, ##args); } while (0)

/* The mbox.resource stubs resolve the base from a symbol of this name. */
APTR MBoxBase;

#define VCMB_OFFSET_BCM2712     0x013880

/* Captured by dispdump on a Pi 500+ (D0), HDMI0 at 148.5 MHz; see
 * DISPLAY-SPEC.md section 15. */
static const ULONG pll_misc[9] =
{
    0x810c6000, 0x00b8c451, 0x46402e31, 0x00b8c005, 0x42410261,
    0xcc021001, 0xc8301c80, 0xb0804444, 0xf80f8000,
};
/* CTL_0, CTL_1, CTL_2, CTL_CK for TMDS below 222 MHz */
static const ULONG lane_ctl[4] =
{
    0x80828700, 0x80828700, 0x80828700, 0x80828700,
};
#define LANE_BAND_MAX           222000          /* kHz */

static const ULONG pv_off[2]   = { 0x410000, 0x411000 };
static const ULONG hdmi_off[2] = { 0x701400, 0x706400 };
static const ULONG phy_off[2]  = { 0x701d00, 0x706d00 };
static const ULONG rm_off[2]   = { 0x702000, 0x707000 };
#define HD_FRAME_COUNT          (0x720000 + 0x060)

#define PV_CONTROL              0x00
#define PV_V_CONTROL            0x04
#define PV_CONTROL_EN           (1 << 0)
#define PV_CONTROL_FIFO_CLR     (1 << 1)
#define PV_VC_VIDEN             (1 << 0)

#define HDMI_FIFO_CTL           0x07c
#define HDMI_HOTPLUG            0x1c8
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

#define REFCLK_SEL_CMOS         (1 << 13)
#define REFFRQ_54               54
#define POST_KDIV_BYPASS_EN     (1 << 4)
#define POST_KDIV_RUN           ((2 << 2) | 1)  /* CLK0_SEL 2, KDIV 1 */
#define VCODIV_EN               (1 << 10)
#define POWERUP_VIDEO           0x1cf           /* BG LDO BIAS TX_CK TX_2..0 */
#define PLL_PWRUP               (1 << 0)
#define PLL_RESETB              (1 << 0)
#define RESET_CTL_FW            0x4f
#define RESET_CTL_SPEC          0x7f

static IPTR pv, phy, rm, hdmi;
static ULONG *mb;

static inline ULONG rd(IPTR a)
{
    return *(volatile ULONG *)a;
}

static inline void wr(IPTR a, ULONG v)
{
    *(volatile ULONG *)a = v;
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

static void measure(const char *when)
{
    ULONG f0, f1;

    f0 = rd(periiobase + HD_FRAME_COUNT);
    Delay(50);
    f1 = rd(periiobase + HD_FRAME_COUNT);
    P("  %s: HDMI %u frames/s, pixel clock %u Hz, FIFO_CTL %08x\n", when,
      (unsigned)(f1 - f0), (unsigned)measured_pixel_clock(),
      (unsigned)rd(hdmi + HDMI_FIFO_CTL));
}

static void phy_print(const char *when)
{
    ULONG i;

    P("  %s: RESET %08x POWERUP %08x REFCLK %08x POST_KDIV %08x VCOCLK_DIV %08x\n"
      "      PLL_CFG %08x WORD_SEL %08x PLL_RESET %08x PLL_POWERUP %08x RM_OFFSET %08x\n"
      "      lanes", when,
      (unsigned)rd(phy + PHY_RESET_CTL), (unsigned)rd(phy + PHY_POWERUP_CTL),
      (unsigned)rd(phy + PHY_PLL_REFCLK), (unsigned)rd(phy + PHY_PLL_POST_KDIV),
      (unsigned)rd(phy + PHY_PLL_VCOCLK_DIV), (unsigned)rd(phy + PHY_PLL_CFG),
      (unsigned)rd(phy + PHY_TMDS_CLK_WORD_SEL), (unsigned)rd(phy + PHY_PLL_RESET_CTL),
      (unsigned)rd(phy + PHY_PLL_POWERUP_CTL), (unsigned)rd(rm + RM_OFFSET));
    for (i = 0; i < 4; i++)
        P(" %08x", (unsigned)rd(phy + PHY_CTL(i)));
    P("\n      misc ");
    for (i = 0; i < 9; i++)
        P(" %08x", (unsigned)rd(phy + PHY_PLL_MISC(i)));
    P("\n");
}

/* DISPLAY-SPEC 10.2 steps 1, 2 and 5. */
static void pv_stop(ULONG *ctl, ULONG *vctl)
{
    ULONG i;

    *ctl  = rd(pv + PV_CONTROL);
    *vctl = rd(pv + PV_V_CONTROL);

    wr(pv + PV_V_CONTROL, *vctl & ~PV_VC_VIDEN);
    for (i = 0; i < 10 && (rd(pv + PV_V_CONTROL) & PV_VC_VIDEN); i++)
        Delay(1);
    Delay(2);

    wr(pv + PV_CONTROL, *ctl & ~PV_CONTROL_EN);
    wr(pv + PV_CONTROL, (*ctl & ~PV_CONTROL_EN) | PV_CONTROL_FIFO_CLR);
}

static void pv_start(ULONG ctl, ULONG vctl)
{
    wr(pv + PV_CONTROL, (ctl & ~PV_CONTROL_EN) | PV_CONTROL_FIFO_CLR);
    wr(pv + PV_CONTROL, ctl);
    wr(pv + PV_V_CONTROL, vctl);
}

/* DISPLAY-SPEC 9.4, steps 1-16. */
static void phy_init(ULONG khz, ULONG vco_div, ULONG rm_offset, ULONG reset_ctl)
{
    ULONG i;

    wr(phy + PHY_RESET_CTL, 0);
    wr(phy + PHY_POWERUP_CTL, 0);
    wr(phy + PHY_PLL_POST_KDIV, POST_KDIV_BYPASS_EN);

    for (i = 0; i < 9; i++)
        wr(phy + PHY_PLL_MISC(i), pll_misc[i]);
    wr(phy + PHY_PLL_REFCLK, REFCLK_SEL_CMOS | REFFRQ_54);
    wr(phy + PHY_RESET_CTL, reset_ctl);

    wr(rm + RM_OFFSET, RM_OFFSET_ONLY | rm_offset);
    wr(phy + PHY_PLL_VCOCLK_DIV, VCODIV_EN | vco_div);
    wr(phy + PHY_PLL_CFG, 0);
    wr(phy + PHY_PLL_POST_KDIV, POST_KDIV_RUN);

    for (i = 0; i < 4; i++)
        wr(phy + PHY_CTL(i), lane_ctl[i]);
    wr(phy + PHY_TMDS_CLK_WORD_SEL, (khz >= 340000) ? 3 : 0);
    wr(phy + PHY_POWERUP_CTL, POWERUP_VIDEO);

    wr(phy + PHY_PLL_POWERUP_CTL, PLL_PWRUP);
    wr(phy + PHY_PLL_RESET_CTL, rd(phy + PHY_PLL_RESET_CTL) & ~PLL_RESETB);
    wr(phy + PHY_PLL_RESET_CTL, rd(phy + PHY_PLL_RESET_CTL) | PLL_RESETB);
}

/* DISPLAY-SPEC 10.1. */
static void recenter(void)
{
    ULONG drift = rd(hdmi + HDMI_FIFO_CTL) & 0xefff, i;

    wr(hdmi + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(hdmi + HDMI_FIFO_CTL, drift | FIFO_RECENTER);
    Delay(1);
    wr(hdmi + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(hdmi + HDMI_FIFO_CTL, drift | FIFO_RECENTER);

    for (i = 0; i < 50 && !(rd(hdmi + HDMI_FIFO_CTL) & FIFO_RECENTER_DONE); i++)
        Delay(1);
    if (i == 50)
        P("  RECENTER_DONE never came\n");
}

/* Count the differences between what we would write and what is live. */
static ULONG compare(ULONG vco_div, ULONG rm_offset)
{
    ULONG i, bad = 0, v;

    if ((v = rd(phy + PHY_PLL_VCOCLK_DIV)) != (VCODIV_EN | vco_div))
    {
        P("  VCOCLK_DIV live %08x, computed %08x\n", (unsigned)v,
          (unsigned)(VCODIV_EN | vco_div));
        bad++;
    }
    if ((v = rd(rm + RM_OFFSET)) != (RM_OFFSET_ONLY | rm_offset))
    {
        P("  RM_OFFSET live %08x, computed %08x\n", (unsigned)v,
          (unsigned)(RM_OFFSET_ONLY | rm_offset));
        bad++;
    }
    for (i = 0; i < 9; i++)
        if ((v = rd(phy + PHY_PLL_MISC(i))) != pll_misc[i])
        {
            P("  PLL_MISC_%u live %08x, captured %08x\n", (unsigned)i,
              (unsigned)v, (unsigned)pll_misc[i]);
            bad++;
        }
    for (i = 0; i < 4; i++)
        if ((v = rd(phy + PHY_CTL(i))) != lane_ctl[i])
        {
            P("  CTL_%u live %08x, captured %08x\n", (unsigned)i,
              (unsigned)v, (unsigned)lane_ctl[i]);
            bad++;
        }
    return bad;
}

#define TEMPLATE "PORT/N,RATE/N,GO/S,SPEC/S,FORCE/S"

enum { ARG_PORT, ARG_RATE, ARG_GO, ARG_SPEC, ARG_FORCE, ARG_COUNT };

int main(void)
{
    APTR KernelBase = OpenResource("kernel.resource");
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    ULONG khz = 148500, vco_div, rm_offset, bad, ctl, vctl;
    ULONG reset_ctl;
    APTR mbbuf = NULL;
    int port, rc = RETURN_FAIL;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "phytest");
        return RETURN_FAIL;
    }

    periiobase = KernelBase ? (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) : 0;
    if (periiobase != BCM2712_PERIIOBASE)
    {
        P("phytest: not a BCM2712 - refusing\n");
        goto out;
    }
    if (!(MBoxBase = OpenResource("mbox.resource"))
        || !(mbbuf = AllocMem(MBOX_MSG_ALIGN + 64, MEMF_31BIT | MEMF_CLEAR)))
        goto out;
    mb = (ULONG *)(((IPTR)mbbuf + MBOX_MSG_ALIGN - 1) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    if (args[ARG_PORT])
        port = *(LONG *)args[ARG_PORT] & 1;
    else
        port = (rd(periiobase + hdmi_off[0] + HDMI_HOTPLUG) & 1) ? 0 : 1;
    if (args[ARG_RATE])
        khz = *(LONG *)args[ARG_RATE];
    reset_ctl = args[ARG_SPEC] ? RESET_CTL_SPEC : RESET_CTL_FW;

    pv   = periiobase + pv_off[port];
    phy  = periiobase + phy_off[port];
    rm   = periiobase + rm_off[port];
    hdmi = periiobase + hdmi_off[port];

    P("phytest: HDMI%d, TMDS %u kHz\n", port, (unsigned)khz);
    if (khz >= LANE_BAND_MAX)
    {
        P("phytest: only the lane settings below %u kHz are known - refusing\n",
          (unsigned)LANE_BAND_MAX);
        goto out;
    }
    if (!pll_params(khz, &vco_div, &rm_offset))
    {
        P("phytest: no vco_div puts %u kHz in the 8-12 GHz VCO window\n", (unsigned)khz);
        goto out;
    }
    P("  computed: vco_div %u, RM_OFFSET %08x, RESET_CTL %02x\n", (unsigned)vco_div,
      (unsigned)(RM_OFFSET_ONLY | rm_offset), (unsigned)reset_ctl);

    phy_print("live    ");
    bad = compare(vco_div, rm_offset);
    P("  %u difference(s) from the live PHY\n", (unsigned)bad);

    if (!args[ARG_GO])
    {
        P("phytest: dry run - add GO to write\n");
        rc = RETURN_OK;
        goto out;
    }
    if (bad && !args[ARG_FORCE])
    {
        P("phytest: the PHY does not hold what we would write - add FORCE to go on\n");
        goto out;
    }
    if (!(rd(pv + PV_CONTROL) & PV_CONTROL_EN) || !(rd(pv + PV_V_CONTROL) & PV_VC_VIDEN))
    {
        P("phytest: pixelvalve%d is not running - refusing\n", port);
        goto out;
    }

    measure("before");

    P("phytest: taking the HDMI%d PHY over\n", port);
    pv_stop(&ctl, &vctl);
    phy_init(khz, vco_div, rm_offset, reset_ctl);
    Delay(1);       /* PLL lock */
    pv_start(ctl, vctl);
    recenter();

    phy_print("now     ");
    measure("after");

    P("phytest: done\n");
    rc = RETURN_OK;

out:
    if (mbbuf)
        FreeMem(mbbuf, MBOX_MSG_ALIGN + 64);
    FreeArgs(rda);
    return rc;
}
