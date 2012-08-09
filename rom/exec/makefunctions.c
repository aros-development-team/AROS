/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create the jumptable for a shared library or a device.
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_debug.h"

AROS_LD3(void, CacheClearE,
	 AROS_LHA(APTR, address, A0),
	 AROS_LHA(ULONG, length, D0),
	 AROS_LHA(ULONG, caches, D1),
	 struct ExecBase *, SysBase, 107, Exec);

/*****************************************************************************

    NAME */

	AROS_LH3(ULONG, MakeFunctions,

/*  SYNOPSIS */
	AROS_LHA(APTR, target,        A0),
	AROS_LHA(CONST_APTR, functionArray, A1),
	AROS_LHA(CONST_APTR, funcDispBase,  A2),

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

    DCREATELIBRARY("MakeFunctions(0x%p, 0x%p, 0x%p)", target, functionArray, funcDispBase);

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
		__AROS_SETVECADDR(target,n,(void *)funcDispBase+*fp);

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

#ifdef __AROS_USE_FULLJMP
    /* Clear instruction cache for the whole jumptable. We need to do it only if
       the jumptable actually contains executable code. __AROS_USE_FULLJMP must
       be defined in cpu.h in this case.

       Note that we call this function directly because MakeFunctions() is also
       used for building ExecBase itself. */
    if (SysBase->LibNode.lib_Node.ln_Type != NT_LIBRARY) {
        AROS_CALL3NR(void, AROS_SLIB_ENTRY(CacheClearE, Exec, 107),
	      AROS_UFCA(APTR, lastvec, A0),
	      AROS_UFCA(ULONG, n, D0),
	      AROS_UFCA(ULONG, CACRF_ClearI|CACRF_ClearD, D1),
	      struct ExecBase *, SysBase);
    } else {
	/* call CacheClearE() indirectly if SysBase is already valid.
	 * CacheClearE may have been SetFunction()'d for specific CPU type.
	 */
    	CacheClearE(lastvec, n, CACRF_ClearI|CACRF_ClearD);
    }
#endif

    /* Return size of jumptable */
    DCREATELIBRARY("Created %lu vectors", n);
    return n;

    AROS_LIBFUNC_EXIT
} /* MakeFunctions */

