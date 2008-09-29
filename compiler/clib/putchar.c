/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ISO C function putchar()
*/

/*****************************************************************************

    NAME */

#include <stdio.h>

	int putchar(

/*  SYNOPSIS */
	int c)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    return putc(c, stdout);
}

