/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/19 17:07:27  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.4  1996/08/13 13:56:04  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:14  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <aros/libcall.h>
#include <aros/machine.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH5(struct Library *, MakeLibrary,

/*  SYNOPSIS */
	__AROS_LHA(APTR,       funcInit,   A0),
	__AROS_LHA(APTR,       structInit, A1),
	__AROS_LHA(ULONG_FUNC, libInit,    A2),
	__AROS_LHA(ULONG,      dataSize,   D0),
	__AROS_LHA(BPTR,       segList,    D1),

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
	AddLibrary(), RemLibrary(), MakeFunctions(), InitStruct(), SumLibrary().

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    struct Library *library;
    ULONG negsize=0;

    /* Calculate the jumptable's size */
    if(*(WORD *)funcInit==-1)
    {
	/* Count offsets */
	WORD *fp=(WORD *)funcInit+1;
	while(*fp++!=-1)
	    negsize+=sizeof(struct JumpVec);
    }
    else
    {
	/* Count function pointers */
	void **fp=(void **)funcInit;
	while(*fp++!=(void *)-1)
	    negsize+=sizeof(struct JumpVec);
    }

    /* Align library base */
    negsize=(negsize+(LIBALIGN-1))&~(LIBALIGN-1);

    /* Allocate memory */
    library=(struct Library *)AllocMem(dataSize+negsize,MEMF_PUBLIC|MEMF_CLEAR);

    /* And initilize the library */
    if(library!=NULL)
    {
	/* Get real library base */
	library=(struct Library *)((char *)library+negsize);

	/* Build jumptable */
	if(*(WORD *)funcInit==-1)
	    /* offsets */
	    MakeFunctions(library,(WORD *)funcInit+1,(WORD *)funcInit+1);
	else
	    /* function pointers */
	    MakeFunctions(library,funcInit,NULL);

	/* Write sizes */
	library->lib_NegSize=negsize;
	library->lib_PosSize=dataSize;

	/* Create structure */
	if(structInit!=NULL)
	    InitStruct(structInit,library,0);

	/* Call init vector */
	if(libInit!=NULL)
	    library=__AROS_ABS_CALL3(struct Library *,libInit,
				     library,D0,segList,A0,SysBase,A6);
    }
    /* All done */
    return library;

    __AROS_FUNC_EXIT
} /* MakeLibrary */

