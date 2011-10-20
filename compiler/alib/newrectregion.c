/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function NewRectRegion()
    Lang: english
*/

#include <graphics/regions.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */
#include <proto/alib.h>

	struct Region *NewRectRegion(

/*  SYNOPSIS */
    	WORD MinX,
	WORD MinY,
	WORD MaxX,
	WORD MaxY)

/*  FUNCTION
    	Creates a new rectangular Region
		
    INPUTS
    	MinX, MinY, MaxX, MaxY - The extent of the Rect
		
    RESULT
    	Pointer to the newly created Region. NULL on failure.

    NOTES
	This function is a shorthand for:

	    struct Rectangle rect;
	    struct Region *region;

	    rect.MinX = MinX;
	    rect.MinY = MinY;
	    rect.MaxX = MaxX;
	    rect.MaxY = MaxY;

	    region = NewRegion();
	    OrRectRegion(region, &rect);

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	15-12-2000  stegerg implemented

*****************************************************************************/
{
    struct Region *region = NewRegion();
    
    if (region)
    {
    	struct Rectangle rect = {MinX, MinY, MaxX, MaxY};
    	BOOL res = OrRectRegion(region, &rect);

	if (res)
	    return region;

	DisposeRegion(region);
    }

    return NULL;

} /* NewRectRegion */
