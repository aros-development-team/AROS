/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free a trap.
    Lang: english
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, FreeTrap,

/*  SYNOPSIS */
	AROS_LHA(long, trapNum, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 58, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Very similar to FreeSignal()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(trapNum!=-1)
    {
        /* No more atomic problem - i beleive THIS is atomic. - sonic */
        struct Task *me = SysBase->ThisTask;

        if (me->tc_Flags & TF_ETASK) {
	    struct ETask *et = me->tc_UnionETask.tc_ETask;
	    
	    et->et_TrapAlloc &= ~(1<<trapNum);
	} else
	    me->tc_TrapAlloc &= ~(1<<trapNum);
    }

    AROS_LIBFUNC_EXIT
} /* FreeTrap */
