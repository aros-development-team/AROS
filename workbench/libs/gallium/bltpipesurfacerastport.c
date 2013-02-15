/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"
#include <graphics/rastport.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/gallium.h>
#include <gallium/pipe/p_state.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */

      AROS_LH8(void, BltPipeSurfaceRastPort,

/*  SYNOPSIS */ 
      AROS_LHA(struct pipe_surface * , srcPipeSurface, A0),
      AROS_LHA(LONG                  , xSrc, D0),
      AROS_LHA(LONG                  , ySrc, D1),
      AROS_LHA(struct RastPort *     , destRP, A1),
      AROS_LHA(LONG                  , xDest, D2),
      AROS_LHA(LONG                  , yDest, D3),
      AROS_LHA(LONG                  , xSize, D4),
      AROS_LHA(LONG                  , ySize, D5),

/*  LOCATION */
      struct Library *, GalliumBase, 7, Gallium)

/*  NAME
 
    FUNCTION
    Copies part of pipe surface onto rast port. Clips output by using layers of
    rastport.
 
    INPUTS
	srcPipeSurface - Copy from this pipe surface.
	xSrc, ySrc - This is the upper left corner of the area to copy.
	destRP - Destination RastPort.
	xDest, yDest - Upper left corner where to place the copy
	xSize, ySize - The size of the area to copy
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BltPipeResourceRastPort(srcPipeSurface->texture, xSrc, ySrc, destRP, xDest,
        yDest, xSize, ySize);
    
    AROS_LIBFUNC_EXIT
}
