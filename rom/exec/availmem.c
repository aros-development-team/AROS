/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tell how much memory is available.
    Lang: english
*/

#define MDEBUG 1

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "memory.h"

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
	MEMF_LARGEST is set in the attributes.

    NOTES
	Due to the nature of multitasking the returned value may already
	be obsolete when this function returns.

    EXAMPLE
	Print the total available memory.

	printf("Free memory: %lu bytes\n", AvailMem(0));

	Print the size of the largest chunk of chip memory.

	printf("Largest chipmem chunk: %lu bytes\n",
	       AvailMem(MEMF_CHIP|MEMF_LARGEST));

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG ret;

    ret = nommu_AvailMem(attributes, SysBase);

    if ((PrivExecBase(SysBase)->IntFlags & EXECF_MungWall) &&
        (attributes & MEMF_CLEAR))
    {
    	struct List 	    	*allocmemlist;
   	struct MungwallHeader 	*allocnode;
	ULONG	    	    	 alloccount = 0;
	ULONG	    	    	 allocsize = 0;
	
	allocmemlist = (struct List *)&((struct AROSSupportBase *)SysBase->DebugAROSBase)->AllocMemList;
    
    	kprintf("\n=== MUNGWALL MEMORY CHECK ============\n");
	
	Forbid();
	
	ForeachNode(allocmemlist, allocnode)
	{
	    if (allocnode->mwh_magicid != MUNGWALL_HEADER_ID)
	    {
		kprintf(" #%05x BAD MUNGWALL_HEADER_ID\n", alloccount);
	    }
	    
	    CHECK_WALL((UBYTE *)allocnode + MUNGWALLHEADER_SIZE, 0xDB, MUNGWALL_SIZE);
	    CHECK_WALL((UBYTE *)allocnode + MUNGWALLHEADER_SIZE + MUNGWALL_SIZE + allocnode->mwh_allocsize, 0xDB,
		MUNGWALL_SIZE + AROS_ROUNDUP2(allocnode->mwh_allocsize, MEMCHUNK_TOTAL) - allocnode->mwh_allocsize);
	    	    
	    allocsize += allocnode->mwh_allocsize;
	    alloccount++;
	}

	Permit();

	kprintf("\n Num allocations: %d   Memory allocated %d\n", alloccount, allocsize);
    }

    return ret;
    AROS_LIBFUNC_EXIT
} /* AvailMem */

