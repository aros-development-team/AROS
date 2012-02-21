/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context parsing routines.
    Lang: english
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"

static const char *gpr_fmt = "EAX=0x%08lx  EBX=0x%08lx  ECX=0x%08lx  EDX=0x%08lx\n"
			     "ESI=0x%08lx  EDI=0x%08lx  ESP=0x%08lx  EBP=0x%08lx\n"
			     "EIP=0x%08lx  ESP=0x%08lx  EFLAGS=0x%08lx";

static const char *seg_fmt = "\nCS=%04lx  SS=%04lx  DS=%04lx\n"
			       "ES=%04lx  FS=%04lx  GS=%04lx";

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    char *buf;

    buf = NewRawDoFmt(gpr_fmt, RAWFMTFUNC_STRING, buffer,
		      ctx->eax, ctx->ebx, ctx->ecx, ctx->edx,
		      ctx->esi, ctx->edi, ctx->esp, ctx->ebp,
		      ctx->eip, ctx->esp, ctx->eflags);
    if (ctx->Flags & ECF_SEGMENTS)
    {
	buf = NewRawDoFmt(seg_fmt, RAWFMTFUNC_STRING, buf - 1,
			  ctx->cs, ctx->ss, ctx->ds,
			  ctx->es, ctx->fs, ctx->gs);
    }

    return buf - 1;
}

/* Unwind a single stack frame. CPU-dependent. */
APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *ebp = fp;

    *caller = ebp[1];	/* Fill in caller address		*/
    return ebp[0];	/* Return pointer to the previous frame */
}
