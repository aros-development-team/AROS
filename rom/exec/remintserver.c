/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/11/14 08:51:35  aros
    Some work on the kernel:
    Mapping of Linux-Signals to AROS interrupts
    Some documentation to the exec microkernel
    hopefully all holes plugged now


    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <clib/exec_protos.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */

	AROS_LH2(void, RemIntServer,

/*  SYNOPSIS */
	AROS_LHA(ULONG,              intNumber, D0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 29, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Interrupt *prev;
    Disable();
    prev=(struct Interrupt *)&SysBase->IntVects[intNumber].iv_Data;
    while(prev->is_Node.ln_Succ!=&interrupt->is_Node)
	prev=(struct Interrupt *)prev->is_Node.ln_Succ;
    prev->is_Node.ln_Succ=interrupt->is_Node.ln_Succ;
    Enable();
    AROS_LIBFUNC_EXIT
} /* RemIntServer */
