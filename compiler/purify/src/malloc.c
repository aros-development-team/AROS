#include <stdio.h>
#include <stdlib.h>
#include "malloc.h"
#include "hash.h"
#include "posinfo.h"
#include "error.h"
#include "util.h"

void * Purify_malloc (size_t size)
{
    PMemoryNode * node = xmalloc (sizeof (PMemoryNode) + size);
    MemHash * hash;

    Purify_RememberCallers (&node->alloc);

    node->free.nstack = -1; /* Not freed yet */

    node->memptr = (char *)((((long)(&node->mem[8])) + 7) & -8);

    hash = Purify_AddMemory (node->memptr, size,
	PURIFY_MemFlag_Writable
	| PURIFY_MemFlag_Empty
	, PURIFY_MemType_Heap
    );

    hash->data = node;

    return node->memptr;
}

void * Purify_calloc (size_t nemb, size_t size)
{
    PMemoryNode * node = calloc (1, sizeof (PMemoryNode) + nemb*size);
    MemHash * hash;

    Purify_RememberCallers (&node->alloc);

    node->free.nstack = -1; /* Not freed yet */

    node->memptr = (char *)((((long)(&node->mem[8])) + 7) & -8);

    hash = Purify_AddMemory (node->memptr, nemb*size,
	PURIFY_MemFlag_Readable
	| PURIFY_MemFlag_Writable
	, PURIFY_MemType_Heap
    );

    hash->data = node;

    return node->memptr;
}

void * Purify_realloc (void * mem, size_t size)
{
    PMemoryNode * oldnode, * newnode;
    MemHash * oldmem, * newmem;

    if (size)
    {
	newnode = xmalloc (sizeof (PMemoryNode) + size);

	Purify_RememberCallers (&newnode->alloc);

	newnode->free.nstack = -1; /* Not freed yet */

	newnode->memptr = (char *)((((long)(&newnode->mem[8])) + 7) & -8);

	newmem = Purify_AddMemory (newnode->memptr, size,
	    PURIFY_MemFlag_Writable
	    | PURIFY_MemFlag_Empty
	    , PURIFY_MemType_Heap
	);

	newmem->data = newnode;
    }
    else
	newnode = NULL;

    if (mem && size)
    {
	memcpy (newnode->memptr, mem, size);
    }

    if (mem)
    {
	oldmem = Purify_FindMemory (mem);

	if (!oldmem)
	{
	    Purify_Error = IllRealloc;
	    Purify_PrintError ("Pointer=%p", mem);
	}
	else
	{
	    oldnode = (PMemoryNode *)(oldmem->data);

	    if (newnode)
	    {
		if (size > oldmem->size)
		    size = oldmem->size;

		memcpy (newnode->memptr, oldnode->memptr, size);

		Purify_SetMemoryFlags (newmem, 0, size,
		    PURIFY_MemFlag_Readable | PURIFY_MemFlag_Writable
		);
	    }

	    Purify_SetMemoryFlags (oldmem, 0, oldmem->size, PURIFY_MemFlag_Free);
	    Purify_RememberCallers (&oldnode->free);
	}
    }

    return newnode ? newnode->memptr : NULL;
}

void Purify_free (void * mem)
{
    MemHash * hash = Purify_FindMemory (mem);
    PMemoryNode * node;

    if (!hash)
    {
	Purify_Error = IllFree;
	Purify_PrintError ("Pointer=%p", mem);
    }
    else
    {
	node = (PMemoryNode *)(hash->data);

	Purify_SetMemoryFlags (hash, 0, hash->size, PURIFY_MemFlag_Free);
	Purify_RememberCallers (&node->free);
    }
}

void Purify_MemoryExit (void)
{
    extern MemHashTable memHash;
    MemHash * node;
    PMemoryNode * memnode;
    int i;

    Purify_Error = MemLeak;

    for (i=0; i<256; i++)
    {
	for (node=memHash[i];
	    node;
	    node=node->next
	)
	{
	    if (node->type == PURIFY_MemType_Heap)
	    {
		memnode = (PMemoryNode *)(node->data);

		if (memnode->free.nstack == -1)
		{
		    Purify_PrePrintError ();
		    fprintf (stderr,
			"The memory block at %p with the size %d was allocated at\n",
			node->mem, node->size
		    );
		    Purify_PrintCallers (&memnode->alloc);
		    Purify_PostPrintError ();
		}
	    }
	}
    }
}

