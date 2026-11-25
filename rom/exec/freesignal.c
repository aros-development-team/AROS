/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Free a signal.
*/
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, FreeSignal,

/*  SYNOPSIS */
        AROS_LHA(LONG, signalNum, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 56, Exec)

/*  FUNCTION
        Free a signal allocated with AllocSignal().

    INPUTS
        signalNum - Number of the signal to free or -1 to do nothing.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocSignal(), Signal(), Wait()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* A signal number is a byte (-1 = no signal). Callers such as
       DeleteMsgPort() pass mp_SigBit, a UBYTE, so "no signal" arrives here
       as 255 and passed the != -1 test: 1 << 255 is undefined (x86 clears
       bit 31; GCC for 68020+ emits a bit-field clear that writes outside
       tc_SigAlloc). Only clear bits 0-31. */
    signalNum = (BYTE)signalNum;
    if (signalNum >= 0 && signalNum < 32)
    {
        /* No more atomic problem - i beleive THIS is atomic. - sonic */
        struct Task *me = GET_THIS_TASK;

        /* Clear the bit */
        if (me)
            me->tc_SigAlloc &= ~(1 << signalNum);
    }
    AROS_LIBFUNC_EXIT
} /* FreeSignal() */

