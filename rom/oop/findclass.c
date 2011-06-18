/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: Find a public OOP class
    Lang: english
*/

#include <exec/lists.h>
#include <proto/exec.h>

#include "intern.h"
#include "hash.h"

#define MD(x) ((struct metadata *)x)

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH1(APTR, OOP_FindClass,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, classID, A0),

/*  LOCATION */
	struct Library *, OOPBase, 22, OOP)

/*  FUNCTION
	Finds a class with given ID in the list of public classes.

    INPUTS
	classID  - Public ID of the class to find.

    RESULT
    	Pointer to a public class or NULL if there's no such class

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OOP_AddClass()

    INTERNALS

    HISTORY
	V42 - initial implementation

*****************************************************************************/

{
    AROS_LIBFUNC_INIT
    
    OOP_Class *classPtr;

    /* Class list is public, so we must avoid race conditions */
    ObtainSemaphoreShared(&GetOBase(OOPBase)->ob_ClassListLock);

    classPtr = (OOP_Class *)FindName((struct List *)&(GetOBase(OOPBase)->ob_ClassList), classID);

    /* Release lock on list */
    ReleaseSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);
    
    return classPtr;
    
    AROS_LIBFUNC_EXIT
}
