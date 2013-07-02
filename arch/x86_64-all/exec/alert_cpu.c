/*
    Copyright © 2011-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context parsing routines, x86-64 version.
    Lang: english
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"

static const char *gpr_fmt = "RAX=%P  RBX=%P  RCX=%P  RDX=%P\n"
			     "RSI=%P  RDI=%P  RSP=%P  RBP=%P\n"
			     "R8 =%P  R9 =%P  R10=%P  R11=%P\n"
	 		     "R12=%P  R13=%P  R14=%P  R15=%P\n"
			     "RIP=%P  RSP=%P  RFLAGS=%P";

static const char *seg_fmt = "\nCS=%04lx  SS=%04lx  DS=%04lx\n"
			       "ES=%04lx  FS=%04lx  GS=%04lx";

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    VOID_FUNC dest = buffer ? RAWFMTFUNC_STRING : RAWFMTFUNC_SERIAL;
    char *buf;

    buf = NewRawDoFmt(gpr_fmt, dest, buffer,
		      ctx->rax, ctx->rbx, ctx->rcx, ctx->rdx,
		      ctx->rsi, ctx->rdi, ctx->rsp, ctx->rbp,
		      ctx->r8 , ctx->r9 , ctx->r10, ctx->r11,
		      ctx->r12, ctx->r13, ctx->r14, ctx->r15,
		      ctx->rip, ctx->rsp, ctx->rflags);
    if (ctx->Flags & ECF_SEGMENTS)
    {
	buf = NewRawDoFmt(seg_fmt, dest, buf - 1,
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
