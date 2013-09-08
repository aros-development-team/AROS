/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function sigismember().
*/

#include <stddef.h>

/*****************************************************************************

    NAME */
#include <signal.h>

	int sigismember (

/*  SYNOPSIS */
	const sigset_t *set,
	int signum)

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
    if (NULL != set) {
        unsigned int i = (signum >> 5);
        if (0 != (set->__val[i] & (signum & 0x1f)))
            return 1;
        return 0;
    }

    return -1;
} /* sigismember */
