/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    ISO/ANSI C function mblen().
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */

#include <string.h>
#include <stdlib.h>

	size_t mblen(

/*  SYNOPSIS */
	const char *s,
	size_t n)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
	Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
#   warning Implement mblen() properly
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    
    return strlen(s);
}

