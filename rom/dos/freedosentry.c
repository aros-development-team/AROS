/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(void, FreeDosEntry,

/*  SYNOPSIS */
	AROS_LHA(struct DosList *, dlist, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 117, Dos)

/*  FUNCTION
	Free a dos list entry created with MakeDosEntry().

    INPUTS
	dlist  --  pointer to dos list entry. May be NULL.

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

    if (dlist != NULL)
    {
	/* It's important to free OldName here due to BSTR compatibility
	   shit. See MakeDosEntry() */
	FreeVec(BADDR(dlist->dol_OldName));
	FreeMem(dlist, sizeof(struct DosList));
    }

    AROS_LIBFUNC_EXIT
} /* FreeDosEntry */
