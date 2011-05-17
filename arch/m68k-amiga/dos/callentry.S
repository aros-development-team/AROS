/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: setsr.S 36262 2010-12-27 12:17:48Z sonic $

    Desc: SetSR() - Modify the CPU status register.
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_UFH7(ULONG, DOS_CallEntry_A6,

    SYNOPSIS
	AROS_UFHA(STRPTR, argptr, A0),
	AROS_UFHA(ULONG,  argsize, D0),
	AROS_UFHA(LONG_FUNC, entry, A3),
	AROS_UFHA(APTR, globvec, A2),
	AROS_UFHA(APTR, splower, A1),
	AROS_UFHA(APTR, returnaddr, A4),
	AROS_UFHA(APTR, a6, A6))

    LOCATION
        dos.library

    FUNCTION
	Call a DOS process

    INPUTS
	argptr  - Argument pointer (advisory)
	argsize - Length of arguments
	entry   - Routine to call
	globvec - DOS global vector
	splower - Bottom of the stack
	a6      - Value of A6 to pass to the entry point

    RESULT
	Return value from calling entry()

    NOTES
	Internal use only

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	DOS_CallEntry_A6
DOS_CallEntry_A6:

	/* D0 - Argsize
	 * A0 - Argptr
	 * A1 - SPLower
	 * A2 - GlobVec
	 * A3 - Entry
	 * A4 - ReturnAddr
	 * A6 - A6
	 */

	movem.l	%d2-%d7/%a2-%a6,%sp@-
	move.l	%a1,%d1
	neg.l	%d1
	lea.l	%sp@(%d1),%a5
	move.l	%a5,%sp@-		/* sp+ 4 = stksize */
	move.l	%sp,%a4@		/* Save address of return address */
	lea.l	BCPL_jsr,%a5
	jsr	(%a3)
	addq.l	#4,%sp
	movem.l	%sp@+,%d2-%d7/%a2-%a6
	rts
	
