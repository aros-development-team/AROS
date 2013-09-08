/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <grp.h>

	struct group *getgrent(

/*  SYNOPSIS */
	void)

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
    /* TODO: Implement getgrent() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;

    return NULL;
}

