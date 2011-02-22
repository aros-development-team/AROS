/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(LONG, PropChunks,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(const LONG       *, propArray, A1),
	AROS_LHA(LONG              , numPairs, D0),

/*  LOCATION */
    struct Library *, IFFParseBase, 20, IFFParse)

/*  FUNCTION
	Does multiple PropChunk() calls on the supplied list.
	An easy way to install several prop chunk handlers


    INPUTS
	iff	  - pointer to an IFFHandle struct.
	propArray  - pointer to an array of longword chunk types and identifiers.
	numPairs  - number of type/id pairs in the propArray.


    RESULT
	error	   - 0 if successfull, IFFERR_#? otherwise.


    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG count;
    LONG err;

    DEBUG_PROPCHUNKS(dprintf("PropChunks: iff 0x%lx array 0x%lx num %d\n",
			    iff, propArray, numPairs));

#if DEBUG
    {
    LONG * lptr = propArray;
    bug ("PropChunks (iff=%p, [\n", iff);
    for (count = 0; count < numPairs; count++)
    {
	bug ("    {%c%c%c%c,%c%c%c%c}, ",
	    dmkid(lptr[0]),
	    dmkid(lptr[1])
	);
	lptr += 2;
    }
    bug ("    ])\n");
    }
#endif

    for (count = 0; count < numPairs; count ++ )
    {
	if ((err = PropChunk(iff, propArray[0], propArray[1])))
	    ReturnInt("PropChunks",LONG,err);

	propArray += 2;
    }

    ReturnInt("PropChunks",LONG,0L);
    AROS_LIBFUNC_EXIT
} /* PropChunks */
