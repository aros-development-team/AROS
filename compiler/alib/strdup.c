/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AllocVec-based string duplication
    Lang: english
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
	This function allocates enough space, and
        copies the given nul-terminated string into it.

    INPUTS
	str - the string to duplicate

    RESULT
	A nul-terminated string copy of the original string (possibly
	of zero length), or NULL if passed a NULL pointer.

    NOTES
	This functions allocates the new string memory with AllocVec().
	Don't forget to call FreeVec() when you're done with it.

    EXAMPLE

    BUGS

    SEE ALSO
	AllocVec(), FreeVec(), CopyMem()

    INTERNALS

    HISTORY
	09-12-02    dlc added this commonly used function

*****************************************************************************/
{
    STRPTR dup;
    ULONG len;

    if (!str) return NULL;
    len = strlen(str);
    dup = AllocVec(len + 1, MEMF_PUBLIC | MEMF_CLEAR);
    if (dup) CopyMem(str, dup, len + 1);
    return dup;

} /* StrDup */
