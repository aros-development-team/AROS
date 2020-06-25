/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include "hpet_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hpet.h>

	AROS_LH1(IPTR, AllocTSUnit,

/*  SYNOPSIS */
	AROS_LHA(const struct Node *, owner, A0),

/*  LOCATION */
	struct HPETBase *, base, 2, Hpet)

/*  FUNCTION
	Allocate a free HPET timer for use.

    INPUTS
	owner - a Node specifying the consumer of the time source. Can not be NULL.

    RESULT
	An opaque handle for the HPET timer unit allocated for exclusive use, or -1 if
	there was no free HPET.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG i;

    if (owner)
    {
        ObtainSemaphore(&base->lock);

        for (i = 0; i < base->unitCnt; i++)
        {
            if (!base->units[i].Owner)
            {
                base->units[i].Owner = owner;

                ReleaseSemaphore(&base->lock);
                return i;
            }
        }
        
        ReleaseSemaphore(&base->lock);
    }
    return (IPTR)-1;

    AROS_LIBFUNC_EXIT
}
