/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1996/12/10 13:51:35  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.6  1996/10/24 15:50:41  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.5  1996/10/19 17:07:23  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.4  1996/08/13 13:55:56  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:02  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <aros/machine.h>
#include "memory.h"
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <exec/memory.h>
#include <clib/exec_protos.h>

AROS_LH5(void, AddMemList,

/*  SYNOPSIS */
	AROS_LHA(ULONG,  size,       D0),
	AROS_LHA(ULONG,  attributes, D1),
	AROS_LHA(LONG,   pri,        D2),
	AROS_LHA(APTR,   base,       A0),
	AROS_LHA(STRPTR, name,       A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 103, Exec)

/*  FUNCTION
	Add a new block of memory to the system memory lists.

    INPUTS
	size	   - Size of the block
	attributes - The attributes the memory will have
	pri	   - Priority in the list of MemHeaders
	base	   - Base address
	name	   - A name associated with the memory

    RESULT

    NOTES
	No argument checking done.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	8-10-95    created by m. fleischer
       16-10-95    increased portability

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemHeader *mh;

    /* Do I have to look here if it matches some other MemHeader? */
    mh=(struct MemHeader *)base;
    mh->mh_Node.ln_Type=NT_MEMORY;
    mh->mh_Node.ln_Pri=pri;
    mh->mh_Node.ln_Name=name;
    mh->mh_Attributes=attributes;
    mh->mh_First=(struct MemChunk *)((UBYTE *)mh+MEMHEADER_TOTAL);
    mh->mh_First->mc_Next=NULL;
    mh->mh_First->mc_Bytes=size-MEMHEADER_TOTAL;
    mh->mh_Lower=mh->mh_First;
    mh->mh_Upper=(APTR)((UBYTE *)base+size);
    mh->mh_Free=mh->mh_First->mc_Bytes;

    /* Protect the memory list. */
    Forbid();
	/* Add MemHeader */
	Enqueue(&SysBase->MemList,&mh->mh_Node);
    Permit();
    AROS_LIBFUNC_EXIT
} /* AddMemList */

