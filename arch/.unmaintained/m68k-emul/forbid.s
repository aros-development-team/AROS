/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec function Forbid
    Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH0(void, Forbid,

    LOCATION
        struct ExecBase *, SysBase, 22, Exec)

    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(Forbid,Exec)
	.type	AROS_SLIB_ENTRY(Forbid,Exec),@function
AROS_SLIB_ENTRY(Forbid,Exec):
#if !UseRegisterArgs
	/* Preserve all registers */
	move.l	%a6,-(%sp)

	/* Get SysBase */
	move.l	8(%sp),%a6
#endif

	/* Increase nesting count */
	addq.b	#1,TDNestCnt(%a6)

	/* All done */
#if !UseRegisterArgs
	move.l	(%sp)+,%a6
#endif
	rts

