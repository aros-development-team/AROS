/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: libinit library - Set the SysBase global variable when needed
*/

#include <proto/exec.h>
#include <aros/symbolsets.h>

struct ExecBase *SysBase;

AROS_SET_LIBFUNC(SetSysBase, struct ExecBase *, sysBase)
{
    SysBase = sysBase;
    
    return TRUE;
}

ADD2SET(SetSysBase, sysinit, 0);
