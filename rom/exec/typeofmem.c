/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:55  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:58  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:09  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:21  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <clib/exec_protos.h>

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

    HISTORY
	18-10-95    Created by M. Fleischer

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

