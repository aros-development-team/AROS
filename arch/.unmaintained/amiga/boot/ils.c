/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Amiga bootloader -- InternalLoadSeg support routines
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/lists.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "boot.h"
#include "registers.h"

extern struct ilsMemList ils_mem;

LONG ils_read(BPTR handle __d1, void *buffer __d2, LONG length __d3,
	      struct DosLibrary *DOSBase __a6)
{
    return( Read(handle, buffer, length) );
}

void *ils_alloc(ULONG size __d0, ULONG attrib __d1,
		struct ExecBase *SysBase __a6)
{
    void *result;

    /*
	Memory to be used for Resident modules can not be any kind of memory
	available. It must be of a special type, MEMF_KICK, which indicates
	that this memory is available very early during the reset procedure.
	Also allocate memory from the top of the memory list, MEMF_REVERSE,
	to keep all our allocations in one place, and to keep potential early
	memory fragmentation down.
    */
    attrib |= MEMF_KICK|MEMF_REVERSE;

    result = AllocMem(size, attrib);

    /*
	all memory that is allocated during the LoadSeg has to be entered
	into the KickMemPtr for protection during reset. We keep a list of
	our allocations so we can make this MemList
    */
    if(result)
    {
	struct ilsMemNode *node;

	if( (node = AllocMem(sizeof(struct ilsMemNode), MEMF_CLEAR)) )
	{
	    node->imn_Addr = result;
	    node->imn_Size = size;
	    AddHead((struct List *)&ils_mem, (struct Node *)node);

	    /*
		Keep a counter so we don't have to count nodes later.
	    */
	    ils_mem.iml_Num++;

	    /*
		This counts number of nodes since the loading of the last
		module. This field is reset in the FindResMod() routine.
	    */
	    ils_mem.iml_NewNum++;
	}
	return(result);
    }

    return 0;
}

void ils_free(void *block __a1, ULONG size __d0,
	      struct ExecBase *SysBase __a6)
{
    void *saveblock = block;
    struct ilsMemNode *node;

    FreeMem(block, size);

    /* now remove this block from our list */
    /* find it */
    for(node = (struct ilsMemNode *)ils_mem.iml_List.mlh_Head;
	node->imn_Node.mln_Succ;
	node = (struct ilsMemNode *)node->imn_Node.mln_Succ)
    {
	/* is this the right block? */
	if(node->imn_Addr == saveblock)
	{
	    /* yes: remove from list and free it's memory */
	    Remove((struct Node *)node);
	    ils_mem.iml_Num--;
	    ils_mem.iml_NewNum--;
	    FreeMem(node, sizeof(struct ilsMemNode));
	}
    }
}

