/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: dispatch.c 12747 2001-12-08 20:11:50Z chodorowski $

    Desc: Dispatch() entry :)
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <exec/alerts.h>

#include <proto/arossupport.h>
#include <aros/asmcall.h>

#include "core.h"

AROS_LH0(void, Dispatch,
         struct ExecBase *, SysBase, 10, Exec)
{
    AROS_LIBFUNC_INIT
    
    CoreDispatch();
    
    AROS_LIBFUNC_EXIT
}
