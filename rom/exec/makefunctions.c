/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create the jumptable for a shared library or a device.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH3(ULONG, MakeFunctions,

/*  SYNOPSIS */
	AROS_LHA(APTR, target,        A0),
	AROS_LHA(APTR, functionArray, A1),
	AROS_LHA(APTR, funcDispBase,  A2),

/*  LOCATION */
	struct ExecBase *, SysBase, 15, Exec)

/*  FUNCTION
	Creates the jumptable for a shared library and flushes the processor's
	instruction cache. Does not checksum the library.

    INPUTS
	target	      - The highest byte +1 of the jumptable. Typically
			this is the library's base address.
	functionArray - Pointer to either an array of function pointers or
			an array of WORD displacements to a given location
			in memory. A value of -1 terminates the array in both
			cases.
	funcDispBase  - The base location for WORD displacements or NULL
			for function pointers.

    RESULT
	Size of the jumptable.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    long n;
    APTR lastvec;

    n = 1;

    if (funcDispBase!=NULL)
    {
	/* If FuncDispBase is non-NULL it's an array of relative offsets */
	WORD *fp=(WORD *)functionArray;

	/* -1 terminates the array */
	while(*fp!=-1)
	{
	    /* Decrement vector pointer by one and install vector */
	    __AROS_INITVEC(target,n);
	    if (*fp)
		__AROS_SETVECADDR(target,n,funcDispBase+*fp);

	    /* Use next array entry */
	    fp++;
	    n++;
	}
    }
    else
    {
	/* If FuncDispBase is NULL it's an array of function pointers */
	void **fp=(void **)functionArray;

	/* -1 terminates the array */
	while(*fp!=(void *)-1)
	{
	    /* Decrement vector pointer by one and install vector */
	    __AROS_INITVEC(target,n);
	    if (*fp)
		__AROS_SETVECADDR(target,n,*fp);

	    /* Use next array entry */
	    fp++;
	    n++;
	}
    }

    lastvec = __AROS_GETJUMPVEC(target,n);
    n = (IPTR)target-(IPTR)lastvec;

    /* Clear instruction cache for the whole jumptable */
    CacheClearE(lastvec, n, CACRF_ClearI|CACRF_ClearD);

    /* Return size of jumptable */
    return n;
    AROS_LIBFUNC_EXIT
} /* MakeFunctions */

