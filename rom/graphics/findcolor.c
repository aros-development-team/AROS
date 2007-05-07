/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Find the closest matching color in a colormap
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/view.h>

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH5(ULONG, FindColor,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm    , A3),
	AROS_LHA(ULONG            , r     , D1),
	AROS_LHA(ULONG            , g     , D2),
	AROS_LHA(ULONG            , b     , D3),
	AROS_LHA(ULONG            , maxpen, D4),

/*  LOCATION */
	struct GfxBase *, GfxBase, 168, Graphics)

/*  FUNCTION
        Find the closest matching color in the given colormap.

    INPUTS
        cm - colormap structure
        r  - red level   (32 bit left justified fraction)
        g  - green level (32 bit left justified fraction)
        b  - blue level  (32 bit left justified fraction)
        maxpen - the maximum entry in the color table to search.

    RESULT
        The pen number with the closest match will be returned.
        No new pens are allocated and therefore the returned pen
        should not be released via ReleasePen().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG retval = 0;

    if (NULL != cm)
    {
	ULONG index = 0;
	ULONG best_distance = (ULONG)-1;
	ULONG distance;

	if (-1 == maxpen && NULL != cm->PalExtra)
	{
	    /* pe_SharableColors is not the number of colors but the last color index! */
	    maxpen = cm->PalExtra->pe_SharableColors;
	}
	
	if (maxpen >= cm->Count) maxpen = cm->Count - 1;
	
	while (index <= maxpen)
	{
            distance = color_distance(cm,r,g,b,index);
            if (distance < best_distance)
            {
                best_distance = distance;
                retval = index;
            }
            index++;
	} 
    }

    return retval;

    AROS_LIBFUNC_EXIT
} /* FindColor */
