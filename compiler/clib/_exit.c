/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function _exit().
*/

#include <aros/debug.h>

#include "__arosc_privdata.h"

#include <exec/types.h>
#include <setjmp.h>
#include <aros/startup.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	void _exit (

/*  SYNOPSIS */
	int code)

/*  FUNCTION
	Terminates the running program immediately. The code is returned to 
        the program which has called the running program. In contrast to
        exit(), this function does not call user exit-handlers added with 
        atexit() or on_exit(). It does, however, close open filehandles.

    INPUTS
	code - Exit code. 0 for success, other values for failure.

    RESULT
	None. This function does not return.

    NOTES
        This function must not be used in a shared library or in a threaded
        application.

   EXAMPLE

    BUGS
        Currently, this function *does* trigger user exit-handlers to be 
        called.

    SEE ALSO
	exit()

    INTERNALS

******************************************************************************/
{
    __arosc_startup_error = code;

    D(bug("_exit() - returning via jmp_buf %p\n", __arosc_startup_jmp_buf));
    longjmp (__arosc_startup_jmp_buf, 1);

    /* TODO: _exit() is not properly implemented */

    /* never reached */
} /* _exit */

