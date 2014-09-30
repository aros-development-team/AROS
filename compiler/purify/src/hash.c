/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include "hash.h"
#include "util.h"
#include "error.h"
#include "memory.h"
#include "debug.h"

MemHashTable memHash;
MemHash * Purify_LastNode;

#define CalcHash(addr) \
    ({ unsigned long x; \
	x = (long)(addr); \
	x >>= 16; \
	((x & 0xFF) ^ (x >> 8)) & 0xFF; \
    })

MemHash * Purify_AddMemory (void * memory, int size, int flag, int type)
{
    MemHash * node = xmalloc (sizeof (MemHash));
    int hashcode;

    node->mem	= memory;
    node->flags = xmalloc (size);
    node->size	= size;
    node->type	= type;
    node->data	= NULL;

    if (flag != -1)
	Purify_SetMemoryFlags (node, 0, size, flag);

    hashcode = CalcHash (memory);

    node->next = memHash[hashcode];
    memHash[hashcode] = node;

    return node;
}

void Purify_RemMemory (const void * mem)
{
    int hashcode = CalcHash (mem);
    MemHash * node = (MemHash *)&memHash[hashcode], * next;

#if LDEBUG
    printf ("Purify_FindMemory (mem=%p)\n", mem);
#endif

    for ( ; (next=node->next); node=next)
    {
#if LDEBUG
	printf ("    Checking against %p:%d (%p)\n",
	    node->mem, node->size, node->mem+node->size);
#endif
	if (next->mem <= mem && next->mem+next->size > mem)
	{
#if LDEBUG
	    printf ("    Node found\n");
#endif
	    node->next = next->next;
	    xfree (next);
	    if (Purify_LastNode == next)
		Purify_LastNode = NULL;
	    return;
	}
    }

#if LDEBUG
    printf ("    Nothing found\n");
#endif
}


void Purify_SetMemoryFlags (MemHash * mem, int offset, int size, int flag)
{
    char * ptr;

#if 0
    printf ("SetMemoryFlags (hash=%p, offset=%d, size=%d, flag=%d)\n",
	mem, offset, size, flag
    );
#endif

    passert (offset+size <= mem->size);

    ptr = mem->flags + offset;

    while (size--)
	*ptr ++ = flag;
}

void Purify_ModifyMemoryFlags (MemHash * mem, int offset, int size, int flag,
    int mask)
{
    char * ptr;

    passert (offset+size <= mem->size);

    ptr = mem->flags + offset;

    flag &= mask;
    mask = ~mask;

    while (size--)
    {
	*ptr = (*ptr & mask) | flag;
	ptr ++;
    }
}

#undef LDEBUG
#define LDEBUG 0

MemHash * Purify_FindMemory (const void * mem)
{
    int hashcode = CalcHash (mem);
    MemHash * node = memHash[hashcode];

#if LDEBUG
    printf ("Purify_FindMemory (mem=%p)\n", mem);
#endif

    for ( ; node; node=node->next)
    {
#if LDEBUG
	printf ("    Checking against %p:%d (%p)\n",
	    node->mem, node->size, node->mem+node->size);
#endif
	if (node->mem <= mem && node->mem+node->size > mem)
	{
	    Purify_LastNode = node;
#if LDEBUG
	    printf ("    Node found\n");
#endif
	    return node;
	}
    }

    Purify_LastNode = NULL;
    Purify_Error = NotOwnMemory;

#if LDEBUG
    printf ("    Nothing found\n");
#endif
    return NULL;
}

int Purify_CheckMemoryAccess (const void * mem, int size, int access)
{
    MemHash * node = Purify_FindMemory (mem);
    long offset, cnt;
    char * ptr;

    if (!node)
	return 0;

    offset = (long)mem - (long)node->mem;

    if (offset+size > node->size)
    {
	Purify_Error = BorderAccess;
	return 0;
    }

    ptr = node->flags + offset;

    if (access == PURIFY_MemAccess_Read)
    {
	cnt = size;
	while (cnt--)
	{
	    if (!(*ptr & PURIFY_MemFlag_Readable) )
	    {
		if (*ptr & PURIFY_MemFlag_Free)
		{
		    Purify_Error = (node->type == PURIFY_MemType_Stack)
			? FreeStackRead : FreeRead;
		    return 0;
		}
		else if (*ptr & PURIFY_MemFlag_Empty)
		{
		    Purify_Error = UndefRead;
		    return 0;
		}
		else
		{
		    Purify_Error = IllRead;
		    return 0;
		}
	    }

	    ptr ++;
	}
    }
    else /* write */
    {
	if (node->type == PURIFY_MemType_Code)
	{
	    Purify_Error = CodeWrite;
	    return 0;
	}

	cnt=size;
	while (cnt--)
	{
	    if (!(*ptr & PURIFY_MemFlag_Writable) )
	    {
		if (*ptr & PURIFY_MemFlag_Free)
		{
		    Purify_Error =
			(node->type == PURIFY_MemType_Stack)
			    ? FreeStackWrite
			    : FreeWrite;
		    return 0;
		}
		else
		{
		    Purify_Error = IllWrite;
		    return 0;
		}
	    }

	    ptr ++;
	}

	Purify_ModifyMemoryFlags (node, offset, size,
	    PURIFY_MemFlag_Readable,
	    PURIFY_MemFlag_Readable|PURIFY_MemFlag_Empty
	);
    }

    return 1;
}

void Purify_PrintMemory (void)
{
    MemHash * node;
    int i;
#if 0
    int t, cnt;
#endif

    for (i=0; i<256; i++)
    {
	if ((node = memHash[i]))
	{
	    printf ("Hashline %3d:\n", i);

	    for ( ;
		node;
		node=node->next
	    )
	    {
		printf ("    Node %p: Memory=%p Size=%d Type=",
		    node,
		    node->mem,
		    node->size
		);

		switch (node->type)
		{
		case PURIFY_MemType_Heap:  printf ("heap "); break;
		case PURIFY_MemType_Stack: printf ("stack"); break;
		case PURIFY_MemType_Code:  printf ("code "); break;
		case PURIFY_MemType_Data:  printf ("data "); break;
		}

		if (node->data)
		{
		    switch (node->type)
		    {
		    case PURIFY_MemType_Stack:
		    case PURIFY_MemType_Code:
		    case PURIFY_MemType_Data:
			printf (" name=\"%s\"", (char *)node->data);
			break;

		    case PURIFY_MemType_Heap:
			{
			    PMemoryNode * mem = (PMemoryNode *)(node->data);

			    printf (" %s()", mem->alloc.current.functionname);
			}
			break;
		    }
		}

		putchar ('\n');

#if 0
#define WIDTH	56

		for (cnt=0,t=0; t<node->size; t++,cnt++)
		{
		    if (cnt == 0)
			printf ("\t%4d: ", t);

		    printf ("%x", node->flags[t]);

		    if (cnt == WIDTH-1)
		    {
			putchar ('\n');
			cnt = -1;
		    }
		    else if ((cnt & 7) == 7)
			putchar (' ');
		}

		if (((t-1) & WIDTH) != WIDTH-1)
		    putchar ('\n');
#endif
	    }
	}
    }
}

#ifndef ABS
#   define ABS(x)       (((x) < 0) ? -(x) : (x))
#endif

MemHash * Purify_FindNextMemory (const void * mem, int * offsetptr)
{
    MemHash * node, * next;
    int i, offset, o;

    next = NULL;
    offset = INT_MAX;

    for (i=0; i<256; i++)
    {
	for (node = memHash[i];
	    node;
	    node=node->next
	)
	{
	    o = (long)mem - (long)node->mem;

	    if (o > 0)
	    {
		if (o < node->size)
		{
		    *offsetptr = o;
		    return node;
		}
		else
		{
		    o -= node->size;
		}
	    }

	    if (ABS(o) < ABS(offset)
		|| (ABS(o) == ABS(offset) && o > 0)
	    )
	    {
		offset = o;
		next = node;
	    }
	}
    }

    *offsetptr = offset;
    return next;
}
