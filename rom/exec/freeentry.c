/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:11  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

	__AROS_LH1(void, FreeEntry,

/*  SYNOPSIS */
	__AROS_LA(struct MemList *, entry,A0),

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
    __AROS_FUNC_INIT
    ULONG i;

    /* First free all blocks in the MemList */
    for(i=0;i<entry->ml_NumEntries;i++)
	FreeMem(entry->ml_ME[i].me_Addr,entry->ml_ME[i].me_Length);

    /* Then free the MemList itself */
    FreeMem(entry,sizeof(struct MemList)-sizeof(struct MemEntry)+
		  sizeof(struct MemEntry)*entry->ml_NumEntries);
    __AROS_FUNC_EXIT
} /* FreeEntry */


