/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function malloc().
*/

#include "__arosc_privdata.h"

#define DEBUG 0

#include <errno.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */
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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    UBYTE *mem = NULL;

    /* Allocate the memory */
    mem = AllocPooled (aroscbase->acb_mempool, size + AROS_ALIGN(sizeof(size_t)));
    if (mem)
    {
	*((size_t *)mem) = size;
	mem += AROS_ALIGN(sizeof(size_t));
    }
    else
        errno = ENOMEM;

    return mem;

} /* malloc */


int __init_memstuff(struct aroscbase *aroscbase)
{
    D(bug("__init_memstuff: task(%x), aroscbase(%x)\n",
          FindTask(NULL), aroscbase
    ));

    aroscbase->acb_mempool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 65536L, 4096L);

    D(bug("__init_memstuff: aroscbase->acb_mempool(%x)\n", aroscbase->acb_mempool));

    if (!aroscbase->acb_mempool)
    {
	return 0;
    }

    return 1;
}


void __exit_memstuff(struct aroscbase *aroscbase)
{
    D(bug("__exit_memstuff: task(%x), aroscbase(%x), acb_mempool(%x)\n",
          FindTask(NULL), aroscbase, aroscbase->acb_mempool
    ));

    if (aroscbase->acb_mempool)
    {
	DeletePool(aroscbase->acb_mempool);
    }
}

ADD2OPENLIB(__init_memstuff, 0);
ADD2CLOSELIB(__exit_memstuff, 0);
