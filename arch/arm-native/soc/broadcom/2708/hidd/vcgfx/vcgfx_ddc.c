/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 HDMI DDC - the sink's EDID and SCDC registers.

    One Broadcom STB I2C controller per HDMI port. Layout checked on a
    Pi 500+ against what the firmware leaves behind after its own EDID
    read: 97.5 kHz, 32-bit little-endian data registers. Polled, and
    serialised by the caller.
*/

#define DEBUG 0
#include <aros/debug.h>

#include "vcgfx_hidd.h"
#include "vcgfx_ddc.h"

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

static inline ULONG ddc_rd(IPTR bsc, ULONG off)
{
    return *(volatile ULONG *)(bsc + off);
}

static inline void ddc_wr(IPTR bsc, ULONG off, ULONG v)
{
    *(volatile ULONG *)(bsc + off) = v;
}

/* One START..STOP transaction of at most DDC_CHUNK bytes, polled. */
static BOOL ddc_xfer(IPTR bsc, UBYTE slave, BOOL read, UBYTE *buf, ULONG len)
{
    ULONG i, en = 0, n;

    ddc_wr(bsc, BSC_IIC_ENABLE, 0);
    ddc_wr(bsc, BSC_CHIP_ADDRESS, (slave << 1) | (read ? 1 : 0));
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

/* The firmware talks to this controller too (SCDC at boot), so every
 * access leaves it set up as it was found. */
struct ddc_saved
{
    ULONG ctl, ctlhi, addr;
};

static IPTR ddc_begin(ULONG port, struct ddc_saved *sv)
{
    IPTR bsc = __arm_periiobase + DDC_OFF(port);

    sv->ctl   = ddc_rd(bsc, BSC_CTL);
    sv->ctlhi = ddc_rd(bsc, BSC_CTLHI);
    sv->addr  = ddc_rd(bsc, BSC_CHIP_ADDRESS);
    ddc_wr(bsc, BSC_CTLHI, sv->ctlhi | CTLHI_DATAREG_32);
    return bsc;
}

static void ddc_end(IPTR bsc, const struct ddc_saved *sv)
{
    ddc_wr(bsc, BSC_CTL, sv->ctl);
    ddc_wr(bsc, BSC_CTLHI, sv->ctlhi);
    ddc_wr(bsc, BSC_CHIP_ADDRESS, sv->addr);
}

BOOL vcgfx_ddc_connected(ULONG port)
{
    return (*(volatile ULONG *)(__arm_periiobase + HDMI_HOTPLUG(port)) & 1) != 0;
}

/* The sink's address pointer survives a STOP, so each chunk is an offset
 * write followed by a plain read - no repeated START needed. */
BOOL vcgfx_ddc_read(ULONG port, UBYTE slave, UBYTE offset, UBYTE *buf, ULONG len)
{
    struct ddc_saved sv;
    IPTR bsc = ddc_begin(port, &sv);
    ULONG done, n;
    BOOL ok = TRUE;

    for (done = 0; ok && (done < len); done += n)
    {
        UBYTE o = offset + done;

        n  = ((len - done) < DDC_CHUNK) ? (len - done) : DDC_CHUNK;
        ok = ddc_xfer(bsc, slave, FALSE, &o, 1) && ddc_xfer(bsc, slave, TRUE, buf + done, n);
    }

    ddc_end(bsc, &sv);
    return ok;
}

BOOL vcgfx_ddc_write(ULONG port, UBYTE slave, const UBYTE *buf, ULONG len)
{
    struct ddc_saved sv;
    IPTR bsc;
    BOOL ok;

    if (len > DDC_CHUNK)
        return FALSE;

    bsc = ddc_begin(port, &sv);
    ok  = ddc_xfer(bsc, slave, FALSE, (UBYTE *)buf, len);
    ddc_end(bsc, &sv);
    return ok;
}
