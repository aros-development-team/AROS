/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set one color register for this Viewport
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/view.h>
#include <proto/graphics.h>

	AROS_LH5(void, SetRGB4,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),
	AROS_LHA(ULONG            , n, D0),
	AROS_LHA(ULONG            , r, D1),
	AROS_LHA(ULONG            , g, D2),
	AROS_LHA(ULONG            , b, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 48, Graphics)

/*  FUNCTION
	Changes a single color of a viewport.

    INPUTS
	vp - Modify this viewport
	n - Change this color. If the color is outside the range of
		valid colors, it will be ignored.
	r, g, b - The new values for the red, green and blue. The
		valid range is from 0x0 (no intensity) to
		0xF (full intensity).

    RESULT
	If there is a ColorMap for this viewport, then the value will
	be stored in the ColorMap.
	The selected color register is changed to match your specs.
	If the color value is unused then nothing will happen.

    NOTES
	Lower order bits of the palette specification will be discarded,
	depending on the color palette resolution of the target graphics
	device. Use 0xf for the full value, 0x7 for 50%,
	etc. You can find out the palette range for your screen by
	querying the graphics data base.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!vp)
        return;

    SetRGB32(vp, n, r * 0x11111111,
    	    	    g * 0x11111111,
		    b * 0x11111111);

    /************************************************************
    / This is the code that works correctly on the real thing
    struct ColorMap * CM = vp->ColorMap;

    / is there a ColorMap connected to the ViewPort??  /
    if (NULL != CM)
    {
      struct View * MyView = ViewAddress();
      if(CM->Count > n)
      {
	WORD * RGBValues = CM->ColorTable;
	RGBValues[n] = ((r & 0x000f) << 8) +
                       ((g & 0x000f) << 4) + 
                	(b & 0x000f);
      }

      / Make the changes visible /
      MakeVPort(MyView, MyView->ViewPort);
      MrgCop(MyView);
      LoadView(MyView);
    }
    ************************************************************/

    AROS_LIBFUNC_EXIT
    
} /* SetRGB4 */
