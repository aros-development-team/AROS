/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Examine memory
    Lang: english
*/
#include <exec/memory.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, TypeOfMem,

/*  SYNOPSIS */
	AROS_LHA(APTR, address, A1),

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

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
    AROS_LIBFUNC_EXIT
} /* TypeOfMem */

