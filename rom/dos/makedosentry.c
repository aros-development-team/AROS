/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Creates an entry for the dos list.
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(struct DosList *, MakeDosEntry,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),
	AROS_LHA(LONG,         type, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 116, Dos)

/*  FUNCTION
	Create an entry for the dos list. Depending on the type this may
	be a device a volume or an assign node.

    INPUTS
	name  --  pointer to name
	type  --  type of list entry to create

    RESULT

        The new device entry or NULL in case it couldn't be created.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    AddDosEntry(), RemDosEntry(), FindDosEntry(), LockDosList(),
    NextDosEntry(), FreeDosEntry()

    INTERNALS

    This call should be replaced by a value for AllocDosObject().

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    CONST_STRPTR    s2;
    STRPTR          s3;
    struct DosList *dl;

    dl = (struct DosList *)AllocMem(sizeof(struct DosList),
				    MEMF_PUBLIC | MEMF_CLEAR);

    if (dl != NULL)
    {
	s2 = name;

	while (*s2++)
	    ;

	/* Use MEMF_CLEAR to make sure that "real" BSTR:s also are always
	   ended with a 0 byte */
	s3 = (STRPTR)AllocVec(s2 - name + 1, MEMF_PUBLIC | MEMF_CLEAR);

	if (s3 != NULL)
	{
	    int i;		/* Loop variable */
	    int length = s2 - name > 255 ? 255 : s2 - name - 1;

	    /* Compatibility */
	    dl->dol_OldName = MKBADDR(s3);
	    AROS_BSTR_setstrlen(s3, length);

	    for (i = 0; i < length; i++)
	    {
		AROS_BSTR_putchar(s3, i, name[i]); 
	    }

	    dl->dol_DevName = AROS_BSTR_ADDR(dl->dol_OldName);
	    dl->dol_Type = type;

	    return dl;
	}
	else
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	}
	
	FreeMem(dl, sizeof(struct DosList));
    }
    else
    {
        SetIoErr(ERROR_NO_FREE_STORE);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* MakeDosEntry */
