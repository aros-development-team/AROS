/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Add a new Process
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/exec.h>
#include "dos_intern.h"

struct Process * AddProcess (
    struct Process    * process,
    STRPTR		argPtr,
    ULONG		argSize,
    APTR		initialPC,
    APTR		finalPC,
    struct DosLibrary * DOSBase)
{
    APTR * sp = process->pr_Task.tc_SPUpper;

    *--sp = SysBase;
    *--sp = (APTR)argSize;
    *--sp = argPtr;

    process->pr_ReturnAddr = sp-3;

    process->pr_Task.tc_SPReg  = (STRPTR)sp-SP_OFFSET;
    process->pr_Task.tc_Flags |= TF_ETASK;

    return (struct Process *)AddTask (&process->pr_Task, initialPC, finalPC);
} /* AddProcess */
