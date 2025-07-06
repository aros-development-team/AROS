/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

/*****************************************************************************

    NAME */
#include <sys/select.h>
#include <stdarg.h>

        int pselect(

/*  SYNOPSIS */
        int nfds,
		fd_set *restrict readfds,
        fd_set *restrict writefds,
		fd_set *restrict exceptfds,
        const struct timespec *restrict timeout,
        const sigset_t *restrict sigmask)

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
