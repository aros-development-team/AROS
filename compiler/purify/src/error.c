#include <stdio.h>
#include <stdarg.h>
#include "error.h"
#include "hash.h"
#include "posinfo.h"
#include "malloc.h"

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

    fprintf (stderr, "*** Purify: %s ***\n", purify_ErrorMsgs[Purify_Error]);
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

void Purify_PrintAccessError (const char * access, void * addr, int size)
{
    if (Purify_LastNode)
    {
	long offset = (long)addr - (long)(Purify_LastNode->mem);

	if (Purify_LastNode->type != PURIFY_MemType_Heap)
	{
	    Purify_PrintError (
		"%s of %d bytes at %p. This is %d bytes into the block\n"
		"%s%s%sat %p with the size of %d bytes (type=%d)",
		access, size, addr, offset,
		Purify_LastNode->data ? "\"" : "",
		Purify_LastNode->data ? (char *)Purify_LastNode->data : "",
		Purify_LastNode->data ? "\" " : "",
		Purify_LastNode->mem, Purify_LastNode->size, Purify_LastNode->type
	    );
	}
	else
	{
	    PMemoryNode * node = (PMemoryNode *)Purify_LastNode->data;

	    Purify_PrePrintError ();
	    fprintf (stderr,
		"%s of %d bytes at %p. This is %ld bytes into the block\n"
		"allocated at %p with the size of %d bytes (type=%d)\n"
		"The block was allocated ",
		access, size, addr, offset,
		Purify_LastNode->mem, Purify_LastNode->size,
		Purify_LastNode->type
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
	Purify_PrintError ("%s of %d bytes at %p",
	    access, size, addr
	);
    }
}

