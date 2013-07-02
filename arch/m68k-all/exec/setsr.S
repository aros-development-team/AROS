/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SetSR() - Modify the CPU status register.
    Lang: english
*/

/*****************************************************************************

    NAME
#include <proto/exec.h>

	AROS_LH2(ULONG, SetSR,

    SYNOPSIS
	AROS_LHA(ULONG, newSR, D0),
	AROS_LHA(ULONG, mask,  D1),

    LOCATION
	struct ExecBase *, SysBase, 24, Exec)

    FUNCTION
	Read/Modify the CPU status register in an easy way. Only the bits
	set it the mask parameter will be changed.

	The bits in the register mapped to those of the Motorola MC680x0
	family of microprocessors.

    INPUTS
	newSR   -   The new contents of the status register.
	mask    -   Mask of bits to change.

    RESULT
	The old contents of the status register or ~0UL if this function
	is not implemented.

    NOTES
	This function is of limited use.

    EXAMPLE
	You can read the status register by calling SetSR(0,0).

    BUGS
	This function may do nothing on non-mc680x0 systems.

    SEE ALSO

    INTERNALS

******************************************************************************/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(SetSR,Exec,24)

AROS_SLIB_ENTRY(SetSR,Exec,24):

	movem.l %d2/%a5,%sp@-
	lea su(%pc),%a5
	jsr Supervisor(%a6)
	movem.l %sp@+,%d2/%a5
	rts

su: move.w %sp@,%d2
	and.w %d1,%d0
	not.w %d1
	and.w %d1,%sp@
	or.w %d0,%sp@
	moveq #0,%d0
    move.w %d2,%d0
    rte
