/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    C function strlwr().
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
