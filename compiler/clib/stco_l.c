/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    SAS C function stco_l().
*/

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <string.h>

	int stco_l (

/*  SYNOPSIS */
	const char 	* in,
	long		* lvalue)

/*  FUNCTION
	Convert octal string to a long integer

    INPUTS
	in     - The octal string to be converted
	lvalue - Pointer to long where the result is saved 

    RESULT
	1 means success. 0 means failure.

    NOTES
	SAS C specific
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    return sscanf(in, "%lo", lvalue);
} /* stco_l */
