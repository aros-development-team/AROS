/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Read the sink's EDID - from the firmware mailbox on BCM283x and
          BCM2711, from the HDMI DDC controller on BCM2712, where the
          firmware's GETEDID tag answers 0x80000001.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/macros.h>
#include <proto/exec.h>
#include <proto/mbox.h>
#include <string.h>

#include "vcgfx_hidd.h"

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      xsd->vcsd_MBoxBase

/*
 * BCM2712 DDC: one Broadcom STB I2C controller per HDMI port. Layout
 * checked on a Pi 500+ against what the firmware leaves behind after
 * its own EDID read: 97.5 kHz, 32-bit little-endian data registers.
 */
#define DDC_OFF(port)           (0x1508200 + (port) * 0x80)
#define HDMI_HOTPLUG(port)      (((port) ? 0x706400 : 0x701400) + 0x1c8)

#define BSC_CHIP_ADDRESS        0x00
#define BSC_DATA_IN(i)          (0x04 + 4 * (i))        /* sent to the sink */
#define BSC_CNT                 0x24
#define BSC_CTL                 0x28
#define BSC_IIC_ENABLE          0x2c
#define BSC_DATA_OUT(i)         (0x30 + 4 * (i))        /* read back */
#define BSC_CTLHI               0x50

#define CTL_DTF_WR              0x00
#define CTL_DTF_RD              0x01
#define CTL_97K5                0x90                    /* SCL_SEL 1 | DIV_CLK */
#define EN_ENABLE               (1 << 0)
#define EN_INTRP                (1 << 1)
#define EN_NOACK                (1 << 2)
#define CTLHI_DATAREG_32        (1 << 6)

#define DDC_CHUNK               32
#define DDC_POLL_LIMIT          2000000
#define EDID_ADDR               (0x50 << 1)

static inline ULONG ddc_rd(IPTR bsc, ULONG off)
{
    return *(volatile ULONG *)(bsc + off);
}

static inline void ddc_wr(IPTR bsc, ULONG off, ULONG v)
{
    *(volatile ULONG *)(bsc + off) = v;
}

/* One START..STOP transaction of at most DDC_CHUNK bytes, polled. */
static BOOL ddc_xfer(IPTR bsc, BOOL read, UBYTE *buf, ULONG len)
{
    ULONG i, en = 0, n;

    ddc_wr(bsc, BSC_IIC_ENABLE, 0);
    ddc_wr(bsc, BSC_CHIP_ADDRESS, EDID_ADDR | (read ? 1 : 0));
    ddc_wr(bsc, BSC_CNT, len);
    ddc_wr(bsc, BSC_CTL, CTL_97K5 | (read ? CTL_DTF_RD : CTL_DTF_WR));

    if (!read)
    {
        ULONG w[DDC_CHUNK / 4] = { 0 };

        for (i = 0; i < len; i++)
            w[i >> 2] |= (ULONG)buf[i] << ((i & 3) * 8);
        for (i = 0; i < DDC_CHUNK / 4; i++)
            ddc_wr(bsc, BSC_DATA_IN(i), w[i]);
    }

    ddc_wr(bsc, BSC_IIC_ENABLE, EN_ENABLE);
    for (n = 0; n < DDC_POLL_LIMIT; n++)
        if ((en = ddc_rd(bsc, BSC_IIC_ENABLE)) & EN_INTRP)
            break;
    ddc_wr(bsc, BSC_IIC_ENABLE, 0);

    if ((n == DDC_POLL_LIMIT) || (en & EN_NOACK))
        return FALSE;

    if (read)
        for (i = 0; i < len; i++)
            buf[i] = ddc_rd(bsc, BSC_DATA_OUT(i >> 2)) >> ((i & 3) * 8);

    return TRUE;
}

/* The EDID's address pointer survives a STOP, so each chunk is an offset
 * write followed by a plain read. Blocks past 1 need the E-DDC segment
 * pointer, which this does not do. */
static BOOL ddc_block(IPTR bsc, ULONG block, UBYTE *buf)
{
    ULONG done;

    for (done = 0; done < 128; done += DDC_CHUNK)
    {
        UBYTE o = block * 128 + done;

        if (!ddc_xfer(bsc, FALSE, &o, 1) || !ddc_xfer(bsc, TRUE, buf + done, DDC_CHUNK))
            return FALSE;
    }
    return TRUE;
}

static BOOL mbox_block(struct VideoCoreGfx_staticdata *xsd, ULONG block, UBYTE *buf)
{
    unsigned int *m = xsd->vcsd_MBoxMessage;
    BOOL ok;
    ULONG i;

    VC4_MBOX_LOCK(xsd);
    m[0] = AROS_LONG2LE(42 * 4);
    m[1] = AROS_LONG2LE(VCTAG_REQ);
    m[2] = AROS_LONG2LE(VCTAG_GETEDID);
    m[3] = AROS_LONG2LE(136);
    m[4] = 0;
    m[5] = AROS_LONG2LE(block);
    for (i = 6; i < 42; i++)
        m[i] = 0;

    /* Answers block, status (0 = ok), then the 128 bytes. */
    ok = (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, m) != (volatile unsigned int *)-1)
         && (m[1] == AROS_LONG2LE(VCTAG_RESP))
         && (AROS_LE2LONG(m[4]) & 0x80000000)
         && (m[6] == 0);
    if (ok)
        CopyMem(&m[7], buf, 128);
    VC4_MBOX_UNLOCK(xsd);

    return ok;
}

static BOOL edid_sum_ok(const UBYTE *b)
{
    UBYTE s = 0;
    ULONG i;

    for (i = 0; i < 128; i++)
        s += b[i];
    return s == 0;
}

static void parse_dtd(struct vcgfx_edid *e, const UBYTE *d)
{
    struct vcgfx_timing *t;
    ULONG hso, hsw, vso, vsw;
    ULONG i;

    if (d[0] || d[1])
    {
        if (e->ntimings == VCGFX_EDID_TIMINGS)
            return;
        t = &e->timings[e->ntimings++];

        hso = d[8] | ((d[11] & 0xc0) << 2);
        hsw = d[9] | ((d[11] & 0x30) << 4);
        vso = (d[10] >> 4) | ((d[11] & 0x0c) << 2);
        vsw = (d[10] & 0x0f) | ((d[11] & 0x03) << 4);

        t->clock  = (d[0] | (d[1] << 8)) * 10;
        t->hdisp  = d[2] | ((d[4] & 0xf0) << 4);
        t->htotal = t->hdisp + (d[3] | ((d[4] & 0x0f) << 8));
        t->hstart = t->hdisp + hso;
        t->hend   = t->hstart + hsw;
        t->vdisp  = d[5] | ((d[7] & 0xf0) << 4);
        t->vtotal = t->vdisp + (d[6] | ((d[7] & 0x0f) << 8));
        t->vstart = t->vdisp + vso;
        t->vend   = t->vstart + vsw;
        t->flags  = ((d[17] & 0x80) ? VCGFX_TIMING_INTERLACE : 0)
                  | ((d[17] & 0x02) ? VCGFX_TIMING_PHSYNC : 0)
                  | ((d[17] & 0x04) ? VCGFX_TIMING_PVSYNC : 0);
        return;
    }

    switch (d[3])
    {
    case 0xfc:
        for (i = 0; i < 13 && d[5 + i] != 0x0a; i++)
            e->name[i] = d[5 + i];
        e->name[i] = 0;
        break;

    case 0xfd:
        e->vmin     = d[5];
        e->vmax     = d[6];
        e->maxclock = d[9] * 10000;
        break;
    }
}

static void parse_cta(struct vcgfx_edid *e, const UBYTE *b)
{
    ULONG i, end = b[2];

    if (end < 4)
        return;

    for (i = 4; i < end && i < 127; i += (b[i] & 0x1f) + 1)
    {
        ULONG len = b[i] & 0x1f, oui;
        const UBYTE *p = b + i + 1;

        /* Vendor-specific blocks: HDMI 1.x and HDMI Forum. */
        if ((b[i] >> 5) != 3 || len < 3)
            continue;
        oui = p[0] | (p[1] << 8) | (p[2] << 16);

        if ((oui == 0x000c03) && (len >= 7) && !e->maxtmds)
            e->maxtmds = p[6] * 5000;
        else if ((oui == 0xc45dd8) && (len >= 6))
        {
            e->maxtmds = p[4] * 5000;
            e->scdc    = (p[5] & 0x80) ? TRUE : FALSE;
        }
    }

    for (i = end; i + 18 <= 127 && (b[i] || b[i + 1]); i += 18)
        parse_dtd(e, b + i);
}

void vcgfx_edid_probe(struct VideoCoreGfx_staticdata *xsd)
{
    struct vcgfx_edid *e = &xsd->vcsd_EDID;
    UBYTE blk[128];
    IPTR bsc = 0;
    ULONG i, n, port, ctl = 0, ctlhi = 0, addr = 0;
    BOOL ok;

    memset(e, 0, sizeof(*e));

    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
    {
        for (port = 0; port < 2; port++)
            if (*(volatile ULONG *)(__arm_periiobase + HDMI_HOTPLUG(port)) & 1)
                break;
        if (port == 2)
        {
            bug("[VideoCoreGfx] EDID: no sink on either HDMI port\n");
            return;
        }

        /* The firmware talks to this controller too (SCDC), so leave it
         * as it was found. */
        bsc   = __arm_periiobase + DDC_OFF(port);
        ctl   = ddc_rd(bsc, BSC_CTL);
        ctlhi = ddc_rd(bsc, BSC_CTLHI);
        addr  = ddc_rd(bsc, BSC_CHIP_ADDRESS);
        ddc_wr(bsc, BSC_CTLHI, ctlhi | CTLHI_DATAREG_32);
    }

    for (n = 0; n < 2; n++)
    {
        ok = bsc ? ddc_block(bsc, n, blk) : mbox_block(xsd, n, blk);
        if (!ok || !edid_sum_ok(blk))
            break;

        if (n == 0)
        {
            if (memcmp(blk, "\x00\xff\xff\xff\xff\xff\xff\x00", 8))
                break;
            e->valid = TRUE;
            for (i = 54; i < 126; i += 18)
                parse_dtd(e, blk + i);
            if (!blk[126])
                break;
        }
        else if (blk[0] == 0x02)
            parse_cta(e, blk);
    }

    if (bsc)
    {
        ddc_wr(bsc, BSC_CTL, ctl);
        ddc_wr(bsc, BSC_CTLHI, ctlhi);
        ddc_wr(bsc, BSC_CHIP_ADDRESS, addr);
    }

    if (!e->valid)
    {
        bug("[VideoCoreGfx] EDID: not available\n");
        return;
    }

    if (e->ntimings)
    {
        const struct vcgfx_timing *t = &e->timings[0];
        ULONG mhz = (UQUAD)t->clock * 1000000 / (t->htotal * t->vtotal);

        if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
            bug("[VideoCoreGfx] EDID: \"%s\" %ux%u@%u.%03u (%u kHz), max TMDS %u MHz%s\n",
                e->name, t->hdisp, t->vdisp, mhz / 1000, mhz % 1000, t->clock,
                e->maxtmds / 1000, e->scdc ? ", SCDC" : "");
        else
            D(bug("[VideoCoreGfx] EDID: \"%s\" %ux%u@%u.%03u (%u kHz), max TMDS %u MHz%s\n",
                e->name, t->hdisp, t->vdisp, mhz / 1000, mhz % 1000, t->clock,
                e->maxtmds / 1000, e->scdc ? ", SCDC" : ""));
    }
}

/* First progressive detailed timing of this size, or NULL. */
const struct vcgfx_timing *vcgfx_edid_find(struct VideoCoreGfx_staticdata *xsd,
                                           ULONG width, ULONG height)
{
    struct vcgfx_edid *e = &xsd->vcsd_EDID;
    ULONG i;

    for (i = 0; i < e->ntimings; i++)
        if ((e->timings[i].hdisp == width) && (e->timings[i].vdisp == height)
            && !(e->timings[i].flags & VCGFX_TIMING_INTERLACE))
            return &e->timings[i];

    return NULL;
}
