/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1997/01/01 03:46:03  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.1  1996/11/14 08:51:34  aros
    Some work on the kernel:
    Mapping of Linux-Signals to AROS interrupts
    Some documentation to the exec microkernel
    hopefully all holes plugged now


    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */

	AROS_LH2(void, AddIntServer,

/*  SYNOPSIS */
	AROS_LHA(ULONG,              intNumber, D0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 28, Exec)

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
    while(prev->is_Node.ln_Succ!=NULL&&
	  prev->is_Node.ln_Succ->ln_Pri<interrupt->is_Node.ln_Pri)
	prev=(struct Interrupt *)prev->is_Node.ln_Succ;
    interrupt->is_Node.ln_Succ=prev->is_Node.ln_Succ;
    prev->is_Node.ln_Succ=&interrupt->is_Node;
    Enable();
    AROS_LIBFUNC_EXIT
} /* AddIntServer */
