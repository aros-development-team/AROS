/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SAS C function stco_l()
    Lang: english
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

    HISTORY
	15.12.2000 stegerg created.

******************************************************************************/
{

    return sscanf(in, "%lo", lvalue);
    
} /* stco_l */
