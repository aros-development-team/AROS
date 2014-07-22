/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SetRGB32CM()
    Lang: english
*/
#include <graphics/view.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH5(void, SetRGB32CM,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm, A0),
	AROS_LHA(ULONG            , n , D0),
	AROS_LHA(ULONG            , r , D1),
	AROS_LHA(ULONG            , g , D2),
	AROS_LHA(ULONG            , b , D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 166, Graphics)

/*  FUNCTION
	Set one color in the ColorMap.

    INPUTS
	cm - ColorMap structure obtained via GetColorMap()
        n  - the number of the color register to set
	r  - red level   (32 bit left justified fraction)
	g  - green level (32 bit left justified fraction)
	b  - blue level  (32 bit left justified fraction)
	
    RESULT
	Store the (r,g,b) triplet at index n in the ColorMap structure.
	The changes will not be immediately displayed. Use this function
	before linking the ColorMap to a ViewPort. Do not access the entries
	in the ColorTable yourself, as the ColorTable format is subject to
	change.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetColorMap(), SetRGB32(), GetRGB32(), SetRGB4CM(), graphics/view.h

    INTERNALS
	This function depends on the ColorMap->ColorTable structure

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (NULL != cm && n < cm->Count)
    {
	/* Preserve the highest nibble. Needed for interoperability
	   with m68k graphics.library. Exact purpose is currently
	   unknown - sonic */
        UWORD a = cm->ColorTable[n];
	
        cm->ColorTable[n] = (a & 0xF000) |
            ((r >> 20) & 0x0f00) |
            ((g >> 24) & 0x00f0) |
            ((b >> 28) & 0x000f);

        if (cm->Type > COLORMAP_TYPE_V1_2)
            cm->LowColorBits[n] = ((r >> 16) & 0x0f00) |
                ((g >> 20) & 0x00f0) |
                ((b >> 24) & 0x000f);
    }

    AROS_LIBFUNC_EXIT
    
} /* SetRGB32CM */
