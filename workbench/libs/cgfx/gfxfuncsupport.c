/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <clib/macros.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <proto/graphics.h>

#include "gfxfuncsupport.h"

/****************************************************************************************/

static const UBYTE rectfmt_bytes_per_pixel[] =
{
    3,    /* RECTFMT_RGB */
    4,    /* RECTFMT_RGBA */
    4,    /* RECTFMT_ARGB */
    1,    /* RECTFMT_LUT8 */
    1     /* RECTFMT_GREY8 */
};

static const UBYTE ext_rectfmt_bytes_per_pixel[] =
{
    2,    /* RECTFMT_RGB15 */
    2,    /* RECTFMT_BGR15 */
    2,    /* RECTFMT_RGB15PC */
    2,    /* RECTFMT_BGR15PC */
    2,    /* RECTFMT_RGB16 */
    2,    /* RECTFMT_BGR16 */
    2,    /* RECTFMT_RGB16PC */
    2,    /* RECTFMT_BGR16PC */
    0,
    3,    /* RECTFMT_BGR24 */
    0,
    4,    /* RECTFMT_BGRA32 */
    0,
    4,    /* RECTFMT_ABGR32 */
    4,    /* RECTFMT_0RGB32 */
    4,    /* RECTFMT_BGR032 */
    4,    /* RECTFMT_RGB032 */
    4     /* RECTFMT_0BGR32 */
};

static const HIDDT_StdPixFmt hidd_rectfmt[] =
{
    vHidd_StdPixFmt_RGB24,
    vHidd_StdPixFmt_RGBA32,
    vHidd_StdPixFmt_ARGB32,
    0,
    0,
    vHidd_StdPixFmt_Native
};

static const HIDDT_StdPixFmt ext_hidd_rectfmt[] =
{
    vHidd_StdPixFmt_RGB15,
    vHidd_StdPixFmt_BGR15,
    vHidd_StdPixFmt_RGB15_LE,
    vHidd_StdPixFmt_BGR15_LE,
    vHidd_StdPixFmt_RGB16,
    vHidd_StdPixFmt_BGR16,
    vHidd_StdPixFmt_RGB16_LE,
    vHidd_StdPixFmt_BGR16_LE,
    0,
    vHidd_StdPixFmt_BGR24,
    0,
    vHidd_StdPixFmt_BGRA32,
    0,
    vHidd_StdPixFmt_ABGR32,
    vHidd_StdPixFmt_0RGB32,
    vHidd_StdPixFmt_BGR032,
    vHidd_StdPixFmt_RGB032,
    vHidd_StdPixFmt_0BGR32
};

UBYTE GetRectFmtBytesPerPixel(UBYTE rectfmt, struct RastPort *rp,
    struct Library *CyberGfxBase)
{
    UBYTE result;
    OOP_Object *pf = 0;
    IPTR oop_result;

    if (rectfmt == RECTFMT_RAW)
    {
    	OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &oop_result);
        result = oop_result;
    }
    else if (rectfmt < RECTFMT_RGB15)
        result = rectfmt_bytes_per_pixel[rectfmt];
    else
        result = ext_rectfmt_bytes_per_pixel[rectfmt - RECTFMT_RGB15];

    return result;
}

HIDDT_StdPixFmt GetHIDDRectFmt(UBYTE rectfmt, struct RastPort *rp,
    struct Library *CyberGfxBase)
{
    HIDDT_StdPixFmt result;

    if (rectfmt < RECTFMT_RGB15)
        result = hidd_rectfmt[rectfmt];
    else
        result = ext_hidd_rectfmt[rectfmt - RECTFMT_RGB15];

    return result;
}
