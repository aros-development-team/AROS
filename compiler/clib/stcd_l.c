/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    SAS/C function stcd_l().
*/

#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <string.h>

	int stcd_l (

/*  SYNOPSIS */
	const char 	* in,
	long		* lvalue)

/*  FUNCTION
	Convert decimal string to a long integer

    INPUTS
	in     - The decimal string to be converted
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

    l = strtol(in, &s, 10);

    if (s != in)
    {
        *lvalue = l;
        return 1;
    }
    else
        return 0;
} /* stcd_l */
