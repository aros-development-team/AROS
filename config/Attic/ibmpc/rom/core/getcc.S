/*
    Copyright (C) 1995-1999 AROS - The Amiga Research OS
    $Id$

    Desc: GetCC() - Read the CPU condition codes in an easy way.
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_LH0(UWORD, GetCC,

    LOCATION
	struct ExecBase *, SysBase, 88, Exec)

    FUNCTION
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
	Why not. It works now on i386 :-))

    SEE ALSO
	SetSR()

    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"

/*
    This function changes this:
    ....V...NZ.X...C <- Conditionals from i386
    to this:
    ...........XNZVC <- Conditionals from 68K
*/

	    .text
	    .globl	AROS_SLIB_ENTRY(GetCC,Exec)
	    .type	AROS_SLIB_ENTRY(GetCC,Exec),@function
AROS_SLIB_ENTRY(GetCC,Exec):
	    pushl	%edx	/* Store %ebx */
	    pushf		/* Push flags */
	    popl	%edx	/* Get them */
	    movl	$0,%eax
	    bt		$4,%edx	/* AF flag mapped to X flag */
	    rcl		$1,%eax
	    bt		$7,%edx	/* SF flag mapped to N flag */
	    rcl		$1,%eax
	    bt		$6,%edx	/* ZF flag mapped to Z flag */
	    rcl		$1,%eax
	    bt		$11,%edx /*OF flag mapped to V flag */
	    rcl		$1,%eax
	    bt		$0,%eax	/* CF flag mapped to C flag */
	    rcl		$1,%eax
	    popl	%edx
	    ret

