/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

	__AROS_LH3I(LONG, Strnicmp,

/*  SYNOPSIS */
	__AROS_LHA(STRPTR, string1, A0),
	__AROS_LHA(STRPTR, string2, A1),
	__AROS_LHA(LONG,   length,  D0),

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
