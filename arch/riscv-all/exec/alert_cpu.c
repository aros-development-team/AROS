/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved.

    Desc: RISC-V CPU context parsing routines.
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

/* %P prints a whole register (IPTR) at its width; %lx would read a ULONG */
static const char *gpr_fmt = "X5 =0x%P  X6 =0x%P  X7 =0x%P\n"
                             "X8 =0x%P  X9 =0x%P  X10=0x%P  X11=0x%P\n"
                             "X12=0x%P  X13=0x%P  X14=0x%P  X15=0x%P\n"
                             "X16=0x%P  X17=0x%P  X18=0x%P  X19=0x%P\n"
                             "X20=0x%P  X21=0x%P  X22=0x%P  X23=0x%P\n"
                             "X24=0x%P  X25=0x%P  X26=0x%P  X27=0x%P\n"
                             "X28=0x%P  X29=0x%P  X30=0x%P  X31=0x%P\n"
                             "SP =0x%P  RA =0x%P  PC =0x%P\n";

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
     * restores ra from one register slot below fp and the caller's fp
     * from two slots below.
     */
    APTR *frame = fp;

    *caller = frame[-1];
    return frame[-2];
}
