/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 pixelvalve takeover test - step 3 of real modesetting.

    Stops the pixelvalve behind the boot display and starts it again at
    the same mode, leaving the HDMI controller and PHY alone. Plain GO
    writes back what the firmware programmed, which tests the sequence.
    ODD switches to 1 pixel/clock with ODD_TIMING and unhalved horizontal
    counts, which tests the timing arithmetic, and restores the firmware
    values after HOLD seconds.

    Without GO nothing is written. Everything goes to the serial log as
    well, since a failed run can take the picture with it.

    Usage: pvtest [PORT=<0|1>] [GO] [ODD] [HOLD=<s>] [HVSRESET] [RECENTER]
*/

#include <aros/debug.h>
#include <aros/kernel.h>

#include <exec/types.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/dos.h>

#include <stdio.h>

static IPTR periiobase;

#define ARM_PERIIOBASE periiobase
#include <hardware/bcm2708.h>

#define P(fmt, args...) do { printf(fmt, ##args); bug(fmt, ##args); } while (0)

/* Offsets from the peripheral base, see DISPLAY-SPEC.md sections 1, 5-7. */
static const ULONG pv_off[2]   = { 0x410000, 0x411000 };
static const ULONG hdmi_off[2] = { 0x701400, 0x706400 };
#define HVS_CHAN(ch)            (0x580100 + 0x40 * (ch))   /* D0 layout */
#define HD_FRAME_COUNT          (0x720000 + 0x060)

#define PV_CONTROL              0x00
#define PV_V_CONTROL            0x04
#define PV_VSYNCD_EVEN          0x08
#define PV_HORZA                0x0c
#define PV_HORZB                0x10
#define PV_VERTA                0x14
#define PV_VERTB                0x18
#define PV_VERTA_EVEN           0x1c
#define PV_VERTB_EVEN           0x20
#define PV_INTEN                0x24
#define PV_STAT                 0x2c
#define PV_MUX_CFG              0x34
#define PV_PIPE_INIT_CTRL       0x94

#define PV_CONTROL_EN           (1 << 0)
#define PV_CONTROL_FIFO_CLR     (1 << 1)
#define PV_CONTROL_CLK_SELECT   (3 << 2)
#define PV_VC_VIDEN             (1 << 0)
#define PV_VC_INTERLACE         (1 << 4)
#define PV_VC_ODD_TIMING        (1 << 29)

#define HVS_CTRL0_ENB           (1UL << 31)
#define HVS_CTRL0_RESET         (1UL << 30)
#define HVS_STATUS              0x18
#define HVS_FRCNT(s)            (((s) >> 16) & 0x3f)

#define HDMI_FIFO_CTL           0x07c
#define HDMI_HOTPLUG            0x1c8
#define FIFO_RECENTER           (1 << 6)
#define FIFO_RECENTER_DONE      (1 << 14)

struct pvregs
{
    ULONG ctl, vctl, vsyncd, horza, horzb, verta, vertb, vertae, vertbe, mux, pipe;
};

static IPTR pv;

static inline ULONG rd(IPTR a)
{
    return *(volatile ULONG *)a;
}

static inline void wr(IPTR a, ULONG v)
{
    *(volatile ULONG *)a = v;
    asm volatile ("dsb sy" ::: "memory");
}

static void pv_read(struct pvregs *r)
{
    r->ctl    = rd(pv + PV_CONTROL);
    r->vctl   = rd(pv + PV_V_CONTROL);
    r->vsyncd = rd(pv + PV_VSYNCD_EVEN);
    r->horza  = rd(pv + PV_HORZA);
    r->horzb  = rd(pv + PV_HORZB);
    r->verta  = rd(pv + PV_VERTA);
    r->vertb  = rd(pv + PV_VERTB);
    r->vertae = rd(pv + PV_VERTA_EVEN);
    r->vertbe = rd(pv + PV_VERTB_EVEN);
    r->mux    = rd(pv + PV_MUX_CFG);
    r->pipe   = rd(pv + PV_PIPE_INIT_CTRL);
}

static void pv_print(const char *name, const struct pvregs *r)
{
    ULONG ppc = (r->vctl & PV_VC_ODD_TIMING) ? 1 : 2;

    P("%s: CONTROL %08x V_CONTROL %08x HORZA %08x HORZB %08x VERTA %08x VERTB %08x\n"
      "    MUX_CFG %08x PIPE_INIT_CTRL %08x -> %u ppc, %u/%u wide, %u/%u high\n",
      name, (unsigned)r->ctl, (unsigned)r->vctl, (unsigned)r->horza,
      (unsigned)r->horzb, (unsigned)r->verta, (unsigned)r->vertb,
      (unsigned)r->mux, (unsigned)r->pipe, (unsigned)ppc,
      (unsigned)((r->horzb & 0xffff) * ppc),
      (unsigned)(((r->horza >> 16) + (r->horza & 0xffff) + (r->horzb >> 16)
                  + (r->horzb & 0xffff)) * ppc),
      (unsigned)(r->vertb & 0xffff),
      (unsigned)((r->verta >> 16) + (r->verta & 0xffff) + (r->vertb >> 16)
                 + (r->vertb & 0xffff)));
}

/* Unhalved horizontal counts for 1 pixel/clock; vertical never halves. */
static void pv_to_odd(const struct pvregs *fw, struct pvregs *r)
{
    *r = *fw;
    r->horza = ((fw->horza >> 16) * 2) << 16 | ((fw->horza & 0xffff) * 2);
    r->horzb = ((fw->horzb >> 16) * 2) << 16 | ((fw->horzb & 0xffff) * 2);
    r->vctl |= PV_VC_ODD_TIMING;
}

/* DISPLAY-SPEC 10.2 steps 1, 2 and 5, then 10 steps 14-16. */
static BOOL pv_program(const struct pvregs *r, IPTR hvs)
{
    ULONG ctl, ctrl0 = 0, i;
    BOOL stopped = FALSE;

    wr(pv + PV_V_CONTROL, rd(pv + PV_V_CONTROL) & ~PV_VC_VIDEN);
    for (i = 0; i < 10 && !stopped; i++)
    {
        if (!(rd(pv + PV_V_CONTROL) & PV_VC_VIDEN))
            stopped = TRUE;
        else
            Delay(1);
    }

    /* A pixel left in the PV-to-HDMI FIFO shifts every line by one. */
    Delay(2);

    ctl = rd(pv + PV_CONTROL) & ~PV_CONTROL_EN;
    wr(pv + PV_CONTROL, ctl);
    wr(pv + PV_CONTROL, ctl | PV_CONTROL_FIFO_CLR);

    if (hvs)
    {
        ctrl0 = rd(hvs);
        wr(hvs, ctrl0 | HVS_CTRL0_RESET);
    }

    wr(pv + PV_VSYNCD_EVEN, r->vsyncd);
    wr(pv + PV_HORZA, r->horza);
    wr(pv + PV_HORZB, r->horzb);
    wr(pv + PV_VERTA, r->verta);
    wr(pv + PV_VERTB, r->vertb);
    wr(pv + PV_VERTA_EVEN, r->vertae);
    wr(pv + PV_VERTB_EVEN, r->vertbe);
    wr(pv + PV_MUX_CFG, r->mux);
    wr(pv + PV_PIPE_INIT_CTRL, r->pipe);

    if (hvs)
        wr(hvs, ctrl0 & ~HVS_CTRL0_RESET);

    wr(pv + PV_CONTROL, (r->ctl & ~PV_CONTROL_EN) | PV_CONTROL_FIFO_CLR);
    wr(pv + PV_CONTROL, r->ctl);
    wr(pv + PV_V_CONTROL, r->vctl);

    return stopped;
}

/* DISPLAY-SPEC 10.1. */
static void recenter(IPTR hdmi)
{
    ULONG drift = rd(hdmi + HDMI_FIFO_CTL) & 0xefff, i;

    wr(hdmi + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(hdmi + HDMI_FIFO_CTL, drift | FIFO_RECENTER);
    Delay(1);
    wr(hdmi + HDMI_FIFO_CTL, drift & ~FIFO_RECENTER);
    wr(hdmi + HDMI_FIFO_CTL, drift | FIFO_RECENTER);

    for (i = 0; i < 50; i++)
    {
        if (rd(hdmi + HDMI_FIFO_CTL) & FIFO_RECENTER_DONE)
            break;
        Delay(1);
    }
    P("  FIFO_CTL %08x after recentre%s\n", (unsigned)rd(hdmi + HDMI_FIFO_CTL),
      (i == 50) ? " - RECENTER_DONE never came" : "");
}

/* Frames seen by the HDMI frame counter over ~1 s, and by the HVS
 * channel's 6-bit FRCNT over ~0.5 s (doubled). */
static void measure(const char *when, IPTR hvs_status)
{
    ULONG f0, f1, s0, s1;

    s0 = HVS_FRCNT(rd(hvs_status));
    Delay(25);
    s1 = HVS_FRCNT(rd(hvs_status));
    f0 = rd(periiobase + HD_FRAME_COUNT);
    Delay(50);
    f1 = rd(periiobase + HD_FRAME_COUNT);

    P("  %s: HDMI %u frames/s, HVS ch %u frames/s, PV STAT %08x\n", when,
      (unsigned)(f1 - f0), (unsigned)(((s1 - s0) & 0x3f) * 2),
      (unsigned)rd(pv + PV_STAT));
}

#define TEMPLATE "PORT/N,GO/S,ODD/S,HOLD/N,HVSRESET/S,RECENTER/S"

enum { ARG_PORT, ARG_GO, ARG_ODD, ARG_HOLD, ARG_HVSRESET, ARG_RECENTER, ARG_COUNT };

int main(void)
{
    APTR KernelBase = OpenResource("kernel.resource");
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct pvregs fw, plan, now;
    ULONG hold = 5;
    IPTR hvs, hdmi;
    int port;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "pvtest");
        return RETURN_FAIL;
    }

    periiobase = KernelBase ? (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) : 0;
    if (periiobase != BCM2712_PERIIOBASE)
    {
        P("pvtest: not a BCM2712 - refusing\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    if (args[ARG_PORT])
        port = *(LONG *)args[ARG_PORT] & 1;
    else
        port = (rd(periiobase + hdmi_off[0] + HDMI_HOTPLUG) & 1) ? 0 : 1;
    if (args[ARG_HOLD])
        hold = *(LONG *)args[ARG_HOLD];

    /* Fixed wiring: HVS channel n -> pixelvalve n -> HDMIn. */
    pv   = periiobase + pv_off[port];
    hvs  = periiobase + HVS_CHAN(port);
    hdmi = periiobase + hdmi_off[port];

    pv_read(&fw);
    P("pvtest: HDMI%d, pixelvalve%d\n", port, port);
    pv_print("  firmware", &fw);

    if (!(fw.ctl & PV_CONTROL_EN) || !(fw.vctl & PV_VC_VIDEN)
        || (fw.ctl & PV_CONTROL_CLK_SELECT) || (fw.vctl & PV_VC_INTERLACE)
        || (fw.vctl & PV_VC_ODD_TIMING))
    {
        P("pvtest: pixelvalve%d is not running a progressive 2 ppc HDMI mode - refusing\n",
          port);
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    if (args[ARG_ODD])
        pv_to_odd(&fw, &plan);
    else
        plan = fw;
    pv_print("  planned ", &plan);

    if (!args[ARG_GO])
    {
        P("pvtest: dry run - add GO to write\n");
        FreeArgs(rda);
        return RETURN_OK;
    }

    measure("before", hvs + HVS_STATUS);

    P("pvtest: taking pixelvalve%d over\n", port);
    if (!pv_program(&plan, args[ARG_HVSRESET] ? hvs : 0))
        P("  VIDEN did not read back clear within 200 ms\n");
    if (args[ARG_RECENTER])
        recenter(hdmi);

    pv_read(&now);
    pv_print("  now     ", &now);
    measure("after", hvs + HVS_STATUS);

    if (args[ARG_ODD])
    {
        P("pvtest: holding %u s, then restoring the firmware values\n", (unsigned)hold);
        Delay(hold * 50);
        pv_program(&fw, args[ARG_HVSRESET] ? hvs : 0);
        if (args[ARG_RECENTER])
            recenter(hdmi);
        pv_read(&now);
        pv_print("  restored", &now);
        measure("restored", hvs + HVS_STATUS);
    }

    P("pvtest: done\n");
    FreeArgs(rda);
    return RETURN_OK;
}
