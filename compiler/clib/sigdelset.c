/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sigdelset().
*/

/*****************************************************************************

    NAME */

#include "__arosc_privdata.h"

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

******************************************************************************/
{
	if (NULL != set) {
		ULONG i = (signum >> 5);
		set->__val[i] &= ~(1 << (signum & 0x1f));
		return 0;
	}
	
	return -1;
} /* sigdelset */
