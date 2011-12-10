/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function getdtablesize().
*/

#define getdtablesize getdtablesize

#include <sys/resource.h>

/*****************************************************************************

    NAME */

	int getdtablesize (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Get the limits of certain system resources

    INPUTS

    RESULT
	On success, returns maximum number of file
	descriptors.
	
	Returns -1 and errno on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	getrlimit()

    INTERNALS

******************************************************************************/
{
    int retval = 0;
    struct rlimit rl;

    retval = getrlimit(RLIMIT_NOFILE, &rl);
    if (retval == 0)
        retval = rl.rlim_cur;

    return retval;
}
