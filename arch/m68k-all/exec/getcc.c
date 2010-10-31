/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetCC() - Read the CPU condition codes in an easy way.
    Lang: english
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(UWORD, GetCC,

/*  LOCATION */
	struct ExecBase *, SysBase, 88, Exec)

/*  FUNCTION
	Read the contents of the CPU condition code register in a system
	independant way. The flags return will be in the same format as
	the Motorola MC680x0 family of microprocessors.

    INPUTS
	None.

    RESULT
	The CPU condition codes or ~0ul if this function has not been
	implemented.

    NOTES

    EXAMPLE

    BUGS
	This function may not be implemented on platforms other than
	Motorola mc680x0 processors.

    SEE ALSO
	SetSR()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    register volatile UWORD sr asm("%d0");

    /*  As with SetSR() you can either do nothing, or alternatively read
	you own registers and assemble them into the form of the MC680x0
	condition codes.
     */
    if (SysBase->AttnFlags & AFF_68010) {
    	// Sad trick to get this to compile with -m68000
    	// asm volatile ( "move.w %%ccr,%%d0\n" : "=d" (sr) : : );
    	asm volatile ( ".word 0x42c0\n" : "=d" (sr) : : );
    } else {
    	asm volatile ( "move.w %%sr,%0\n" : "=g" (sr) : : );
    }

    return sr;

    AROS_LIBFUNC_EXIT
} /* GetCC() */
