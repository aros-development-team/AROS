/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.10  1997/01/01 03:46:11  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.9  1996/12/10 13:51:48  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.8  1996/12/03 08:43:12  aros
    Ooops :-) Wrong address

    Revision 1.7  1996/10/24 15:50:51  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/23 14:28:53  aros
    Use the respective macros to access and manipulate a libraries' jumptable

    Revision 1.5  1996/10/19 17:07:26  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.4  1996/08/13 13:56:03  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:13  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/machine.h>
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

    HISTORY

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
	    __AROS_SETVECADDR(target,n,*fp);

	    /* Use next array entry */
	    fp++;
	    n++;
	}
    }

    lastvec = __AROS_GETJUMPVEC(target,n);
    n = (IPTR)target-(IPTR)lastvec;

    /* Clear instruction cache for the whole jumptable */
    CacheClearE(lastvec, n, CACRF_ClearI);

    /* Return size of jumptable */
    return n;
    AROS_LIBFUNC_EXIT
} /* MakeFunctions */

