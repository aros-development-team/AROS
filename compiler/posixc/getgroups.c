/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <unistd.h>

	int getgroups(

/*  SYNOPSIS */
	int gidsetlen,
	gid_t *gidset)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
	Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    /* TODO: Implement getgroups() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;

    return 0;
}

