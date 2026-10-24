/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Can a screen bitmap carry composited layer surfaces?
          AROS private, for layers.library.
*/

#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/gfx.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "graphics_display.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(BOOL, CompositeLayersSupported,

/*  SYNOPSIS */
        AROS_LHA(struct BitMap *, screenbm, A0),
        AROS_LHA(ULONG *, alphaformat, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 202, Graphics)

/*  FUNCTION
        Tells whether layers.library may give layers on this screen bitmap a
        surface of their own that the display composes onto the screen
        (LA_Composited).

    INPUTS
        screenbm    - the screen's bitmap
        alphaformat - may be NULL. Receives the cybergraphics PIXFMT_#? a
                      surface with per pixel alpha (LA_AlphaSurface) has to
                      be allocated in: the screen's own pixel layout with
                      its spare byte, or its alpha byte, as alpha channel.
                      Receives (ULONG)-1 when the screen's format has no
                      room for one (15, 16 and 24 bit, LUT): pens are mapped
                      for the screen's layout, so a surface in any other
                      layout would get them wrong.

    RESULT
        TRUE when the bitmap is a HIDD bitmap whose display composes its
        screens in software, i.e. it has a compositor that can present
        surfaces. FALSE for anything else (planar bitmaps, drivers that
        scan the screen bitmap out directly or compose in hardware).

    NOTES
        AROS private, for layers.library. Can be moved at any moment.

    SEE ALSO
        LayerSurfacesChanged()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct gfxdisplay_data *display;

    if (alphaformat)
        *alphaformat = (ULONG)-1;

    if (!screenbm || !IS_HIDD_BM(screenbm))
        return FALSE;

    display = (struct gfxdisplay_data *)HIDD_BM_DRVDATA(screenbm);
    if (!display || HIDD_BM_HIDDMODE(screenbm) == vHidd_ModeID_Invalid)
        return FALSE;

    if (!display_SoftComposes(display, HIDD_BM_HIDDMODE(screenbm), GfxBase))
        return FALSE;

    if (alphaformat)
    {
        OOP_Object *pf = NULL;
        IPTR stdpf = vHidd_StdPixFmt_Unknown;

        OOP_GetAttr(HIDD_BM_OBJ(screenbm), aHidd_BitMap_PixFmt, (IPTR *)&pf);
        if (pf)
            OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);

        switch (stdpf)
        {
        case vHidd_StdPixFmt_ARGB32:
        case vHidd_StdPixFmt_0RGB32: *alphaformat = PIXFMT_ARGB32; break;
        case vHidd_StdPixFmt_BGRA32:
        case vHidd_StdPixFmt_BGR032: *alphaformat = PIXFMT_BGRA32; break;
        case vHidd_StdPixFmt_RGBA32:
        case vHidd_StdPixFmt_RGB032: *alphaformat = PIXFMT_RGBA32; break;
        case vHidd_StdPixFmt_ABGR32:
        case vHidd_StdPixFmt_0BGR32: *alphaformat = PIXFMT_ABGR32; break;
        default: break;
        }
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* CompositeLayersSupported */
