/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sysconf().
*/

#include <errno.h>
#include <limits.h>

/*****************************************************************************

    NAME */
#include <time.h>
#include <unistd.h>

        long sysconf(

/*  SYNOPSIS */
        int name)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        Only _SC_ARG_MAX, _SC_PAGESIZE/_SC_PAGE_SIZE and _SC_CLK_TCK are
        implemented; other names fail with EINVAL.

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

        case _SC_CLK_TCK:
            /* What times() and clock() count in; CPython refuses to start
               when this query fails. */
            return CLOCKS_PER_SEC;

        default:
            errno = EINVAL;
            return -1;
    };
} /* sysconf */
