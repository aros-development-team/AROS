/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function gcvt().
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

    EXAMPLE

    BUGS

    SEE ALSO
	ecvt(), fcvt(), sprintf()

    INTERNALS

******************************************************************************/
{
    sprintf (buf, "%.*G", ndigit, number);

    return buf;
} /* sprintf */

