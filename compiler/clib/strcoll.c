/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function strcmp()
    Lang: english
*/

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

    HISTORY

******************************************************************************/
{
    #warning implement strcoll() properly
    
    return strcmp(str1, str2);
} /* strcmp */

