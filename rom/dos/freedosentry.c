/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:22  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(void, FreeDosEntry,

/*  SYNOPSIS */
	__AROS_LA(struct DosList *, dlist, D1),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    if(dlist!=NULL)
    {
        STRPTR s2;
    	s2=dlist->dol_Name;
    	while(*s2++)
    	   ;
    	FreeMem(dlist->dol_Name-1,s2-dlist->dol_Name+2);
    	FreeMem(dlist,sizeof(struct DosList));
    }

    __AROS_FUNC_EXIT
} /* FreeDosEntry */
