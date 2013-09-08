/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2001 function gcvt().
    Function is deprecated and removed from POSIX.1-2008
*/

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	char * gcvt (

/*  SYNOPSIS */
	double	  number,
	int	  ndigit,
	char	* buf
	)

/*  FUNCTION
	Converts a number to a minimal length NULL terminated ASCII string.
	It produces ndigit significant digits in either printf F format or
	E format.

    INPUTS
	number  - The number to convert.
	ndigits - The number of significan digits that the string has to have.
	buf     - The buffer that will contain the result string.

    RESULT
	The address of the string pointed to by buf.

    NOTES
        This function is deprecated and not present anymore in POSIX.1-2008.
        This function should not be used in new code and old code should
        be fixed to remove usage.
        This function is part of libposixc.a and may be removed in the future.

    EXAMPLE

    BUGS

    SEE ALSO
	stdc.library/sprintf()

    INTERNALS

******************************************************************************/
{
    sprintf (buf, "%.*G", ndigit, number);

    return buf;
} /* sprintf */

