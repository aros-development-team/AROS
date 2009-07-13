/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function sysconf().
*/

#include <errno.h>
#include <limits.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

        long sysconf(

/*  SYNOPSIS */
        int name)

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
    switch (name)
    {
        case _SC_ARG_MAX: return ARG_MAX;

        default:
            errno = EINVAL;
            return -1;
    };
} /* sysconf */
