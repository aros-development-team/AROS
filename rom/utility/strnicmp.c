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

	__AROS_LH3I(LONG, Strnicmp,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, string1, A0),
	__AROS_LA(STRPTR, string2, A1),
	__AROS_LA(LONG,   length,  D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 28, Utility)

/*  FUNCTION
	Compares two strings treating lower and upper case characters
	as identical up to a given maximum number of characters.

    INPUTS
	string1, string2 - The strings to compare.
	length           - maximum number of characters to compare.

    RESULT
	<0  if string1 <  string2
	==0 if string1 == string2
	>0  if string1 >  string2

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    __AROS_FUNC_INIT
    UBYTE c1, c2;

    /* 0 characters are always identical */
    if(!length)
        return 0;

    /* Loop as long as the strings are identical and valid. */
    do
    {
    	/* Get characters, convert them to lower case. */
	c1=*string1++;
	c1=TOLOWER(c1);
	c2=*string2++;
	c2=TOLOWER(c2);
    }while(c1==c2&&c1&&--length);

    /* Get result. */
    return (LONG)c1-(LONG)c2;
    __AROS_FUNC_EXIT
} /* Strnicmp */
