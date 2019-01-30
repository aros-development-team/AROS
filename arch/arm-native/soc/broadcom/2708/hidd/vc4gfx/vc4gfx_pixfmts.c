/*
    Copyright � 2013-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <graphics/gfx.h>
#include <hidd/gfx.h>
#include <oop/oop.h>

#include "vc4gfx_hidd.h"

#define ARRAYSIZE_TRUECOLOR     13
#define ARRAYSIZE_LUT           15

#if defined(VC_FMT_32)
IPTR pftags_32bpp[ARRAYSIZE_TRUECOLOR] =
{
    ARRAYSIZE_TRUECOLOR,
    8,
    16,
    24,
    0,
    0x00FF0000,
    0x0000FF00,
    0x000000FF,
    0x00000000,
    32,
    4,
    32,
    vHidd_StdPixFmt_BGR032
};
#endif

#if defined(VC_FMT_24)
IPTR pftags_24bpp[ARRAYSIZE_TRUECOLOR] =
{
    ARRAYSIZE_TRUECOLOR,
    8,
    16,
    24,
    0,
    0x00FF0000,
    0x0000FF00,
    0x000000FF,
    0x00000000,
    24,
    3,
    24,
    vHidd_StdPixFmt_BGR24
};
#endif

#if defined(VC_FMT_16)
IPTR pftags_16bpp[ARRAYSIZE_TRUECOLOR] =
{
    ARRAYSIZE_TRUECOLOR,
    16,
    21,
    27,
    0,
    0x0000F800,
    0x000007E0,
    0x0000001F,
    0x00000000,
    16,
    2,
    16,
    vHidd_StdPixFmt_RGB16_LE
};
#endif

#if defined(VC_FMT_15)
IPTR pftags_15bpp[ARRAYSIZE_TRUECOLOR] =
{
    ARRAYSIZE_TRUECOLOR,
    17,
    22,
    27,
    0,
    0x00007C00,
    0x000003E0,
    0x0000001F,
    0x00000000,
    15,
    2,
    15,
    vHidd_StdPixFmt_RGB15_LE
};
#endif

#if defined(VC_FMT_8)
IPTR pftags_8bpp[ARRAYSIZE_LUT] =
{
    ARRAYSIZE_LUT,
    0,
    0,
    0,
    0,
    0x00FF0000,
    0x0000FF00,
    0x000000FF,
    0x00000000,
    0x000000FF,
    0,
    8,
    1,
    8,
    vHidd_StdPixFmt_LUT8
};
#endif

IPTR vc_fmts[6] =
{
#if defined(VC_FMT_32)
    (IPTR)pftags_32bpp,
#endif
#if defined(VC_FMT_24)
    (IPTR)pftags_24bpp,
#endif
#if defined(VC_FMT_16)
    (IPTR)pftags_16bpp,
#endif
#if defined(VC_FMT_15)
    (IPTR)pftags_15bpp,
#endif
#if defined(VC_FMT_8)
    (IPTR)pftags_8bpp,
#endif
    0
};

APTR FNAME_SUPPORT(GenPixFmts)(OOP_Class *cl)
{
    struct TagItem *pixfmtarray = NULL;
    IPTR  **supportedfmts = (IPTR**)vc_fmts;
    int fmtcount = 0;

    while (supportedfmts[fmtcount] != 0)
        fmtcount++;

    D(bug("[VideoCore] %s: Allocating storage for %d pixfmts\n", __PRETTY_FUNCTION__, fmtcount));

    if ((pixfmtarray = AllocVec((fmtcount + 1) * sizeof(struct TagItem), MEMF_PUBLIC)) != NULL)
    {
        for (fmtcount = 0; supportedfmts[fmtcount] != 0; fmtcount++)
        {
            struct TagItem *newfmt_tags = NULL;
            if (supportedfmts[fmtcount][0] == ARRAYSIZE_TRUECOLOR)
            {
                D(bug("[VideoCore] %s: %dbit TRUECOLOR pixfmt\n", __PRETTY_FUNCTION__, supportedfmts[fmtcount][9]));
                newfmt_tags = AllocVec((ARRAYSIZE_TRUECOLOR + 2) * sizeof(struct TagItem), MEMF_PUBLIC);
                newfmt_tags[0].ti_Tag = aHidd_PixFmt_RedShift;
                newfmt_tags[0].ti_Data = supportedfmts[fmtcount][1];
                newfmt_tags[1].ti_Tag = aHidd_PixFmt_GreenShift;
                newfmt_tags[1].ti_Data = supportedfmts[fmtcount][2];
                newfmt_tags[2].ti_Tag = aHidd_PixFmt_BlueShift;
                newfmt_tags[2].ti_Data = supportedfmts[fmtcount][3];
                newfmt_tags[3].ti_Tag = aHidd_PixFmt_AlphaShift;
                newfmt_tags[3].ti_Data = supportedfmts[fmtcount][4];
                newfmt_tags[4].ti_Tag = aHidd_PixFmt_RedMask;
                newfmt_tags[4].ti_Data = supportedfmts[fmtcount][5];
                newfmt_tags[5].ti_Tag = aHidd_PixFmt_GreenMask;
                newfmt_tags[5].ti_Data = supportedfmts[fmtcount][6];
                newfmt_tags[6].ti_Tag = aHidd_PixFmt_BlueMask;
                newfmt_tags[6].ti_Data = supportedfmts[fmtcount][7];
                newfmt_tags[7].ti_Tag = aHidd_PixFmt_AlphaMask;
                newfmt_tags[7].ti_Data = supportedfmts[fmtcount][8];
                newfmt_tags[8].ti_Tag = aHidd_PixFmt_ColorModel;
                newfmt_tags[8].ti_Data = vHidd_ColorModel_TrueColor;
                newfmt_tags[9].ti_Tag = aHidd_PixFmt_Depth;
                newfmt_tags[9].ti_Data = supportedfmts[fmtcount][9];
                newfmt_tags[10].ti_Tag = aHidd_PixFmt_BytesPerPixel;
                newfmt_tags[10].ti_Data = supportedfmts[fmtcount][10];
                newfmt_tags[11].ti_Tag = aHidd_PixFmt_BitsPerPixel;
                newfmt_tags[11].ti_Data = supportedfmts[fmtcount][11];
                newfmt_tags[12].ti_Tag = aHidd_PixFmt_StdPixFmt;
                newfmt_tags[12].ti_Data = supportedfmts[fmtcount][12];
                newfmt_tags[13].ti_Tag = aHidd_PixFmt_BitMapType;
                newfmt_tags[13].ti_Data = vHidd_BitMapType_Chunky;
                newfmt_tags[14].ti_Tag = TAG_DONE;
            }
            else if (supportedfmts[fmtcount][0] == ARRAYSIZE_LUT)
            {
                D(bug("[VideoCore] %s: %dbit LUT pixfmt\n", __PRETTY_FUNCTION__, supportedfmts[fmtcount][11]));
                newfmt_tags = AllocVec((ARRAYSIZE_LUT + 2) * sizeof(struct TagItem), MEMF_PUBLIC);
                newfmt_tags[0].ti_Tag = aHidd_PixFmt_RedShift;
                newfmt_tags[0].ti_Data = supportedfmts[fmtcount][1];
                newfmt_tags[1].ti_Tag = aHidd_PixFmt_GreenShift;
                newfmt_tags[1].ti_Data = supportedfmts[fmtcount][2];
                newfmt_tags[2].ti_Tag = aHidd_PixFmt_BlueShift;
                newfmt_tags[2].ti_Data = supportedfmts[fmtcount][3];
                newfmt_tags[3].ti_Tag = aHidd_PixFmt_AlphaShift;
                newfmt_tags[3].ti_Data = supportedfmts[fmtcount][4];
                newfmt_tags[4].ti_Tag = aHidd_PixFmt_RedMask;
                newfmt_tags[4].ti_Data = supportedfmts[fmtcount][5];
                newfmt_tags[5].ti_Tag = aHidd_PixFmt_GreenMask;
                newfmt_tags[5].ti_Data = supportedfmts[fmtcount][6];
                newfmt_tags[6].ti_Tag = aHidd_PixFmt_BlueMask;
                newfmt_tags[6].ti_Data = supportedfmts[fmtcount][7];
                newfmt_tags[7].ti_Tag = aHidd_PixFmt_AlphaMask;
                newfmt_tags[7].ti_Data = supportedfmts[fmtcount][8];
                newfmt_tags[8].ti_Tag = aHidd_PixFmt_CLUTMask;
                newfmt_tags[8].ti_Data = supportedfmts[fmtcount][9];
                newfmt_tags[9].ti_Tag = aHidd_PixFmt_CLUTShift;
                newfmt_tags[9].ti_Data = supportedfmts[fmtcount][10];
                newfmt_tags[10].ti_Tag = aHidd_PixFmt_ColorModel;
                newfmt_tags[10].ti_Data = vHidd_ColorModel_Palette;
                newfmt_tags[11].ti_Tag = aHidd_PixFmt_Depth;
                newfmt_tags[11].ti_Data = supportedfmts[fmtcount][11];
                newfmt_tags[12].ti_Tag = aHidd_PixFmt_BytesPerPixel;
                newfmt_tags[12].ti_Data = supportedfmts[fmtcount][12];
                newfmt_tags[13].ti_Tag = aHidd_PixFmt_BitsPerPixel;
                newfmt_tags[13].ti_Data = supportedfmts[fmtcount][13];
                newfmt_tags[14].ti_Tag = aHidd_PixFmt_StdPixFmt;
                newfmt_tags[14].ti_Data = supportedfmts[fmtcount][14];
                newfmt_tags[15].ti_Tag = aHidd_PixFmt_BitMapType;
                newfmt_tags[15].ti_Data = vHidd_BitMapType_Chunky;
                newfmt_tags[16].ti_Tag = TAG_DONE;
            }
            else
            {
                D(bug("[VideoCore] %s: ERROR - Unhandled pixfmt\n", __PRETTY_FUNCTION__));
                supportedfmts++;
                continue;
            }
            pixfmtarray[fmtcount].ti_Tag = aHidd_Gfx_PixFmtTags;
            pixfmtarray[fmtcount].ti_Data = (IPTR)newfmt_tags;
        }
        pixfmtarray[fmtcount].ti_Tag = TAG_DONE;
    }
#if defined(DEBUGPIXFMT)
    if (pixfmtarray)
    {
        struct TagItem *cur_pixfmt = pixfmtarray;
        while (cur_pixfmt->ti_Tag != TAG_DONE)
        {
            D(bug("[VideoCoreGfx] %s: 0x%p: %08x, %08x\n", __PRETTY_FUNCTION__, cur_pixfmt, cur_pixfmt->ti_Tag, cur_pixfmt->ti_Data));
            cur_pixfmt++;
        }
    }
#endif
    return (APTR)pixfmtarray;
}
