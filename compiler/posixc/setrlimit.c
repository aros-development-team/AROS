/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function setrlimit().
*/

#include <errno.h>
#include <sys/resource.h>

/*****************************************************************************

    NAME */

	int setrlimit (

/*  SYNOPSIS */
	int resource,
	const struct rlimit *rlp)

/*  FUNCTION
	Get the limits of certain system resources

    INPUTS
	resource - the resource type to get
	rlp      - resource information to update

    RESULT
	On success, returns 0. -1 and errno on error.

    NOTES
        Currently always returns -1 and errno is set to EINVAL

    EXAMPLE

    BUGS

    SEE ALSO
	getrlimit()

    INTERNALS

******************************************************************************/
{
    int retval = 0;

    /* As of yet, no resource can be updated on AROS */

    switch (resource) {
    default:
        retval = -1;
        errno = EINVAL;
        break;
    }

    return retval;
}
