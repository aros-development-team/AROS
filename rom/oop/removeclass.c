/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, RemoveClass,

/*  SYNOPSIS */
	AROS_LHA(Class *, classPtr, A0),

/*  LOCATION */
	struct Library *, OOPBase, 11, OOP)

/*  FUNCTION

    INPUTS

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,OopsBase)

    if (classPtr)
    {
	/* Changed to Semaphores during boopsi.library creation */
	ObtainSemaphore( &GetOBase(OOPBase)->ob_ClassListLock );
	Remove ((struct Node *)classPtr);
	ReleaseSemaphore( &GetOBase(OOPBase)->ob_ClassListLock );

    }
    
    return;

    AROS_LIBFUNC_EXIT
} /* RemoveClass */
