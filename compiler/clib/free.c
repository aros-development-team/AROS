/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function free()
    Lang: english
*/

#include <exec/memory.h>
#include <proto/exec.h>

#ifndef _CLIB_KERNEL_
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
        This function must not be used in a shared library or in a threaded
	application.

    EXAMPLE

    BUGS

    SEE ALSO
	malloc()

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    if (memory)
    {
        GETUSER;
	AROS_GET_SYSBASE_OK
	
	unsigned char *mem;
	size_t         size;

        mem = ((UBYTE *)memory) - AROS_ALIGN(sizeof(size_t));
	size = *((size_t *)mem) + AROS_ALIGN(sizeof(size_t));

	FreePooled (__startup_mempool, mem, size);
    }

} /* free */

