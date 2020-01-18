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


    INPUTS
	stream - Modify this stream

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fopen64(), ftello64()

    INTERNALS

******************************************************************************/
{

    return __ftello64(stream);

} /* ftello64 */
