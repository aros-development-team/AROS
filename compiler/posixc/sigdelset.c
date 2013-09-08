/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function sigdelset().
*/

#include <stddef.h>

/*****************************************************************************

    NAME */
#include <signal.h>

	int sigdelset (

/*  SYNOPSIS */
	sigset_t *set,
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
        set->__val[i] &= ~(1 << (signum & 0x1f));
        return 0;
    }
	
    return -1;
} /* sigdelset */
