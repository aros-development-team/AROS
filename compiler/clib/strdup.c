/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function strdup()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <string.h>
#include <memory.h>

	char * strdup (

/*  SYNOPSIS */
	const char * orig)

/*  FUNCTION
	Create a copy of a string. The copy can be freed with free() or will
	be freed when then program ends.

    INPUTS
	str1 - Strings to duplicate

    RESULT
	A copy of the string which can be freed with free().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    char * copy;
    char * ptr;

    if ((copy = malloc (strlen (orig)+1)))
    {
	ptr = copy;

	while ((*ptr ++ = *orig ++));
    }

    return copy;
} /* strdup */

