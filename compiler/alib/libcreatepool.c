/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    Original version from libnix
    $Id$

    Desc:
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include "pool.h"

/*****************************************************************************

    NAME */
#include <clib/aros_protos.h>

	APTR LibCreatePool (

/*  SYNOPSIS */
	ULONG requirements,
	ULONG puddleSize,
	ULONG threshSize)

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
    if (SysBase->LibNode.lib_Version >= 39)
	return (CreatePool (requirements, puddleSize, threshSize));

    {
	POOL * pool = NULL;

	if (threshSize <= puddleSize)
	{
	    if ((pool = (POOL *)AllocMem (sizeof (POOL), MEMF_ANY)) != NULL)
	    {
		NEWLIST (&pool->PuddleList);

		puddleSize = ((puddleSize + 7) & ~7);

		pool->MemoryFlags = requirements;
		pool->PuddleSize  = puddleSize;
		pool->ThreshSize  = threshSize;
	    }
	}

	return (APTR)pool;
    }
} /* LibCreatePool */

