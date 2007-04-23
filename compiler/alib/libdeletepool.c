/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    Original version from libnix
    $Id$
*/

#include "pool.h"

/*****************************************************************************

    NAME */
#include <proto/alib.h>

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
	    ULONG * poolMem,
		    size;

	    while ((poolMem = (ULONG *)RemHead (
		    (struct List *)&poolHeader->PuddleList)
		)!=NULL
	    )
	    {
		size = *--poolMem;
		FreeMem (poolMem, size);
	    }

	    FreeMem (poolHeader, sizeof (POOL));
	}
    }
} /* LibDeletePool */

