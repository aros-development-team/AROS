/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function sigsuspend()
    Lang: English
*/

/*****************************************************************************

    NAME */
#ifndef AROS_NO_SIGNAL_H
#include <signal.h>

	int sigsuspend (

/*  SYNOPSIS */
	const sigset_t *mask)

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
#warning TODO: implement sigsuspend()

    return -1; /* return failure */

} /* sigsuspend */

#endif