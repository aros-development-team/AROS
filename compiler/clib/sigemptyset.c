/*
    Copyright � 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sigemptyset().
*/

/*****************************************************************************

    NAME */

#include <signal.h>
#include <string.h>

	int sigemptyset (

/*  SYNOPSIS */
	sigset_t *set)

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
		memset(&set->__val, 0x00, sizeof(set->__val));
		return 0;
	}

	return -1; /* return failure */
} /* sigemptyset */
