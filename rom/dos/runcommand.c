/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:40:57  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>

LONG RunProcess(struct Process *proc, struct StackSwapStruct *sss,
STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct DosLibrary *DOSBase);

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH4(LONG, RunCommand,

/*  SYNOPSIS */
	__AROS_LA(BPTR,   segList,   D1),
	__AROS_LA(ULONG,  stacksize, D2),
	__AROS_LA(STRPTR, argptr,    D3),
	__AROS_LA(ULONG,  argsize,   D4),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

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
    sss.stk_Upper=(ULONG)stack+stacksize;

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
    ret=RunProcess(me,&sss,argptr,argsize,(APTR)BADDR(segList+1),DOSBase);
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
    __AROS_FUNC_EXIT
} /* RunCommand */
