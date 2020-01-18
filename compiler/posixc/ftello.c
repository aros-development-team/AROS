/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        off_t ftello (

/*  SYNOPSIS */
        FILE *stream)

/*  FUNCTION
	Returns the current position in a stream.

    INPUTS
	stream - Query this stream

    RESULT

    NOTES
	on 32bit platforms, off_t is a 32bit value, and so the 64bit
        version (ftello64) is needed to work with large files.
        off_t is 64bit natively on 64bit platforms.


    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{

    return __ftello(stream);

} /* ftello */
