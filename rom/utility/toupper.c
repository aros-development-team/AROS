/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/09/13 17:16:35  digulla
    Removed the macro TOUPPER. Use this library function instead

    Revision 1.1  1996/08/31 12:58:13  aros
    Merged in/modified for FreeBSD.


    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <aros/libcall.h>
#include "utility_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/utility_protos.h>

	__AROS_LH1I(UBYTE, ToUpper,

/*  SYNOPSIS */
	__AROS_LHA(unsigned long, character, D0),

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
	utility/ToLower()

    INTERNALS
	This function is patched by locale.library.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	10-08-96    iaint   Created from tolower.c from AROSdev15
*****************************************************************************/
{
    __AROS_FUNC_INIT

    return
    (
	(character >= 'a' && character <= 'z')
	|| (character >= 0xE0
	    && character <= 0xEE
	    && character != 0xE7)
	? character - 0x20
	: character
    );

    __AROS_FUNC_EXIT
} /* ToLower */
