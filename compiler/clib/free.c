/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function free()
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>

extern struct SignalSemaphore __startup_memsem;
extern APTR __startup_mempool;

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void free (

/*  SYNOPSIS */
	void * memory)

/*  FUNCTION
	Return memory allocated with malloc() or a similar function to the
	system.

    INPUTS
	memory - The result of the previous call to malloc(), etc. or
		NULL.

    RESULT
	None.

    NOTES
        This function must not be used in a shared library

    EXAMPLE

    BUGS

    SEE ALSO
	malloc()

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    UBYTE * mem;
    size_t size;

    ObtainSemaphore(&__startup_memsem);
    
    if (memory && __startup_mempool)
    {
	mem = ((UBYTE *)memory) - AROS_ALIGN(sizeof(size_t));
	size = *((size_t *)mem) + AROS_ALIGN(sizeof(size_t));

	FreePooled (__startup_mempool, mem, size);
    }

    ReleaseSemaphore(&__startup_memsem);
    
} /* free */

