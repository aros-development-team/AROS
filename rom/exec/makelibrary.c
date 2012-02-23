/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Make a library ready for use.
    Lang: english
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <proto/exec.h>

#include "exec_debug.h"

/*****************************************************************************

    NAME */

	AROS_LH5(struct Library *, MakeLibrary,

/*  SYNOPSIS */
	AROS_LHA(CONST_APTR, funcInit,   A0),
	AROS_LHA(CONST_APTR, structInit, A1),
	AROS_LHA(ULONG_FUNC, libInit,    A2),
	AROS_LHA(ULONG,      dataSize,   D0),
	AROS_LHA(BPTR,       segList,    D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 14, Exec)

/*  FUNCTION
	Allocates memory for the library, builds it and calls the library's
	init vector. Generally this function is for internal use and for
	use by library programmers that don't want to use the automatic
	initialization procedure.

    INPUTS
	funcInit   - Either a pointer to an array of function offsets
		     (starts with -1, relative to funcInit) or to an array
		     of absolute function pointers.
	structInit - Pointer to a InitStruct() data region or NULL.
	libInit    - The library's init vector or NULL.
		     The init vector is called with the library address (D0),
		     the segList (A0) and ExecBase (A6) as arguments.
		     If the init fails the init code has to free the base memory
		     and return NULL (the library address for success).
	dataSize   - Size of the library structure including system structures.
		     Must be at least sizeof(struct Library).
	segList    - BCPL pointer to the library segments. Used to free the
		     library later.

    RESULT
	The library base address or NULL.

    NOTES
	The library base is always aligned to the maximum of sizeof(LONG)
	and double alignment restrictions.

    EXAMPLE

    BUGS

    SEE ALSO
	AddLibrary(), RemLibrary(), MakeFunctions(), InitStruct(), SumLibrary()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Library *library;
    ULONG negsize;
    ULONG count = 0;

    DCREATELIBRARY("MakeLibrary: functions table at 0x%p, data size is %lu", funcInit, dataSize);

    /* Can't do the following check on machines like x86, because 0x????ffff is
       a valid address for code there! */
    
#if defined(__mc68000) || defined(__ppc__) || defined(__powerpc__)
    /* Calculate the jumptable's size */
    if (*(WORD *)funcInit==-1)
    {
	/* Count offsets */
	WORD *fp=(WORD *)funcInit+1;

	while(*fp++!=-1)
	    count++;
	DCREATELIBRARY("Table contains %lu relative offsets", count);
    }
    else
#endif
    {
	/* Count function pointers */
	void **fp=(void **)funcInit;

	while(*fp++!=(void *)-1)
	    count++;

	DCREATELIBRARY("Table contains %lu absolute pointers", count);
    }

    /* Align library base */
    negsize=AROS_ALIGN(count * LIB_VECTSIZE);

    /* Allocate memory */
    library=(struct Library *)AllocMem(dataSize+negsize,MEMF_PUBLIC|MEMF_CLEAR);
    DCREATELIBRARY("Allocated vector table at 0x%p, size is %lu", library, negsize);

    /* And initilize the library */
    if (library!=NULL)
    {
	/* Get real library base */
	library=(struct Library *)((char *)library+negsize);

	/* Build jumptable */
    #if defined(__mc68000) || defined(__ppc__) || defined(__powerpc__)
	if(*(WORD *)funcInit==-1)
	    /* offsets */
	    MakeFunctions(library,(WORD *)funcInit+1,(WORD *)funcInit);
	else
    #endif
	    /* function pointers */
	    MakeFunctions(library,funcInit,NULL);

	/* Write sizes */
	library->lib_NegSize=negsize;
	library->lib_PosSize=dataSize;

	/* Create structure, do not clear struct Library */
	if(structInit!=NULL)
	    InitStruct(structInit,library,0);

	/* Call init vector */
	if (libInit!=NULL)
	{
	    DCREATELIBRARY("Calling init function 0x%p", libInit);

	    library=AROS_UFC3(struct Library *, libInit,
		AROS_UFCA(struct Library *,  library, D0),
		AROS_UFCA(BPTR,              segList, A0),
		AROS_UFCA(struct ExecBase *, SysBase, A6)
	    );
	}
    }

    /* All done */
    DCREATELIBRARY("Created library 0x%p", library);
    return library;

    AROS_LIBFUNC_EXIT
} /* MakeLibrary */

