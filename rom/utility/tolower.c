/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:26  digulla
    Initial revision

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
	__AROS_LA(ULONG, character, D0),

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
    return TOLOWER(character);
    __AROS_FUNC_EXIT
} /* ToLower */
