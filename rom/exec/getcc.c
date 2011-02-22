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

    /*  As with SetSR() you can either do nothing, or alternatively read
	you own registers and assemble them into the form of the MC680x0
	condition codes.
    */
    /* TODO: Write exec/GetCC() */
    aros_print_not_implemented("GetCC");
    return ~0;

    AROS_LIBFUNC_EXIT
} /* GetCC() */
