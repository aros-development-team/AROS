/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: 64bit RISC-V CPU context parsing routines.
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

static const char *gpr_fmt = "X5 =0x%016lx  X6 =0x%016lx  X7 =0x%016lx\n"
                             "X8 =0x%016lx  X9 =0x%016lx  X10=0x%016lx  X11=0x%016lx\n"
                             "X12=0x%016lx  X13=0x%016lx  X14=0x%016lx  X15=0x%016lx\n"
                             "X16=0x%016lx  X17=0x%016lx  X18=0x%016lx  X19=0x%016lx\n"
                             "X20=0x%016lx  X21=0x%016lx  X22=0x%016lx  X23=0x%016lx\n"
                             "X24=0x%016lx  X25=0x%016lx  X26=0x%016lx  X27=0x%016lx\n"
                             "X28=0x%016lx  X29=0x%016lx  X30=0x%016lx  X31=0x%016lx\n"
                             "SP =0x%016lx  RA =0x%016lx  PC =0x%016lx\n";

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    VOID_FUNC dest = buffer ? RAWFMTFUNC_STRING : RAWFMTFUNC_SERIAL;
    char *buf;

    buf = NewRawDoFmt(gpr_fmt, dest, buffer,
                      ctx->x[2], ctx->x[3], ctx->x[4] ,
                      ctx->x[5], ctx->x[6], ctx->x[7] , ctx->x[8] ,
                      ctx->x[9], ctx->x[10], ctx->x[11], ctx->x[12],
                      ctx->x[13], ctx->x[14], ctx->x[15], ctx->x[16],
                      ctx->x[17], ctx->x[18], ctx->x[19], ctx->x[20],
                      ctx->x[21], ctx->x[22], ctx->x[23], ctx->x[24],
                      ctx->x[25], ctx->x[26], ctx->x[27], ctx->x[28],
                      ctx->sp  , ctx->ra   , ctx->pc);

    return buf - 1;
}

APTR UnwindFrame(APTR fp, APTR *caller)
{
    /*
     * The RISC-V frame pointer (s0) points to the CFA; the epilogue
     * restores ra from fp-8 and the caller's fp from fp-16.
     */
    APTR *frame = fp;

    *caller = frame[-1];
    return frame[-2];
}
