/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C function strlwr()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <string.h>

	char * strlwr (

/*  SYNOPSIS */
	char * str)

/*  FUNCTION
        
        Converts a string to all lower case characters. Modifies
        the given string.

    INPUTS
	str - The string to convert.

    RESULT
        The same string buffer is passed back with all characters converted.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	11.12.1996 digulla created

******************************************************************************/
{
	unsigned int i = 0;
	while (str[i]) {
		if (str[i] >= 'A' && str[i] <= 'Z')
			str[i] -= ('A'-'a');
		i++;
	}

	return str;
} /* strlwr */
