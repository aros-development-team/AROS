/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <proto/exec.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

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

    struct Region *new;

#if REGIONS_USE_MEMPOOL
    ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);
    new = AllocPooled(PrivGBase(GfxBase)->regionpool, sizeof(struct Region));
    ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);
#else    
    new = AllocMem(sizeof(struct Region), MEMF_ANY);
#endif
 
    if (new) InitRegion(new);

    return new;

    AROS_LIBFUNC_EXIT
    
} /* NewRegion */


