/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga bootloader -- InternalLoadSeg support routines
    Lang: C
*/

/*
 * For more information: autodocs/dos/InternalLoadSeg()
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/lists.h>

#include <aros/asmcall.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "boot.h"
#include "main.h"

#define D(x) if (debug) x
#define bug Printf

extern struct ilsMemList ils_mem;

AROS_UFH4(LONG, ils_read,
    AROS_UFHA(BPTR,                handle,  D1),
    AROS_UFHA(void *,              buffer,  D2),
    AROS_UFHA(LONG,                length,  D3),
    AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    D(bug(" ils_read: size %ld\n", length));

    return( Read(handle, buffer, length) );
}

AROS_UFH3(void *, ils_alloc,
    AROS_UFHA(ULONG,             size,    D0),
    AROS_UFHA(ULONG,             attrib,  D1),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    void *result;

    D(bug(" ils_alloc: size %ld == ", size));

    /*
     * Memory to be used for Resident modules can not be any kind of memory
     * available. It must be of a special type, MEMF_KICK, which indicates
     * that this memory is available very early during the reset procedure.
     * Also allocate memory from the top of the memory list, MEMF_REVERSE,
     * to keep all our allocations in one place, and to keep potential early
     * memory fragmentation down.
     *
     * Addition: Pre V39 exec doesn't know about MEMF_KICK, so fall back to
     * MEMF_CHIP (memtype is set in main()).
     */
    attrib |= memtype|MEMF_REVERSE;

    result = AllocMem(size, attrib);

    /*
     * all memory that is allocated during the LoadSeg has to be entered
     * into the KickMemPtr for protection during reset. We keep a list of
     * our allocations so we can later make this MemList
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
	     * Keep a counter so we don't have to count nodes later.
	     */
	    ils_mem.iml_Num++;

	    /*
	     * This counts number of nodes since the loading of the last
	     * module. This field is reset in the FindResMod() routine.
	     */
	    ils_mem.iml_NewNum++;
	}
	D(bug("$%08lx\n", (ULONG)result));
	return(result);
    }

    D(bug("0\n"));
    return 0;
}

AROS_UFH3(void, ils_free,
    AROS_UFHA(void *,            block,   A1),
    AROS_UFHA(ULONG,             size,    D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    void *saveblock = block;
    struct ilsMemNode *node;

    D(bug(" ils_free: block $%08lx  size %ld\n", (ULONG)block, size));

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

