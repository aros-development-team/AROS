/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function free().
*/

#include "__arosc_privdata.h"
#include "__memalign.h"

#include <exec/memory.h>
#include <proto/exec.h>

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
        This function must not be used in a shared library or in a threaded
	application.

    EXAMPLE

    BUGS

    SEE ALSO
	malloc()

    INTERNALS

******************************************************************************/
{
    if (memory)
    {
	unsigned char *mem;
	size_t         size;

        mem = ((UBYTE *)memory) - AROS_ALIGN(sizeof(size_t));

        size = *((size_t *) mem);
        if (size == MEMALIGN_MAGIC)
            free(((void **) mem)[-1]);
        else {
            size += AROS_ALIGN(sizeof(size_t));
	    FreePooled (__mempool, mem, size);
        }
    }

} /* free */

