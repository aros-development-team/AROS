/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#define DEBUG 0
#include <aros/debug.h>
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(LONG, CollectionChunks,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG             *, propArray, A1),
	AROS_LHA(LONG              , numPairs, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 24, IFFParse)

/*  FUNCTION
	Does multiple CollectionChunk() calls on the supplied list.
	An easy way to install several collction chunks.

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
	CollectionChunk()

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    LONG count;
    LONG err;

#if DEBUG
    LONG * lptr = propArray;
    bug ("CollectionChunks (iff=%p, [\n", iff);
    for (count = 0; count < numPairs; count++)
    {
	bug ("    {%c%c%c%c,%c%c%c%c}, ",
	    *lptr>>24, *lptr>>16, *lptr>>8, *lptr,
	    lptr[1]>>24, lptr[1]>>16, lptr[1]>>8, lptr[1]
	);
	lptr += 2;
    }
    bug ("    ])\n");
#endif

    for (count = 0; count < numPairs; count ++ )
    {
	if ((err = CollectionChunk(iff, propArray[0], propArray[1])))
	    return (err);
	propArray += 2;
    }

    ReturnInt("CollectionChunks",LONG,0L);
    AROS_LIBFUNC_EXIT
} /* CollectionChunks */
