/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <aros/libcall.h>
#include "intern.h"

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
