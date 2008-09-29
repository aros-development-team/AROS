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

	AROS_LH1I(UBYTE, ToUpper,

/*  SYNOPSIS */
	AROS_LHA(ULONG, character, D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 29, Utility)

/*  FUNCTION
	Convert a character to uppercase

    INPUTS
	character   - The character that you want changed.

    RESULT
	The uppercase version of that character.

    NOTES
	Currently only works for ASCII characters. Would not be difficult
	to adapt for other character sets (Unicode for example).

	This function is patched by the locale.library, so you should be
	prepared for different results when running under different
	languages.

    EXAMPLE
	STRPTR string; UBYTE chr;

	\* Convert a string to uppercase *\
	while( chr = *string )
	{
	    *string = ToUpper( chr );
	    string++;
	}

    BUGS

    SEE ALSO
	ToLower()

    INTERNALS
	This function is patched by locale.library.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	10-08-96    iaint   Created from tolower.c from AROSdev15
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return
    (
	(character >= 'a' && character <= 'z')
	|| (character >= 0xE0
	    && character <= 0xEE
	    && character != 0xE7)
	? character - 0x20
	: character
    );

    AROS_LIBFUNC_EXIT
} /* ToLower */
