/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a new Process
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <aros/asmcall.h>
#include "dos_intern.h"

LONG DosEntry (
    STRPTR argPtr,
    ULONG argSize,
    APTR initialPC,
    struct ExecBase * sysBase)
{
    return AROS_UFC3 (LONG, initialPC,
	AROS_UFCA (STRPTR,            argPtr,  A0),
	AROS_UFCA (ULONG,             argSize, D0),
	AROS_UFCA (struct ExecBase *, sysBase, A6)
	);
}



struct Process *AddProcess(struct Process *process, STRPTR argPtr,
			   ULONG argSize, APTR initialPC, APTR finalPC,
			   struct DosLibrary *DOSBase)
{
    APTR *sp = process->pr_Task.tc_SPUpper;
  
    *--sp = SysBase;
    *--sp = initialPC;
    *--sp = (APTR)argSize;
    *--sp = argPtr;
    
    process->pr_ReturnAddr = sp - 4;
    
    process->pr_Task.tc_SPReg  = (STRPTR)sp - SP_OFFSET;
    process->pr_Task.tc_Flags |= TF_ETASK;
    
    addprocesstoroot(process, DOSBase);
    
    return (struct Process *)AddTask(&process->pr_Task, (APTR)DosEntry,
				     finalPC);
} /* AddProcess */
