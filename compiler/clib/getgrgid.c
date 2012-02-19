/*
    Copyright © 2004-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <grp.h>

/*  SYNOPSIS */
	struct group *getgrgid(

/*  SYNOPSIS */
	gid_t gid)

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
    /* TODO: Implement getgrgid() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;

    return NULL;
}

