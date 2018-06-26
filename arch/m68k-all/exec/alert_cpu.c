/*
    Copyright © 2010-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context parsing routines.
    Lang: english
*/

#include <string.h>

#include <exec/rawfmt.h>

#include "exec_intern.h"
#include "exec_util.h"

AROS_UFP3(UBYTE*, SegTrack,
    AROS_UFPA(ULONG, Address, A0),
    AROS_UFPA(ULONG*, SegNum, A1),
    AROS_UFPA(ULONG*, Offset, A2));

struct SegSem
{
    struct SignalSemaphore seg_Semaphore;
    APTR seg_Find;
};
#define SEG_SEM "SegTracker"

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    struct SegSem *segSem;
    VOID_FUNC dest = buffer ? RAWFMTFUNC_STRING : RAWFMTFUNC_SERIAL;
    char *buf;

    RawDoFmt("D0: %08lx %08lx %08lx %08lx\n"
                   "D4: %08lx %08lx %08lx %08lx\n"
                   "A0: %08lx %08lx %08lx %08lx\n"
                   "A4: %08lx %08lx %08lx %08lx\n"
                   "SR:     %04x\n"
                   "PC: %08lx", (RAWARG)ctx, dest, buffer);
    buf = buffer + strlen(buffer);

    segSem = (struct SegSem *)FindSemaphore(SEG_SEM);
    if (segSem) {
        ULONG SegNum, SegOffset, SegList;
        Forbid();
        UBYTE *name = AROS_UFC3(UBYTE*, segSem->seg_Find,
            AROS_UFCA(ULONG, ctx->pc, A0),
            AROS_UFCA(ULONG*, &SegNum, A1),
            AROS_UFCA(ULONG*, &SegOffset, A2));
        if (name) {
            AROS_UFC3(UBYTE*, segSem->seg_Find,
               AROS_UFCA(ULONG, ctx->pc, A0),
               AROS_UFCA(ULONG*, &SegList, A1),
               AROS_UFCA(ULONG*, &SegList, A2));
            buf = NewRawDoFmt("\nSegTracker: %s\nHunk %ld, Offset $%08lx, SegList $%08lx\n", dest, buf, name, SegNum, SegOffset, SegList);
        }
        Permit();
    }
    return buf - 1;
}

/* Unwind a single stack frame */
APTR UnwindFrame(APTR fp, APTR *caller)
{
    return NULL;
}
