/*
    Copyright (C) 1999-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include <proto/datatypes.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(ULONG *, CopyDTMethods,

/*  SYNOPSIS */
	AROS_LHA(ULONG *, methods, A0),
	AROS_LHA(ULONG *, include, A1),
	AROS_LHA(ULONG *, exclude, A2),

/*  LOCATION */
	struct Library *, DataTypesBase, 45, DataTypes)

/*  FUNCTION

    Copy and modify an array of methods. This is used by subclass implementors
    who want to add supported methods to an existing class.

    INPUTS

    methods  --  array of methods; may be NULL
    include  --  array of methods to include terminated with ~0UL; may be NULL
    method   --  array of methods to exclude terminated with ~0UL; may be NULL.

    RESULT

    The new array of methods or NULL if something went wrong (like out of
    memory).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    FindMethod(), CopyDTTriggerMethods(), FreeDTMethods()

    INTERNALS

    When a method is specified both in the 'include' and the 'exclude',
    it will be included.

    HISTORY

    2.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG *inc = include;
    ULONG *exc = exclude;
    ULONG *met = methods;
    ULONG  nMethods = 0;
    ULONG *newM;
    ULONG *newmets;

    if(methods == NULL)
	return NULL;

    if(inc != NULL)
    {
	while(*inc++ != ~0UL)
	    nMethods++;
    }

    if(exc != NULL)
    {
	while(*exc != ~0UL)
	{
	    if(FindMethod(methods, *exc) != NULL)
		nMethods--;
	    
	    exc++;
	}
    }

    while(*met++ != ~0UL)
	nMethods++;

    newM = AllocVec((nMethods + 1)*sizeof(ULONG), MEMF_PUBLIC);

    /* No memory available? */
    if(newM == NULL)
	return NULL;

    newmets = newM;
    met     = methods;

    /* Copy new methods */
    if(include != NULL)
    {
	while(*include != ~0UL)
	    *newmets++ = *include++;
    }
 
    /* Copy old methods except the excluded ones */
    while(*met != ~0UL)
    {
	if(FindMethod(exclude, *met) == NULL)
	    *newmets++ = *met;
	
	met++;
    }
    
    *newmets = ~0UL;

    return newM;

    AROS_LIBFUNC_EXIT
} /* CopyDTMethods */
