/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <errno.h>

/*****************************************************************************

    NAME */

#include <sys/ioctl.h>

	int ioctl(

/*  SYNOPSIS */
	int fd,
	int request,
	...)

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
#   warning Implement ioctl()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");

    errno = ENOSYS;
    return -1;
}

