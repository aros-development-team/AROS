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
#include <proto/kernel.h>

#include "exec_intern.h"
#include "memory.h"

AROS_LH5(void, AddMemList,
	AROS_LHA(IPTR,   size,       D0),
	AROS_LHA(ULONG,  attributes, D1),
	AROS_LHA(LONG,   pri,        D2),
	AROS_LHA(APTR,   base,       A0),
	AROS_LHA(STRPTR, name,       A1),
	struct ExecBase *, SysBase, 103, Exec)
{
    AROS_LIBFUNC_INIT

    struct MemHeader *mh;

    /* We may be called by some special boot ROM (UAE extra Chip RAM)
     * or user may manually add some memory region(s).
     * We need to remove invalid mapping in MMU debugging mode.
     */
    KrnSetProtection((void*)base, size, (attributes & MEMF_CHIP) ?
        MAP_Readable | MAP_Writable | MAP_Executable | MAP_CacheInhibit :
        MAP_Readable | MAP_Writable | MAP_Executable);

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
}


