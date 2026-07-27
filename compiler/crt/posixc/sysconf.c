/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sysconf().
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
        Currently only _SC_ARG_MAX handling is implemented

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    switch (name)
    {
        /* TODO: Implement other names */
        case _SC_ARG_MAX: return ARG_MAX;

        case _SC_PAGESIZE: /* same value expected for _SC_PAGE_SIZE */
        case _SC_PAGE_SIZE:
            return 4096;

        default:
            errno = EINVAL;
            return -1;
    };
} /* sysconf */
