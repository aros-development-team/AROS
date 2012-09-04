/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
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
    VOID_FUNC dest = buffer ? RAWFMTFUNC_STRING : RAWFMTFUNC_SERIAL;
    char *buf;

    buf = NewRawDoFmt(gpr_fmt, dest, buffer,
		      ctx->r[0], ctx->r[1], ctx->r[2] , ctx->r[3] , 
		      ctx->r[4], ctx->r[5], ctx->r[6] , ctx->r[7] , 
		      ctx->r[8], ctx->r[9], ctx->r[10], ctx->r[11], 
		      ctx->ip  , ctx->sp  , ctx->lr   , ctx->pc   ,
		      ctx->cpsr);

    return buf - 1;
}

APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *frame = fp;

    *caller = frame[0];
    return frame[-1];
}
