/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function malloc().
*/

#include "__arosc_privdata.h"

#include <errno.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <sys/types.h>
#include <stdlib.h>

	void *malloc (

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
        This function must not be used in a shared library or in a threaded
	application.

    EXAMPLE

    BUGS

    SEE ALSO
	free()

    INTERNALS

******************************************************************************/
{
    UBYTE *mem = NULL;
    AROS_GET_SYSBASE_OK

    /* Allocate the memory */
    mem = AllocPooled (__startup_mempool, size + AROS_ALIGN(sizeof(size_t)));
    if (mem)
    {
	*((size_t *)mem) = size;
	mem += AROS_ALIGN(sizeof(size_t));
    }
    else
        errno = ENOMEM;

    return mem;

} /* malloc */


int __init_memstuff(void)
{
    AROS_GET_SYSBASE_OK

    __startup_mempool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 4096L, 2000L);

    if (!__startup_mempool)
    {
	return 0;
    }

    return 1;
}


void __exit_memstuff(void)
{
    AROS_GET_SYSBASE_OK

    if (__startup_mempool)
    {
	DeletePool(__startup_mempool);
    }
}

ADD2INIT(__init_memstuff, 0);
ADD2EXIT(__exit_memstuff, 0);
