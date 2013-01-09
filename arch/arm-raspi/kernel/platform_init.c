/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <strings.h>

#include "exec_intern.h"
#include "etask.h"

#include "kernel_intern.h"
#include "kernel_fb.h"
#include "kernel_romtags.h"

static int PlatformInit(struct KernelBase *KernelBase)
{
    D(bug("[Kernel] PlatformInit()\n"));
    return TRUE;
}

ADD2INITLIB(PlatformInit, 0)

struct KernelBase *getKernelBase()
{
    return (struct KernelBase *)KernelBase;
}
