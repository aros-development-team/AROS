/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function malloc()
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>

extern struct SignalSemaphore __startup_memsem;
extern APTR __startup_mempool;

/*****************************************************************************

    NAME */
#include <sys/types.h>
#include <stdlib.h>

	void * malloc (

/*  SYNOPSIS */
	size_t size)

/*  FUNCTION
	Allocate size bytes of memory and return the address of the
	first byte.

    INPUTS
	size - How much memory to allocate.

    RESULT
	A pointer to the allocated memory or NULL. If you don't need the
	memory anymore, you can pass this pointer to free(). If you don't,
	the memory will be freed for you when the application exits.

    NOTES
        This function must not be used in a shared library.

    EXAMPLE

    BUGS

    SEE ALSO
	free()

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    UBYTE * mem = NULL;

    ObtainSemaphore(&__startup_memsem);

    /* Check if there is a pool already */
    if (!__startup_mempool)
    {
	/* Create one if not */
	__startup_mempool = CreatePool (MEMF_ANY, 4096L, 2000L);
    }

    if (__startup_mempool)
    {
	/* Allocate the memory */
	mem = AllocPooled (__startup_mempool, size + AROS_ALIGN(sizeof(size_t)));    
	if (mem)
	{	
	    *((size_t *)mem) = size;
	    mem += AROS_ALIGN(sizeof(size_t));
	}
    }
    
    ReleaseSemaphore(&__startup_memsem);
    
    return mem;
    
} /* malloc */

