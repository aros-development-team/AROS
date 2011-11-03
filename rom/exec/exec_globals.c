/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Global variables referenced by exec.library
 */

#include <aros/symbolsets.h>
#include <exec/execbase.h>

struct ExecBase *AbsExecBase;   /* Global SysBase */

static int Exec_init_globals(struct ExecBase *lh)
{
    AbsExecBase = lh;

    return TRUE;
}

ADD2INITLIB(Exec_init_globals,0)
