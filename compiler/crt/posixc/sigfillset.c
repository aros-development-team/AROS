/*
    Copyright (C) 1995-2012, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sigfillset().
*/

#include <string.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <signal.h>

        int sigfillset (

/*  SYNOPSIS */
        sigset_t *set)

/*  FUNCTION
        Initialise the signal set

    INPUTS
        Set to initialize

    RESULT
        "0" for success, "-1" for failure (errno contains error)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        POSIX.1-2001.

******************************************************************************/
{
    if (NULL != set) {
        memset(&set->__val, 0xff, sizeof(set->__val));
        return 0;
    }

    errno = EFAULT;

    return -1; /* return failure */
} /* sigfillset */
