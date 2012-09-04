/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context parsing routines. Dummy nonfunctional template.
          See code in arch/i386/all/exec/alert_cpu.c for working example.
    Lang: english
*/

#include <string.h>

#include <exec/rawfmt.h>

#include "exec_intern.h"
#include "exec_util.h"

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    VOID_FUNC dest = buffer ? RAWFMTFUNC_STRING : RAWFMTFUNC_SERIAL;
    char *buf;
    
    buf = RawDoFmt("D0: %08lx %08lx %08lx %08lx\n"
                   "D4: %08lx %08lx %08lx %08lx\n"
                   "A0: %08lx %08lx %08lx %08lx\n"
                   "A4: %08lx %08lx %08lx %08lx\n"
                   "SR:     %04x\n"
                   "PC: %08lx", ctx, dest, buffer);

    return buf - 1;
}

/* Unwind a single stack frame */
APTR UnwindFrame(APTR fp, APTR *caller)
{
    return NULL;
}
