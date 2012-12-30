/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add memory to the public list of memory.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "memory.h"

/*****************************************************************************

    NAME */

AROS_LH5(void, AddMemList,

/*  SYNOPSIS */
	AROS_LHA(IPTR,   size,       D0),
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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemHeader *mh;

    /* If the end is less than (1 << 31), MEMF_31BIT is implied */
    if (((IPTR)base+size) < (1UL << 31))
        attributes |= MEMF_31BIT;
    else
        attributes &= ~MEMF_31BIT;

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
    MEM_LOCK;

    /* Add MemHeader */
    Enqueue(&SysBase->MemList,&mh->mh_Node);

    MEM_UNLOCK;

    AROS_LIBFUNC_EXIT
} /* AddMemList */

