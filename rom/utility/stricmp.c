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

	AROS_LH2(LONG, Stricmp,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, string1, A0),
	AROS_LHA(CONST_STRPTR, string2, A1),

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
    AROS_LIBFUNC_INIT
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
    AROS_LIBFUNC_EXIT
} /* Stricmp */
