/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tell how much memory is available.
    Lang: english
*/
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, AvailMem,

/*  SYNOPSIS */
	AROS_LHA(ULONG, attributes, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 36, Exec)

/*  FUNCTION
	Return either the total available memory or the largest available
	chunk of a given type of memory.

    INPUTS
	attributes - The same attributes you would give to AllocMem().

    RESULT
	Either the total of the available memory or the largest chunk if
	MEMF_LARGEST ist set in the attributes.

    NOTES
	Due to the nature of multitasking the returned value may already
	be obsolete if this function returns.

    EXAMPLE
	Print the total available memory.

	printf("Free memory: %lu bytes\n",AvailMem(0));

	Print the size of the largest chunk of chip memory.

	printf("Largest chipmem chunk: %lu bytes\n",
	       AvailMem(MEMF_CHIP|MEMF_LARGEST));

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG ret=0;
    struct MemHeader *mh;

    /* Nobody else should access the memory lists now. */
    Forbid();

	/* Get pointer to first memory header... */
	mh=(struct MemHeader *)SysBase->MemList.lh_Head;
	/* And follow the list. */
	while(mh->mh_Node.ln_Succ!=NULL)
	{
	    /*
		The current memheader is OK if there's no bit in the
		'attributes' that isn't set in the 'mh->mh_Attributes'.
		MEMF_CLEAR, MEMF_REVERSE, MEMF_NO_EXPUNGE, MEMF_TOTAL and
		MEMF_LARGEST are treated as if they were always set in
		the memheader.
	    */
	    if(!(attributes&~(MEMF_CLEAR|MEMF_REVERSE|MEMF_NO_EXPUNGE
			|MEMF_TOTAL|MEMF_LARGEST|mh->mh_Attributes)))
	    {
		/* Find largest chunk? */
		if(attributes&MEMF_LARGEST)
		{
		    /*
			Yes. Follow the list of MemChunks and set 'ret' to
			each value that is bigger than all previous ones.
		    */
		    struct MemChunk *mc=mh->mh_First;
		    while(mc!=NULL)
		    {
#if !defined(NO_CONSISTENCY_CHECKS)
			/*
			    Do some constistency checks:
			    1. All MemChunks must be aligned to
			       sizeof(struct MemChunk).
			    2. The end (+1) of the current MemChunk
			       must be lower than the start of the next one.
			*/
			if(  ((IPTR)mc|mc->mc_Bytes)&(sizeof(struct MemChunk)-1)
			   ||(  (UBYTE *)mc+mc->mc_Bytes>=(UBYTE *)mc->mc_Next
			      &&mc->mc_Next!=NULL))
			    Alert(AT_DeadEnd|AN_MemoryInsane);
#endif
			if(mc->mc_Bytes>ret)
			    ret=mc->mc_Bytes;
			mc=mc->mc_Next;
		    }
		}
		else if(attributes&MEMF_TOTAL)
		    /* Determine total size. */
		    ret+=(STRPTR)mh->mh_Upper-(STRPTR)mh->mh_Lower;
		else
		    /* Sum up free memory. */
		    ret+=mh->mh_Free;
	    }
	    mh=(struct MemHeader *)mh->mh_Node.ln_Succ;
	}
    /* All done. Permit dispatches and return. */
    Permit();
    return ret;
    AROS_LIBFUNC_EXIT
} /* AvailMem */

