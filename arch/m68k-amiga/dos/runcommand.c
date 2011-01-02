/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Execute a loaded command synchronously
    Lang: english
*/
#define DEBUG 1
#include <aros/debug.h>

#include <exec/memory.h>
#include "../exec/etask.h"
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <dos/stdio.h>
#include "dos_intern.h"

LONG AROS_SLIB_ENTRY(RunProcess,Dos)
(
	struct Process	       * proc,
	struct StackSwapStruct * sss,
	CONST_STRPTR		 argptr,
	ULONG			 argsize,
	LONG_FUNC		 entry,
	struct DosLibrary      * DOSBase
);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(LONG, RunCommand,

/*  SYNOPSIS */
	AROS_LHA(BPTR,   segList,   D1),
	AROS_LHA(ULONG,  stacksize, D2),
	AROS_LHA(CONST_STRPTR, argptr,    D3),
	AROS_LHA(ULONG,  argsize,   D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 84, Dos)

/*  FUNCTION
	RunCommand() will run the command loaded in the |segList| with the
	arguments specified with a new stack of |stacksize| bytes. Note
	that the stacksize may be extended if this is required.

	The return code of the command run will be returned.

	AROS ONLY: RunCommand() automatically computes argsize
	automatically if you pass -1.

	This call will not return until the command has completed.

    INPUTS
	segList		- segment of program to run.
	stacksize	- size of the stack to use.
	argptr		- pointer to NULL-terminated arguments.
	argsize		- size of the arguments string.

    RESULT
	The return code from the program. See also IoErr().

    NOTES
	Programs expect the argument string to end with a newline ('\n')
	character (ReadArgs() requires it to work properly).

    EXAMPLE

    BUGS

    SEE ALSO
	SystemTagList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR oldargs;
    LONG oldresult;
    struct aros_startup * oldstartup;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    UBYTE *stack;
    LONG ret;
    struct StackSwapStruct sss;
    BPTR oldinput = BNULL;
    struct FileHandle *fhinput = NULL;

    if(stacksize < AROS_STACKSIZE)
	stacksize = AROS_STACKSIZE;

    stack=(UBYTE *)AllocMem(stacksize,MEMF_ANY);
    if(stack==NULL)
	return -1;

    sss.stk_Lower=stack;
    sss.stk_Upper=stack+stacksize;
    sss.stk_Pointer = sss.stk_Upper;

    oldresult=me->pr_Result2;
    /* we have to save iet_startup field because it's overwritten in 
       startup code */
    oldstartup = (struct aros_startup *)GetIntETask(me)->iet_startup;
    
    me->pr_Result2=oldresult;

    oldargs=me->pr_Arguments;
    me->pr_Arguments=(STRPTR)argptr;

    /* Need to inject command arguments to the beginning of input handle.
     * Guru Book mentions this (but related to CreateNewProc())
     * which means something isn't 100% correct..
     *
     * This fixes for example C:Execute
     * Must be always buffered or EndCLI won't work
     */
    oldinput = Input();
    if (oldinput) {
    	fhinput = BADDR(oldinput);
    	if (vbuf_alloc(fhinput, 208, DOSBase) && IsInteractive(oldinput)) {
    	    int size = argsize < 0 ? strlen(argptr) : argsize;
    	    if (size > 0) {
    	    	if (size > 208)
    	    	    size = 208;
	        /* ugly hack */
	        memcpy(fhinput->fh_Buf, argptr, size);
	        fhinput->fh_Pos = fhinput->fh_Buf;
	        fhinput->fh_End = fhinput->fh_Buf + size;
	    }
	}
    }

    D(bug("RunCommand: segList @%p I=%x O=%x Args='%s'\n", BADDR(segList), BADDR(Input()), BADDR(Output()), argptr));

    ret=AROS_SLIB_ENTRY(RunProcess,Dos)(me,&sss,argptr,argsize,
		(LONG_FUNC)((BPTR *)BADDR(segList)+1),DOSBase);

    me->pr_Arguments=oldargs;

    oldresult=me->pr_Result2;
    /* restore saved iet_startup */
    GetIntETask(me)->iet_startup = oldstartup;

    me->pr_Result2=oldresult;

    /* remove buffered argument stream */
    /* must be original stream, command might have called SelectInput() */
    Flush(oldinput);

    FreeMem(stack,stacksize);
    
    return ret;
    
    AROS_LIBFUNC_EXIT
    
} /* RunCommand */
