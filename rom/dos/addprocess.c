/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1997/03/17 17:07:20  digulla
    Pass args to process

    Revision 1.3  1997/01/27 00:36:13  ldp
    Polish

    Revision 1.2  1996/08/01 17:40:47  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/exec.h>
#include "dos_intern.h"

struct Process *AddProcess(struct Process *process, STRPTR argPtr,
ULONG argSize, APTR initialPC, APTR finalPC, struct DosLibrary *DOSBase)
{
    APTR *sp=process->pr_Task.tc_SPUpper;
    *--sp=SysBase;
    *--sp=(APTR)argSize;
    *--sp=argPtr;
    process->pr_ReturnAddr=sp-1;
    process->pr_Task.tc_SPReg=(STRPTR)sp-SP_OFFSET;
    return (struct Process *)AddTask(&process->pr_Task,initialPC,finalPC);
} /* AddProcess */
