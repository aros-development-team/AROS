#include <exec/execbase.h>
#include "machine.h"
#include "memory.h"
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

__AROS_LH5(void, AddMemList,

/*  SYNOPSIS */
	__AROS_LA(ULONG,  size,       D0),
	__AROS_LA(ULONG,  attributes, D1),
	__AROS_LA(LONG,   pri,        D2),
	__AROS_LA(APTR,   base,       A0),
	__AROS_LA(STRPTR, name,       A1),

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
    __AROS_FUNC_INIT

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
    __AROS_FUNC_EXIT
} /* AddMemList */

