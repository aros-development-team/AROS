/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:00  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/tasks.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

LONG RunProcess(struct Process *proc, struct StackSwapStruct *sss,
STRPTR argptr, ULONG argsize, LONG_FUNC entry)
{
    LONG ret;
    APTR *sp;
    extern struct DosLibrary *DOSBase;
    sp=(APTR *)sss->stk_Upper;
    *--sp=SysBase;
    *--sp=(APTR)argsize;
    *--sp=argptr;
    sss->stk_Pointer=sp;
    StackSwap(sss);
    ret=entry();
    StackSwap(sss);
    return ret;
}
