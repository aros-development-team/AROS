/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        off64_t ftello64 (

/*  SYNOPSIS */
        FILE *stream)

/*  FUNCTION
	Returns the current position in a stream.

    INPUTS
	stream - Query this stream

    RESULT

    NOTES
	Returns the position in files that may be larger than 2GB, if the
	underlying filesystem supports it.

    EXAMPLE

    BUGS

    SEE ALSO
	fopen64(), ftello64()

    INTERNALS

******************************************************************************/
{

    return __ftello64(stream);

} /* ftello64 */
