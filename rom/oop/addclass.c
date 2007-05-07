/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a class to the list of puvlic classes
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH1(VOID, OOP_AddClass,

/*  SYNOPSIS */
	AROS_LHA(OOP_Class  *, classPtr, A0),

/*  LOCATION */
	struct Library *, OOPBase, 8, OOP)

/*  FUNCTION
	Adds a class to the public list of classes.
	This means that any process can create objects of this
	class.

    INPUTS
    	classPtr - Pointer to the class to make public.

    RESULT
    	None.

    NOTES

    EXAMPLE

    BUGS
    	Would be faster to use a hashtable to look up class IDs

    SEE ALSO
    	OOP_RemoveClass()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (classPtr)
    {
    
    	ObtainSemaphore( &GetOBase(OOPBase)->ob_ClassListLock );
    
    	AddTail((struct List *)&GetOBase(OOPBase)->ob_ClassList
    		,(struct Node *)classPtr);
    
    	ReleaseSemaphore( & GetOBase(OOPBase)->ob_ClassListLock );
    }
    
    return;
    AROS_LIBFUNC_EXIT
} /* OOP_AddClass */
