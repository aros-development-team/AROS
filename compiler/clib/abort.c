/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function exit().
*/

#include <exec/types.h>
#include <setjmp.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#include "__arosc_privdata.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void abort (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Causes abnormal program termination. If there is a signal handler
	for SIGABORT, then the handler will be called. If the handler
	returns, then the program is aborted anyway.

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
    raise(SIGABRT);

    struct aroscbase *aroscbase = __GM_GetBase();

    /* Abort anyway */
    aroscbase->acb_flags |= ABNORMAL_EXIT;
    longjmp(__arosc_startup_jmp_buf, 20);

    assert(0); /* Should not be reached and will likely bomb recursively */
}
