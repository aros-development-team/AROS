/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:30  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/10/10 13:20:49  digulla
    Use dol_DevName(STRPTR) instead of dol_Name(BSTR) (Fleischer)

    Revision 1.3  1996/08/13 13:52:47  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:52  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	AROS_LH1(void, FreeDosEntry,

/*  SYNOPSIS */
	AROS_LHA(struct DosList *, dlist, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 117, Dos)

/*  FUNCTION
	Frees a dos list entry created with MakeDosEntry().

    INPUTS
	dlist - pointer to dos list entry. May be NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    if(dlist!=NULL)
    {
	STRPTR s2;
	s2=dlist->dol_DevName;
	while(*s2++)
	   ;
	FreeMem(dlist->dol_DevName-1,s2-dlist->dol_DevName+2);
	FreeMem(dlist,sizeof(struct DosList));
    }

    AROS_LIBFUNC_EXIT
} /* FreeDosEntry */
