/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: alert_cpu.c 36262 2010-12-27 12:17:48Z sonic $

    Desc: CPU context parsing routines, x86-64 version.
    Lang: english
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"

static const char *gpr_fmt = "RAX=0x%016lx  RBX=0x%016lx  RCX=0x%016lx  RDX=0x%016lx\n"
			     "RSI=0x%016lx  RDI=0x%016lx  RSP=0x%016lx  RBP=0x%016lx\n"
			     "R8 =0x%016lx  R9 =0x%016lx  R10=0x%016lx  R11=0x%016lx\n"
	 		     "R12=0x%016lx  R13=0x%016lx  R14=0x%016lx  R15=0x%016lx\n"
			     "RIP=0x%016lx  RSP=0x%016lx  RFLAGS=0x%016lx";

static const char *seg_fmt = "\nCS=%04lx  SS=%04lx  DS=%04lx\n"
			       "ES=%04lx  FS=%04lx  GS=%04lx";

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    char *buf;

    buf = NewRawDoFmt(gpr_fmt, RAWFMTFUNC_STRING, buffer,
		      ctx->rax, ctx->rbx, ctx->rcx, ctx->rdx,
		      ctx->rsi, ctx->rdi, ctx->rsp, ctx->rbp,
		      ctx->r8 , ctx->r9 , ctx->r10, ctx->r11,
		      ctx->r12, ctx->r13, ctx->r14, ctx->r15,
		      ctx->rip, ctx->rsp, ctx->rflags);
    if (ctx->Flags & ECF_SEGMENTS)
    {
	buf = NewRawDoFmt(seg_fmt, RAWFMTFUNC_STRING, buf - 1,
			  ctx->cs, ctx->ss, ctx->ds,
			  ctx->es, ctx->fs, ctx->gs);
    }

    return buf - 1;
}

/*
 * Unwind a single stack frame. CPU-dependent.
 * Note that for this to work you need to supply -fno-omit-frame-pointer to gcc.
 */
APTR UnwindFrame(APTR fp, APTR *caller)
{
    APTR *rbp = fp;

    *caller = rbp[1];	/* Fill in caller address		*/
    return rbp[0];	/* Return pointer to the previous frame */
}
