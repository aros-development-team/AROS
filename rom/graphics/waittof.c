/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <exec/tasks.h>
#include <graphics/gfxbase.h>

static int bug; /* Added because of bug in cxref */

/*****************************************************************************

    NAME */

	AROS_LH0(VOID, WaitTOF,

/*  SYNOPSIS */

/*  LOCATION */
	struct GfxBase *, GfxBase, 45, Graphics)

/*  FUNCTION

    Wait for vertical blank.

    INPUTS

    RESULT

    Adds the task to the TOF queue; it will be signalled when the vertical
    blank interrupt occurs.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct Node wait;  /* We cannot use the task's node here as that is
			  used to queue the task in Wait() */

    wait.ln_Name = (char *)FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);

    Disable();

    AddTail((struct List *)&GfxBase->TOF_WaitQ, (struct Node *)&wait);
    Wait(SIGF_SINGLE);
    Remove((struct Node *)&wait);

    Enable();

    AROS_LIBFUNC_EXIT
    
} /* WaitTOF */
