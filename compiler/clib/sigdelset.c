/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function sigdelset()
    Lang: English
*/

/*****************************************************************************

    NAME */
#ifndef AROS_NO_SIGNAL_H
#include <signal.h>

	int sigdelset (

/*  SYNOPSIS */
	sigset_t *set,
	int signum)

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
	if (NULL != set) {
		ULONG i = (signum >> 5);
		set->__val[i] &= ~(1 << (signum & 0x1f));
		return 0;
	}
	
	return -1;
} /* sigdelset */

#endif
