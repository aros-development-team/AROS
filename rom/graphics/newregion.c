/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH0(struct Region *, NewRegion,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct GfxBase *, GfxBase, 86, Graphics)

/*  FUNCTION
	Allocates memory for a new Region and initializes it
	to an empty Region.

    INPUTS

    RESULT
	region - pointer to a newly created Region structure that
		 should be freed by a call to DisposeRegion()

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	DisposeRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	15-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region* new = AllocMem(sizeof(struct Region), MEMF_ANY);
    if (new) {
	new->bounds.MinX = new->bounds.MaxX = 0;
	new->bounds.MinY = new->bounds.MaxY = 0;
	new->RegionRectangle = NULL;
    }
    return new;

    AROS_LIBFUNC_EXIT
} /* NewRegion */


