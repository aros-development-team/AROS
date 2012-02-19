/*
    Copyright © 2004-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <pwd.h>

	struct passwd *getpwuid(

/*  SYNOPSIS */
	uid_t uid)

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
    /* TODO: Implement getpwuid() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;

    return NULL;
}

