/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
    
    SVID function putw().
*/

/*****************************************************************************

    NAME */

#include <stdio.h>

	int putw(

/*  SYNOPSIS */
	int word,
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
    if (fwrite(&word, sizeof(word), 1, stream) > 0) return 0;
    else                                            return EOF;
}

