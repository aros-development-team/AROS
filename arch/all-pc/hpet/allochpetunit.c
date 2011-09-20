#include <proto/exec.h>

#include "hpet_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hpet.h>

AROS_LH1(ULONG, AllocHPETUnit,

/*  SYNOPSIS */
	AROS_LHA(const char *, user, A0),

/*  LOCATION */
	struct HPETBase *, base, 2, Hpet)

/*  FUNCTION
	Allocate a free HPET timer for use.

    INPUTS
	user - a string specifying the name of current user. Can not be NULL.

    RESULT
	A number of HPET timer unit allocated for exclusive use, or -1 if
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

    ObtainSemaphore(&base->lock);

    for (i = 0; i < base->unitCnt; i++)
    {
    	if (!base->units[i].Owner)
    	{
    	    base->units[i].Owner = user;

    	    ReleaseSemaphore(&base->lock);
    	    return i;
    	}
    }
    
    ReleaseSemaphore(&base->lock);
    return -1;

    AROS_LIBFUNC_EXIT
}
