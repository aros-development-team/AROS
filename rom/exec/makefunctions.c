/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:11  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include "machine.h"

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH3(ULONG, MakeFunctions,

/*  SYNOPSIS */
	__AROS_LA(APTR, target,        A0),
	__AROS_LA(APTR, functionArray, A1),
	__AROS_LA(APTR, funcDispBase,  A2),

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
    __AROS_FUNC_INIT

    /* Cast for easier access */
    struct JumpVec *jv=(struct JumpVec *)target;

    if(funcDispBase!=NULL)
    {
	/* If FuncDispBase is non-NULL it's an array of relative offsets */
	WORD *fp=(WORD *)functionArray;

	/* -1 terminates the array */
	while(*fp!=-1)
	{
	    /* Decrement vector pointer by one and install vector */
	    jv--;
	    SET_JMP(jv);
	    SET_VEC(jv,(BYTE *)funcDispBase+*fp);

	    /* Use next array entry */
	    fp++;
	}
    }else
    {
	/* If FuncDispBase is NULL it's an array of function pointers */
	void **fp=(void **)functionArray;

	/* -1 terminates the array */
	while(*fp!=(void *)-1)
	{
	    /* Decrement vector pointer by one and install vector */
	    jv--;
	    SET_JMP(jv);
	    SET_VEC(jv,*fp);

	    /* Use next array entry */
	    fp++;
	}
    }

    /* Clear instruction cache for the whole jumptable */
    CacheClearE(jv,(BYTE *)funcDispBase-(BYTE *)jv,CACRF_ClearI);

    /* Return size of jumptable */
    return (BYTE *)funcDispBase-(BYTE *)jv;
    __AROS_FUNC_EXIT
} /* MakeFunctions */

