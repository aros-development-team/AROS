/*
    (C) 1995-97 AROS - The Amiga Research OS
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
	name - pointer to name
	type - type of list entry to create

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

    CONST_STRPTR s2;
    STRPTR s3;
    struct DosList *dl;

    dl=(struct DosList *)AllocMem(sizeof(struct DosList),MEMF_PUBLIC|MEMF_CLEAR);
    if(dl!=NULL)
    {
	s2=name;
	while(*s2++)
	    ;
	s3=(STRPTR)AllocVec(s2-name+1,MEMF_PUBLIC);
	if(s3!=NULL)
	{
	    /* Compatibility */
	    dl->dol_OldName=MKBADDR(s3);
	    *s3++=s2-name>256?255:s2-name-1;

	    CopyMem(name,s3,s2-name);
	    dl->dol_DevName=s3;
	    dl->dol_Type=type;
	    return dl;
	} else
	    SetIoErr(ERROR_NO_FREE_STORE);
	FreeMem(dl,sizeof(struct DosList));
    } else
        SetIoErr(ERROR_NO_FREE_STORE);
    return NULL;
    AROS_LIBFUNC_EXIT
} /* MakeDosEntry */
