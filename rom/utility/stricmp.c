/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/09/13 17:33:30  digulla
    Use the ToLower function instead of the macro

    Revision 1.3  1996/08/13 14:10:31  digulla
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:41:41  digulla
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

	__AROS_LH2(LONG, Stricmp,

/*  SYNOPSIS */
	__AROS_LHA(STRPTR, string1, A0),
	__AROS_LHA(STRPTR, string2, A1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 27, Utility)

/*  FUNCTION
	Compares two strings treating lower and upper case characters
	as identical.

    INPUTS
	string1, string2 - The strings to compare.

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

    /* Loop as long as the strings are identical and valid. */
    do
    {
	/* Get characters, convert them to lower case. */
	c1=ToLower(*string1++);
	c2=ToLower(*string2++);
    }while(c1==c2&&c1);

    /* Get result. */
    return (LONG)c1-(LONG)c2;
    __AROS_FUNC_EXIT
} /* Stricmp */
