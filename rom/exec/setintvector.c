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

	AROS_LH2(struct Interrupt *, SetIntVector,

/*  SYNOPSIS */
	AROS_LHA(ULONG,              intNumber, D0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 27, Exec)

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
    struct Interrupt *old;
    Disable();
    old=(struct Interrupt *)SysBase->IntVects[intNumber].iv_Node;
    SysBase->IntVects[intNumber].iv_Data=interrupt->is_Data;
    SysBase->IntVects[intNumber].iv_Code=interrupt->is_Code;
    SysBase->IntVects[intNumber].iv_Node=&interrupt->is_Node;
    Enable();
    return old;
    AROS_LIBFUNC_EXIT
} /* SetIntVector */
