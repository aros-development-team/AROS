/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PowerPC CPU context parsing routines.
    Lang: english
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"

static const char *gpr_fmt = "MSR=%08lx  IP=%08lx   CR =%08lx  XER=%08lx\n"
			     "CTR=%08lx  LR=%08lx DSISR=%08lx  DAR=%08lx\n"
			     "R0 =%08lx  R1=%08lx   R2 =%08lx  R3 =%08lx\n"
			     "R4 =%08lx  R5=%08lx   R6 =%08lx  R7 =%08lx\n"
			     "R8 =%08lx  R9=%08lx   R10=%08lx  R11=%08lx\n"	\
			     "R12=%08lx";

static const char *gp2_fmt =          " R13=%08lx   R14=%08lx  R15=%08lx\n"
			     "R16=%08lx R17=%08lx   R18=%08lx  R19=%08lx\n"
			     "R20=%08lx R21=%08lx   R22=%08lx  R23=%08lx\n"
			     "R24=%08lx R25=%08lx   R26=%08lx  R27=%08lx\n"
			     "R28=%08lx R29=%08lx   R30=%08lx  R31=%08lx";

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    char *buf;

    buf = NewRawDoFmt(gpr_fmt, RAWFMTFUNC_STRING, buffer,
		      ctx->msr    , ctx->ip    , ctx->cr     , ctx->xer,
		      ctx->ctr    , ctx->lr    , ctx->dsisr  , ctx->dar,
		      ctx->gpr[0] , ctx->gpr[1], ctx->gpr[2] , ctx->gpr[3],
		      ctx->gpr[4] , ctx->gpr[5], ctx->gpr[6] , ctx->gpr[7],
		      ctx->gpr[8] , ctx->gpr[9], ctx->gpr[10], ctx->gpr[11],
		      ctx->gpr[12]);
    if (ctx->Flags & ECF_FULL_GPRS)
    {
	buf = NewRawDoFmt(gp2_fmt, RAWFMTFUNC_STRING, buf - 1,
			  ctx->gpr[13], ctx->gpr[14], ctx->gpr[15],
			  ctx->gpr[16], ctx->gpr[17], ctx->gpr[18],  ctx->gpr[19],
			  ctx->gpr[20], ctx->gpr[21], ctx->gpr[22],  ctx->gpr[23],
			  ctx->gpr[24], ctx->gpr[25], ctx->gpr[26],  ctx->gpr[27],
			  ctx->gpr[28], ctx->gpr[29], ctx->gpr[30],  ctx->gpr[31]);
    }

    return buf - 1;
}

/* Unwind a single stack frame. CPU-dependent. */
APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *sp = fp;
    
    sp      = sp[0];	/* Go to previous frame first			*/
    *caller = sp[1];	/* Caller address is stored in *caller's* frame */

    return sp;
}
