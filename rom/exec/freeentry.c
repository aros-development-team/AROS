/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:49  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:02  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:11  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include "exec_intern.h"
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

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

    HISTORY
	18-10-95    created by m. fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    ULONG i;

    /* First free all blocks in the MemList */
    for(i=0;i<entry->ml_NumEntries;i++)
	FreeMem(entry->ml_ME[i].me_Addr,entry->ml_ME[i].me_Length);

    /* Then free the MemList itself */
    FreeMem(entry,sizeof(struct MemList)-sizeof(struct MemEntry)+
		  sizeof(struct MemEntry)*entry->ml_NumEntries);
    AROS_LIBFUNC_EXIT
} /* FreeEntry */


