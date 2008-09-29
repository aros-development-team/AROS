/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
    
    SVID function getw().
*/

/*****************************************************************************

    NAME */

#include <stdio.h>

	int getw(

/*  SYNOPSIS */
	FILE *stream)

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
    int word;
    
    if (fread(&word, sizeof(word), 1, stream) > 0) return word;
    else                                           return EOF;
}

