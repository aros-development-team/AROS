/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Formats a message and makes sure the user will see it.
    Lang: english
*/

#include <aros/config.h>
#include <proto/exec.h>
#include "exec_private.h"

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
#define SysBase 	*(void **)4
#endif

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

	void RawPutChars (

/*  SYNOPSIS */
	const UBYTE * str,
	int	      len)

/*  FUNCTION
	Emits len bytes of fmt via RawPutChar()

    INPUTS
	str - string to print
	len - how many bytes of str to print

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
#ifdef CREATE_ROM
    struct ExecBase * SysBase = *(struct ExecBase **)0x04;
#endif
    while (*str && len --)
	RawPutChar (*str ++);
} /* RawPutChars */

