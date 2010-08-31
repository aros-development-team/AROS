/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix version  of Switch().
    Lang: english
*/

#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "exec_intern.h"

AROS_LH0(void, Switch,
    struct ExecBase *, SysBase, 9, Exec)
{
    AROS_LIBFUNC_INIT

    struct Task *this = SysBase->ThisTask;

    /*
	If the state is not TS_RUN then the task is already in a list
    */

    Disable();
    
    if( this->tc_State != TS_RUN )
	KrnDispatch();

    Enable();
    
    AROS_LIBFUNC_EXIT
} /* Switch() */
