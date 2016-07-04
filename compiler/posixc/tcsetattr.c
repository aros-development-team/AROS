/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
    $Id$
*/



/*****************************************************************************

    NAME */

#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>

	int tcsetattr(

/*  SYNOPSIS */
	int fd,
	int opt,
	const struct termios *t)

/*  FUNCTION
        Set terminal attributes.

    INPUTS
        fd      - file descriptor
        opt     - optional actions
        t       - struct termios containing the requested changes

    RESULT
        0       - success
        1       - error 

    NOTES
        Will return success, if *any* of the changes were successful.

    EXAMPLE

    BUGS
        Not implemented.

    SEE ALSO
        ioctl()

    INTERNALS

******************************************************************************/
{
    /* TODO: Implement tcsetattr() */
    return 0;
}
