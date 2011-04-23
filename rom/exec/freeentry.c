/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated with AllocEntry().
    Lang: english
*/
#include <aros/libcall.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_debug.h"

#ifndef DEBUG_FreeEntry
#   define DEBUG_FreeEntry 0
#endif
#undef DEBUG
#if DEBUG_FreeEntry
#   define DEBUG 1
#endif

#include "exec_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, FreeEntry,

/*  SYNOPSIS */
	AROS_LHA(struct MemList *, entry,A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 38, Exec)


/*  FUNCTION
	Free some memory allocated with AllocEntry().

    INPUTS
	entry - The MemList you got from AllocEntry().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocEntry()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    ULONG i;

    /* First free all blocks in the MemList */
    for(i=0;i<entry->ml_NumEntries;i++)
    {
        D(bug("[FreeEntry] Freeing size %d at 0x%p\n", entry->ml_ME[i].me_Length,entry->ml_ME[i].me_Addr));
	FreeMem(entry->ml_ME[i].me_Addr,entry->ml_ME[i].me_Length);
    }

    /* Then free the MemList itself */
    FreeMem(entry,sizeof(struct MemList)-sizeof(struct MemEntry)+
		  sizeof(struct MemEntry)*entry->ml_NumEntries);
    AROS_LIBFUNC_EXIT
} /* FreeEntry */


