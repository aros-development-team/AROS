/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: HDMI EDID probe - reads the sink's EDID over the BCM2712 DDC
          controllers (Broadcom STB I2C), or from the firmware mailbox.

    The firmware's GETEDID tag answers 0x80000001 on BCM2712, so EDID
    drives the DDC controller directly, polled. Without EDID it only
    dumps the DDC registers and the HDMI hotplug bits, which shows what
    the firmware left behind after reading the EDID itself at boot.
    MBOX asks the firmware instead and works on every Pi; it is how the
    same decoder gets tested on BCM2835..2711.

    Usage: ddcprobe [PORT=<0|1>] [EDID] [MBOX] [RAW]
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
#define VCTAG_GETEDID_DISPLAY   0x00030023
/* Firmware display IDs for the two HDMI ports. */
static const ULONG fw_display[2] = { 2, 7 };

static IPTR   vcmb;
static ULONG *mb;
static BOOL   use_mbox;
static int    cur_port;

/* Offsets from the peripheral base (bus 0x7C000000 on BCM2712). */
static const ULONG ddc_off[2]  = { 0x1508200, 0x1508280 };
static const ULONG hdmi_off[2] = { 0x701400, 0x706400 };
#define HDMI_HOTPLUG            0x1c8           /* bit 0 = connected    */

#define BSC_SIZE                0x58
#define BSC_CHIP_ADDRESS        0x00            /* 8-bit address incl. R/W */
#define BSC_DATA_IN(i)          (0x04 + 4 * (i))/* bytes sent to the slave */
#define BSC_CNT                 0x24            /* 5:0 = transfer length   */
#define BSC_CTL                 0x28
#define BSC_IIC_ENABLE          0x2c
#define BSC_DATA_OUT(i)         (0x30 + 4 * (i))/* bytes read back         */
#define BSC_CTLHI               0x50

#define CTL_DTF_MASK            0x03
#define CTL_DTF_WR              0x00
#define CTL_DTF_RD              0x01
#define CTL_SCL_SEL_SHIFT       4
#define CTL_SCL_SEL_MASK        (3 << CTL_SCL_SEL_SHIFT)
#define CTL_INT_EN              (1 << 6)
#define CTL_DIV_CLK             (1 << 7)

#define EN_ENABLE               (1 << 0)
#define EN_INTRP                (1 << 1)        /* transfer done           */
#define EN_NOACK                (1 << 2)

#define CTLHI_IGNORE_ACK        (1 << 1)
#define CTLHI_DATAREG_32        (1 << 6)        /* 4 bytes per data reg    */

#define DATA_REGS               8
#define POLL_LIMIT              2000000

#define EDID_ADDR               (0x50 << 1)

static IPTR bsc;
static ULONG chunk;             /* bytes per transfer: 32 or 8 */
static BOOL  swapped;           /* data regs turned out big-endian */

static inline ULONG rd(ULONG off)
{
    return *(volatile ULONG *)(bsc + off);
}

static inline void wr(ULONG off, ULONG v)
{
    *(volatile ULONG *)(bsc + off) = v;
}

static void dump_regs(const char *name, IPTR base, ULONG bytes)
{
    ULONG off;

    P("\n%s @ 0x%p:\n", name, (APTR)base);
    for (off = 0; off < bytes; off += 16)
    {
        ULONG i;

        P("  +%03x:", (unsigned)off);
        for (i = 0; i < 16 && off + i < bytes; i += 4)
            P(" %08x", (unsigned)*(volatile ULONG *)(base + off + i));
        P("\n");
    }
}

/* One START..STOP transaction of up to `chunk` bytes. */
static LONG bsc_xfer(BOOL read, UBYTE *buf, ULONG len)
{
    ULONG i, en, n;

    wr(BSC_IIC_ENABLE, 0);
    wr(BSC_CHIP_ADDRESS, EDID_ADDR | (read ? 1 : 0));
    wr(BSC_CNT, len);
    wr(BSC_CTL, (rd(BSC_CTL) & ~(CTL_DTF_MASK | CTL_INT_EN))
                | (read ? CTL_DTF_RD : CTL_DTF_WR));

    if (!read)
    {
        ULONG w[DATA_REGS] = { 0 };

        for (i = 0; i < len; i++)
        {
            if (chunk == 32)
                w[i >> 2] |= (ULONG)buf[i] << (swapped ? (3 - (i & 3)) * 8 : (i & 3) * 8);
            else
                w[i] = buf[i];
        }
        for (i = 0; i < DATA_REGS; i++)
            wr(BSC_DATA_IN(i), w[i]);
    }

    wr(BSC_IIC_ENABLE, EN_ENABLE);
    for (n = 0; n < POLL_LIMIT; n++)
        if ((en = rd(BSC_IIC_ENABLE)) & EN_INTRP)
            break;
    wr(BSC_IIC_ENABLE, 0);

    if (n == POLL_LIMIT)
        return -1;
    if (en & EN_NOACK)
        return -2;

    if (read)
    {
        for (i = 0; i < len; i++)
        {
            if (chunk == 32)
            {
                ULONG sh = swapped ? (3 - (i & 3)) * 8 : (i & 3) * 8;

                buf[i] = rd(BSC_DATA_OUT(i >> 2)) >> sh;
            }
            else
                buf[i] = rd(BSC_DATA_OUT(i));
        }
    }
    return 0;
}

/* Offset write then a separate read, per chunk: the EDID's address
 * pointer survives the STOP, so no repeated START is needed. */
static LONG edid_read(UBYTE offset, UBYTE *buf, ULONG len)
{
    ULONG done;
    LONG err;

    for (done = 0; done < len; done += chunk)
    {
        UBYTE o = offset + done;

        if ((err = bsc_xfer(FALSE, &o, 1)) != 0)
            return err;
        if ((err = bsc_xfer(TRUE, buf + done, chunk)) != 0)
            return err;
    }
    return 0;
}

/* Port 0 uses the original tag; port 1 needs the per-display one. Both
 * answer block, status/display, then the 128 EDID bytes. */
static LONG mbox_edid(ULONG block, UBYTE *buf)
{
    ULONG tag = cur_port ? VCTAG_GETEDID_DISPLAY : VCTAG_GETEDID;
    ULONG i, c = 0;

    mb[c++] = 0;
    mb[c++] = AROS_LONG2LE(VCTAG_REQ);
    mb[c++] = AROS_LONG2LE(tag);
    mb[c++] = AROS_LONG2LE(136);
    mb[c++] = 0;
    mb[c++] = AROS_LONG2LE(block);
    mb[c++] = AROS_LONG2LE(cur_port ? fw_display[1] : 0);
    for (i = 0; i < 32; i++)
        mb[c++] = 0;
    mb[c++] = 0;
    mb[0] = AROS_LONG2LE(c * 4);

    if (MBoxCall((void *)vcmb, VCMB_PROPCHAN, mb) == (volatile unsigned int *)-1)
        return -4;

    if (mb[1] != AROS_LONG2LE(VCTAG_RESP) || !(AROS_LE2LONG(mb[4]) & 0x80000000)
        || (tag == VCTAG_GETEDID && mb[6]))
    {
        P("  mailbox %08x: status %08x, tag %08x, words %08x %08x\n",
          (unsigned)tag, (unsigned)AROS_LE2LONG(mb[1]),
          (unsigned)AROS_LE2LONG(mb[4]), (unsigned)AROS_LE2LONG(mb[5]),
          (unsigned)AROS_LE2LONG(mb[6]));
        return -4;
    }

    memcpy(buf, &mb[7], 128);
    return 0;
}

static LONG get_block(ULONG n, UBYTE *buf)
{
    LONG err;

    if (use_mbox)
        return mbox_edid(n, buf);
    if (n > 1)
        return -3;

    err = edid_read(n * 128, buf, 128);

    /* Header bytes landing word-reversed means the data registers are
     * big-endian; the checksum cannot tell, it sums the same either way. */
    if (!err && !n && chunk == 32 && buf[0] == 0xff && buf[3] == 0x00 && buf[4] == 0x00)
    {
        P("  data registers are big-endian - reading again\n");
        swapped = TRUE;
        err = edid_read(0, buf, 128);
    }
    return err;
}

static const char *errstr(LONG err)
{
    switch (err)
    {
    case -1: return "timeout";
    case -2: return "no ACK from 0x50";
    case -3: return "needs the E-DDC segment pointer - not read";
    default: return "mailbox failed";
    }
}

static void hexdump(const UBYTE *b, ULONG len)
{
    ULONG i;

    for (i = 0; i < len; i++)
        P("%s%02x%s", (i & 15) ? "" : "  ", b[i], ((i & 15) == 15) ? "\n" : " ");
}

static BOOL edid_sum_ok(const UBYTE *b)
{
    UBYTE s = 0;
    ULONG i;

    for (i = 0; i < 128; i++)
        s += b[i];
    return s == 0;
}

static void decode_dtd(const UBYTE *d)
{
    ULONG clk = (d[0] | (d[1] << 8)) * 10;     /* kHz */

    if (clk)
    {
        ULONG ha = d[2] | ((d[4] & 0xf0) << 4), hb = d[3] | ((d[4] & 0x0f) << 8);
        ULONG va = d[5] | ((d[7] & 0xf0) << 4), vb = d[6] | ((d[7] & 0x0f) << 8);
        ULONG hso = d[8] | ((d[11] & 0xc0) << 2), hsw = d[9] | ((d[11] & 0x30) << 4);
        ULONG vso = (d[10] >> 4) | ((d[11] & 0x0c) << 2);
        ULONG vsw = (d[10] & 0x0f) | ((d[11] & 0x03) << 4);
        ULONG mhz = (UQUAD)clk * 1000000 / ((ha + hb) * (va + vb));  /* mHz */

        P("    DTD %ux%u%s @ %u.%03u Hz, %u.%03u MHz\n"
          "        h %u %u %u %u  v %u %u %u %u  %csync %cvsync\n",
          (unsigned)ha, (unsigned)va, (d[17] & 0x80) ? "i" : "",
          (unsigned)(mhz / 1000), (unsigned)(mhz % 1000),
          (unsigned)(clk / 1000), (unsigned)(clk % 1000),
          (unsigned)ha, (unsigned)(ha + hso), (unsigned)(ha + hso + hsw),
          (unsigned)(ha + hb),
          (unsigned)va, (unsigned)(va + vso), (unsigned)(va + vso + vsw),
          (unsigned)(va + vb),
          (d[17] & 0x02) ? '+' : '-', (d[17] & 0x04) ? '+' : '-');
        return;
    }

    switch (d[3])
    {
    case 0xfc:
    case 0xfe:
    case 0xff:
    {
        char s[14];
        ULONG i;

        for (i = 0; i < 13 && d[5 + i] != 0x0a; i++)
            s[i] = d[5 + i];
        s[i] = 0;
        P("    %s \"%s\"\n", (d[3] == 0xfc) ? "name  " :
                             (d[3] == 0xff) ? "serial" : "text  ", s);
        break;
    }
    case 0xfd:
        P("    range V %u-%u Hz, H %u-%u kHz, max %u MHz\n",
          d[5], d[6], d[7], d[8], d[9] * 10);
        break;
    }
}

static void decode_base(const UBYTE *b)
{
    UWORD mfg = (b[8] << 8) | b[9];
    ULONG i;

    P("  vendor %c%c%c product %04x, week %u %u, EDID %u.%u, %ux%u cm\n",
      '@' + ((mfg >> 10) & 0x1f), '@' + ((mfg >> 5) & 0x1f), '@' + (mfg & 0x1f),
      b[10] | (b[11] << 8), b[16], 1990 + b[17], b[18], b[19], b[21], b[22]);
    P("  established %02x %02x %02x\n", b[35], b[36], b[37]);

    for (i = 38; i < 54; i += 2)
    {
        static const char *aspect[] = { "16:10", "4:3", "5:4", "16:9" };

        if (b[i] == 0x01 && b[i + 1] == 0x01)
            continue;
        P("    std %u %s @ %u Hz\n", (b[i] + 31) * 8, aspect[b[i + 1] >> 6],
          (b[i + 1] & 0x3f) + 60);
    }

    for (i = 54; i < 126; i += 18)
        decode_dtd(b + i);

    P("  %u extension block(s)\n", b[126]);
}

static void decode_cta(const UBYTE *b)
{
    ULONG i = 4, end = b[2];

    P("  CTA-861 rev %u, %s%s\n", b[1],
      (b[3] & 0x80) ? "underscan " : "", (b[3] & 0x40) ? "audio" : "");

    while (end >= 4 && i < end && i < 127)
    {
        ULONG tag = b[i] >> 5, len = b[i] & 0x1f, j;
        const UBYTE *p = b + i + 1;

        if (tag == 2)
        {
            P("    VICs:");
            for (j = 0; j < len; j++)
                P(" %u%s", p[j] & 0x7f, (p[j] & 0x80) ? "*" : "");
            P("\n");
        }
        else if (tag == 3 && len >= 3)
        {
            ULONG oui = p[0] | (p[1] << 8) | (p[2] << 16);

            if (oui == 0x000c03)
                P("    HDMI VSDB, phys %x.%x.%x.%x, max TMDS %u MHz\n",
                  p[3] >> 4, p[3] & 15, p[4] >> 4, p[4] & 15,
                  (len >= 7) ? p[6] * 5 : 0);
            else if (oui == 0xc45dd8 && len >= 5)
                P("    HDMI Forum VSDB, max TMDS %u MHz, SCDC %s\n",
                  p[4] * 5, (p[5] & 0x80) ? "yes" : "no");
        }
        i += len + 1;
    }

    for (i = end; end >= 4 && i + 18 <= 127; i += 18)
    {
        if (!b[i] && !b[i + 1])
            break;
        decode_dtd(b + i);
    }
}

static void read_edid(void)
{
    UBYTE edid[128];
    ULONG n, blocks;
    LONG err;

    if ((err = get_block(0, edid)) != 0)
    {
        P("  EDID block 0: %s\n", errstr(err));
        return;
    }

    for (n = 0; n < 128 && !edid[n]; n++)
        ;
    if (n == 128)
    {
        P("  EDID block 0: all zero - no sink on this port?\n");
        return;
    }

    hexdump(edid, 128);
    if (memcmp(edid, "\x00\xff\xff\xff\xff\xff\xff\x00", 8) || !edid_sum_ok(edid))
    {
        P("  EDID block 0: bad header or checksum\n");
        return;
    }
    decode_base(edid);

    blocks = edid[126];
    for (n = 1; n <= blocks; n++)
    {
        if ((err = get_block(n, edid)) != 0)
        {
            P("  EDID block %u: %s\n", (unsigned)n, errstr(err));
            break;
        }
        hexdump(edid, 128);
        if (!edid_sum_ok(edid))
            P("  EDID block %u: bad checksum\n", (unsigned)n);
        else if (edid[0] == 0x02)
            decode_cta(edid);
    }
}

static void ddc_edid(void)
{
    ULONG ctl = rd(BSC_CTL), ctlhi = rd(BSC_CTLHI), addr = rd(BSC_CHIP_ADDRESS);

    /* 97.5 kHz, the standard DDC rate; the bus is polled, not interrupted. */
    wr(BSC_CTL, (ctl & ~(CTL_SCL_SEL_MASK | CTL_INT_EN))
                | (1 << CTL_SCL_SEL_SHIFT) | CTL_DIV_CLK);
    wr(BSC_CTLHI, (ctlhi & ~CTLHI_IGNORE_ACK) | CTLHI_DATAREG_32);
    chunk = (rd(BSC_CTLHI) & CTLHI_DATAREG_32) ? 32 : 8;
    P("\n  CTL %08x -> %08x, CTLHI %08x -> %08x, %u bytes per transfer\n",
      (unsigned)ctl, (unsigned)rd(BSC_CTL), (unsigned)ctlhi,
      (unsigned)rd(BSC_CTLHI), (unsigned)chunk);

    read_edid();

    wr(BSC_CTL, ctl);
    wr(BSC_CTLHI, ctlhi);
    wr(BSC_CHIP_ADDRESS, addr);
}

#define TEMPLATE "PORT/N,EDID/S,MBOX/S,RAW/S"

enum { ARG_PORT, ARG_EDID, ARG_MBOX, ARG_RAW, ARG_COUNT };

int main(void)
{
    APTR KernelBase = OpenResource("kernel.resource");
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    APTR mbbuf = NULL;
    BOOL is2712;
    int port, first = 0, last = 1, rc = RETURN_FAIL;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "ddcprobe");
        return RETURN_FAIL;
    }
    if (args[ARG_PORT])
        first = last = *(LONG *)args[ARG_PORT] & 1;
    use_mbox = args[ARG_MBOX] ? TRUE : FALSE;

    periiobase = KernelBase ? (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) : 0;
    is2712 = (periiobase == BCM2712_PERIIOBASE);
    P("ddcprobe: periiobase 0x%p%s\n", (APTR)periiobase, is2712 ? " (BCM2712)" : "");

    if (use_mbox)
    {
        if (!(MBoxBase = OpenResource("mbox.resource")))
        {
            P("ddcprobe: no mbox.resource\n");
            goto out;
        }
        /* MBoxWrite truncates the buffer address to 32 bits, and the
         * tag buffer must own its cache lines - see <proto/mbox.h>. */
        if (!(mbbuf = AllocMem(MBOX_MSG_ALIGN + 256, MEMF_31BIT | MEMF_CLEAR)))
            goto out;
        mb = (ULONG *)(((IPTR)mbbuf + MBOX_MSG_ALIGN - 1) & ~(IPTR)(MBOX_MSG_ALIGN - 1));
        vcmb = periiobase + (is2712 ? VCMB_OFFSET_BCM2712 : VCMB_OFFSET);
    }
    else if (!is2712)
    {
        P("ddcprobe: the DDC controllers are BCM2712 only - use MBOX\n");
        goto out;
    }

    for (port = first; port <= last; port++)
    {
        BOOL connected = TRUE;

        cur_port = port;
        P("\nHDMI%d:\n", port);

        /* The HDMI and DDC blocks below are laid out for BCM2712 only. */
        if (is2712)
        {
            ULONG hpd = *(volatile ULONG *)(periiobase + hdmi_off[port] + HDMI_HOTPLUG);
            char name[16];

            connected = hpd & 1;
            bsc = periiobase + ddc_off[port];
            P("  HOTPLUG %08x (%s)\n", (unsigned)hpd, connected ? "connected" : "no sink");

            if (args[ARG_RAW] || (!args[ARG_EDID] && !use_mbox))
            {
                snprintf(name, sizeof(name), "DDC%d", port);
                dump_regs(name, bsc, BSC_SIZE);
            }
        }

        if (use_mbox)
            read_edid();
        else if (args[ARG_EDID] && connected)
            ddc_edid();
    }
    rc = RETURN_OK;

out:
    if (mbbuf)
        FreeMem(mbbuf, MBOX_MSG_ALIGN + 256);
    FreeArgs(rda);
    return rc;
}
