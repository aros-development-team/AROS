/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function free()
    Lang: english
*/

#include <exec/memory.h>
#include <proto/exec.h>

#ifndef _CLIB_KERNEL_
extern struct SignalSemaphore __startup_memsem;
extern APTR __startup_mempool;
#endif

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
    struct memheader
    {
        struct SignalSemaphore *memsem;
        APTR                    mempool;
        size_t                  memsize;
    };

    if (memory)
    {
    	struct memheader       *mh     = (struct memheader *)(((UBYTE *)memory) - AROS_ALIGN(sizeof(struct memheader)));
	struct SignalSemaphore *memsem;

	ObtainSemaphore(mh->memsem);

	memsem = mh->memsem;

	FreePooled (mh->mempool, mh, mh->memsize + AROS_ALIGN(sizeof(struct memheader)));
        ReleaseSemaphore(memsem);
    }

} /* free */

