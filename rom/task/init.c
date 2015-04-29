/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "taskres_intern.h"

static LONG taskres_Init(struct TaskResBase *TaskResBase)
{
    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    	return FALSE;

    return TRUE;
}

ADD2INITLIB(taskres_Init, 0)
