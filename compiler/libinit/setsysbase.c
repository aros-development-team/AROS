/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$

    Set the global SysBase variable when needed.
*/

#include <proto/exec.h>
#include <aros/symbolsets.h>

struct ExecBase *SysBase;

AROS_SET_LIBFUNC(SetSysBase, struct ExecBase *, sysBase)
{
    AROS_SET_LIBFUNC_INIT
    
    SysBase = sysBase;
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2SET(SetSysBase, sysinit, 0);
