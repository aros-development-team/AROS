/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    C function strupr().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	char * strupr (

/*  SYNOPSIS */
	char * str)

/*  FUNCTION
        
        Converts a string to all upper case characters. Modifies
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
		if (str[i] >= 'a' && str[i] <= 'z')
			str[i] += ('A'-'a');
		i++;
	}

	return str;
} /* strupr */
