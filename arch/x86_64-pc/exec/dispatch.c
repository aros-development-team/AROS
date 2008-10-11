/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Dispatch() entry :)
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <exec/alerts.h>

#include <proto/arossupport.h>
#include <proto/kernel.h>
#include <aros/asmcall.h>
#include <aros/kernel.h>


AROS_LH0(void, Dispatch,
         struct ExecBase *, SysBase, 10, Exec)
{
    AROS_LIBFUNC_INIT

    void *KernelBase = TLS_GET(KernelBase);

    KrnDispatch();
    
    AROS_LIBFUNC_EXIT
}
