/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/27 00:32:33  ldp
    Polish

    Revision 1.6  1996/12/10 14:00:16  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.5  1996/10/24 15:51:38  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/09/13 17:14:47  digulla
    Removed the TOLOWER() macros. Use the library function instead

    Revision 1.3  1996/08/13 14:10:31  digulla
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:41:42  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <aros/libcall.h>
#include "utility_intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH1I(UBYTE, ToLower,

/*  SYNOPSIS */
	AROS_LHA(ULONG, character, D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 30, Utility)

/*  FUNCTION
	Convert a character to lower case.

    INPUTS
	character - The character to convert.

    RESULT
	Equivalent lower case character.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return
    (
	(character >= 'A' && character <= 'Z')
	|| (character >= 0xC0
	    && character <= 0xDE
	    && character != 0xD7)
	? character + 0x20
	: character
    );

    AROS_LIBFUNC_EXIT
} /* ToLower */
