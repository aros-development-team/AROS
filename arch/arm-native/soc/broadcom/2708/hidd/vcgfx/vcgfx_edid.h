#ifndef _VIDEOCOREGFX_EDID_H
#define _VIDEOCOREGFX_EDID_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

#define VCGFX_EDID_TIMINGS      32

#define VCGFX_TIMING_INTERLACE  (1 << 0)
#define VCGFX_TIMING_PHSYNC     (1 << 1)
#define VCGFX_TIMING_PVSYNC     (1 << 2)
#define VCGFX_TIMING_VIC        (1 << 3)        /* from the VIC list, not a DTD */

/* One detailed timing descriptor, clock in kHz. */
struct vcgfx_timing
{
    ULONG   clock;
    UWORD   hdisp, hstart, hend, htotal;
    UWORD   vdisp, vstart, vend, vtotal;
    UBYTE   flags;
};

/* What the driver keeps from the sink's EDID: detailed timings, then the
 * CTA formats it lists by VIC. timings[0] is the preferred mode when the
 * base block has one. */
struct vcgfx_edid
{
    BOOL                valid;
    char                name[14];
    UBYTE               vmin, vmax;     /* range limits, Hz  */
    ULONG               maxclock;       /* range limits, kHz */
    ULONG               maxtmds;        /* kHz, 0 = not stated */
    BOOL                scdc;
    UBYTE               ntimings;
    struct vcgfx_timing timings[VCGFX_EDID_TIMINGS];
};

struct VideoCoreGfx_staticdata;

void vcgfx_edid_probe(struct VideoCoreGfx_staticdata *xsd);
const struct vcgfx_timing *vcgfx_edid_find(struct VideoCoreGfx_staticdata *xsd,
                                           ULONG width, ULONG height);

#endif /* _VIDEOCOREGFX_EDID_H */
