/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function exit().
*/

#include <exec/types.h>
#include <setjmp.h>

#ifndef _CLIB_KERNEL_
    extern jmp_buf __startup_jmp_buf;
    extern LONG    __startup_error;
#endif

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void abort (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Causes abnormal program termination. If there is a signal handler
	for SIGABORT, then the handler will be called. If the handler
	returns, then the program is continued.

    INPUTS
	None.

    RESULT
	None. This function does not return.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE
	if (fatal_error)
	    abort ();

    BUGS
	Signal handling is not implemented yet.

    SEE ALSO
	signal(), exit()

    INTERNALS

******************************************************************************/
{
    GETUSER;

    __startup_error = 20;

    longjmp (__startup_jmp_buf, 1);

    /* never reached */
} /* abort */

