/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1997/03/19 16:35:07  digulla
    Removed log

    Corrected pr_ReturnAddr


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
    process->pr_ReturnAddr=sp-3;
    process->pr_Task.tc_SPReg=(STRPTR)sp-SP_OFFSET;
    return (struct Process *)AddTask(&process->pr_Task,initialPC,finalPC);
} /* AddProcess */
