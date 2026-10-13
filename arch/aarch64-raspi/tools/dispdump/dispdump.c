/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 display boot-state dump - step 1 of real modesetting.

    Read-only. Records what the firmware programmed for the boot mode and
    nothing can be derived from: the HDMI PHY PLL_MISC words and lane
    drive settings, the CSC coefficients, the HVS memory sizes, the
    pixelvalve FIFO level and the infoframes in the packet RAM. Ends with
    the constants as C.

    Usage: dispdump [PORT=<0|1>]      default = the port with a sink
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

/* Offsets from the peripheral base, see DISPLAY-SPEC.md section 1. */
#define HVS_OFF                 0x580000
#define DVP_OFF                 0x700000
#define HD_OFF                  0x720000
static const ULONG pv_off[2]   = { 0x410000, 0x411000 };
static const ULONG csc_off[2]  = { 0x700100, 0x700180 };
static const ULONG dvp_off[2]  = { 0x701000, 0x706000 };
static const ULONG hdmi_off[2] = { 0x701400, 0x706400 };
static const ULONG phy_off[2]  = { 0x701d00, 0x706d00 };
static const ULONG rm_off[2]   = { 0x702000, 0x707000 };
static const ULONG pkt_off[2]  = { 0x703800, 0x708800 };

/* Packet RAM: slot n holds infoframe type 0x80 + n, 9 words each - a
 * header word (HB0-2), then four 7-byte subpackets as 4 + 3 bytes. */
#define PKT_SLOTS               14
#define PKT_SLOT_WORDS          9
#define HDMI_RAM_PACKET_CONFIG  0x0c4
#define HDMI_RAM_PACKET_STATUS  0x0cc

#define HDMI_HOTPLUG            0x1c8

static ULONG faults;

/* Barriers make an unclaimed-address SError land on the offending read;
 * DISR_EL1 records it when the error is containable. */
static ULONG rd(IPTR addr)
{
    ULONG v;
    UQUAD disr;

    asm volatile (
        "msr    daifset, #4             \n"
        "msr    s3_0_c12_c1_1, xzr      \n"
        "isb                            \n"
        "ldr    %w0, [%2]               \n"
        "dsb    sy                      \n"
        "isb                            \n"
        "mrs    %1, s3_0_c12_c1_1       \n"
        "msr    s3_0_c12_c1_1, xzr      \n"
        "isb                            \n"
        "msr    daifclr, #4             \n"
        : "=&r" (v), "=&r" (disr)
        : "r" (addr)
        : "memory");

    if (disr & (1ULL << 31))
        faults++;
    return v;
}

#define R(off)  rd(periiobase + (off))

static void dump_regs(const char *name, ULONG off, ULONG bytes)
{
    ULONG o, i;

    P("\n%s (+0x%06x):\n", name, (unsigned)off);
    for (o = 0; o < bytes; o += 16)
    {
        P("  +%03x:", (unsigned)o);
        for (i = 0; i < 16 && o + i < bytes; i += 4)
            P(" %08x", (unsigned)R(off + o + i));
        P("\n");
    }
}

static ULONG *mb;

static ULONG clock_tag(ULONG tag, ULONG id)
{
    mb[0] = AROS_LONG2LE(8 * 4);
    mb[1] = AROS_LONG2LE(VCTAG_REQ);
    mb[2] = AROS_LONG2LE(tag);
    mb[3] = AROS_LONG2LE(8);
    mb[4] = 0;
    mb[5] = AROS_LONG2LE(id);
    mb[6] = 0;
    mb[7] = 0;

    if (MBoxCall((void *)(periiobase + VCMB_OFFSET_BCM2712), VCMB_PROPCHAN, mb)
            == (volatile unsigned int *)-1 || mb[1] != AROS_LONG2LE(VCTAG_RESP))
        return 0;
    return AROS_LE2LONG(mb[6]);
}

static void dump_clocks(void)
{
    static const struct { ULONG id; const char *name; } clk[] =
    {
        {  9, "PIXEL" }, { 13, "M2MC (HSM)" }, { 14, "PIXEL_BVB" },
    };
    APTR buf;
    ULONG i;

    if (!(MBoxBase = OpenResource("mbox.resource")))
        return;
    if (!(buf = AllocMem(MBOX_MSG_ALIGN + 64, MEMF_31BIT | MEMF_CLEAR)))
        return;
    mb = (ULONG *)(((IPTR)buf + MBOX_MSG_ALIGN - 1) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    P("\nFirmware clocks (set / measured, Hz):\n");
    for (i = 0; i < sizeof(clk) / sizeof(clk[0]); i++)
        P("  %2u %-11s %10u / %10u\n", (unsigned)clk[i].id, clk[i].name,
          (unsigned)clock_tag(VCTAG_GETCLKRATE, clk[i].id),
          (unsigned)clock_tag(VCTAG_GETCLKMEASURED, clk[i].id));

    FreeMem(buf, MBOX_MSG_ALIGN + 64);
}

static void dump_hvs(BOOL *d0)
{
    ULONG ver = R(HVS_OFF + 0x000);
    ULONG ch;

    *d0 = ((ver & 0xff) == 0x54);
    P("\nHVS: VERSION %08x (%s), CXM %u words, LBM %u, UBM %u, COBA %u, COB %u\n",
      (unsigned)ver, *d0 ? "D0" : ((ver & 0xff) == 0x53) ? "C0" : "unknown",
      (unsigned)R(HVS_OFF + 0x004), (unsigned)R(HVS_OFF + 0x008),
      (unsigned)R(HVS_OFF + 0x00c), (unsigned)R(HVS_OFF + 0x010),
      (unsigned)R(HVS_OFF + 0x014));
    P("  CONTROL %08x, PRI_MAP %08x %08x\n", (unsigned)R(HVS_OFF + 0x020),
      (unsigned)R(HVS_OFF + (*d0 ? 0x038 : 0x0b8)),
      (unsigned)R(HVS_OFF + (*d0 ? 0x03c : 0x0bc)));

    for (ch = 0; ch < 3; ch++)
    {
        ULONG b = HVS_OFF + (*d0 ? 0x100 + 0x40 * ch : 0x030 + 0x20 * ch);
        ULONG ctrl0 = R(b), cob = R(b + (*d0 ? 0x14 : 0x10));

        P("  ch%u CTRL0 %08x (%s %ux%u) CTRL1 %08x COB %u..%u STATUS %08x\n",
          (unsigned)ch, (unsigned)ctrl0, (ctrl0 & 0x80000000) ? "on" : "off",
          (unsigned)(((ctrl0 >> 16) & 0x1fff) + 1), (unsigned)((ctrl0 & 0x1fff) + 1),
          (unsigned)R(b + 0x04), (unsigned)(cob & 0xffff), (unsigned)(cob >> 16),
          (unsigned)R(b + (*d0 ? 0x18 : 0x14)));
    }
}

static void dump_pv(int port)
{
    ULONG b = pv_off[port];
    ULONG ctl = R(b + 0x00), vctl = R(b + 0x04);
    ULONG ha = R(b + 0x0c), hb = R(b + 0x10), va = R(b + 0x14), vb = R(b + 0x18);
    ULONG ppc = (vctl & (1 << 29)) ? 1 : 2;
    ULONG hact = hb & 0xffff, hfp = hb >> 16, hsync = ha & 0xffff, hbp = ha >> 16;

    dump_regs(port ? "pixelvalve1" : "pixelvalve0", b, 0x38);
    P("  CONTROL: EN %u, CLK_SELECT %u, PIXEL_REP %u, FORMAT %u,"
      " FIFO_LEVEL %u, FIFO_LEVEL_HIGH %u, WAIT_HSTART %u, TRIG_UF %u, CLR_AT_START %u\n",
      (unsigned)(ctl & 1), (unsigned)((ctl >> 2) & 3), (unsigned)(((ctl >> 4) & 3) + 1),
      (unsigned)((ctl >> 21) & 7), (unsigned)((ctl >> 15) & 0x3f),
      (unsigned)((ctl >> 25) & 3), (unsigned)((ctl >> 12) & 1),
      (unsigned)((ctl >> 13) & 1), (unsigned)((ctl >> 14) & 1));
    P("  V_CONTROL: VIDEN %u, CONTINUOUS %u, INTERLACE %u, ODD_TIMING %u -> %u pixel/clock\n",
      (unsigned)(vctl & 1), (unsigned)((vctl >> 1) & 1), (unsigned)((vctl >> 4) & 1),
      (unsigned)((vctl >> 29) & 1), (unsigned)ppc);
    P("  h: active %u fp %u sync %u bp %u (x%u = %u/%u)  v: active %u fp %u sync %u bp %u\n",
      (unsigned)hact, (unsigned)hfp, (unsigned)hsync, (unsigned)hbp, (unsigned)ppc,
      (unsigned)(hact * ppc), (unsigned)((hact + hfp + hsync + hbp) * ppc),
      (unsigned)(vb & 0xffff), (unsigned)(vb >> 16), (unsigned)(va & 0xffff),
      (unsigned)(va >> 16));
    P("  MUX_CFG %08x, PIPE_INIT_CTRL %08x\n", (unsigned)R(b + 0x34), (unsigned)R(b + 0x94));
}

static void dump_hdmi(int port)
{
    ULONG b = hdmi_off[port];
    ULONG ha = R(b + 0x0ec), hb = R(b + 0x0f0), va = R(b + 0x0f4), vb = R(b + 0x0f8);

    P("\nHDMI%d controller:\n", port);
    P("  HORZA %08x HORZB %08x VERTA0 %08x VERTB0 %08x VERTA1 %08x VERTB1 %08x\n",
      (unsigned)ha, (unsigned)hb, (unsigned)va, (unsigned)vb,
      (unsigned)R(b + 0x100), (unsigned)R(b + 0x104));
    P("  h: active %u fp %u sync %u bp %u %csync  v: active %u fp %u sync %u bp %u %cvsync\n",
      (unsigned)(ha & 0x3fff), (unsigned)((ha >> 16) & 0x1fff), (unsigned)(hb & 0x7ff),
      (unsigned)((hb >> 16) & 0x7ff), (ha & (1 << 14)) ? '+' : '-',
      (unsigned)(va & 0x1fff), (unsigned)((va >> 16) & 0x7f), (unsigned)((va >> 24) & 0x1f),
      (unsigned)(vb & 0x1ff), (ha & (1 << 15)) ? '+' : '-');
    P("  FIFO_CTL %08x MISC_CONTROL %08x DEEP_COLOR_1 %08x GCP_CONFIG %08x GCP_WORD_1 %08x\n",
      (unsigned)R(b + 0x07c), (unsigned)R(b + 0x114), (unsigned)R(b + 0x18c),
      (unsigned)R(b + 0x194), (unsigned)R(b + 0x198));
    P("  SCHEDULER_CONTROL %08x SCRAMBLER_CTL %08x HOTPLUG %08x\n",
      (unsigned)R(b + 0x0e8), (unsigned)R(b + 0x1e4), (unsigned)R(b + HDMI_HOTPLUG));

    b = dvp_off[port];
    P("  dvp: CLOCK_STOP %08x VEC_INTERFACE_CFG %08x VEC_INTERFACE_XBAR %08x\n",
      (unsigned)R(b + 0x0bc), (unsigned)R(b + 0x0f0), (unsigned)R(b + 0x0f4));
    P("  hd: DVP_CTL %08x VID_CTL %08x FRAME_COUNT %08x\n", (unsigned)R(HD_OFF + 0x000),
      (unsigned)R(HD_OFF + 0x044), (unsigned)R(HD_OFF + 0x060));
    P("  DVP clock/reset: SW_INIT %08x MISC_CONFIG %08x\n",
      (unsigned)R(DVP_OFF + 0x04), (unsigned)R(DVP_OFF + 0x08));
}

static void dump_lane(const char *name, ULONG v)
{
    P("  %-6s %08x: ext %u%s, int %u%s%s, term %u, main %u post %u%s%s, slew %u/%u/%u\n",
      name, (unsigned)v, (unsigned)(v >> 28), (v & (1 << 17)) ? "" : " (off)",
      (unsigned)((v >> 12) & 15), (v & (1 << 16)) ? "" : " (off)",
      (v & (1 << 11)) ? " hs" : "", (unsigned)((v >> 18) & 3),
      (unsigned)((v >> 8) & 7), (unsigned)((v >> 5) & 7),
      (v & (1 << 27)) ? " ffe" : "", (v & (1 << 25)) ? " post-tap" : "",
      (unsigned)((v >> 26) & 1), (unsigned)((v >> 3) & 3), (unsigned)((v >> 1) & 3));
}

static void dump_phy(int port, ULONG *misc, ULONG *lane)
{
    ULONG b = phy_off[port], i, vcodiv, rmoff;
    UQUAD vco;

    P("\nHDMI%d PHY:\n", port);
    vcodiv = R(b + 0x02c);
    rmoff  = R(rm_off[port] + 0x018) & 0x7fffffff;
    vco    = (UQUAD)rmoff * 54000000 / (1 << 21);
    P("  RESET_CTL %08x POWERUP_CTL %08x PLL_RESET_CTL %08x PLL_POWERUP_CTL %08x\n",
      (unsigned)R(b + 0x000), (unsigned)R(b + 0x004), (unsigned)R(b + 0x190),
      (unsigned)R(b + 0x194));
    P("  PLL_REFCLK %08x POST_KDIV %08x VCOCLK_DIV %08x PLL_CFG %08x TMDS_CLK_WORD_SEL %08x\n",
      (unsigned)R(b + 0x01c), (unsigned)R(b + 0x028), (unsigned)vcodiv,
      (unsigned)R(b + 0x044), (unsigned)R(b + 0x054));

    for (i = 0; i < 4; i++)
        lane[i] = R(b + 0x008 + 4 * i);
    dump_lane("CTL_0", lane[0]);
    dump_lane("CTL_1", lane[1]);
    dump_lane("CTL_2", lane[2]);
    dump_lane("CTL_CK", lane[3]);

    P("  PLL_MISC:");
    for (i = 0; i < 9; i++)
        P(" %08x", (unsigned)(misc[i] = R(b + 0x060 + 4 * i)));
    P("\n");

    P("  rm: CONTROL %08x OFFSET %08x FORMAT %08x\n",
      (unsigned)R(rm_off[port] + 0x000), (unsigned)R(rm_off[port] + 0x018),
      (unsigned)R(rm_off[port] + 0x01c));
    if (vcodiv & 0x3ff)
    {
        ULONG tmds = vco / ((vcodiv & 0x3ff) * 10);

        P("  -> VCO %u kHz, vco_div %u, TMDS %u.%03u MHz\n", (unsigned)(vco / 1000),
          (unsigned)(vcodiv & 0x3ff), (unsigned)(tmds / 1000000),
          (unsigned)((tmds / 1000) % 1000));
    }
}

static void dump_packets(int port)
{
    static const char *names[] = { "?", "vendor", "AVI", "SPD", "audio", "MPEG", "?", "DRM" };
    ULONG cfg = R(hdmi_off[port] + HDMI_RAM_PACKET_CONFIG);
    ULONG sta = R(hdmi_off[port] + HDMI_RAM_PACKET_STATUS);
    ULONG slot, i;

    P("\nHDMI%d packet RAM: RAM_PACKET_CONFIG %08x STATUS %08x\n", port,
      (unsigned)cfg, (unsigned)sta);

    for (slot = 0; slot < PKT_SLOTS; slot++)
    {
        ULONG base = pkt_off[port] + slot * PKT_SLOT_WORDS * 4;
        ULONG hdr = R(base);
        UBYTE pb[28];

        if (!(cfg & (1UL << slot)) && ((hdr & 0xff) != 0x80 + slot))
            continue;

        for (i = 0; i < 4; i++)
        {
            ULONG a = R(base + 4 + i * 8), b = R(base + 8 + i * 8);

            pb[i * 7 + 0] = a;       pb[i * 7 + 1] = a >> 8;
            pb[i * 7 + 2] = a >> 16; pb[i * 7 + 3] = a >> 24;
            pb[i * 7 + 4] = b;       pb[i * 7 + 5] = b >> 8;
            pb[i * 7 + 6] = b >> 16;
        }

        P("  slot %2u %s %-6s HB %02x %02x %02x  PB", (unsigned)slot,
          (cfg & (1UL << slot)) ? "on " : "off",
          ((hdr & 0xff) >= 0x80 && (hdr & 0xff) <= 0x87) ? names[(hdr & 0xff) - 0x80] : "-",
          (unsigned)(hdr & 0xff), (unsigned)((hdr >> 8) & 0xff), (unsigned)((hdr >> 16) & 0xff));
        for (i = 0; i <= ((hdr >> 16) & 0x1f) && i < 28; i++)
            P(" %02x", pb[i]);
        P("\n");

        if ((hdr & 0xff) == 0x82)
            P("          AVI: Y %u (0 RGB, 1 422, 2 444), Q %u (0 default, 1 limited, 2 full),"
              " VIC %u, YQ %u, aspect %u\n", pb[1] >> 5 & 3, pb[3] >> 2 & 3, pb[4] & 0x7f,
              pb[5] >> 6 & 3, pb[2] >> 4 & 3);
    }
}

static void dump_csc(int port, ULONG *coef)
{
    ULONG b = csc_off[port], i;

    P("\nHDMI%d CSC: CTL %08x CHANNEL_CTL %08x\n  coefficients:", port,
      (unsigned)R(b + 0x000), (unsigned)R(b + 0x02c));
    for (i = 0; i < 6; i++)
        P(" %08x", (unsigned)(coef[i] = R(b + 0x004 + 4 * i)));
    P("\n");
}

#define TEMPLATE "PORT/N"

int main(void)
{
    APTR KernelBase = OpenResource("kernel.resource");
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    ULONG misc[9], lane[4], coef[6], i;
    BOOL d0;
    int port;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "dispdump");
        return RETURN_FAIL;
    }

    periiobase = KernelBase ? (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) : 0;
    if (periiobase != BCM2712_PERIIOBASE)
    {
        P("dispdump: not a BCM2712 - refusing\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    if (args[0])
        port = *(LONG *)args[0] & 1;
    else
        port = (R(hdmi_off[0] + HDMI_HOTPLUG) & 1) ? 0 : 1;
    FreeArgs(rda);

    P("dispdump: HDMI%d, hotplug %s\n", port,
      (R(hdmi_off[port] + HDMI_HOTPLUG) & 1) ? "connected" : "no sink");

    dump_clocks();
    dump_hvs(&d0);
    dump_pv(port);
    dump_hdmi(port);
    dump_phy(port, misc, lane);
    dump_csc(port, coef);
    dump_packets(port);

    P("\n/* BCM2712 %s, HDMI%d, captured at the boot mode by dispdump */\n",
      d0 ? "D0" : "C0", port);
    P("static const ULONG hdmi_phy_pll_misc[9] =\n{\n   ");
    for (i = 0; i < 9; i++)
        P(" 0x%08x,%s", (unsigned)misc[i], (i == 4) ? "\n   " : "");
    P("\n};\n");
    P("/* CTL_0, CTL_1, CTL_2, CTL_CK for this TMDS band */\n");
    P("static const ULONG hdmi_phy_lane_ctl[4] =\n{\n   ");
    for (i = 0; i < 4; i++)
        P(" 0x%08x,", (unsigned)lane[i]);
    P("\n};\n");
    P("static const ULONG hdmi_csc_coef[6] =\n{\n   ");
    for (i = 0; i < 6; i++)
        P(" 0x%08x,", (unsigned)coef[i]);
    P("\n};\n");

    P("\ndispdump: done, %u reads faulted\n", (unsigned)faults);
    return RETURN_OK;
}
