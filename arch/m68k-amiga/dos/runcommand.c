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
	BOOL			 is_bcpl,
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
    BPTR oldinput = BNULL, newinput = BNULL;

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

    /* Need to create filehandle that points to arguments.
     * Lets (badly) misuse buffered NIL: and hope it works..
     * Guru Book mentions this (but related to CreateNewProc())
     * which means something isn't 100% correct..
     *
     * This fixes for example C:Execute
     */
    {
    	newinput = Open("NIL:", MODE_NEWFILE);
	if (newinput) {
	    struct FileHandle *fh = BADDR(newinput);
	    if (!SetVBuf(newinput, NULL, BUF_FULL, argsize)) {
	    	/* ugly hack */
	    	memcpy(fh->fh_Buf, argptr, argsize);
	    	fh->fh_Pos = fh->fh_Buf;
	    	fh->fh_End = fh->fh_Buf + argsize;
	    	oldinput = SelectInput(newinput);
	    }
	}
    }

    D(bug("RunCommand: segList @%p Args='%s'\n", BADDR(segList), argptr));
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    do {
    	extern BOOL BCPL_Setup(struct Process *me, BPTR segList, APTR DOSBase);
    	extern void BCPL_Cleanup(struct Process *me);
    	APTR old_GlobVec;
    	old_GlobVec = me->pr_GlobVec;
    	ret = BCPL_Setup(me, segList, DOSBase);
    	if (ret < 0) {
    	    break;
    	}
    	if (0 && ret == 1) {
    	    ret=AROS_SLIB_ENTRY(RunProcess,Dos)(me,&sss,argptr,argsize,
    	                (LONG_FUNC)(((ULONG *)me->pr_GlobVec)[1]),TRUE,DOSBase);
    	} else
#endif
    ret=AROS_SLIB_ENTRY(RunProcess,Dos)(me,&sss,argptr,argsize,
		(LONG_FUNC)((BPTR *)BADDR(segList)+1),FALSE,DOSBase);
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    	BCPL_Cleanup(me);
    	me->pr_GlobVec = old_GlobVec;
    } while (0);
#endif
    me->pr_Arguments=oldargs;

    oldresult=me->pr_Result2;
    /* restore saved iet_startup */
    GetIntETask(me)->iet_startup = oldstartup;

    me->pr_Result2=oldresult;

    if (oldinput)
    	SelectInput(oldinput);
    if (newinput)
    	Close(newinput);

    FreeMem(stack,stacksize);
    
    return ret;
    
    AROS_LIBFUNC_EXIT
    
} /* RunCommand */
