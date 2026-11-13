/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.
    Original version from libnix
*/

#define AROS_LIBREQ(base,ver)   /* We test for versions manually */

#include "pool.h"

/*****************************************************************************

    NAME */
#include <proto/alib.h>

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

