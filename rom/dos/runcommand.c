/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.11  1997/03/19 16:35:42  digulla
    Removed log


    Desc: Execute a loaded command synchonously
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"

LONG AROS_SLIB_ENTRY(RunProcess,Dos)(struct Process *proc,
	struct StackSwapStruct *sss, STRPTR argptr, ULONG argsize,
	LONG_FUNC entry, struct DosLibrary *DOSBase);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(LONG, RunCommand,

/*  SYNOPSIS */
	AROS_LHA(BPTR,   segList,   D1),
	AROS_LHA(ULONG,  stacksize, D2),
	AROS_LHA(STRPTR, argptr,    D3),
	AROS_LHA(ULONG,  argsize,   D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 84, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    STRPTR oldargs;
    LONG oldresult;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    UBYTE *stack;
    LONG ret;
    struct StackSwapStruct sss;

    stack=(UBYTE *)AllocMem(stacksize,MEMF_ANY);
    if(stack==NULL)
	return -1;

    sss.stk_Lower=stack;
    sss.stk_Upper=(IPTR)stack+stacksize;

    oldresult=me->pr_Result2;
    if(me->pr_CIS)
	Flush(me->pr_CIS);
    if(me->pr_COS)
	Flush(me->pr_COS);
    if(me->pr_CES)
	Flush(me->pr_CES);
    me->pr_Result2=oldresult;

    oldargs=me->pr_Arguments;
    me->pr_Arguments=argptr;
    ret=AROS_SLIB_ENTRY(RunProcess,Dos)(me,&sss,argptr,argsize,
		(LONG_FUNC)((BPTR *)BADDR(segList)+1),DOSBase);
    me->pr_Arguments=oldargs;

    oldresult=me->pr_Result2;
    if(me->pr_CIS)
	Flush(me->pr_CIS);
    if(me->pr_COS)
	Flush(me->pr_COS);
    if(me->pr_CES)
	Flush(me->pr_CES);
    me->pr_Result2=oldresult;

    FreeMem(stack,stacksize);
    return ret;
    AROS_LIBFUNC_EXIT
} /* RunCommand */
