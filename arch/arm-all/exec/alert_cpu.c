/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ARM CPU context parsing routines.
    Lang: english
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

static const char *gpr_fmt = "R0=0x%08lx  R1=0x%08lx  R2 =0x%08lx  R3 =0x%08lx\n"
			     "R4=0x%08lx  R5=0x%08lx  R6 =0x%08lx  R7 =0x%08lx\n"
			     "R8=0x%08lx  R9=0x%08lx  R10=0x%08lx  R11=0x%08lx\n"
			     "IP=0x%08lx  SP=0x%08lx  LR =0x%08lx  PC =0x%08lx\n"
			     "CPSR=0x%08lx";

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    char *buf;

    buf = RawDoFmt(gpr_fmt, ctx->r, RAWFMTFUNC_STRING, buffer);

    return buf - 1;
}

/*
 * On ARM we don't have frame pointer and can't do a full backtrace.
 * However in case of CPU trap we can trace down one call. This is done
 * by remembering value of lr register in iet_AlertStack. This routine
 * will then unwind this pseudo-frame.
 */
APTR UnwindFrame(APTR fp, APTR *caller)
{
    *caller = fp;
    return NULL;
}
