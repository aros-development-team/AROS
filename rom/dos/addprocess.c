/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a new Process
    Lang: english
*/
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <aros/asmcall.h>
#include "dos_intern.h"

#ifdef SysBase
#undef SysBase
#endif

LONG DosEntry (
    STRPTR argPtr,
    ULONG argSize,
    APTR initialPC,
    struct ExecBase * SysBase)
{
	LONG result = AROS_UFC3 (LONG, initialPC,
 	              AROS_UFCA (STRPTR,            argPtr,  A0),
	              AROS_UFCA (ULONG,             argSize, D0),
	              AROS_UFCA (struct ExecBase *, SysBase, A6));

        /* Place the return code in the tc_Userdata field of the Task
	   structure, so that it can be used by pr_ExitCode-
	   
	   tc_UserData is safe to use because at this point we are out of the process'
	   main code
	*/
        FindTask(NULL)->tc_UserData = (APTR)result;

	return result;
}

#undef SysBase
#define SysBase (DOSBase->dl_SysBase)

struct Process *AddProcess(struct Process *process, STRPTR argPtr,
			   ULONG argSize, APTR initialPC, APTR finalPC,
			   struct DosLibrary *DOSBase)
{
#if 1
    struct TagItem tags[] =
    {
    	{TASKTAG_ARG1, (IPTR)argPtr 	},
	{TASKTAG_ARG2, (IPTR)argSize	},
	{TASKTAG_ARG3, (IPTR)initialPC	},
	{TASKTAG_ARG4, (IPTR)SysBase	},
	{TAG_DONE   	    	    	}
    };
    
#if AROS_STACK_GROWS_DOWNWARDS
    process->pr_Task.tc_SPReg = (STRPTR)process->pr_Task.tc_SPUpper - SP_OFFSET;
#else
    process->pr_Task.tc_SPReg = (STRPTR)process->pr_Task.tc_SPLower + SP_OFFSET;
#endif

    process->pr_ReturnAddr = (APTR *)process->pr_Task.tc_SPReg - 4; /* ???? */
    process->pr_Task.tc_Flags |= TF_ETASK;

    addprocesstoroot(process, DOSBase);

    return (struct Process *)NewAddTask(&process->pr_Task, (APTR)DosEntry,
				     	finalPC, tags);

#else
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

#endif

} /* AddProcess */
