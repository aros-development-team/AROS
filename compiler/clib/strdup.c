/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/21 15:40:51  digulla
    Duplicate strings

    Revision 1.1  1996/08/01 18:46:31  digulla
    Simple string compare function

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <clib/aros_protos.h>

/*****************************************************************************

    NAME */
	#include <clib/aros_protos.h>

	UBYTE * StrDup (

/*  SYNOPSIS */
	const UBYTE * orig)

/*  FUNCTION
	Create a copy of a string. The copy must be freed with FreeVec().

    INPUTS
	str1 - Strings to duplicate

    RESULT
	A copy of the string which must be freed with FreeVec().

    NOTES
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    UBYTE * copy;
    UBYTE * ptr;

    if ((copy = AllocVec (StrLen (orig)+1, MEMF_ANY)))
    {
	ptr = copy;

	while ((*ptr ++ = *orig ++));
    }

    return copy;
} /* StrDup */

