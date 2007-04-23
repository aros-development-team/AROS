/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SetSR() - Modify the CPU status register.
    Lang: english
*/

#include <exec/types.h>
#include <exec/ptrace.h>
#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH2(ULONG, SetSR,

/*  SYNOPSIS */
	AROS_LHA(ULONG, newSR, D0),
	AROS_LHA(ULONG, mask,  D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 24, Exec)

/*  FUNCTION
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
{
	AROS_LIBFUNC_INIT
	ULONG res;

	__asm__ __volatile__ ("swi #1");
	return res;

	AROS_LIBFUNC_EXIT
} /* SetSR() */


void _sys_SetSR(struct pt_regs * regs, LONG adjust) 
{
	ULONG new  = regs->r0;
	ULONG mask = regs->r1;
	regs->r0   = regs->cpsr;
	regs->cpsr = (regs->cpsr & ~mask) | (new & mask);
//	D(bug("in _sys_SetSR! new cpsr = %x\n",regs->cpsr));
}
