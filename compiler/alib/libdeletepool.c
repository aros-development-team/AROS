/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    Original version from libnix
    $Id$

    Desc:
    Lang: english
*/
#include "pool.h"

/*****************************************************************************

    NAME */
#include <clib/alib_protos.h>

	VOID LibDeletePool (

/*  SYNOPSIS */
	APTR pool)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	06.12.96 digulla Created after original from libnix

******************************************************************************/
{
#   define poolHeader ((POOL*)pool)

    if (SysBase->LibNode.lib_Version>=39)
	DeletePool(poolHeader);
    else
    {
	if (poolHeader != NULL)
	{
	    ULONG * pool,
		    size;

	    while ((pool = (ULONG *)RemHead (
		    (struct List *)&poolHeader->PuddleList)
		)!=NULL
	    )
	    {
		size = *--pool;
		FreeMem (pool, size);
	    }

	    FreeMem (poolHeader, sizeof (POOL));
	}
    }
} /* LibDeletePool */

