/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function ScrollRasterBF()
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

        AROS_LH7(void, ScrollRasterBF,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),
        AROS_LHA(WORD             , dx, D0),
        AROS_LHA(WORD             , dy, D1),
        AROS_LHA(WORD             , xMin, D2),
        AROS_LHA(WORD             , yMin, D3),
        AROS_LHA(WORD             , xMax, D4),
        AROS_LHA(WORD             , yMax, D5),

/*  LOCATION */
        struct GfxBase *, GfxBase, 167, Graphics)

/*  FUNCTION
      Scroll the contents of a rastport (dx,dy) towards (0,0).
      The empty spaces is filled by a call to EraseRect().
      Only the pixel in the rectangle (xMin,yMin)-(xMax,yMax)
      will be affected. The lower right corner (xMax, yMax) is
      automatically adjusted to the lower right corner in case
      it would be outside.
      After this operation the Flags bit of the layer associated
      with this rastport, if there is any layer, should be tested
      for simple layers in case there has any damage been created.


    INPUTS
      rp    - pointer to rastport
      dx,dy - distance to move in x and y direction. Positive values go
              towards (0,0)
      xMin,yMin - upper left  hand corner of the affected rectangle
      xMax,yMax - lower right hand corner of the affected rectangle

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FIX_GFXCOORD(xMin);
    FIX_GFXCOORD(yMin);
    FIX_GFXCOORD(xMax);
    FIX_GFXCOORD(yMax);

    internal_ScrollRaster(rp, dx, dy, xMin, yMin, xMax, yMax, TRUE, GfxBase);

    AROS_LIBFUNC_EXIT
} /* ScrollRasterBF */
