/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SetRGB4CM()
    Lang: english
*/
#include <graphics/view.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH5(void, SetRGB4CM,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm, A0),
	AROS_LHA(WORD 	          , n , D0),
	AROS_LHA(UBYTE            , r , D1),
	AROS_LHA(UBYTE            , g , D2),
	AROS_LHA(UBYTE            , b , D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 105, Graphics)

/*  FUNCTION
	Set one color in the ColorMap.

    INPUTS
	cm - ColorMap structure obtained via GetColorMap()
        n  - the number of the color register to set
	r  - red level   (0-15)
	g  - green level (0-15)
	b  - blue level  (0-15)
	
    RESULT
	Store the (r,g,b) triplet at index n in the ColorMap structure.
	The chnages will not be immediately displayed. Use this function
	before linking the ColorMap to a ViewPort.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetColorMap() SetRGB4() GetRGB4() graphics/view.h

    INTERNALS
	This function depends on the ColorMap->ColorTable structure

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  SetRGB32CM(cm, n, r * 0x11111111, g * 0x11111111, b * 0x11111111);

  AROS_LIBFUNC_EXIT
  
} /* SetRGB4CM */
