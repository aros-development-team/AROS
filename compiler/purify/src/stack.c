/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <assert.h>
#include "debug.h"
#include "hash.h"

static MemHash * Purify_CurrentStackNode;

void Purify_InitStack (char * stackBase, long stackSize)
{
    MemHash * node;
    char ** argv;
    int argc, t;
    long offset;

    stackBase +=
	4 /* two arguments */
	+4 /* return address for main() */
	+8 /* two arguments for main() */
    ;

#if DEBUG
    printf ("Stack goes from %p to %p\n", stackBase - stackSize, stackBase);
#endif

    node = Purify_AddMemory (stackBase - stackSize
	, stackSize
	, PURIFY_MemFlag_Empty
	, PURIFY_MemType_Stack
    );

    node->data = "Stack";

    Purify_CurrentStackNode = node;

    /* Major hack :-/ */
    argv = ((char ***)stackBase)[-1];
    argc = ((int *)stackBase)[-2];

#if 0
    printf ("&argc=%p\n", &((int *)stackBase)[-2]);
    printf ("argc=%d argv=%p\n", argc, argv);
#endif

    offset = (long)(&((int *)stackBase)[-2]) - (long)(node->mem);

    Purify_SetMemoryFlags (node, offset, 8,
	PURIFY_MemFlag_Readable|PURIFY_MemFlag_Writable
    );

    node = Purify_AddMemory (argv
	, (argc+1) * sizeof (char *)
	, PURIFY_MemFlag_Readable|PURIFY_MemFlag_Writable
	, PURIFY_MemType_Stack
    );

    node->data = "argv[] array";

    for (t=0; t<argc; t++)
    {
	node = Purify_AddMemory (argv[t]
	    , strlen (argv[t]) + 1
	    , PURIFY_MemFlag_Readable|PURIFY_MemFlag_Writable
	    , PURIFY_MemType_Stack
	);

	node->data = "argument string";
    }

#if DEBUG
    Purify_PrintMemory ();
#endif
}

void Purify_Push (char * stackBase, long pushSize)
{
    long offset;

    passert (Purify_CurrentStackNode);
    passert (Purify_CurrentStackNode->mem);

#if 0
    printf ("Push(): sp=%p, size=%ld fp=%p  stackBase=%p\n",
	stackBase, pushSize, (&offset)-1, Purify_CurrentStackNode->mem);
#endif

    stackBase += 4;
    stackBase -= pushSize;

    offset = (long)stackBase - (long)(Purify_CurrentStackNode->mem);

#if 0
    printf ("offset=%d\n", offset);
#endif

    passert (offset >= 0);

    Purify_SetMemoryFlags (Purify_CurrentStackNode, offset, pushSize,
	PURIFY_MemFlag_Readable|PURIFY_MemFlag_Writable
    );
}

void Purify_Pop (char * stackBase, long popSize)
{
    long offset;

    passert (Purify_CurrentStackNode);
    passert (Purify_CurrentStackNode->mem);

    stackBase += 4;

    offset = (long)stackBase - (long)(Purify_CurrentStackNode->mem);

    passert (offset >= 0);

    Purify_SetMemoryFlags (Purify_CurrentStackNode, offset, popSize,
	PURIFY_MemFlag_Free
    );
}

void Purify_Alloca (char * stackBase, long allocSize)
{
    long offset;

    passert (Purify_CurrentStackNode);
    passert (Purify_CurrentStackNode->mem);

#if 0
    printf ("alloca(): sp=%p, size=%ld fp=%p  stackBase=%p\n",
	stackBase, allocSize, (&offset)-1, Purify_CurrentStackNode->mem);
#endif

    stackBase += 4;
    stackBase -= allocSize;

    offset = (long)stackBase - (long)(Purify_CurrentStackNode->mem);

#if 0
    printf ("offset=%ld\n", offset);
#endif

    passert (offset >= 0);

    Purify_SetMemoryFlags (Purify_CurrentStackNode, offset, allocSize,
	PURIFY_MemFlag_Empty|PURIFY_MemFlag_Writable
    );
}

void Purify_MoveSP (char * SP, char * newSP)
{
    long dist;

    dist = (long)newSP - (long)SP;

    if (dist > 0)
	Purify_Pop (SP, dist);
    else if (dist < 0)
	Purify_Alloca (SP, -dist);
}
