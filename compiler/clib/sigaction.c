/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function sigaction()
    Lang: English
*/

/*****************************************************************************

    NAME */
#ifndef AROS_NO_SIGNAL_H
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

    HISTORY

******************************************************************************/
{
#warning TODO: implement sigaction()

    return -1; /* return failure */

} /* sigaction */

#endif /* #ifndef AROS_NO_SIGNAL_H */
