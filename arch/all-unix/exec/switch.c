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

    KrnSwitch();

    AROS_LIBFUNC_EXIT
} /* Switch() */
