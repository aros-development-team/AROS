/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context parsing routines. Dummy nonfunctional template.
          See code in arch/i386-all/exec/alert_cpu.c for working example.
    Lang: english
*/

#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx,
    struct ExecBase *SysBase)
{
    char *buf;
    
    buf = Alert_AddString(buffer, "Not implemented");
    *buf = 0;

    return buf;
}

/* Unwind a single stack frame */
APTR UnwindFrame(APTR fp, APTR *caller)
{
    return NULL;
}
