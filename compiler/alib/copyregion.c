/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: (AROS only) Graphics function CopyRegion()
    Lang: english
*/

#include <graphics/regions.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */
#include <proto/alib.h>

	struct Region *CopyRegion(

/*  SYNOPSIS */
	struct Region *region)

/*  FUNCTION
    	Make a copy of the given Region.

    INPUTS
	region - pointer to a Region structure

    RESULT
	the copy of the Region, or NULL if not enough memory.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	13-12-2000  stegerg implemented

*****************************************************************************/
{
    struct Region *nreg = NewRegion();
    
    if (nreg)
    {
    	if (OrRegionRegion(region, nreg))
    	    return nreg;

        DisposeRegion(nreg);
    }
    
    return NULL;
} /* CopyRegion */
