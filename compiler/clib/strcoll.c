/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function strcoll().
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <string.h>

	int strcoll (

/*  SYNOPSIS */
	const char * str1,
	const char * str2)

/*  FUNCTION
	Calculate str1 - str2. The operation is based on strings interpreted
	as appropriate for the program's current locale  for  category  LC_COLLATE.

    INPUTS
	str1, str2 - Strings to compare

    RESULT
	The difference of the strings. The difference is 0, if both are
	equal, < 0 if str1 < str2 and > 0 if str1 > str2. Note that
	it may be greater then 1 or less than -1.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
#   warning Implement strcoll() properly
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");

    return strcmp(str1, str2);
} /* strcoll */

