/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function free().
*/

#include "__stdc_intbase.h"
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

    EXAMPLE

    BUGS

    SEE ALSO
	malloc()

    INTERNALS

******************************************************************************/
{
    if (memory)
    {
        struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();

	unsigned char *mem;
	size_t         size;

        mem = ((UBYTE *)memory) - AROS_ALIGN(sizeof(size_t));

        size = *((size_t *) mem);
        if (size == MEMALIGN_MAGIC)
            free(((void **) mem)[-1]);
        else {
            size += AROS_ALIGN(sizeof(size_t));
	    FreePooled (StdCBase->mempool, mem, size);
        }
    }

} /* free */

