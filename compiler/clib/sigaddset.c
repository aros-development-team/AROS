/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sigaddset().
*/

/*****************************************************************************

    NAME */

#include "__arosc_privdata.h"

#include <signal.h>

	int sigaddset (

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
		set->__val[i] |= (1 << (signum & 0x1f));
		return 0;
	}
	
	return -1;
} /* sigaddset */
