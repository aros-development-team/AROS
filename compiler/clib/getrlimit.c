/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function getrlimit().
*/

#include <errno.h>
#include <sys/resource.h>

#include "__fdesc.h"

/*****************************************************************************

    NAME */

	int getrlimit (

/*  SYNOPSIS */
	int resource,
	struct rlimit *rlp)

/*  FUNCTION
	Get the limits of certain system resources

    INPUTS
	resource - the resource type to get
	rlp      - returned resource information

    RESULT
	On success, returns 0. -1 and errno on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	setrlimit()

    INTERNALS

******************************************************************************/
{
    int retval = 0;

    switch (resource) {
    case RLIMIT_NOFILE: /* needed for getdtablesize() */
        rlp->rlim_cur = rlp->rlim_max = __getfdslots(); 
        break;
    default:
        retval = -1;
        errno = EINVAL;
        break;
    }

    return retval;
}
