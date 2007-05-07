/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "icon_intern.h"
#include <proto/utility.h>

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH2(BOOL, MatchToolValue,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, typeString, A0),
	AROS_LHA(UBYTE *, value, A1),

/*  LOCATION */
	struct Library *, IconBase, 17, Icon)

/*  FUNCTION
	Checks if the given tooltype has the supplied value.

    INPUTS
	typeString - string containing the tooltype.
	value - the value to match for.

    RESULT
	TRUE if match, else FALSE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG    value_len;
    UBYTE   c;
    UBYTE * str;

    /* Check if value has a bar in it */
    str = value;

    while(*str)
    {
	if (*str++=='|')
	    return (FALSE);
    }

    /* Compare loop */
    while (*typeString)
    {
	/* Are they alike ? */
	value_len = strlen (value);

	if (!Strnicmp (typeString, value, value_len))
	{
	    /* Check that we have matched the *whole* word in typeString with value */
	    c = *(typeString + value_len);

	    if (c == '|' || c == 0)
		return (TRUE);
	}


	/* Goto next entry in typeString */
	while (c = *typeString, !((c == '|') || (c == 0)))
	    typeString++;

	/* If we have a "|", skip it */
	if (*typeString == '|')
	    typeString ++;
    }

    return FALSE;
    AROS_LIBFUNC_EXIT
} /* MatchToolValue */
