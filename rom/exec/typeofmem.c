/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:19  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(ULONG, TypeOfMem,

/*  SYNOPSIS */
	__AROS_LA(APTR, address, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 89, Exec)

/*  FUNCTION
	Return type of memory at a given address or 0 if there is no memory
	there.

    INPUTS
	address - Address to test

    RESULT
	The memory flags you would give to AllocMem().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	18-10-95    Created by M. Fleischer

******************************************************************************/
{
    __AROS_FUNC_INIT

    ULONG ret=0;
    struct MemHeader *mh;

    /* Nobody should change the memory list now. */
    Forbid();

    /* Follow the list of MemHeaders */
    mh=(struct MemHeader *)SysBase->MemList.lh_Head;
    while(mh->mh_Node.ln_Succ!=NULL)
    {
	/* Check if this MemHeader fits */
	if(address>=mh->mh_Lower&&address<mh->mh_Upper)
	{
	    /* Yes. Prepare returncode */
	    ret=mh->mh_Attributes;
	    break;
	}
	/* Go to next MemHeader */
	mh=(struct MemHeader *)mh->mh_Node.ln_Succ;
    }
    /* Allow Taskswitches and return */
    Permit();
    return ret;
    __AROS_FUNC_EXIT
} /* TypeOfMem */

