/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    SAS C function stch_l().
*/

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <string.h>

	int stch_l (

/*  SYNOPSIS */
	const char 	* in,
	long		* lvalue)

/*  FUNCTION
	Convert hexadecimal string to a long integer

    INPUTS
	in     - The hexadecimal string to be converted
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
    return sscanf(in, "%lx", lvalue);   
} /* stch_l */
