/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/09/13 17:14:47  digulla
    Removed the TOLOWER() macros. Use the library function instead

    Revision 1.3  1996/08/13 14:10:31  digulla
    Replaced __AROS_LA by __AROS_LHA

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
	#include <clib/utility_protos.h>

	__AROS_LH1I(UBYTE, ToLower,

/*  SYNOPSIS */
	__AROS_LHA(ULONG, character, D0),

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
    __AROS_FUNC_INIT

    return
    (
	(character >= 'A' && character <= 'Z')
	|| (character >= 0xC0
	    && character <= 0xDE
	    && character != 0xD7)
	? character + 0x20
	: character
    );

    __AROS_FUNC_EXIT
} /* ToLower */
