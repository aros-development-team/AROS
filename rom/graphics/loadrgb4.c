/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <proto/graphics.h>

	AROS_LH3(void, LoadRGB4,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),
	AROS_LHA(UWORD           *, colors, A1),
	AROS_LHA(LONG             , count, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 32, Graphics)

/*  FUNCTION
	Load RGB color values from table.

    INPUTS
	vp     - ViewPort
	colors - pointer to table of RGB values (0...15)
		 	background--  0x0RGB
			color1	  --  0x0RGB
			color2    --  0x0RGB
			...
	count	- number of UWORDs in the table
	
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	LoadRGB32()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG t;

    if (!vp)
        return;

    ASSERT_VALID_PTR(vp);
    ASSERT_VALID_PTR(colors);

    /* TODO: Optimization */

    for (t = 0; t < count; t ++ )
    {
    	ULONG red   = (colors[t] & 0xF00) >> 8;
	ULONG green = (colors[t] & 0x0F0) >> 4;
	ULONG blue  = (colors[t] & 0x00F);
	
	SetRGB32(vp, t, red   * 0x11111111,
	    	    	green * 0x11111111,
			blue  * 0x11111111);        
    }

    AROS_LIBFUNC_EXIT
    
} /* LoadRGB4 */
