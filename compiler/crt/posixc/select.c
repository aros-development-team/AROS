/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

/*****************************************************************************

    NAME */
#include <sys/select.h>
#include <stdarg.h>

        int select(

/*  SYNOPSIS */
        int nfds,
		fd_set *restrict readfds,
        fd_set *restrict writefds,
		fd_set *restrict exceptfds,
        struct timeval *restrict timeout)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	return 0;
}
