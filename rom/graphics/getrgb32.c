/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetRGB32()
    Lang: english
*/
#include <graphics/view.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH4(void, GetRGB32,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm        , A0),
	AROS_LHA(ULONG            , firstcolor, D0),
	AROS_LHA(ULONG            , ncolors   , D1),
	AROS_LHA(ULONG *          , table     , A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 150, Graphics)

/*  FUNCTION
	Fill the table with with the 32 bit fractional RGB values from the
	given colormap.

    INPUTS
	cm         - ColorMap structure obtained via GetColorMap()
	firstcolor - the index of first color register to get (starting with 0)
	ncolors    - the number of color registers to examine and write
	             into the table
	table      - a pointer to an array of 32 bit RGB triplets

    RESULT
	the ULONG pointed to by table will be filled with the 32 bit
	fractional RGB values from the colormap
	
    NOTES
	table should point to an array of at least 3*ncolors longwords.

    EXAMPLE

    BUGS

    SEE ALSO
	GetColorMap() FreeColorMap() SetRGB4() LoadRGB4()
	LoadRGB32() SetRGB32CM() graphics/view.h

    INTERNALS
	This function depends on the ColorMap->ColorTable structure

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    ULONG i,n;

    for (i = firstcolor, n = 0; i < (ncolors+firstcolor); i++ )
    {
        ULONG red, green, blue;
	
	color_get(cm, &red, &green, &blue, i);
	
	table[n++] = red;
	table[n++] = green;
	table[n++] = blue;
    }

    AROS_LIBFUNC_EXIT
    
} /* GetRGB32 */
