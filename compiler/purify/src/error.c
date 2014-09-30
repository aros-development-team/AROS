/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdarg.h>
#include "error.h"
#include "hash.h"
#include "posinfo.h"
#include "memory.h"

static char * _Purify_MemTypeNames[] =
{
    "Heap",
    "Stack",
    "Code",
    "Data"
};

int Purify_Error;

static const char * purify_ErrorMsgs[] =
{
#undef _ERROR_DEF
#define _ERROR_DEF(n,s) s,
#include "error.def"
};

void Purify_PrePrintError (void)
{
    RememberData rd;

    Purify_RememberCallers (&rd);

    fprintf (stderr, "*** Purify %s ***\n", purify_ErrorMsgs[Purify_Error]);
    Purify_PrintCallers (&rd);
}

void Purify_PostPrintError (void)
{
    fprintf (stderr, "***\n");
}

void Purify_PrintError (const char * fmt, ...)
{
    va_list args;

    Purify_PrePrintError ();
    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end (args);
    putc ('\n', stderr);
    Purify_PostPrintError ();
}

void Purify_PrintAccessError (const char * access, const void * addr, int size)
{
    if (Purify_LastNode)
    {
	long offset = (long)addr - (long)(Purify_LastNode->mem);

	if (Purify_LastNode->type != PURIFY_MemType_Heap)
	{
	    Purify_PrintError (
		"%s of %d bytes at %p. This is %d bytes into the block\n"
		"%s%s%sat %p with the size of %d bytes (type=%s)",
		access, size, addr, offset,
		Purify_LastNode->data ? "\"" : "",
		Purify_LastNode->data ? (char *)Purify_LastNode->data : "",
		Purify_LastNode->data ? "\" " : "",
		Purify_LastNode->mem, Purify_LastNode->size,
		_Purify_MemTypeNames[Purify_LastNode->type]
	    );
	}
	else
	{
	    PMemoryNode * node = (PMemoryNode *)Purify_LastNode->data;

	    Purify_PrePrintError ();
	    fprintf (stderr,
		"%s of %d bytes at %p. This is %ld bytes into the block\n"
		"allocated at %p with the size of %d bytes (type=%s)\n"
		"The block was allocated ",
		access, size, addr, offset,
		Purify_LastNode->mem, Purify_LastNode->size,
		_Purify_MemTypeNames[Purify_LastNode->type]
	    );
	    Purify_PrintCallers (&node->alloc);
	    if (node->free.nstack != -1)
	    {
		fprintf (stderr, "It was freed ");
		Purify_PrintCallers (&node->free);
	    }
	    Purify_PostPrintError ();
	}
    }
    else
    {
	int offset;
	MemHash * next = Purify_FindNextMemory (addr, &offset);

	if (next->type != PURIFY_MemType_Heap)
	{
	    Purify_PrintError (
		"%s of %d bytes at %p. This is %d bytes %s the block\n"
		"%s%s%sat %p with the size of %d bytes (type=%s)",
		access, size, addr, offset, (offset < 0 ? "before" : "after"),
		next->data ? "\"" : "",
		next->data ? (char *)next->data : "",
		next->data ? "\" " : "",
		next->mem, next->size,
		_Purify_MemTypeNames[next->type]
	    );
	}
	else
	{
	    PMemoryNode * node = (PMemoryNode *)next->data;

	    Purify_PrePrintError ();
	    fprintf (stderr,
		"%s of %d bytes at %p. This is %d bytes %s the block\n"
		"allocated at %p with the size of %d bytes (type=%s)\n"
		"The block was allocated ",
		access, size, addr, offset, (offset < 0 ? "before" : "after"),
		next->mem, next->size,
		_Purify_MemTypeNames[next->type]
	    );
	    Purify_PrintCallers (&node->alloc);
	    if (node->free.nstack != -1)
	    {
		fprintf (stderr, "It was freed ");
		Purify_PrintCallers (&node->free);
	    }
	    Purify_PostPrintError ();
	}
    }
}

