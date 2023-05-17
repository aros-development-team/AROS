/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: RISC-V CPU context parsing routines.
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

static const char *gpr_fmt = "X5=0x%08lx  X6 =0x%08lx  X7 =0x%08lx\n"
                             "X8=0x%08lx  X9=0x%08lx  X10 =0x%08lx  X11 =0x%08lx\n"
                             "X12=0x%08lx  X13=0x%08lx  X14=0x%08lx  X15=0x%08lx\n"
                             "X16=0x%08lx  X17=0x%08lx  X18=0x%08lx  X19=0x%08lx\n"
                             "X20=0x%08lx  X21=0x%08lx  X22=0x%08lx  X23=0x%08lx\n"
                             "X24=0x%08lx  X25=0x%08lx  X26=0x%08lx  X27=0x%08lx\n"
                             "X28=0x%08lx  X29=0x%08lx  X30=0x%08lx  X31=0x%08lx\n"
                             "SP=0x%08lx  RA =0x%08lx  PC =0x%08lx\n";

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
    APTR *frame = fp;

    *caller = frame[0];
    return frame[-1];
}
