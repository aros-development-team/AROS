/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Add a class to the list of puvlic classes
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include <oop/root.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH1(VOID, AddClass,

/*  SYNOPSIS */
	AROS_LHA(Class  *, classPtr, A0),

/*  LOCATION */
	struct Library *, OOPBase, 8, OOP)

/*  FUNCTION

    INPUTS

    RESULT

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
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    if (classPtr)
    {
    
    	ObtainSemaphore( &GetOBase(OOPBase)->ob_ClassListLock );
    
    	AddTail((struct List *)&GetOBase(OOPBase)->ob_ClassList
    		,(struct Node *)classPtr);
    
    	ReleaseSemaphore( & GetOBase(OOPBase)->ob_ClassListLock );
    }
    
    return;
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
