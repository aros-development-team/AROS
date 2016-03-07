/*
    Copyright � 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function sigfillset().
*/

#include <string.h>

/*****************************************************************************

    NAME */
#include <signal.h>

	int sigfillset (

/*  SYNOPSIS */
	sigset_t *set)

/*  FUNCTION
        Initialise the signal set

    INPUTS
        Set to initialise

    RESULT
        "0" for success, "-1" for failure (errno contains error)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    if (NULL != set) {
        memset(&set->__val, 0xff, sizeof(set->__val));
        return 0;
    }

    return -1; /* return failure */
} /* sigfillset */
