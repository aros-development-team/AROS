/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function malloc()
    Lang: English
*/
#include <errno.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#ifndef _CLIB_KERNEL_
struct SignalSemaphore __startup_memsem;
APTR __startup_mempool;
#endif

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
    struct memheader
    {
        struct SignalSemaphore *memsem;
        APTR                    mempool;
        size_t                  memsize;
    };

    GETUSER;

    UBYTE *mem = NULL;

    ObtainSemaphore(&__startup_memsem);

    /* Allocate the memory */
    mem = AllocPooled(__startup_mempool,
		      size + AROS_ALIGN(sizeof(struct memheader)));
    if (mem)
    {
	struct memheader *mh = (struct memheader *)mem;

	mh->memsem  = &__startup_memsem;
	mh->mempool = __startup_mempool;
	mh->memsize = size;

	mem += AROS_ALIGN(sizeof(struct memheader));
    }
    else
    {
        errno = ENOMEM;
    }

    ReleaseSemaphore(&__startup_memsem);

    return mem;

} /* malloc */


int __init_memstuff(void)
{
    GETUSER;

    InitSemaphore(&__startup_memsem);
    __startup_mempool = CreatePool(MEMF_ANY, 4096L, 2000L);

    if (!__startup_mempool)
    {
	return RETURN_FAIL;
    }

    return 0;
}


void __exit_memstuff(void)
{
    GETUSER;

    if (__startup_mempool)
    {
	DeletePool(__startup_mempool);
    }
}

ADD2INIT(__init_memstuff, 0);
ADD2EXIT(__exit_memstuff, 0);
