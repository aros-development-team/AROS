/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(LONG, StopChunks,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG             *, propArray, A1),
	AROS_LHA(LONG              , numPairs, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 22, IFFParse)

/*  FUNCTION
	Declares multiple stop chunks from th typ/id pairs supplied.

    INPUTS
      iff	- pointer to an IFFHandle struct.
	propArray  - pointer to an array of longword chunk types and identifiers.
	numPairs  - number of type/id pairs in the propArray.

    RESULT
	error	   - 0 if successfull, IFFERR_#? otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	StopChunk()

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

    for (count = 0; count < numPairs; count ++ )
    {
	if ((err = StopChunk(iff, propArray[0], propArray[1])))
	    return (err);
	propArray = &propArray[2];
    }
    return (NULL);

    AROS_LIBFUNC_EXIT
} /* StopChunks */
