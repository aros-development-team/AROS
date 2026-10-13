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
#include "vcgfx_ddc.h"

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      xsd->vcsd_MBoxBase

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

#define P_HV (VCGFX_TIMING_PHSYNC | VCGFX_TIMING_PVSYNC)

/* CTA-861 progressive formats a VIC can name, clock in kHz. */
static const struct { UBYTE vic; struct vcgfx_timing t; } cta_vics[] =
{
    {  1, {  25175,  640,  656,  752,  800,  480,  490,  492,  525, 0    } },
    {  2, {  27000,  720,  736,  798,  858,  480,  489,  495,  525, 0    } },
    {  3, {  27000,  720,  736,  798,  858,  480,  489,  495,  525, 0    } },
    {  4, {  74250, 1280, 1390, 1430, 1650,  720,  725,  730,  750, P_HV } },
    { 16, { 148500, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125, P_HV } },
    { 17, {  27000,  720,  732,  796,  864,  576,  581,  586,  625, 0    } },
    { 18, {  27000,  720,  732,  796,  864,  576,  581,  586,  625, 0    } },
    { 19, {  74250, 1280, 1720, 1760, 1980,  720,  725,  730,  750, P_HV } },
    { 31, { 148500, 1920, 2448, 2492, 2640, 1080, 1084, 1089, 1125, P_HV } },
    { 32, {  74250, 1920, 2558, 2602, 2750, 1080, 1084, 1089, 1125, P_HV } },
    { 33, {  74250, 1920, 2448, 2492, 2640, 1080, 1084, 1089, 1125, P_HV } },
    { 34, {  74250, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125, P_HV } },
    { 47, { 148500, 1280, 1390, 1430, 1650,  720,  725,  730,  750, P_HV } },
    { 63, { 297000, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125, P_HV } },
    { 64, { 297000, 1920, 2448, 2492, 2640, 1080, 1084, 1089, 1125, P_HV } },
    { 93, { 297000, 3840, 5116, 5204, 5500, 2160, 2168, 2178, 2250, P_HV } },
    { 94, { 297000, 3840, 4896, 4984, 5280, 2160, 2168, 2178, 2250, P_HV } },
    { 95, { 297000, 3840, 4016, 4104, 4400, 2160, 2168, 2178, 2250, P_HV } },
    { 96, { 594000, 3840, 4896, 4984, 5280, 2160, 2168, 2178, 2250, P_HV } },
    { 97, { 594000, 3840, 4016, 4104, 4400, 2160, 2168, 2178, 2250, P_HV } },
};

static void add_vic(struct vcgfx_edid *e, UBYTE svd)
{
    /* SVD values 129-192 are VICs 1-64 flagged native. */
    UBYTE vic = ((svd >= 129) && (svd <= 192)) ? (svd & 0x7f) : svd;
    ULONG i;

    for (i = 0; i < sizeof(cta_vics) / sizeof(cta_vics[0]); i++)
        if ((cta_vics[i].vic == vic) && (e->ntimings < VCGFX_EDID_TIMINGS))
        {
            e->timings[e->ntimings] = cta_vics[i].t;
            e->timings[e->ntimings++].flags |= VCGFX_TIMING_VIC;
            return;
        }
}

static void parse_cta(struct vcgfx_edid *e, const UBYTE *b)
{
    ULONG i, j, end = b[2];

    if (end < 4)
        return;

    for (i = 4; i < end && i < 127; i += (b[i] & 0x1f) + 1)
    {
        ULONG len = b[i] & 0x1f, oui;
        const UBYTE *p = b + i + 1;

        /* Video data block: the formats the sink lists by VIC. */
        if ((b[i] >> 5) == 2)
        {
            for (j = 0; j < len; j++)
                add_vic(e, p[j]);
            continue;
        }

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
    ULONG i, n, port = 0;
    BOOL ddc = FALSE, ok;

    memset(e, 0, sizeof(*e));

    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
    {
        for (port = 0; port < 2; port++)
            if (vcgfx_ddc_connected(port))
                break;
        if (port == 2)
        {
            bug("[VideoCoreGfx] EDID: no sink on either HDMI port\n");
            return;
        }
        ddc = TRUE;
    }

    for (n = 0; n < 2; n++)
    {
        /* Blocks past 1 need the E-DDC segment pointer, not done here. */
        ok = ddc ? vcgfx_ddc_read(port, VCGFX_DDC_EDID, n * 128, blk, 128)
                 : mbox_block(xsd, n, blk);
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

/* First progressive detailed timing of this size, or NULL. VIC formats
 * do not count: a sink may list one size at several rates. */
const struct vcgfx_timing *vcgfx_edid_find(struct VideoCoreGfx_staticdata *xsd,
                                           ULONG width, ULONG height)
{
    struct vcgfx_edid *e = &xsd->vcsd_EDID;
    ULONG i;

    for (i = 0; i < e->ntimings; i++)
        if ((e->timings[i].hdisp == width) && (e->timings[i].vdisp == height)
            && !(e->timings[i].flags & (VCGFX_TIMING_INTERLACE | VCGFX_TIMING_VIC)))
            return &e->timings[i];

    return NULL;
}

/* The CEA format a timing is, if any: its VIC, and the picture aspect for
 * the AVI infoframe (1 = 4:3, 2 = 16:9). 0 for anything else. */
UBYTE vcgfx_edid_vic(const struct vcgfx_timing *t, UBYTE *aspect)
{
    const struct vcgfx_timing *v;
    ULONG i;

    for (i = 0; i < sizeof(cta_vics) / sizeof(cta_vics[0]); i++)
    {
        v = &cta_vics[i].t;
        if ((v->clock == t->clock) && (v->hdisp == t->hdisp) && (v->hstart == t->hstart)
            && (v->hend == t->hend) && (v->htotal == t->htotal) && (v->vdisp == t->vdisp)
            && (v->vstart == t->vstart) && (v->vend == t->vend) && (v->vtotal == t->vtotal))
        {
            *aspect = ((cta_vics[i].vic == 1) || (cta_vics[i].vic == 2)
                       || (cta_vics[i].vic == 17)) ? 1 : 2;
            return cta_vics[i].vic;
        }
    }

    *aspect = 0;
    return 0;
}
