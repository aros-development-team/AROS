/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    C function tmpnam().
*/

/*****************************************************************************

    NAME */
#include <stdio.h>
char _tmpnam_internal_buffer[L_tmpnam];

	char *tmpnam (

/*  SYNOPSIS */
	char *name_buf)

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
    static int _tmpnam_internal_count = 0;
    char *buf;

#warning how many internal filenames have to be cached?
    if(name_buf == NULL)
	buf = _tmpnam_internal_buffer;
    else
	buf = name_buf;

#warning find a better way for a unique filename
    sprintf(buf, "tmp%06d", _tmpnam_internal_count++);

    return buf;
} /* tmpnam */
