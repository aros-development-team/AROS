/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Install an interrupt handler.
*/
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <aros/libcall.h>

#include "exec_intern.h"
#include "exec_debug.h"
#include "exec_locks.h"

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Interrupt *oldint;
    BOOL ishandler = intNumber <= 2 || (intNumber >= 6 && intNumber <= 12);

    ExecLog(SysBase, EXECDEBUGF_EXCEPTHANDLER, "SetIntVector: Int %d, Interrupt %p\n", intNumber, interrupt);

    EXEC_LOCK_LIST_WRITE_AND_DISABLE(&SysBase->IntrList);

    oldint = (struct Interrupt *)SysBase->IntVects[intNumber].iv_Node;
    SysBase->IntVects[intNumber].iv_Node = ishandler ? (struct Node *)interrupt : NULL;

    if (interrupt)
    {
        SysBase->IntVects[intNumber].iv_Data = interrupt->is_Data;
        SysBase->IntVects[intNumber].iv_Code = interrupt->is_Code;
    }
    else
    {
        SysBase->IntVects[intNumber].iv_Data = (APTR)~0;
        SysBase->IntVects[intNumber].iv_Code = (void *)~0;
    }

    EXEC_UNLOCK_LIST_AND_ENABLE(&SysBase->IntrList);

    return oldint;

    AROS_LIBFUNC_EXIT
} /* SetIntVector */
