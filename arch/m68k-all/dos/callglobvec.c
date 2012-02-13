/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file with the specified mode.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "dos_intern.h"
#include "bcpl.h"

extern void BCPL_thunk(void);
#define  __is_process(task)  (((struct Task *)task)->tc_Node.ln_Type == NT_PROCESS)

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_UFH5(LONG, CallGlobVec,

/*  SYNOPSIS */
        AROS_UFHA(LONG, function, D0),
        AROS_UFHA(LONG, d1,       D1),
        AROS_UFHA(LONG, d2,       D2),
        AROS_UFHA(LONG, d3,       D3),
        AROS_UFHA(LONG, d4,       D4)

/*  LOCATION */
        )

/*  FUNCTION
        Private function to call a BCPL routine from C

    INPUTS
        function        - BCPL Global Vector function index
        d1..d4          - Parameter to the function

    RESULT
        return value of the BCPL routine

    NOTES
        This works only on m68k.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_USERFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    LONG ret;
    APTR oldReturnAddr;

    D(bug("%s: 0x%d (0x%x, 0x%x, 0x%x, 0x%x) GV %p\n", __func__, function, d1, d2, d3, d4, __is_process(me) ? me->pr_GlobVec : NULL));

    if (!__is_process(me))
        return DOSFALSE;

    oldReturnAddr = me->pr_ReturnAddr;
    ret = AROS_UFC8(LONG, BCPL_thunk,
            AROS_UFCA(LONG,  d1, D1),
            AROS_UFCA(LONG,  d2, D2),
            AROS_UFCA(LONG,  d3, D3),
            AROS_UFCA(LONG,  d4, D4),
            AROS_UFCA(APTR, me->pr_Task.tc_SPLower, A1),
            AROS_UFCA(APTR, me->pr_GlobVec, A2),
            AROS_UFCA(APTR, &me->pr_ReturnAddr, A3),
            AROS_UFCA(LONG_FUNC, ((LONG_FUNC *)me->pr_GlobVec)[function], A4));
    me->pr_ReturnAddr = oldReturnAddr;

    return ret;

    AROS_USERFUNC_EXIT
} /* CallGlobVec */

