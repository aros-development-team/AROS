/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate memory.
    Lang: english
*/
#include <aros/config.h>
#include "exec_intern.h"
#include <aros/libcall.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_debug.h"
#ifndef DEBUG_AllocEntry
#   define DEBUG_AllocEntry 0
#endif
#undef DEBUG
#if DEBUG_AllocEntry
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH1(struct MemList *, AllocEntry,

/*  SYNOPSIS */
	AROS_LHA(struct MemList *, entry, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 37, Exec)

/*  FUNCTION
	Allocate a number of memory blocks through a MemList structure.

    INPUTS
	entry - The MemList with one MemEntry for each block you want to get

    RESULT
	The allocation was successful if the most significant bit of the
	result is 0. The result then contains a pointer to a copy of
	the MemList structure with the me_Addr fields filled.
	If the most significant bit is set the result contains the type of
	memory that couldn't be allocated.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeEntry()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemList *ret;
    ULONG   	    ret_flags;

    if (NewAllocEntry(entry, &ret, &ret_flags))
    {
    	/* Check nasty case where the returncode is misleading :-(
	   Like when memory was allocated at address 0x8???????. And
	   0x80000000 flag is at the same time used to indicate allocentry
	   failure! */
	   
    	if(!AROS_CHECK_ALLOCENTRY(ret))
    	{
	    FreeEntry(ret);
	    ret = AROS_ALLOCENTRY_FAILED(MEMF_PUBLIC);
	}
    }
    else
    {
    	ret = AROS_ALLOCENTRY_FAILED(ret_flags);
    }
    
    return ret;
    
    AROS_LIBFUNC_EXIT
} /* AllocEntry */

