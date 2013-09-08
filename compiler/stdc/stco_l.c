/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    SAS/C function stco_l().
*/

#include <stdlib.h>

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
	SAS/C specific
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char *s;
    long l;

    l = strtol(in, &s, 8);

    if (s != in)
    {
        *lvalue = l;
        return 1;
    }
    else
        return 0;
} /* stco_l */
