/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: i386unix version of PrepareContext().
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include "sigcore.h"

#include <aros/libcall.h>

AROS_LH3(APTR, PrepareContext,
    AROS_LHA(SP_TYPE *, stackPointer, A0),
    AROS_LHA(APTR, entryPoint, A1),
    AROS_LHA(APTR, fallBack, A2),
    struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    /*
	There is not much to do here, or at least that is how it
	appears. Most of the work is done in the sigcore.h macros.
    */

    /* First we push the return address */
    _PUSH(stackPointer, fallBack);

    /* Then set up the frame to be used by Dispatch() */
    PREPARE_INITIAL_FRAME(stackPointer, entryPoint);

    /* We return the new stack pointer back to the caller. */
    return stackPointer;

    AROS_LIBFUNC_EXIT
}
