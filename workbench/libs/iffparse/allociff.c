/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH0(struct IFFHandle *, AllocIFF,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct Library *, IFFParseBase, 5, IFFParse)

/*  FUNCTION
      Allocates an IFFHandle struct.

    INPUTS


    RESULT
	An unitialized IFFHandle structure.

    NOTES
	The default context-node is created in AllocIFF() and persists until
	FreeIFF().

    EXAMPLE

    BUGS

    SEE ALSO
	FreeIFF()

    INTERNALS
	Since the default contextnode persistes from AllocIFF until FreeIFF, it
	is built-in into the internal IFFHandle structure

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntIFFHandle *intiff;

    DEBUG_ALLOCIFF(dprintf("AllocIff: entry\n"));

    if ( (intiff=(struct IntIFFHandle*)AllocMem (sizeof(struct IntIFFHandle),
	    MEMF_ANY|MEMF_CLEAR)
    ) )
    {

	GetIH(intiff)->iff_Flags = (0L|IFFF_READ);


	/* No need for buffering yet */
	intiff->iff_BufferStartDepth = 0;

	/* Initialize the context-stack list */
	NewList ((struct List*)&(intiff->iff_CNStack));


	/* Initialize the default context node */
	NewList ((struct List*)&(intiff->iff_DefaultCN.cn_LCIList));

	/* And add it to the stack */
	AddHead
	(
	    (struct List*)&(intiff->iff_CNStack),
	    (struct Node*)&(intiff->iff_DefaultCN)
	);

	/* Depth is 0 even if we have a default contextnode */
	GetIH(intiff)->iff_Depth = 0;

    }

    DEBUG_ALLOCIFF(dprintf("AllocIff: return %p\n", intiff));
    return ((struct IFFHandle*)intiff);

    AROS_LIBFUNC_EXIT
} /* AllocIFF */
