/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

static int test; /* cxref bug */

/*****************************************************************************

    NAME */

	AROS_LH1(void, OOP_RemoveClass,

/*  SYNOPSIS */
	AROS_LHA(OOP_Class *, classPtr, A0),

/*  LOCATION */
	struct Library *, OOPBase, 11, OOP)

/*  FUNCTION
	Remove a class from the list of public classes.
	The class must have previously added with AddClass().
	
    INPUTS
    	classPtr - Pointer to class that should be removed.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    	OOP_AddClass()

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
	ObtainSemaphore( &GetOBase(OOPBase)->ob_ClassListLock );
	Remove ((struct Node *)classPtr);
	ReleaseSemaphore( &GetOBase(OOPBase)->ob_ClassListLock );

    }
    
    return;

    AROS_LIBFUNC_EXIT
} /* OOP_RemoveClass */
