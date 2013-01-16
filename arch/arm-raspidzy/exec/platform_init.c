/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <proto/exec.h>
#include <strings.h>

static int PlatformInit(struct ExecBase *SysBase)
{
    return TRUE;
}

ADD2INITLIB(PlatformInit, 0)
