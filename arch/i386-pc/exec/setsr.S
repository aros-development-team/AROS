/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
    $Id$

    Desc: SetSR() - Modify the CPU status register.
    Lang: english
*/

/*****************************************************************************

    NAME

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

    HISTORY

******************************************************************************/

#include "machine.i"

	    .text
	    .globl	AROS_SLIB_ENTRY(SetSR,Exec)
	    .type	AROS_SLIB_ENTRY(SetSR,Exec),@function
AROS_SLIB_ENTRY(SetSR,Exec):
	    xorl	%eax,%eax
	    ret
