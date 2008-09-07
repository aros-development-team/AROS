/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$

    AllocVec-based string duplication.
*/

#include "alib_intern.h"
#include <exec/memory.h>
#include <string.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/exec.h>

	STRPTR StrDup (

/*  SYNOPSIS */
	CONST_STRPTR    str)

/*  FUNCTION
	This function allocates enough space, and copies the given string 
        into it.

    INPUTS
	str - the string to duplicate

    RESULT
	A string copy of the original string (possibly of zero length) or 
        NULL if passed a NULL pointer.

    NOTES
	This functions allocates the new string memory with AllocVec().
	Don't forget to call FreeVec() when you're done with it.

    EXAMPLE

    BUGS

    SEE ALSO
	exec.library/AllocVec(), exec.library/FreeVec(), exec.library/CopyMem()

    INTERNALS

*****************************************************************************/
{
    STRPTR dup;
    ULONG  len;

    if (str == NULL) return NULL;
    
    len = strlen(str);
    dup = AllocVec(len + 1, MEMF_PUBLIC);
    if (dup != NULL) CopyMem(str, dup, len + 1);
    
    return dup;

} /* StrDup */
