/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sigaction().
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <signal.h>

	int sigaction (

/*  SYNOPSIS */
	int signum,
	const  struct  sigaction  *act,
	struct sigaction *oldact)

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
#   warning Implement sigaction()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    
    return -1;
} /* sigaction */
