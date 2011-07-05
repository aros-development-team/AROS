/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Execute a loaded command synchronously
    Lang: english
*/

#include <aros/debug.h>

#include <exec/memory.h>
#include "../exec/etask.h"
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <dos/stdio.h>
#include "dos_intern.h"

#ifdef __mc68000

ULONG BCPL_CallEntry(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me);

#else

/* On non-m68k systems we don't implement BCPL ABI, and use the same entry code */

#define BCPL_CallEntry CallEntry

#endif

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
    volatile APTR oldReturnAddr;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    UBYTE *stack;
    LONG ret;
    struct StackSwapStruct sss;
    struct StackSwapArgs args;
    BPTR oldinput = BNULL;

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

    /* FIXME: should this be (argsize == -1), as documented? Or it's Amiga-compatible quirk? */
    if (argsize < 0)
    	argsize = strlen(argptr);

    /*
     * Inject command arguments to the beginning of input handle. Guru Book mentions this.
     * This fixes for example AmigaOS' C:Execute
     */
    oldinput = Input();
    D(bug("RunCommand: segList @%p I=0x%p O=%p Args='%s' Argsize=%u\n", BADDR(segList), oldinput, Output(), argptr, argsize));
    vbuf_inject(oldinput, argptr, argsize, DOSBase);

    /* pr_ReturnAddr is set by CallEntry routine */
    oldReturnAddr = me->pr_ReturnAddr;

    args.Args[0] = (IPTR)argptr;
    args.Args[1] = argsize;
    args.Args[2] = (IPTR)BADDR(segList) + sizeof(BPTR);
    args.Args[3] = (IPTR)me;

    ret = NewStackSwap(&sss, BCPL_CallEntry, &args);

    me->pr_ReturnAddr = oldReturnAddr;
    me->pr_Arguments  = oldargs;

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
