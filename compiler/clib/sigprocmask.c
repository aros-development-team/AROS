/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sigprocmask().
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */

#include <signal.h>

	int sigprocmask (

/*  SYNOPSIS */
	int  how,
	const  sigset_t *set,
	sigset_t *oldset)

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
#   warning Implement sigprocmask()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    
    return -1;
} /* sigprocmask */
